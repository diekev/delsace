/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "adn.hh"

#include "biblinternes/outils/conditions.h"

FluxSortieCPP &operator<<(FluxSortieCPP &flux, const IdentifiantADN &ident)
{
	return flux << ident.nom_cpp();
}

FluxSortieKuri &operator<<(FluxSortieKuri &flux, const IdentifiantADN &ident)
{
	return flux << ident.nom_kuri();
}

FluxSortieKuri &operator<<(FluxSortieKuri &os, Type const &type)
{
	for (auto i = type.specifiants.taille() - 1; i >= 0; --i) {
		const auto spec = type.specifiants[i];
		os << chaine_du_lexeme(spec);
	}

	if (type.nom == "chaine_statique") {
		os << "chaine";
	}
	else {
		os << type.nom;
	}
	return os;
}

FluxSortieCPP &operator<<(FluxSortieCPP &os, Type const &type)
{
	const auto &specifiants = type.specifiants;

	const auto est_tableau = (specifiants.taille() > 0 && specifiants.derniere() == GenreLexeme::TABLEAU);

	if (type.est_const) {
		os << "const ";
	}

	if (est_tableau) {
		if (type.est_compresse) {
			os << "kuri::tableau_compresse<";
		}
		else if (type.est_synchrone) {
			os << "tableau_synchrone<";
		}
		else {
			os << "kuri::tableau<";
		}
	}

	if (type.nom == "rien") {
		os << "void";
	}
	else if (type.nom == "chaine_statique") {
		os << "kuri::chaine_statique";
	}
	else if (type.nom == "chaine") {
		os << "kuri::chaine";
	}
	else if (type.nom == "z32") {
		os << "int";
	}
	else if (type.nom == "Monomorphisations") {
		os << "Monomorphisations<CetteClasse>";
	}
	else {		
		os << supprime_accents(type.nom);
	}

	for (auto i = type.specifiants.taille() - 1 - est_tableau; i >= 0; --i) {
		const auto spec = type.specifiants[i];
		os << chaine_du_lexeme(spec);
	}

	if (est_tableau) {
		if (type.est_synchrone) {
			os << ">";
		}
		else {
			os << ", int>"; // À FAIRE: ceci échouera pour les NoeudsCodes
		}
	}

	return os;
}

Proteine::Proteine(IdentifiantADN nom)
	: m_nom(nom)
{}

ProteineStruct::ProteineStruct(IdentifiantADN nom)
	: Proteine(nom)
	, m_nom_code(nom)
{}

