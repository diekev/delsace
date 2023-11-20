/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

/* Fichier de génération du code pour les lexèmes. */

#include <fstream>
#include <iostream>

#include "biblinternes/outils/definitions.h"

#include "structures/chaine.hh"
#include "structures/chemin_systeme.hh"
#include "structures/tableau.hh"

#include "outils_independants_des_lexemes.hh"

enum {
    EST_MOT_CLE = (1 << 0),
    EST_ASSIGNATION_COMPOSEE = (1 << 1),
    EST_OPERATEUR_BOOL = (1 << 2),
    EST_OPERATEUR_COMPARAISON = (1 << 3),
    EST_CHAINE_LITTERALE = (1 << 4),
    EST_SPECIFIANT_TYPE = (1 << 5),
    EST_IDENTIFIANT_TYPE = (1 << 6),
    EST_OPERATEUR_UNAIRE = (1 << 7),
};

struct Lexeme {
    kuri::chaine_statique chaine = "";
    kuri::chaine_statique nom_enum = "";

    kuri::chaine nom_enum_sans_accent = "";

    uint32_t drapeaux = 0;
};

struct ListeLexemes {
    kuri::tableau<Lexeme> lexemes{};

    void ajoute_mot_cle(kuri::chaine_statique chaine, uint32_t drapeaux = 0)
    {
        auto lexeme = Lexeme{};
        lexeme.chaine = chaine;
        lexeme.drapeaux = EST_MOT_CLE | drapeaux;

        lexemes.ajoute(lexeme);
    }

    void ajoute_ponctuation(kuri::chaine_statique chaine,
                            kuri::chaine_statique nom_enum,
                            uint32_t drapeaux = 0)
    {
        auto lexeme = Lexeme{};
        lexeme.chaine = chaine;
        lexeme.nom_enum = nom_enum;
        lexeme.drapeaux = drapeaux;

        lexemes.ajoute(lexeme);
    }

    void ajoute_extra(kuri::chaine_statique chaine,
                      kuri::chaine_statique nom_enum,
                      uint32_t drapeaux = 0)
    {
        auto lexeme = Lexeme{};
        lexeme.chaine = chaine;
        lexeme.nom_enum = nom_enum;
        lexeme.drapeaux = drapeaux;

        lexemes.ajoute(lexeme);
    }
};

