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

/* Fichier de génération du code pour l'IPA de la Compilatrice. */

#include <filesystem>
#include <fstream>

#include "biblinternes/moultfilage/synchrone.hh"

#include "parsage/base_syntaxeuse.hh"
#include "parsage/gerante_chaine.hh"
#include "parsage/identifiant.hh"
#include "parsage/lexeuse.hh"
#include "parsage/modules.hh"

#include "structures/tableau.hh"

#include "outils.hh"

template <typename T>
struct FormatKuri;

template <typename T>
struct FormatCPP;

struct Type {
	kuri::chaine nom = "rien";
	kuri::tableau<GenreLexeme> specifiants{};

	bool valide() const
	{
		return nom != "";
	}
};

template <>
struct FormatKuri<Type> {
	const Type &type;
};

std::ostream &operator<<(std::ostream &os, FormatKuri<Type> format)
{
	for (auto i = format.type.specifiants.taille() - 1; i >= 0; --i) {
		const auto spec = format.type.specifiants[i];
		os << chaine_du_lexeme(spec);
	}

	os << format.type.nom;
	return os;
}

template <>
struct FormatCPP<Type> {
	const Type &type;
};

std::ostream &operator<<(std::ostream &os, FormatCPP<Type> format)
{
	const auto &specifiants = format.type.specifiants;

	const auto est_tableau = (specifiants.taille() > 0 && specifiants.derniere() == GenreLexeme::TABLEAU);

	if (est_tableau) {
		os << "kuri::tableau<";
	}

	if (format.type.nom == "rien") {
		os << "void";
	}
	else if (format.type.nom == "chaine") {
		os << "kuri::chaine_statique";
	}
	else if (format.type.nom == "z32") {
		os << "int";
	}
	else {
		os << supprime_accents(format.type.nom);
	}

	for (auto i = format.type.specifiants.taille() - 1 - est_tableau; i >= 0; --i) {
		const auto spec = format.type.specifiants[i];
		os << chaine_du_lexeme(spec);
	}

	if (est_tableau) {
		os << ">";
	}

	return os;
}

struct Parametre {
	kuri::chaine nom = "";
	Type type{};
};

struct Fonction {
	kuri::chaine nom = "";
	kuri::chaine nom_sans_accent = "";

	kuri::tableau<Parametre> parametres{};
	Type type_sortie{};
};

struct SyntaxeuseDescription : BaseSyntaxeuse {
	kuri::tableau<Fonction> fonctions{};

	SyntaxeuseDescription(Fichier *fichier)
		: BaseSyntaxeuse(fichier)
	{}

	void analyse_une_chose() override
	{
		if (!apparie("fonction")) {
			rapporte_erreur("Attendu le mot « fonction » en début de ligne");
		}
		consomme();

		auto fonction = Fonction();

		if (!apparie(GenreLexeme::CHAINE_CARACTERE)) {
			rapporte_erreur("Attendu une chaine de caractère après « fonction »");
		}

		fonction.nom = lexeme_courant()->chaine;
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

			fonction.parametres.ajoute(parametre);

			if (apparie(GenreLexeme::VIRGULE)) {
				consomme();
			}
		}

		// consomme la parenthèse fermante
		consomme();

		// type de retour
		if (apparie(GenreLexeme::RETOUR_TYPE)) {
			consomme();
			parse_type(fonction.type_sortie);
		}

		if (apparie(GenreLexeme::POINT_VIRGULE)) {
			consomme();
		}