void ProteineStruct::genere_code_cpp(FluxSortieCPP &os, bool pour_entete)
{
	if (pour_entete) {
		os << "struct " << m_nom.nom_cpp();

		if (m_mere) {
			os << " : public " << m_mere->nom().nom_cpp();
		}

		os << " {\n";

		// À FAIRE : spécialise le nom du genre
		if (!accede_nom_genre().est_nul()) {
			os << "\t" << m_nom << "() { genre = GenreNoeud::" << accede_nom_genre() << "; }\n";
			os << "\tCOPIE_CONSTRUCT(" << m_nom << ");\n";
			os << "\n";
		}

		// petit hack pour pouvoir sainement déclarer les Monomorphisations
		os << "\tusing CetteClasse = " << m_nom << ";\n";
		os << "\tPOINTEUR_NUL(" << m_nom << ")\n";

		if (!m_membres.est_vide()) {
			os << "\n";
		}

		POUR (m_membres) {
			os << "\t" << it.type << ' ' << it.nom.nom_cpp();

			if (it.valeur_defaut != "") {
				os << " = ";

				if (it.valeur_defaut_est_acces) {
					os << it.type << "::";
				}

				if (dls::outils::est_element(it.type.nom, "chaine", "chaine_statique")) {
					os << '"' << it.valeur_defaut << '"';
				}
				else if (it.valeur_defaut == "vrai") {
					os << "true";
				}
				else if (it.valeur_defaut == "faux") {
					os << "false";
				}
				else {
					os << supprime_accents(it.valeur_defaut);
				}
			}
			else {
				if (it.type.est_pointeur && !it.type.est_tableau) {
					os << " = nullptr";
				}
				else {
					os << " = {}";
				}
			}

			os << ";\n";
		}

		// Prodéclare les fonctions de discrimination.
		if (est_racine_hierarchie()) {
			pour_chaque_derivee_recursif([&os](const ProteineStruct &derivee) {
				if (derivee.m_nom_comme.nom_cpp() == "") {
					return;
				}

				const auto nom_comme = derivee.m_nom_comme;
				const auto nom_noeud = derivee.m_nom;

				os << "\n";
				os << "\tinline bool est_" << nom_comme << "() const;\n";
				os << "\tinline " << nom_noeud << " *comme_" << nom_comme << "();\n";
				os << "\tinline const " << nom_noeud << " *comme_" << nom_comme << "() const;\n";
			});
		}

		os << "};\n\n";
	}

	os << "std::ostream &operator<<(std::ostream &os, " << m_nom.nom_cpp() << " const &valeur)";

	if (pour_entete) {
		os << ";\n\n";
	}
	else {
		os << "\n";
		os << "{\n";

		os << "\tos << \"" << m_nom.nom_cpp() << " : {\\n\";" << '\n';

		pour_chaque_membre_recursif([&os](Membre const &it) {
			if (it.type.est_tableau) {
				return;
			}

			// À FAIRE : ostream << spécialisé pour ValeurExpression, TransformationType
			if (dls::outils::est_element(it.type.nom, "ValeurExpression", "TransformationType")) {
				return;
			}

			os << "\tos << \"\\t" << it.nom.nom_cpp() << " : \" << valeur." <<  it.nom.nom_cpp() << " << '\\n';" << '\n';
		});

		os << "\tos << \"}\\n\";\n";

		os << "\treturn os;\n";
		os << "}\n\n";
	}

	os << "bool est_valide(" << m_nom.nom_cpp() << " const &valeur)";

	if (pour_entete) {
		os << ";\n\n";
	}
	else {
		os << "\n";
		os << "{\n";

		// À FAIRE : nom_genre_énum
		if (!accede_nom_genre().est_nul()) {
			os << "\tif (valeur.genre != GenreNoeud::" << accede_nom_genre() << ") {\n";
			os << "\t\treturn false;\n";
			os << "\t}\n";
		}

		pour_chaque_membre_recursif([&os](Membre const &it) {
			if (it.type.est_enum) {
				os << "\tif (!est_valeur_legale(valeur." << it.nom.nom_cpp() << ")) {\n";
				os << "\t\treturn false;\n";
				os << "\t}\n";
			}
		});

		os << "\treturn true;\n";
		os << "}\n\n";
	}
}

void ProteineStruct::genere_code_cpp_apres_declaration(FluxSortieCPP &os)
{
	// Implémente les fonctions de discrimination.
	// Nous devons attendre que toutes les structures soient déclarées avant de
	// pouvoir faire ceci.
	if (est_racine_hierarchie()) {
		pour_chaque_derivee_recursif([&os, this](const ProteineStruct &derivee) {
			if (derivee.m_nom_comme.nom_cpp() == "") {
				return;
			}

			const auto &nom_comme = derivee.m_nom_comme;
			const auto &nom_noeud = derivee.m_nom;
			const auto &nom_genre = derivee.m_nom_genre;

			// À FAIRE: si/discr
			if (derivee.est_racine_soushierachie() && nom_genre.nom_cpp() == "") {
				os << "inline bool " << m_nom << "::est_" << nom_comme << "() const\n";
				os << "{\n";
				os << "\t ";

				auto separateur = "return";

				for (auto &derive : derivee.derivees()) {
					os << separateur << " this->est_" << derive->accede_nom_comme() << "()";
					separateur = "|| ";
				}

				os << ";\n";
				os << "}\n\n";
			}
			else {
				os << "inline bool " << m_nom << "::est_" << nom_comme << "() const\n";
				os << "{\n";

				if (derivee.est_racine_soushierachie()) {
					os << "\treturn this->genre == GenreNoeud::" << nom_genre;

					for (auto &derive : derivee.derivees()) {
						os << " || this->genre == GenreNoeud::" << derive->accede_nom_genre();
					}

					os << ";\n";
				}
				else {
					os << "\treturn this->genre == GenreNoeud::" << nom_genre << ";\n";
				}

				os << "}\n\n";
			}

			os << "inline " << nom_noeud << " *" << m_nom << "::comme_" << nom_comme << "()\n";
			os << "{\n";
			os << "\tassert(est_" << nom_comme << "());\n";
			os << "\treturn static_cast<" << nom_noeud << " *>(this);\n";
			os << "}\n\n";

			os << "inline const " << nom_noeud << " *" << m_nom << "::comme_" << nom_comme << "() const\n";
			os << "{\n";
			os << "\tassert(est_" << nom_comme << "());\n";
			os << "\treturn static_cast<const " << nom_noeud << " *>(this);\n";
			os << "}\n\n";
		});
	}
}

