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

/* Fichier de génération du code pour les lexèmes. */

#include <filesystem>
#include <fstream>

#include "biblinternes/outils/definitions.h"

#include "structures/chaine.hh"
#include "structures/tableau.hh"

struct Lexeme {
	kuri::chaine_statique chaine = "";
	kuri::chaine_statique nom_enum = "";

	kuri::chaine nom_enum_sans_accent = "";

	bool est_mot_cle = false;
};

struct ListeLexemes {
	kuri::tableau<Lexeme> lexemes{};

	void ajoute_mot_cle(kuri::chaine_statique chaine)
	{
		auto lexeme = Lexeme{};
		lexeme.chaine = chaine;
		lexeme.est_mot_cle = true;

		lexemes.ajoute(lexeme);
	}

	void ajoute_ponctuation(kuri::chaine_statique chaine, kuri::chaine_statique nom_enum)
	{
		auto lexeme = Lexeme{};
		lexeme.chaine = chaine;
		lexeme.nom_enum = nom_enum;

		lexemes.ajoute(lexeme);
	}

	void ajoute_extra(kuri::chaine_statique chaine, kuri::chaine_statique nom_enum)
	{
		auto lexeme = Lexeme{};
		lexeme.chaine = chaine;
		lexeme.nom_enum = nom_enum;

		lexemes.ajoute(lexeme);
	}
};