static void construit_lexemes(ListeLexemes &lexemes)
{
    lexemes.ajoute_mot_cle("arrête");
    lexemes.ajoute_mot_cle("bool", EST_IDENTIFIANT_TYPE);
    lexemes.ajoute_mot_cle("boucle");
    lexemes.ajoute_mot_cle("chaine", EST_IDENTIFIANT_TYPE);
    lexemes.ajoute_mot_cle("charge");
    lexemes.ajoute_mot_cle("comme");
    lexemes.ajoute_mot_cle("continue");
    lexemes.ajoute_mot_cle("corout");
    lexemes.ajoute_mot_cle("dans");
    lexemes.ajoute_mot_cle("diffère");
    lexemes.ajoute_mot_cle("discr");
    lexemes.ajoute_mot_cle("dyn");
    lexemes.ajoute_mot_cle("définis");
    lexemes.ajoute_mot_cle("eini", EST_IDENTIFIANT_TYPE);
    lexemes.ajoute_mot_cle("eini_erreur", EST_IDENTIFIANT_TYPE);
    lexemes.ajoute_mot_cle("empl");
    lexemes.ajoute_mot_cle("erreur");
    lexemes.ajoute_mot_cle("faux");
    lexemes.ajoute_mot_cle("fonc");
    lexemes.ajoute_mot_cle("garde");
    lexemes.ajoute_mot_cle("importe");
    lexemes.ajoute_mot_cle("info_de");
    lexemes.ajoute_mot_cle("init_de");
    lexemes.ajoute_mot_cle("mémoire");
    lexemes.ajoute_mot_cle("n16", EST_IDENTIFIANT_TYPE);
    lexemes.ajoute_mot_cle("n32", EST_IDENTIFIANT_TYPE);
    lexemes.ajoute_mot_cle("n64", EST_IDENTIFIANT_TYPE);
    lexemes.ajoute_mot_cle("n8", EST_IDENTIFIANT_TYPE);
    lexemes.ajoute_mot_cle("nonatteignable");
    lexemes.ajoute_mot_cle("nonsûr");
    lexemes.ajoute_mot_cle("nul");
    lexemes.ajoute_mot_cle("octet", EST_IDENTIFIANT_TYPE);
    lexemes.ajoute_mot_cle("opérateur");
    lexemes.ajoute_mot_cle("piège");
    lexemes.ajoute_mot_cle("pour");
    lexemes.ajoute_mot_cle("pousse_contexte");
    lexemes.ajoute_mot_cle("r16", EST_IDENTIFIANT_TYPE);
    lexemes.ajoute_mot_cle("r32", EST_IDENTIFIANT_TYPE);
    lexemes.ajoute_mot_cle("r64", EST_IDENTIFIANT_TYPE);
    lexemes.ajoute_mot_cle("reprends");
    lexemes.ajoute_mot_cle("retiens");
    lexemes.ajoute_mot_cle("retourne");
    lexemes.ajoute_mot_cle("rien", EST_IDENTIFIANT_TYPE);
    lexemes.ajoute_mot_cle("répète");
    lexemes.ajoute_mot_cle("sansarrêt");
    lexemes.ajoute_mot_cle("saufsi");
    lexemes.ajoute_mot_cle("si");
    lexemes.ajoute_mot_cle("sinon");
    lexemes.ajoute_mot_cle("struct");
    lexemes.ajoute_mot_cle("taille_de");
    lexemes.ajoute_mot_cle("tantque");
    lexemes.ajoute_mot_cle("tente");
    lexemes.ajoute_mot_cle("type_de", EST_SPECIFIANT_TYPE);
    lexemes.ajoute_mot_cle("type_de_données", EST_IDENTIFIANT_TYPE);
    lexemes.ajoute_mot_cle("union");
    lexemes.ajoute_mot_cle("vrai");
    lexemes.ajoute_mot_cle("z16", EST_IDENTIFIANT_TYPE);
    lexemes.ajoute_mot_cle("z32", EST_IDENTIFIANT_TYPE);
    lexemes.ajoute_mot_cle("z64", EST_IDENTIFIANT_TYPE);
    lexemes.ajoute_mot_cle("z8", EST_IDENTIFIANT_TYPE);
    lexemes.ajoute_mot_cle("énum");
    lexemes.ajoute_mot_cle("énum_drapeau");
    lexemes.ajoute_ponctuation("!", "EXCLAMATION", EST_OPERATEUR_UNAIRE | EST_OPERATEUR_BOOL);
    lexemes.ajoute_ponctuation("\"", "GUILLEMET");
    lexemes.ajoute_ponctuation("#", "DIRECTIVE");
    lexemes.ajoute_ponctuation("$", "DOLLAR", EST_SPECIFIANT_TYPE);
    lexemes.ajoute_ponctuation("%", "POURCENT");
    lexemes.ajoute_ponctuation("&", "ESPERLUETTE", EST_SPECIFIANT_TYPE);
    lexemes.ajoute_ponctuation("'", "APOSTROPHE");
    lexemes.ajoute_ponctuation("(", "PARENTHESE_OUVRANTE");
    lexemes.ajoute_ponctuation(")", "PARENTHESE_FERMANTE");
    lexemes.ajoute_ponctuation("*", "FOIS", EST_SPECIFIANT_TYPE);
    lexemes.ajoute_ponctuation("+", "PLUS");
    lexemes.ajoute_ponctuation(",", "VIRGULE");
    lexemes.ajoute_ponctuation("-", "MOINS");
    lexemes.ajoute_ponctuation(".", "POINT");
    lexemes.ajoute_ponctuation("/", "DIVISE");
    lexemes.ajoute_ponctuation(":", "DOUBLE_POINTS");
    lexemes.ajoute_ponctuation(";", "POINT_VIRGULE");
    lexemes.ajoute_ponctuation("<", "INFERIEUR", EST_OPERATEUR_COMPARAISON | EST_OPERATEUR_BOOL);
    lexemes.ajoute_ponctuation("=", "EGAL");
    lexemes.ajoute_ponctuation(">", "SUPERIEUR", EST_OPERATEUR_COMPARAISON | EST_OPERATEUR_BOOL);
    lexemes.ajoute_ponctuation("@", "AROBASE");
    lexemes.ajoute_ponctuation("[", "CROCHET_OUVRANT", EST_OPERATEUR_UNAIRE | EST_SPECIFIANT_TYPE);
    lexemes.ajoute_ponctuation("]", "CROCHET_FERMANT");
    lexemes.ajoute_ponctuation("^", "CHAPEAU");
    lexemes.ajoute_ponctuation("{", "ACCOLADE_OUVRANTE");
    lexemes.ajoute_ponctuation("|", "BARRE");
    lexemes.ajoute_ponctuation("}", "ACCOLADE_FERMANTE");
    lexemes.ajoute_ponctuation("~", "TILDE", EST_OPERATEUR_UNAIRE);
    lexemes.ajoute_ponctuation("!=", "DIFFERENCE", EST_OPERATEUR_COMPARAISON | EST_OPERATEUR_BOOL);
    lexemes.ajoute_ponctuation("%=", "MODULO_EGAL", EST_ASSIGNATION_COMPOSEE);
    lexemes.ajoute_ponctuation("&&", "ESP_ESP", EST_OPERATEUR_BOOL);
    lexemes.ajoute_ponctuation("&=", "ET_EGAL", EST_ASSIGNATION_COMPOSEE);
    lexemes.ajoute_ponctuation("*/", "FIN_BLOC_COMMENTAIRE");
    lexemes.ajoute_ponctuation("*=", "MULTIPLIE_EGAL", EST_ASSIGNATION_COMPOSEE);
    lexemes.ajoute_ponctuation("+=", "PLUS_EGAL", EST_ASSIGNATION_COMPOSEE);
    lexemes.ajoute_ponctuation("-=", "MOINS_EGAL", EST_ASSIGNATION_COMPOSEE);
    lexemes.ajoute_ponctuation("->", "RETOUR_TYPE");
    lexemes.ajoute_ponctuation("/*", "DEBUT_BLOC_COMMENTAIRE");
    lexemes.ajoute_ponctuation("//", "DEBUT_LIGNE_COMMENTAIRE");
    lexemes.ajoute_ponctuation("/=", "DIVISE_EGAL", EST_ASSIGNATION_COMPOSEE);
    lexemes.ajoute_ponctuation("::", "DECLARATION_CONSTANTE");
    lexemes.ajoute_ponctuation(":=", "DECLARATION_VARIABLE");
    lexemes.ajoute_ponctuation("`", "ACCENT_GRAVE", EST_OPERATEUR_UNAIRE);
    lexemes.ajoute_ponctuation("<<", "DECALAGE_GAUCHE");
    lexemes.ajoute_ponctuation(
        "<=", "INFERIEUR_EGAL", EST_OPERATEUR_COMPARAISON | EST_OPERATEUR_BOOL);
    lexemes.ajoute_ponctuation("==", "EGALITE", EST_OPERATEUR_COMPARAISON | EST_OPERATEUR_BOOL);
    lexemes.ajoute_ponctuation(
        ">=", "SUPERIEUR_EGAL", EST_OPERATEUR_COMPARAISON | EST_OPERATEUR_BOOL);
    lexemes.ajoute_ponctuation(">>", "DECALAGE_DROITE");
    lexemes.ajoute_ponctuation("^=", "OUX_EGAL", EST_ASSIGNATION_COMPOSEE);
    lexemes.ajoute_ponctuation("|=", "OU_EGAL", EST_ASSIGNATION_COMPOSEE);
    lexemes.ajoute_ponctuation("||", "BARRE_BARRE", EST_OPERATEUR_BOOL);
    lexemes.ajoute_ponctuation("---", "NON_INITIALISATION");
    lexemes.ajoute_ponctuation("...", "TROIS_POINTS", EST_SPECIFIANT_TYPE);
    lexemes.ajoute_ponctuation("<<=", "DEC_GAUCHE_EGAL", EST_ASSIGNATION_COMPOSEE);
    lexemes.ajoute_ponctuation(">>=", "DEC_DROITE_EGAL", EST_ASSIGNATION_COMPOSEE);
    lexemes.ajoute_extra("", "NOMBRE_REEL");
    lexemes.ajoute_extra("", "NOMBRE_ENTIER");
    lexemes.ajoute_extra("-", "PLUS_UNAIRE", EST_OPERATEUR_UNAIRE);
    lexemes.ajoute_extra("+", "MOINS_UNAIRE", EST_OPERATEUR_UNAIRE);
    lexemes.ajoute_extra("*", "FOIS_UNAIRE", EST_OPERATEUR_UNAIRE);
    lexemes.ajoute_extra("&", "ESP_UNAIRE", EST_OPERATEUR_UNAIRE);
    lexemes.ajoute_extra("", "CHAINE_CARACTERE", EST_IDENTIFIANT_TYPE);
    lexemes.ajoute_extra("", "CHAINE_LITTERALE", EST_CHAINE_LITTERALE);
    lexemes.ajoute_extra("", "CARACTÈRE", EST_CHAINE_LITTERALE);
    lexemes.ajoute_extra("*", "POINTEUR");
    lexemes.ajoute_extra("[]", "TABLEAU", EST_OPERATEUR_UNAIRE);
    lexemes.ajoute_extra("&", "REFERENCE");
    lexemes.ajoute_extra("", "CARACTERE_BLANC");
    lexemes.ajoute_extra("// commentaire", "COMMENTAIRE");
    lexemes.ajoute_extra("...", "EXPANSION_VARIADIQUE");
    lexemes.ajoute_extra("...", "INCONNU");
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
    os << "enum class GenreLexeme : uint32_t {\n";
    POUR (lexemes.lexemes) {
        os << "\t" << it.nom_enum_sans_accent << ",\n";
    }
    os << "};\n";
}