void ProteineStruct::genere_code_kuri(FluxSortieKuri &os)
{
	os << m_nom_code.nom_kuri() << " :: struct {\n";
	if (m_mere) {
		os << "\templ base: " << m_mere->nom().nom_kuri() << "\n\n";
	}

	POUR (m_membres) {
		os << "\t" << it.nom.nom_kuri();

		if (it.valeur_defaut == "") {
			os << ": " << it.type;
		}
		else {
			os << " := ";
			if (it.valeur_defaut_est_acces) {
				os << it.type << ".";
			}

			if (dls::outils::est_element(it.type.nom, "chaine", "chaine_statique")) {
				os << '"' << it.valeur_defaut << '"';
			}
			else {
				os << it.valeur_defaut;
			}
		}

		os << "\n";
	}
	os << "}\n\n";
}

void ProteineStruct::ajoute_membre(const Membre membre)
{
	if (membre.est_a_copier) {
		m_possede_membre_a_copier = true;
	}

	if (membre.est_enfant) {
		m_possede_enfant = true;
	}

	m_possede_tableaux |= membre.type.est_tableau;

	if (membre.est_code && m_paire) {
		m_paire->m_possede_enfant = membre.est_enfant;
		m_paire->m_possede_membre_a_copier = membre.est_a_copier;
		m_paire->m_membres.ajoute(membre);
	}

	m_membres.ajoute(membre);
}

void ProteineStruct::pour_chaque_membre_recursif(std::function<void (const Membre &)> rappel)
{
	if (m_mere) {
		m_mere->pour_chaque_membre_recursif(rappel);
	}

	POUR (m_membres) {
		rappel(it);
	}
}

void ProteineStruct::pour_chaque_copie_extra_recursif(std::function<void (const Membre &)> rappel)
{
	if (m_mere) {
		m_mere->pour_chaque_copie_extra_recursif(rappel);
	}

	POUR (m_membres) {
		if (it.est_a_copier) {
			rappel(it);
		}
	}
}

void ProteineStruct::pour_chaque_enfant_recursif(std::function<void (const Membre &)> rappel)
{
	if (m_mere) {
		m_mere->pour_chaque_enfant_recursif(rappel);
	}

	POUR (m_membres) {
		if (it.est_enfant) {
			rappel(it);
		}
	}
}

void ProteineStruct::pour_chaque_derivee_recursif(std::function<void (const ProteineStruct &)> rappel)
{
	POUR (derivees()) {
		rappel(*it);

		if (it->est_racine_soushierachie()) {
			it->pour_chaque_derivee_recursif(rappel);
		}
	}
}

ProteineEnum::ProteineEnum(IdentifiantADN nom)
	: Proteine(nom)
{}