static void construit_lexemes(ListeLexemes &lexemes)
{
	lexemes.ajoute_mot_cle("arrête");
	lexemes.ajoute_mot_cle("bool");
	lexemes.ajoute_mot_cle("boucle");
	lexemes.ajoute_mot_cle("chaine");
	lexemes.ajoute_mot_cle("charge");
	lexemes.ajoute_mot_cle("comme");
	lexemes.ajoute_mot_cle("continue");
	lexemes.ajoute_mot_cle("corout");
	lexemes.ajoute_mot_cle("dans");
	lexemes.ajoute_mot_cle("diffère");
	lexemes.ajoute_mot_cle("discr");
	lexemes.ajoute_mot_cle("dyn");
	lexemes.ajoute_mot_cle("définis");
	lexemes.ajoute_mot_cle("eini");
	lexemes.ajoute_mot_cle("eini_erreur");
	lexemes.ajoute_mot_cle("empl");
	lexemes.ajoute_mot_cle("erreur");
	lexemes.ajoute_mot_cle("externe");
	lexemes.ajoute_mot_cle("faux");
	lexemes.ajoute_mot_cle("fonc");
	lexemes.ajoute_mot_cle("garde");
	lexemes.ajoute_mot_cle("importe");
	lexemes.ajoute_mot_cle("info_de");
	lexemes.ajoute_mot_cle("init_de");
	lexemes.ajoute_mot_cle("mémoire");
	lexemes.ajoute_mot_cle("n16");
	lexemes.ajoute_mot_cle("n32");
	lexemes.ajoute_mot_cle("n64");
	lexemes.ajoute_mot_cle("n8");
	lexemes.ajoute_mot_cle("nonatteignable");
	lexemes.ajoute_mot_cle("nonsûr");
	lexemes.ajoute_mot_cle("nul");
	lexemes.ajoute_mot_cle("octet");
	lexemes.ajoute_mot_cle("opérateur");
	lexemes.ajoute_mot_cle("piège");
	lexemes.ajoute_mot_cle("pour");
	lexemes.ajoute_mot_cle("pousse_contexte");
	lexemes.ajoute_mot_cle("r16");
	lexemes.ajoute_mot_cle("r32");
	lexemes.ajoute_mot_cle("r64");
	lexemes.ajoute_mot_cle("reprends");
	lexemes.ajoute_mot_cle("retiens");
	lexemes.ajoute_mot_cle("retourne");
	lexemes.ajoute_mot_cle("rien");
	lexemes.ajoute_mot_cle("répète");
	lexemes.ajoute_mot_cle("sansarrêt");
	lexemes.ajoute_mot_cle("saufsi");
	lexemes.ajoute_mot_cle("si");
	lexemes.ajoute_mot_cle("sinon");
	lexemes.ajoute_mot_cle("struct");
	lexemes.ajoute_mot_cle("taille_de");
	lexemes.ajoute_mot_cle("tantque");
	lexemes.ajoute_mot_cle("tente");
	lexemes.ajoute_mot_cle("type_de");
	lexemes.ajoute_mot_cle("type_de_données");
	lexemes.ajoute_mot_cle("union");
	lexemes.ajoute_mot_cle("vrai");
	lexemes.ajoute_mot_cle("z16");
	lexemes.ajoute_mot_cle("z32");
	lexemes.ajoute_mot_cle("z64");
	lexemes.ajoute_mot_cle("z8");
	lexemes.ajoute_mot_cle("énum");
	lexemes.ajoute_mot_cle("énum_drapeau");
	lexemes.ajoute_ponctuation("!", "EXCLAMATION");
	lexemes.ajoute_ponctuation("\"", "GUILLEMET");
	lexemes.ajoute_ponctuation("#", "DIRECTIVE");
	lexemes.ajoute_ponctuation("$", "DOLLAR");
	lexemes.ajoute_ponctuation("%", "POURCENT");
	lexemes.ajoute_ponctuation("&", "ESPERLUETTE");
	lexemes.ajoute_ponctuation("'", "APOSTROPHE");
	lexemes.ajoute_ponctuation("(", "PARENTHESE_OUVRANTE");
	lexemes.ajoute_ponctuation(")", "PARENTHESE_FERMANTE");
	lexemes.ajoute_ponctuation("*", "FOIS");
	lexemes.ajoute_ponctuation("+", "PLUS");
	lexemes.ajoute_ponctuation(",", "VIRGULE");
	lexemes.ajoute_ponctuation("-", "MOINS");
	lexemes.ajoute_ponctuation(".", "POINT");
	lexemes.ajoute_ponctuation("/", "DIVISE");
	lexemes.ajoute_ponctuation(":", "DOUBLE_POINTS");
	lexemes.ajoute_ponctuation(";", "POINT_VIRGULE");
	lexemes.ajoute_ponctuation("<", "INFERIEUR");
	lexemes.ajoute_ponctuation("=", "EGAL");
	lexemes.ajoute_ponctuation(">", "SUPERIEUR");
	lexemes.ajoute_ponctuation("@", "AROBASE");
	lexemes.ajoute_ponctuation("[", "CROCHET_OUVRANT");
	lexemes.ajoute_ponctuation("]", "CROCHET_FERMANT");
	lexemes.ajoute_ponctuation("^", "CHAPEAU");
	lexemes.ajoute_ponctuation("{", "ACCOLADE_OUVRANTE");
	lexemes.ajoute_ponctuation("|", "BARRE");
	lexemes.ajoute_ponctuation("}", "ACCOLADE_FERMANTE");
	lexemes.ajoute_ponctuation("~", "TILDE");
	lexemes.ajoute_ponctuation("!=", "DIFFERENCE");
	lexemes.ajoute_ponctuation("%=", "MODULO_EGAL");
	lexemes.ajoute_ponctuation("&&", "ESP_ESP");
	lexemes.ajoute_ponctuation("&=", "ET_EGAL");
	lexemes.ajoute_ponctuation("*/", "FIN_BLOC_COMMENTAIRE");
	lexemes.ajoute_ponctuation("*=", "MULTIPLIE_EGAL");
	lexemes.ajoute_ponctuation("+=", "PLUS_EGAL");
	lexemes.ajoute_ponctuation("-=", "MOINS_EGAL");
	lexemes.ajoute_ponctuation("->", "RETOUR_TYPE");
	lexemes.ajoute_ponctuation("/*", "DEBUT_BLOC_COMMENTAIRE");
	lexemes.ajoute_ponctuation("//", "DEBUT_LIGNE_COMMENTAIRE");
	lexemes.ajoute_ponctuation("/=", "DIVISE_EGAL");
	lexemes.ajoute_ponctuation("::", "DECLARATION_CONSTANTE");
	lexemes.ajoute_ponctuation(":=", "DECLARATION_VARIABLE");
	lexemes.ajoute_ponctuation("<<", "DECALAGE_GAUCHE");
	lexemes.ajoute_ponctuation("<=", "INFERIEUR_EGAL");
	lexemes.ajoute_ponctuation("==", "EGALITE");
	lexemes.ajoute_ponctuation(">=", "SUPERIEUR_EGAL");
	lexemes.ajoute_ponctuation(">>", "DECALAGE_DROITE");
	lexemes.ajoute_ponctuation("^=", "OUX_EGAL");
	lexemes.ajoute_ponctuation("|=", "OU_EGAL");
	lexemes.ajoute_ponctuation("||", "BARRE_BARRE");
	lexemes.ajoute_ponctuation("---", "NON_INITIALISATION");
	lexemes.ajoute_ponctuation("...", "TROIS_POINTS");
	lexemes.ajoute_ponctuation("<<=", "DEC_GAUCHE_EGAL");
	lexemes.ajoute_ponctuation(">>=", "DEC_DROITE_EGAL");
	lexemes.ajoute_extra("", "NOMBRE_REEL");
	lexemes.ajoute_extra("", "NOMBRE_ENTIER");
	lexemes.ajoute_extra("-", "PLUS_UNAIRE");
	lexemes.ajoute_extra("+", "MOINS_UNAIRE");
	lexemes.ajoute_extra("*", "FOIS_UNAIRE");
	lexemes.ajoute_extra("&", "ESP_UNAIRE");
	lexemes.ajoute_extra("", "CHAINE_CARACTERE");
	lexemes.ajoute_extra("", "CHAINE_LITTERALE");
	lexemes.ajoute_extra("", "CARACTÈRE");
	lexemes.ajoute_extra("*", "POINTEUR");
	lexemes.ajoute_extra("", "TABLEAU");
	lexemes.ajoute_extra("&", "REFERENCE");
	lexemes.ajoute_extra("", "CARACTERE_BLANC");
	lexemes.ajoute_extra("// commentaire", "COMMENTAIRE");
	lexemes.ajoute_extra("...", "EXPANSION_VARIADIQUE");
	lexemes.ajoute_extra("...", "INCONNU");
}