		fonctions.ajoute(fonction);
	}

	void parse_type(Type &type)
	{
		if (!apparie(GenreLexeme::CHAINE_CARACTERE) && !est_mot_cle(lexeme_courant()->genre)) {
			rapporte_erreur("Attendu le nom d'un type");
		}
		type.nom = lexeme_courant()->chaine;
		consomme();

		while (est_specifiant_type(lexeme_courant()->genre)) {
			if (lexeme_courant()->genre == GenreLexeme::CROCHET_OUVRANT) {
				type.specifiants.ajoute(GenreLexeme::TABLEAU);
				consomme();
				consomme(GenreLexeme::CROCHET_FERMANT, "Attendu un crochet fermant après le crochet ouvrant");
			}
			else {
				type.specifiants.ajoute(lexeme_courant()->genre);
				consomme();
			}
		}
	}

	void gere_erreur_rapportee(kuri::chaine const &message_erreur) override
	{
		std::cerr << message_erreur;
	}
};

static void genere_code_kuri(const kuri::tableau<Fonction> &fonctions, std::ostream &os)
{
	os << "// ----------------------------------------------------------------------------\n";
	os << "// Prodéclarations de types opaques pour certains types non manipulable directement\n";
	os << "\n";
	os << "EspaceDeTravail :: struct #externe;\n";
	os << "Module :: struct #externe;\n";
	os << "\n";
	os << "// ----------------------------------------------------------------------------\n";
	os << "// Interface de métaprogrammation pour controler ou communiquer avec la Compilatrice\n";
	os << "\n";

	POUR (fonctions) {
		os << it.nom << " :: fonc ";

		auto virgule = "(";

		if (it.parametres.taille() == 0) {
			os << virgule;
		}
		else {
			for (auto &param : it.parametres) {
				os << virgule << param.nom << ": " << FormatKuri<Type>{param.type};
				virgule = ", ";
			}
		}

		os << ")" << " -> " << FormatKuri<Type>{it.type_sortie} << " #compilatrice" << "\n\n";
	}
}