void ProteineEnum::genere_code_cpp(FluxSortieCPP &os, bool pour_entete)
{
	if (pour_entete) {
		auto type_valeur = "int";

		if (m_nom.nom_cpp() == "GenreNoeud") {
			type_valeur = "unsigned char";
		}

		os << "enum class " << m_nom.nom_cpp() << " : " << type_valeur << " {\n";

		POUR (m_membres) {
			os << "\t" << it.nom.nom_cpp() << ",\n";
		}

		os << "};\n\n";
	}

	if (!pour_entete) {
		os << "static kuri::chaine_statique chaines_membres_" << m_nom.nom_cpp() << "["
		   << m_membres.taille() << "] = {\n";

		POUR (m_membres) {
			os << "\t\"" << it.nom.nom_cpp() << "\",\n";
		}

		os << "};\n\n";
	}

	os << "std::ostream &operator<<(std::ostream &os, " << m_nom.nom_cpp() << " valeur)";

	if (pour_entete) {
		os << ";\n\n";
	}
	else {
		os << "\n";
		os << "{\n";
		os << "\treturn os << chaines_membres_" << m_nom.nom_cpp() << "[static_cast<int>(valeur)];\n";
		os << "}\n\n";
	}

	os << "bool est_valeur_legale(" << m_nom.nom_cpp() << " valeur)";

	if (pour_entete) {
		os << ";\n\n";
	}
	else {
		const auto &premier_membre = m_membres[0];
		const auto &dernier_membre = m_membres.derniere();

		os << "\n";
		os << "{\n";
		os << "\treturn valeur >= " << m_nom.nom_cpp() << "::" << premier_membre.nom.nom_cpp()
		   << " && valeur <= " << m_nom.nom_cpp() << "::" << dernier_membre.nom.nom_cpp()
		   << ";\n";
		os << "}\n\n";
	}
}

void ProteineEnum::genere_code_kuri(FluxSortieKuri &os)
{
	os << m_nom.nom_kuri() << " :: énum z32 {\n";
	POUR (m_membres) {
		os << "\t" << it.nom.nom_kuri() << "\n";
	}
	os << "}\n\n";
}

void ProteineEnum::ajoute_membre(const Membre membre)
{
	m_membres.ajoute(membre);
}

ProteineFonction::ProteineFonction(IdentifiantADN nom)
	: Proteine(nom)
{}

void ProteineFonction::genere_code_cpp(FluxSortieCPP &os, bool pour_entete)
{
	os << m_type_sortie << ' ' << m_nom.nom_cpp();

	auto virgule = "(";

	if (m_parametres.taille() == 0) {
		os << virgule;
	}
	else {
		for (auto &param : m_parametres) {
			os << virgule << param.type << ' ' << param.nom.nom_cpp();
			virgule = ", ";
		}
	}

	os << ")";

	if (pour_entete) {
		os << ";\n\n";
	}
	else {
		os << "\n{\n";

		os << "\tassert(false);\n";

		if (m_type_sortie.nom != "rien") {
			os << "\treturn {};\n";
		}

		os << "}\n\n";
	}
}

void ProteineFonction::genere_code_kuri(FluxSortieKuri &os)
{
	os << m_nom.nom_kuri() << " :: fonc ";

	auto virgule = "(";

	if (m_parametres.taille() == 0) {
		os << virgule;
	}
	else {
		for (auto &param : m_parametres) {
			os << virgule << param.nom.nom_kuri() << ": " << param.type;
			virgule = ", ";
		}
	}

	os << ")" << " -> " << m_type_sortie << " #compilatrice" << "\n\n";
}

void ProteineFonction::ajoute_parametre(Parametre const parametre)
{
	m_parametres.ajoute(parametre);
}

SyntaxeuseADN::SyntaxeuseADN(Fichier *fichier)
	: BaseSyntaxeuse(fichier)
{}

SyntaxeuseADN::~SyntaxeuseADN()
{
	POUR (proteines) {
		delete it;
	}

	POUR (proteines_paires) {
		delete it;
	}
}

void SyntaxeuseADN::analyse_une_chose()
{
	if (apparie("énum")) {
		parse_enum();
	}
	else if (apparie("struct")) {
		parse_struct();
	}
	else if (apparie("fonction")) {
		parse_fonction();
	}
	else {
		rapporte_erreur("attendu la déclaration d'une structure ou d'une énumération");
	}
}

