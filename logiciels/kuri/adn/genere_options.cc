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

/* Fichier de génération du code pour les options de compilation. */

#include <filesystem>
#include <fstream>

#include "biblinternes/outils/conditions.h"

#include "parsage/base_syntaxeuse.hh"
#include "parsage/gerante_chaine.hh"
#include "parsage/identifiant.hh"
#include "parsage/lexeuse.hh"
#include "parsage/modules.hh"

#include "structures/tableau.hh"

#include "outils.hh"

struct IdentifiantADN {
private:
	kuri::chaine_statique nom = "";
	kuri::chaine nom_sans_accent = "";

public:
	IdentifiantADN() = default;

	IdentifiantADN(dls::vue_chaine_compacte n)
		: nom(n)
		, nom_sans_accent(supprime_accents(nom))
	{}

	kuri::chaine_statique nom_cpp() const
	{
		return nom_sans_accent;
	}

	kuri::chaine_statique nom_kuri() const
	{
		return nom;
	}
};


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

	if (format.type.nom == "chaine_statique") {
		os << "chaine";
	}
	else {
		os << format.type.nom;
	}

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
	else if (format.type.nom == "chaine_statique") {
		os << "kuri::chaine_statique";
	}
	else if (format.type.nom == "chaine") {
		os << "kuri::chaine";
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

struct Membre {
	IdentifiantADN nom{};
	Type type{};

	bool valeur_defaut_est_acces = false;
	kuri::chaine_statique valeur_defaut = "";
};

struct DefinitionObjet {
	bool est_enum = false;

	IdentifiantADN nom{};

	kuri::tableau<Membre> membres{};
};

struct SyntaxeuseADN : public BaseSyntaxeuse {
	kuri::tableau<DefinitionObjet> objets{};

	SyntaxeuseADN(Fichier *fichier)
		: BaseSyntaxeuse(fichier)
	{}

	void analyse_une_chose() override
	{
		if (apparie("énum")) {
			parse_enum();
		}
		else if (apparie("struct")) {
			parse_struct();
		}
		else {
			rapporte_erreur("attendu la déclaration d'une structure ou d'une énumération");
		}
	}

	void parse_enum()
	{
		consomme();

		auto objet = DefinitionObjet{};
		objet.est_enum = true;

		if (!apparie(GenreLexeme::CHAINE_CARACTERE)) {
			rapporte_erreur("Attendu une chaine de caractère après « énum »");
		}
		objet.nom = lexeme_courant()->chaine;
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

			objet.membres.ajoute(membre);

			consomme();

			if (apparie(GenreLexeme::POINT_VIRGULE)) {
				consomme();
			}
		}

		consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu une accolade fermante à la fin de l'énumération");

		objets.ajoute(objet);
	}

	void parse_struct()
	{
		consomme();

		auto objet = DefinitionObjet{};

		if (!apparie(GenreLexeme::CHAINE_CARACTERE)) {
			rapporte_erreur("Attendu une chaine de caractère après « énum »");
		}
		objet.nom = lexeme_courant()->chaine;
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

			if (apparie(GenreLexeme::POINT_VIRGULE)) {
				consomme();
			}

			objet.membres.ajoute(membre);
		}

		consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu une accolade fermante à la fin de la structure");

		objets.ajoute(objet);
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

	void gere_erreur_rapportee(const kuri::chaine &message_erreur) override
	{
		std::cerr << message_erreur << "\n";
	}
};

static void genere_enum_cpp(std::ostream &os, const DefinitionObjet &objet, bool pour_entete)
{
	if (pour_entete) {
		os << "enum class " << objet.nom.nom_cpp() << " : int {\n";

		POUR (objet.membres) {
			os << "\t" << it.nom.nom_cpp() << ",\n";
		}

		os << "};\n\n";
	}

	if (!pour_entete) {
		os << "static kuri::chaine_statique chaines_membres_" << objet.nom.nom_cpp() << "["
		   << objet.membres.taille() << "] = {\n";

		POUR (objet.membres) {
			os << "\t\"" << it.nom.nom_cpp() << "\",\n";
		}

		os << "};\n\n";
	}

	os << "std::ostream &operator<<(std::ostream &os, " << objet.nom.nom_cpp() << " valeur)";

	if (pour_entete) {
		os << ";\n\n";
	}
	else {
		os << "\n";
		os << "{\n";
		os << "\treturn os << chaines_membres_" << objet.nom.nom_cpp() << "[static_cast<int>(valeur)];\n";
		os << "}\n\n";
	}

	os << "bool est_valeur_legale(" << objet.nom.nom_cpp() << " valeur)";

	if (pour_entete) {
		os << ";\n\n";
	}
	else {
		const auto &premier_membre = objet.membres[0];
		const auto &dernier_membre = objet.membres.derniere();

		os << "\n";
		os << "{\n";
		os << "\treturn valeur >= " << objet.nom.nom_cpp() << "::" << premier_membre.nom.nom_cpp()
		   << " && valeur <= " << objet.nom.nom_cpp() << "::" << dernier_membre.nom.nom_cpp()
		   << ";\n";
		os << "}\n\n";
	}
}