static void genere_fonction_cpp_pour_drapeau(const ListeLexemes &lexemes,
                                             kuri::chaine_statique nom,
                                             uint32_t drapeau,
                                             std::ostream &os)
{
    os << "bool " << nom << "(GenreLexeme genre)\n";
    os << "{\n";
    os << "\tswitch (genre) {\n";
    os << "\t\tdefault:\n";
    os << "\t\t{\n";
    os << "\t\t\treturn false;\n";
    os << "\t\t}\n";
    POUR (lexemes.lexemes) {
        if ((it.drapeaux & drapeau) == 0) {
            continue;
        }
        os << "\t\tcase GenreLexeme::" << it.nom_enum_sans_accent << ":\n";
    }
    os << "\t\t{\n";
    os << "\t\t\treturn true;\n";
    os << "\t\t}\n";
    os << "\t}\n";
    os << "}\n\n";

    os << "bool " << nom << "(const Lexeme &lexeme)\n";
    os << "{\n";
    os << "\treturn " << nom << "(lexeme.genre);\n";
    os << "}\n\n";
}

static void genere_impression_lexeme(const ListeLexemes &lexemes, std::ostream &os)
{
    os << "static kuri::chaine_statique noms_genres_lexemes[" << lexemes.lexemes.taille()
       << "] = {\n";
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

    os << "kuri::chaine_statique chaine_du_genre_de_lexème(GenreLexeme genre)\n";
    os << "{\n";
    os << "\treturn noms_genres_lexemes[static_cast<int>(genre)];\n";
    os << "}\n\n";

    os << "kuri::chaine_statique chaine_du_lexème(GenreLexeme genre)\n";
    os << "{\n";
    os << "\treturn chaines_lexemes[static_cast<int>(genre)];\n";
    os << "}\n\n";
}