void SyntaxeuseADN::parse_fonction()
{
	consomme();

	if (!apparie(GenreLexeme::CHAINE_CARACTERE)) {
		rapporte_erreur("Attendu une chaine de caractère après « fonction »");
	}

	auto fonction = cree_proteine<ProteineFonction>(lexeme_courant()->chaine);
	consomme();

	// paramètres
	if (!apparie(GenreLexeme::PARENTHESE_OUVRANTE)) {
		rapporte_erreur("Attendu une parenthèse ouvrante après le nom de la fonction");
	}
	consomme();

	while (!apparie(GenreLexeme::PARENTHESE_FERMANTE)) {
		auto parametre = Parametre{};
		parse_type(parametre.type);

		if (!apparie(GenreLexeme::CHAINE_CARACTERE)) {
			rapporte_erreur("Attendu une chaine de caractère pour le nom du paramètre après son type");
		}

		parametre.nom = lexeme_courant()->chaine;
		consomme();

		fonction->ajoute_parametre(parametre);

		if (apparie(GenreLexeme::VIRGULE)) {
			consomme();
		}
	}

	// consomme la parenthèse fermante
	consomme();

	// type de retour
	if (apparie(GenreLexeme::RETOUR_TYPE)) {
		consomme();
		parse_type(fonction->type_sortie());
	}

	if (apparie(GenreLexeme::POINT_VIRGULE)) {
		consomme();
	}
}

void SyntaxeuseADN::parse_enum()
{
	consomme();

	if (!apparie(GenreLexeme::CHAINE_CARACTERE)) {
		rapporte_erreur("Attendu une chaine de caractère après « énum »");
	}

	auto proteine = cree_proteine<ProteineEnum>(lexeme_courant()->chaine);

	consomme();

	consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante après le nom de « énum »");

	while (apparie(GenreLexeme::AROBASE)) {
		consomme();

		if (apparie("code")) {
			consomme();
			consomme();
			// À FAIRE : code
		}

		if (apparie(GenreLexeme::POINT_VIRGULE)) {
			consomme();
		}
	}

	while (true) {
		if (!apparie(GenreLexeme::CHAINE_CARACTERE)) {
			break;
		}

		auto membre = Membre{};
		membre.nom = lexeme_courant()->chaine;

		proteine->ajoute_membre(membre);

		consomme();

		if (apparie(GenreLexeme::POINT_VIRGULE)) {
			consomme();
		}
	}

	consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu une accolade fermante à la fin de l'énumération");
}