static void remplace(std::string &std_string, std::string_view motif, std::string_view remplacement)
{
	size_t index = 0;
	while (true) {
		/* Locate the substring to replace. */
		index = std_string.find(motif, index);
		if (index == std::string::npos) break;

		/* Make the replacement. */
		std_string.replace(index, motif.size(), remplacement);

		/* Advance index forward so the next iteration doesn't pick it up as well. */
		index += motif.size();
	}
}

static kuri::chaine supprime_accents(kuri::chaine_statique avec_accent)
{
	auto std_string = std::string(avec_accent.pointeur(), static_cast<size_t>(avec_accent.taille()));

	remplace(std_string, "é", "e");
	remplace(std_string, "è", "e");
	remplace(std_string, "ê", "e");
	remplace(std_string, "û", "u");
	remplace(std_string, "É", "E");
	remplace(std_string, "È", "E");
	remplace(std_string, "Ê", "E");

	return kuri::chaine(std_string.c_str(), static_cast<long>(std_string.size()));
}

static void construit_nom_enums(ListeLexemes &lexemes)
{
	POUR (lexemes.lexemes) {
		if (it.nom_enum == "") {
			auto chaine = supprime_accents(it.chaine);

			for (auto &c : chaine) {
				if (c >= 'a' && c <= 'z') {
					c = (static_cast<char>(c - 0x20));
				}
			}

			it.nom_enum_sans_accent = chaine;
		}
		else {
			it.nom_enum_sans_accent = supprime_accents(it.nom_enum);
		}
	}
}

static void genere_enum(const ListeLexemes &lexemes, std::ostream &os)
{
	os << "enum class GenreLexeme : unsigned int {\n";
	POUR (lexemes.lexemes) {
		os << "\t" << it.nom_enum_sans_accent << ",\n";
	}
	os << "};\n";
}

static void genere_est_mot_cle(const ListeLexemes &lexemes, std::ostream &os)
{
	os << "bool est_mot_cle(GenreLexeme genre)\n";
	os << "{\n";
	os << "\tswitch (genre) {\n";
	os << "\t\tdefault:\n";
	os << "\t\t{\n";
	os << "\t\t\treturn false;\n";
	os << "\t\t}\n";
	POUR (lexemes.lexemes) {
		if (!it.est_mot_cle) {
			continue;
		}
		os << "\t\tcase GenreLexeme::" << it.nom_enum_sans_accent << ":\n";
	}
	os << "\t\t{\n";
	os << "\t\t\treturn true;\n";
	os << "\t\t}\n";
	os << "\t}\n";
	os << "}\n\n";

	os << "bool est_mot_cle(const Lexeme &lexeme)\n";
	os << "{\n";
	os << "\treturn est_mot_cle(lexeme.genre);\n";
	os << "}\n\n";
}

static void genere_impression_lexeme(const ListeLexemes &lexemes, std::ostream &os)
{
	os << "static kuri::chaine_statique noms_genres_lexemes[" << lexemes.lexemes.taille() << "] = {\n";
	POUR (lexemes.lexemes) {
		os << "\t" << '"' << it.nom_enum_sans_accent << '"' << ",\n";
	}
	os << "};\n\n";

	os << "static kuri::chaine_statique chaines_lexemes[" << lexemes.lexemes.taille() << "] = {\n";
	POUR (lexemes.lexemes) {
		if (it.chaine == "\"") {
			os << "\t\"\\\"\",\n";
		}
		else {
			os << "\t" << '"' << it.chaine << '"' << ",\n";
		}
	}
	os << "};\n\n";

	os << "std::ostream &operator<<(std::ostream &os, GenreLexeme genre)\n";
	os << "{\n";
	os << "\treturn os << noms_genres_lexemes[static_cast<int>(genre)];\n";
	os << "}\n\n";

	os << "kuri::chaine_statique chaine_du_genre_de_lexeme(GenreLexeme genre)\n";
	os << "{\n";
	os << "\treturn noms_genres_lexemes[static_cast<int>(genre)];\n";
	os << "}\n\n";

	os << "kuri::chaine_statique chaine_du_lexeme(GenreLexeme genre)\n";
	os << "{\n";
	os << "\treturn chaines_lexemes[static_cast<int>(genre)];\n";
	os << "}\n\n";
}