static void genere_fichier_entete(const ListeLexemes &lexemes, std::ostream &os)
{
    os << "#pragma once\n";
    os << '\n';
    inclus_systeme(os, "iosfwd");
    inclus(os, "biblinternes/structures/chaine.hh");
    os << '\n';
    prodeclare_struct(os, "IdentifiantCode");
    os << '\n';
    genere_enum(lexemes, os);

    const char *declarations = R"(
#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#endif
struct Lexeme {
    dls::vue_chaine_compacte chaine{};

	union {
		uint64_t valeur_entiere;
		double valeur_reelle;
		int64_t index_chaine;
		IdentifiantCode *ident;
	};

    GenreLexeme genre{};
	int fichier = 0;
	int ligne = 0;
	int colonne = 0;
};
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

namespace kuri {
struct chaine_statique;
}

bool est_mot_clé(GenreLexeme genre);
bool est_mot_clé(const Lexeme &lexeme);
bool est_assignation_composée(GenreLexeme genre);
bool est_assignation_composée(const Lexeme &lexeme);
bool est_opérateur_bool(GenreLexeme genre);
bool est_opérateur_bool(const Lexeme &lexeme);
bool est_opérateur_comparaison(GenreLexeme genre);
bool est_opérateur_comparaison(const Lexeme &lexeme);
bool est_chaine_littérale(GenreLexeme genre);
bool est_chaine_littérale(const Lexeme &lexeme);
bool est_spécifiant_type(GenreLexeme genre);
bool est_spécifiant_type(const Lexeme &lexeme);
bool est_identifiant_type(GenreLexeme genre);
bool est_identifiant_type(const Lexeme &lexeme);
bool est_opérateur_unaire(GenreLexeme genre);
bool est_opérateur_unaire(const Lexeme &lexeme);
std::ostream &operator<<(std::ostream &os, GenreLexeme genre);
kuri::chaine_statique chaine_du_genre_de_lexème(GenreLexeme id);
kuri::chaine_statique chaine_du_lexème(GenreLexeme genre);
)";

    os << declarations;
}