void SyntaxeuseADN::parse_struct()
{
	consomme();

	if (!apparie(GenreLexeme::CHAINE_CARACTERE)) {
		rapporte_erreur("Attendu une chaine de caractère après « énum »");
	}

	auto proteine = cree_proteine<ProteineStruct>(lexeme_courant()->chaine);
	consomme();

	if (apparie(GenreLexeme::DOUBLE_POINTS)) {
		consomme();

		if (!apparie(GenreLexeme::CHAINE_CARACTERE)) {
			rapporte_erreur("Attendu le nom de la structure mère après « : »");
		}

		auto nom_struct_mere = lexeme_courant()->chaine;

		POUR (proteines) {
			if (it->nom().nom_kuri() == kuri::chaine_statique(nom_struct_mere)) {
				if (!it->comme_struct()) {
					rapporte_erreur("Impossible de trouver la structure mère !");
				}

				proteine->descend_de(it->comme_struct());
				break;
			}
		}

		consomme();
	}

	consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante après le nom de « struct »");

	while (apparie(GenreLexeme::AROBASE)) {
		consomme();

		// 'comme' est un mot-clé dans le langage
		if (!apparie(GenreLexeme::CHAINE_CARACTERE) && !apparie(GenreLexeme::COMME)) {
			rapporte_erreur("Attendu une chaine de caractère après @");
		}

		// utilise 'lexeme.chaine' car 'comme' est un mot-clé, il n'y a pas d'identifiant
		if (apparie("code")) {
			consomme();

			if (!apparie(GenreLexeme::CHAINE_CARACTERE)) {
				rapporte_erreur("Attendu une chaine de caractère après @code");
			}

			proteine->mute_nom_code(lexeme_courant()->chaine);

			auto paire = new ProteineStruct(lexeme_courant()->chaine);
			proteines_paires.ajoute(paire);
			proteine->mute_paire(paire);

			consomme();
		}
		else if (apparie("comme")) {
			consomme();

			if (!apparie(GenreLexeme::CHAINE_CARACTERE) && !est_mot_cle(lexeme_courant()->genre)) {
				rapporte_erreur("Attendu une chaine de caractère après @code");
			}

			proteine->mute_nom_comme(lexeme_courant()->chaine);
			consomme();
		}
		else if (apparie("genre")) {
			consomme();

			if (!apparie(GenreLexeme::CHAINE_CARACTERE)) {
				rapporte_erreur("Attendu une chaine de caractère après @genre");
			}

			proteine->mute_nom_genre(lexeme_courant()->chaine);
			consomme();
		}
		else {
			rapporte_erreur("attribut inconnu");
		}

		if (apparie(GenreLexeme::POINT_VIRGULE)) {
			consomme();
		}
	}

	while (!apparie(GenreLexeme::ACCOLADE_FERMANTE)) {
		auto membre = Membre{};
		parse_type(membre.type);

		if (!apparie(GenreLexeme::CHAINE_CARACTERE)) {
			rapporte_erreur("Attendu le nom du membre après son type !");
		}

		membre.nom = lexeme_courant()->chaine;
		consomme();

		if (apparie(GenreLexeme::EGAL)) {
			consomme();

			if (apparie(GenreLexeme::POINT)) {
				membre.valeur_defaut_est_acces = true;
				consomme();
			}

			membre.valeur_defaut = lexeme_courant()->chaine;
			consomme();
		}

		if (apparie(GenreLexeme::CROCHET_OUVRANT)) {
			consomme();

			while (apparie(GenreLexeme::CHAINE_CARACTERE)) {
				if (apparie("code")) {
					membre.est_code = true;
				}
				else if (apparie("enfant")) {
					membre.est_enfant = true;
				}
				else if (apparie("copie")) {
					membre.est_a_copier = true;
				}
				else {
					rapporte_erreur("attribut inconnu");
				}

				consomme();
			}

			consomme(GenreLexeme::CROCHET_FERMANT, "Attendu un crochet fermant à la fin de la liste des attributs du membres");
		}

		if (apparie(GenreLexeme::POINT_VIRGULE)) {
			consomme();
		}

		proteine->ajoute_membre(membre);
	}

	consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu une accolade fermante à la fin de la structure");
}

void SyntaxeuseADN::parse_type(Type &type)
{
	if (!apparie(GenreLexeme::CHAINE_CARACTERE) && !est_mot_cle(lexeme_courant()->genre)) {
		rapporte_erreur("Attendu le nom d'un type");
	}

	type.nom = lexeme_courant()->chaine;

	POUR (proteines) {
		if (it->nom().nom_kuri() == type.nom) {
			if (it->comme_enum()) {
				type.est_enum = true;
			}

			break;
		}
	}

	consomme();

	if (apparie(GenreLexeme::CHAINE_CARACTERE)) {
		if (apparie("const")) {
			type.est_const = true;
			consomme();
		}
	}

	while (est_specifiant_type(lexeme_courant()->genre)) {
		if (lexeme_courant()->genre == GenreLexeme::CROCHET_OUVRANT) {
			type.specifiants.ajoute(GenreLexeme::TABLEAU);
			type.est_tableau = true;
			consomme();

			if (apparie(GenreLexeme::CHAINE_CARACTERE)) {
				if (apparie("compressé")) {
					type.est_compresse = true;
					consomme();
				}
				else if (apparie("synchrone")) {
					type.est_synchrone = true;
					consomme();
				}
				else {
					rapporte_erreur("attribut de tableau inconnu");
				}
			}

			consomme(GenreLexeme::CROCHET_FERMANT, "Attendu un crochet fermant après le crochet ouvrant");
		}
		else {
			type.specifiants.ajoute(lexeme_courant()->genre);
			type.est_pointeur = (lexeme_courant()->genre == GenreLexeme::FOIS);
			consomme();
		}
	}
}

void SyntaxeuseADN::gere_erreur_rapportee(const kuri::chaine &message_erreur)
{
	std::cerr << message_erreur << "\n";
}