static void genere_fichier_entete(const ListeLexemes &lexemes, std::ostream &os)
{
	os << "#pragma once\n";
	os << '\n';
	os << "#include <iostream>\n";
	os << "#include \"biblinternes/structures/chaine.hh\"\n";
	os << '\n';
	os << "struct IdentifiantCode;\n";
	os << '\n';
	genere_enum(lexemes, os);

	const char *declarations = R"(
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
struct Lexeme {
	dls::vue_chaine_compacte chaine;

	union {
		unsigned long long valeur_entiere;
		double valeur_reelle;
		long index_chaine;
		IdentifiantCode *ident;
	};

	GenreLexeme genre;
	int fichier = 0;
	int ligne = 0;
	int colonne = 0;
};
#pragma GCC diagnostic pop

namespace kuri {
struct chaine_statique;
}

bool est_mot_cle(GenreLexeme genre);
bool est_mot_cle(const Lexeme &lexeme);
std::ostream &operator<<(std::ostream &os, GenreLexeme genre);
kuri::chaine_statique chaine_du_genre_de_lexeme(GenreLexeme id);
kuri::chaine_statique chaine_du_lexeme(GenreLexeme genre);
)";

	os << declarations;
}

static void genere_fichier_source(const ListeLexemes &lexemes, std::ostream &os)
{
	os << "#include \"lexemes.hh\"\n";
	os << "#include \"structures/chaine_statique.hh\"\n";
	genere_impression_lexeme(lexemes, os);
	genere_est_mot_cle(lexemes, os);
}

static void genere_fichier_kuri(const ListeLexemes &lexemes, std::ostream &os)
{
	os << "/* Fichier générer automatiquement, NE PAS ÉDITER ! */\n\n";
	os << "GenreLexème :: énum n32 {\n";
	POUR (lexemes.lexemes) {
		os << "\t" << it.nom_enum_sans_accent << '\n';
	}
	os << "}\n\n";
	os << "Lexème :: struct {\n";
	os << "\tgenre: GenreLexème\n";
	os << "\ttexte: chaine\n";
	os << "}\n\n";
	os << "est_mot_clé :: fonc (genre: GenreLexème) -> bool\n";
	os << "{\n";
	os << "\tdiscr genre {\n";
	auto virgule = "\t\t";
	POUR (lexemes.lexemes) {
		if (!it.est_mot_cle) {
			continue;
		}

		os << virgule << it.nom_enum_sans_accent;
		virgule = ",\n\t\t";
	}
	os << " { retourne vrai; }\n";
	os << "\t\tsinon { retourne faux; }\n";
	os << "\t}\n";
	os << "}\n\n";
}

int main(int argc, const char **argv)
{
	if (argc != 2) {
		std::cerr << "Utilisation: " << argv[0] << " nom_fichier_sortie\n";
		return 1;
	}

	auto nom_fichier_sortie = std::filesystem::path(argv[1]);

	auto lexemes = ListeLexemes{};
	construit_lexemes(lexemes);
	construit_nom_enums(lexemes);

	if (nom_fichier_sortie.filename() == "lexemes.cc") {
		{
			std::ofstream fichier_sortie(argv[1]);
			genere_fichier_source(lexemes, fichier_sortie);
		}
		{
			// Génère le fichier de lexèmes pour le module Compilatrice
			// Apparemment, ce n'est pas possible de le faire via CMake
			nom_fichier_sortie.replace_filename("../modules/Compilatrice/lexèmes.kuri");
			std::ofstream fichier_sortie(nom_fichier_sortie);
			genere_fichier_kuri(lexemes, fichier_sortie);
		}
	}
	else if (nom_fichier_sortie.filename() == "lexemes.hh") {
		std::ofstream fichier_sortie(argv[1]);
		genere_fichier_entete(lexemes, fichier_sortie);
	}
	else {
		std::cerr << "Fichier de sortie « " << argv[1] << " » inconnu !\n";
		return 1;
	}

	return 0;
}