static void genere_struct_cpp(std::ostream &os, const DefinitionObjet &objet, bool pour_entete)
{
	if (pour_entete) {
		os << "struct " << objet.nom.nom_cpp() << " {\n";

		POUR (objet.membres) {
			os << "\t" << FormatCPP<Type>{it.type} << ' ' << it.nom.nom_cpp();

			if (it.valeur_defaut != "") {
				os << " = ";

				if (it.valeur_defaut_est_acces) {
					os << FormatCPP<Type>{it.type} << "::";
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
				os << " = {}";
			}

			os << ";\n";
		}

		os << "};\n\n";
	}

	os << "std::ostream &operator<<(std::ostream &os, " << objet.nom.nom_cpp() << " const &valeur)";

	if (pour_entete) {
		os << ";\n\n";
	}
	else {
		os << "\n";
		os << "{\n";

		os << "\tos << \"" << objet.nom.nom_cpp() << " : {\\n\";" << '\n';

		POUR (objet.membres) {
			os << "\tos << \"\\t" << it.nom.nom_cpp() << " : \" << valeur." <<  it.nom.nom_cpp() << " << '\\n';" << '\n';
		}

		os << "\tos << \"}\\n\";\n";

		os << "\treturn os;\n";
		os << "}\n\n";
	}

	os << "bool est_valide(" << objet.nom.nom_cpp() << " const &valeur)";

	if (pour_entete) {
		os << ";\n\n";
	}
	else {
		os << "\n";
		os << "{\n";

		POUR (objet.membres) {
			if (dls::outils::est_element(it.type.nom, "TypeCoulisse", "RésultatCompilation", "ArchitectureCible", "NiveauOptimisation")) {
				os << "\tif (!est_valeur_legale(valeur." << it.nom.nom_cpp() << ")) {\n";
				os << "\t\treturn false;\n";
				os << "\t}\n";
			}
		}

		os << "\treturn true;\n";
		os << "}\n\n";
	}
}

static void genere_code_cpp(std::ostream &os, kuri::tableau<DefinitionObjet> const &objets, bool pour_entete)
{
	os << "// Fichier généré automatiquement, NE PAS ÉDITER !\n\n";

	if (pour_entete) {
		os << "#pragma once\n\n";
		inclus_systeme(os, "iostream");
		os << '\n';
		inclus(os, "structures/chaine.hh");
		inclus(os, "structures/chaine_statique.hh");
		os << '\n';
	}
	else {
		inclus(os, "options.hh");
		os << '\n';
	}

	POUR (objets) {
		if (it.est_enum) {
			genere_enum_cpp(os, it, pour_entete);
		}
		else {
			genere_struct_cpp(os, it, pour_entete);
		}
	}
}

static void genere_enum_kuri(std::ostream &os, DefinitionObjet const &objet)
{
	os << objet.nom.nom_kuri() << " :: énum z32 {\n";
	POUR (objet.membres) {
		os << "\t" << it.nom.nom_kuri() << "\n";
	}
	os << "}\n\n";
}

static void genere_struct_kuri(std::ostream &os, DefinitionObjet const &objet)
{
	os << objet.nom.nom_kuri() << " :: struct {\n";
	POUR (objet.membres) {
		os << "\t" << it.nom.nom_kuri();

		if (it.valeur_defaut == "") {
			os << ": " << FormatKuri<Type>{it.type};
		}
		else {
			os << " := ";
			if (it.valeur_defaut_est_acces) {
				os << FormatKuri<Type>{it.type} << ".";
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

static void genere_code_kuri(std::ostream &os, kuri::tableau<DefinitionObjet> const &objets)
{
	POUR (objets) {
		if (it.est_enum) {
			genere_enum_kuri(os, it);
		}
		else {
			genere_struct_kuri(os, it);
		}
	}
}

int main(int argc, const char **argv)
{
	if (argc != 4) {
		std::cerr << "Utilisation: " << argv[0] << " nom_fichier_sortie -i fichier_adn\n";
		return 1;
	}

	const auto chemin_adn_ipa = argv[3];
	auto nom_fichier_sortie = std::filesystem::path(argv[1]);

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

	auto syntaxeuse = SyntaxeuseADN(&fichier);
	syntaxeuse.analyse();

	if (syntaxeuse.possede_erreur()) {
		return 1;
	}

	if (nom_fichier_sortie.filename() == "options.hh") {
		std::ofstream fichier_sortie(argv[1]);
		genere_code_cpp(fichier_sortie, syntaxeuse.objets, true);
	}
	else if (nom_fichier_sortie.filename() == "options.cc") {
		{
			std::ofstream fichier_sortie(argv[1]);
			genere_code_cpp(fichier_sortie, syntaxeuse.objets, false);
		}
		{
			// Génère le fichier de lexèmes pour le module Compilatrice
			// Apparemment, ce n'est pas possible de le faire via CMake
			nom_fichier_sortie.replace_filename("../modules/Compilatrice/options.kuri");
			std::ofstream fichier_sortie(nom_fichier_sortie);
			genere_code_kuri(fichier_sortie, syntaxeuse.objets);
		}
	}

	return 0;
}