static void genere_fichier_source(const ListeLexemes &lexemes, std::ostream &os)
{
    inclus(os, "lexemes.hh");
    inclus_systeme(os, "iostream");
    inclus(os, "structures/chaine_statique.hh");
    genere_impression_lexeme(lexemes, os);
    genere_fonction_cpp_pour_drapeau(lexemes, "est_mot_clé", EST_MOT_CLE, os);
    genere_fonction_cpp_pour_drapeau(
        lexemes, "est_assignation_composée", EST_ASSIGNATION_COMPOSEE, os);
    genere_fonction_cpp_pour_drapeau(lexemes, "est_opérateur_bool", EST_OPERATEUR_BOOL, os);
    genere_fonction_cpp_pour_drapeau(
        lexemes, "est_opérateur_comparaison", EST_OPERATEUR_COMPARAISON, os);
    genere_fonction_cpp_pour_drapeau(lexemes, "est_chaine_littérale", EST_CHAINE_LITTERALE, os);
    genere_fonction_cpp_pour_drapeau(lexemes, "est_spécifiant_type", EST_SPECIFIANT_TYPE, os);
    genere_fonction_cpp_pour_drapeau(lexemes, "est_identifiant_type", EST_IDENTIFIANT_TYPE, os);
    genere_fonction_cpp_pour_drapeau(lexemes, "est_opérateur_unaire", EST_OPERATEUR_UNAIRE, os);
}