static void genere_code_cpp(const kuri::tableau<Fonction> &fonctions, std::ostream &os, bool pour_entete)
{
	os << "// Fichier généré automatiquement, NE PAS ÉDITER\n\n";

	if (pour_entete) {
		os << "#pragma once\n\n";
		inclus(os, "parsage/lexemes.hh");
		inclus(os, "structures/tableau.hh");
		os << "\n";
		prodeclare_struct(os, "EspaceDeTravail");
		prodeclare_struct(os, "IdentifiantCode");
		prodeclare_struct(os, "Message");
		prodeclare_struct(os, "Module");
		prodeclare_struct(os, "OptionsCompilation");
		prodeclare_struct(os, "TableIdentifiant");
		os << "\n";
		prodeclare_struct_espace(os, "chaine_statique", "kuri", "");
		os << "\n";
	}
	else {
		inclus(os, "ipa.hh");
		os << "\n";
		inclus_systeme(os, "cassert");
		os << "\n";
		inclus(os, "parsage/identifiant.hh");
		os << "\n\n";

		os << "// -----------------------------------------------------------------------------\n";
		os << "// Implémentation « symbolique » des fonctions d'interface afin d'éviter les erreurs\n";
		os << "// de liaison des programmes. Ces fonctions sont soit implémentées via Compilatrice,\n";
		os << "// soit via EspaceDeTravail, ou directement dans la MachineVirtuelle.\n";
		os << "\n";
	}

	POUR (fonctions) {
		os << FormatCPP<Type>{it.type_sortie} << ' ' << it.nom_sans_accent;

		auto virgule = "(";

		if (it.parametres.taille() == 0) {
			os << virgule;
		}
		else {
			for (auto &param : it.parametres) {
				os << virgule << FormatCPP<Type>{param.type} << ' ' << param.nom;
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

			if (it.type_sortie.nom != "rien") {
				os << "\treturn {};\n";
			}

			os << "}\n\n";
		}
	}

	// identifiants
	os << "namespace ID {\n";
	if (pour_entete) {
		POUR (fonctions) {
			os << "extern IdentifiantCode *" << it.nom_sans_accent << ";\n";
		}
	}
	else {
		POUR (fonctions) {
			os << "IdentifiantCode *" << it.nom_sans_accent << ";\n";
		}
	}
	os << "}\n\n";

	os << "void initialise_identifiants_ipa(TableIdentifiant &table)";

	if (pour_entete) {
		os << ";\n\n";
	}
	else {
		os << "\n{\n";
		POUR (fonctions) {
			os << "\tID::" << it.nom_sans_accent << " = table.identifiant_pour_chaine(\"" << it.nom << "\");\n";
		}
		os << "}\n\n";
	}

	if (pour_entete) {
		os << "using type_fonction_compilatrice = void(*)();\n\n";
	}

	os << "type_fonction_compilatrice fonction_compilatrice_pour_ident(IdentifiantCode *ident)";

	if (pour_entete) {
		os << ";\n\n";
	}
	else {
		os << "\n{\n";
		POUR (fonctions) {
			os << "\tif (ident == ID::" << it.nom_sans_accent << ") {\n";
			os << "\t\treturn reinterpret_cast<type_fonction_compilatrice>(" << it.nom_sans_accent << ");\n";
			os << "\t}\n\n";
		}

		os << "\treturn nullptr;\n";
		os << "}\n\n";
	}

	os << "bool est_fonction_compilatrice(IdentifiantCode *ident)";

	if (pour_entete) {
		os << ";\n\n";
	}
	else {
		os << "\n{\n";
		POUR (fonctions) {
			os << "\tif (ident == ID::" << it.nom_sans_accent << ") {\n";
			os << "\t\treturn true;\n";
			os << "\t}\n\n";
		}

		os << "\treturn false;\n";
		os << "}\n\n";
	}
}

int main(int argc, const char **argv)
{
	if (argc != 4) {
		std::cerr << "Utilisation: " << argv[0] << " nom_fichier_sortie -i fichier_adn\n";
		return 1;
	}

	auto nom_fichier_sortie = std::filesystem::path(argv[1]);

	const auto chemin_adn_ipa = argv[3];

	auto texte = charge_contenu_fichier(chemin_adn_ipa);
	auto donnees_fichier = DonneesConstantesFichier();
	donnees_fichier.tampon = lng::tampon_source(texte.c_str());

	auto fichier = Fichier();
	fichier.donnees_constantes = &donnees_fichier;
	fichier.donnees_constantes->chemin = chemin_adn_ipa;

	auto gerante_chaine = dls::outils::Synchrone<GeranteChaine>();
	auto table_identifiants = dls::outils::Synchrone<TableIdentifiant>();
	auto rappel_erreur = [](kuri::chaine message) {
		std::cerr << message << '\n';
	};

	auto contexte_lexage = ContexteLexage{gerante_chaine, table_identifiants, rappel_erreur};

	auto lexeuse = Lexeuse(contexte_lexage, &donnees_fichier);
	lexeuse.performe_lexage();

	if (lexeuse.possede_erreur()) {
		return 1;
	}

	auto syntaxeuse = SyntaxeuseDescription(&fichier);
	syntaxeuse.analyse();

	if (syntaxeuse.possede_erreur()) {
		return 1;
	}

	POUR (syntaxeuse.fonctions) {
		it.nom_sans_accent = supprime_accents(it.nom);
	}

	if (nom_fichier_sortie.filename() == "ipa.hh") {
		std::ofstream fichier_sortie(argv[1]);
		genere_code_cpp(syntaxeuse.fonctions, fichier_sortie, true);
	}
	else if (nom_fichier_sortie.filename() == "ipa.cc") {
		{
			std::ofstream fichier_sortie(argv[1]);
			genere_code_cpp(syntaxeuse.fonctions, fichier_sortie, false);
		}
		{
			// Génère le fichier de lexèmes pour le module Compilatrice
			// Apparemment, ce n'est pas possible de le faire via CMake
			nom_fichier_sortie.replace_filename("../modules/Compilatrice/ipa.kuri");
			std::ofstream fichier_sortie(nom_fichier_sortie);
			genere_code_kuri(syntaxeuse.fonctions, fichier_sortie);
		}
	}

	return 0;
}