static void genere_fonction_kuri_pour_drapeau(const ListeLexemes &lexemes,
                                              kuri::chaine_statique nom,
                                              uint32_t drapeau,
                                              std::ostream &os)
{
    os << nom << " :: fonc (genre: GenreLexème) -> bool\n";
    os << "{\n";
    os << "\tdiscr genre {\n";
    auto virgule = "\t\t";
    POUR (lexemes.lexemes) {
        if ((it.drapeaux & drapeau) == 0) {
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
    genere_fonction_kuri_pour_drapeau(lexemes, "est_mot_clé", EST_MOT_CLE, os);
    genere_fonction_kuri_pour_drapeau(
        lexemes, "est_assignation_composée", EST_ASSIGNATION_COMPOSEE, os);
    genere_fonction_kuri_pour_drapeau(lexemes, "est_opérateur_bool", EST_OPERATEUR_BOOL, os);
    genere_fonction_kuri_pour_drapeau(
        lexemes, "est_opérateur_comparaison", EST_OPERATEUR_COMPARAISON, os);
    genere_fonction_kuri_pour_drapeau(lexemes, "est_chaine_littérale", EST_CHAINE_LITTERALE, os);
    genere_fonction_kuri_pour_drapeau(lexemes, "est_spécifiant_type", EST_SPECIFIANT_TYPE, os);
    genere_fonction_kuri_pour_drapeau(lexemes, "est_identifiant_type", EST_IDENTIFIANT_TYPE, os);
    genere_fonction_kuri_pour_drapeau(lexemes, "est_opérateur_unaire", EST_OPERATEUR_UNAIRE, os);
}

static int genere_empreinte_parfaite(const ListeLexemes &lexemes, std::ostream &os)
{
    const char *debut_fichier = R"(
%compare-lengths
%compare-strncmp
%define class-name EmpreinteParfaite
%define hash-function-name calcule_empreinte
%define initializer-suffix ,GenreLexeme::CHAINE_CARACTERE
%define lookup-function-name lexeme_pour_chaine
%define slot-name nom
%enum
%global-table
%language=C++
%readonly-tables
%struct-type

%{
#include "lexemes.hh"
%}

struct EntreeTable {  const char *nom; GenreLexeme genre;  };
)";

    const char *fin_fichier = R"(
inline GenreLexeme lexeme_pour_chaine(dls::vue_chaine_compacte chn)
{
  return EmpreinteParfaite::lexeme_pour_chaine(chn.pointeur(), static_cast<size_t>(chn.taille()));
}
)";

    auto empreinte_parfaire_txt = kuri::chemin_systeme::chemin_temporaire(
        "empreinte_parfaite.txt");
    auto empreinte_parfaite_tmp_hh = kuri::chemin_systeme::chemin_temporaire(
        "empreinte_parfaite_tmp.hh");

    std::ofstream fichier_tmp(vers_std_path(empreinte_parfaire_txt));

    fichier_tmp << debut_fichier;
    fichier_tmp << "%%\n";

    POUR (lexemes.lexemes) {
        if ((it.drapeaux & EST_MOT_CLE) != 0) {
            fichier_tmp << "\"" << it.chaine << "\", GenreLexeme::" << it.nom_enum_sans_accent
                        << '\n';
        }
    }

    fichier_tmp << "%%\n";
    fichier_tmp << fin_fichier;

    fichier_tmp.close();

    std::stringstream ss;
    ss << CHEMIN_GPERF << " ";
    ss << "-m100 ";
    ss << empreinte_parfaire_txt << " ";
    ss << "--output-file=";
    ss << empreinte_parfaite_tmp_hh;

    const auto commande = ss.str();

    if (system(commande.c_str()) != 0) {
        std::cerr << "Ne peut pas exécuter la commande de création du fichier d'empreinte "
                     "parfaite depuis GPerf\n";
        return 1;
    }

    std::ifstream fichier_tmp_entree(vers_std_path(empreinte_parfaite_tmp_hh));

    if (!fichier_tmp_entree.is_open()) {
        std::cerr << "Impossible d'ouvrir le fichier " << empreinte_parfaite_tmp_hh << '\n';
        return 1;
    }

    os << "#pragma once\n\n";

    std::string ligne;
    while (std::getline(fichier_tmp_entree, ligne)) {
        if (ligne.size() > 5 && ligne.substr(0, 5) == "#line") {
            continue;
        }

        if (remplace(ligne, "const struct EntreeTable *", "inline GenreLexeme ")) {
            os << ligne << '\n';
            continue;
        }

        if (remplace(ligne, "return &wordlist[key];", "return wordlist[key].genre;")) {
            os << ligne << '\n';
            continue;
        }

        if (remplace(ligne, "return 0;", "return GenreLexeme::CHAINE_CARACTERE;")) {
            os << ligne << '\n';
            continue;
        }

        if (remplace(ligne,
                     "unsigned int hval = len;",
                     "uint32_t hval = static_cast<uint32_t>(len);")) {
            os << ligne << '\n';
            continue;
        }

        os << ligne << '\n';
    }

    return 0;
}

int main(int argc, const char **argv)
{
    if (argc != 2) {
        std::cerr << "Utilisation: " << argv[0] << " nom_fichier_sortie\n";
        return 1;
    }

    auto nom_fichier_sortie = kuri::chemin_systeme(argv[1]);

    auto lexemes = ListeLexemes{};
    construit_lexemes(lexemes);
    construit_nom_enums(lexemes);

    auto nom_fichier_tmp = kuri::chemin_systeme::chemin_temporaire(
        nom_fichier_sortie.nom_fichier());

    if (nom_fichier_sortie.nom_fichier() == "lexemes.cc") {
        {
            std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
            genere_fichier_source(lexemes, fichier_sortie);
        }
        {
            // Génère le fichier de lexèmes pour le module Compilatrice
            // Apparemment, ce n'est pas possible de le faire via CMake
            nom_fichier_sortie.remplace_nom_fichier("../modules/Compilatrice/lexèmes.kuri");
            std::ofstream fichier_sortie(vers_std_path(nom_fichier_sortie));
            genere_fichier_kuri(lexemes, fichier_sortie);
        }
    }
    else if (nom_fichier_sortie.nom_fichier() == "lexemes.hh") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        genere_fichier_entete(lexemes, fichier_sortie);
    }
    else if (nom_fichier_sortie.nom_fichier() == "empreinte_parfaite.hh") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        if (genere_empreinte_parfaite(lexemes, fichier_sortie) != 0) {
            return 1;
        }
    }
    else {
        std::cerr << "Fichier de sortie « " << argv[1] << " » inconnu !\n";
        return 1;
    }

    if (!remplace_si_different(nom_fichier_tmp, argv[1])) {
        return 1;
    }

    return 0;
}
