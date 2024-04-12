/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

/* Fichier de génération du code pour les lexèmes. */

#include <fstream>
#include <iostream>

#include "structures/chaine.hh"
#include "structures/chemin_systeme.hh"
#include "structures/tableau.hh"

#include "outils_independants_des_lexemes.hh"

enum {
    EST_MOT_CLÉ = (1 << 0),
    EST_ASSIGNATION_COMPOSÉE = (1 << 1),
    EST_OPÉRATEUR_BOOL = (1 << 2),
    EST_OPÉRATEUR_COMPARAISON = (1 << 3),
    EST_CHAINE_LITTÉRALE = (1 << 4),
    EST_SPÉCIFIANT_TYPE = (1 << 5),
    EST_IDENTIFIANT_TYPE = (1 << 6),
    EST_OPÉRATEUR_UNAIRE = (1 << 7),
};

struct DescriptionLexème {
    kuri::chaine_statique chaine = "";
    kuri::chaine_statique nom_énum = "";

    kuri::chaine nom_énum_final = "";

    uint32_t drapeaux = 0;
};

struct ListeLexèmes {
    kuri::tableau<DescriptionLexème> lexèmes{};

    void ajoute_mot_clé(kuri::chaine_statique chaine, uint32_t drapeaux = 0)
    {
        ajoute_lexème(chaine, "", EST_MOT_CLÉ | drapeaux);
    }

    void ajoute_ponctuation(kuri::chaine_statique chaine,
                            kuri::chaine_statique nom_énum,
                            uint32_t drapeaux = 0)
    {
        ajoute_lexème(chaine, nom_énum, drapeaux);
    }

    void ajoute_extra(kuri::chaine_statique chaine,
                      kuri::chaine_statique nom_énum,
                      uint32_t drapeaux = 0)
    {
        ajoute_lexème(chaine, nom_énum, drapeaux);
    }

  private:
    void ajoute_lexème(kuri::chaine_statique chaine,
                       kuri::chaine_statique nom_énum,
                       uint32_t drapeaux = 0)
    {
        auto lexème = DescriptionLexème{};
        lexème.chaine = chaine;
        lexème.nom_énum = nom_énum;
        lexème.drapeaux = drapeaux;

        lexèmes.ajoute(lexème);
    }
};

static void construit_lexèmes(ListeLexèmes &lexèmes)
{
    lexèmes.ajoute_mot_clé("adresse_fonction", EST_IDENTIFIANT_TYPE);
    lexèmes.ajoute_mot_clé("arrête");
    lexèmes.ajoute_mot_clé("bool", EST_IDENTIFIANT_TYPE);
    lexèmes.ajoute_mot_clé("boucle");
    lexèmes.ajoute_mot_clé("chaine", EST_IDENTIFIANT_TYPE);
    lexèmes.ajoute_mot_clé("charge");
    lexèmes.ajoute_mot_clé("comme");
    lexèmes.ajoute_mot_clé("continue");
    lexèmes.ajoute_mot_clé("corout");
    lexèmes.ajoute_mot_clé("dans");
    lexèmes.ajoute_mot_clé("diffère");
    lexèmes.ajoute_mot_clé("discr");
    lexèmes.ajoute_mot_clé("dyn");
    lexèmes.ajoute_mot_clé("définis");
    lexèmes.ajoute_mot_clé("eini", EST_IDENTIFIANT_TYPE);
    lexèmes.ajoute_mot_clé("eini_erreur", EST_IDENTIFIANT_TYPE);
    lexèmes.ajoute_mot_clé("empl");
    lexèmes.ajoute_mot_clé("erreur");
    lexèmes.ajoute_mot_clé("faux");
    lexèmes.ajoute_mot_clé("fonc");
    lexèmes.ajoute_mot_clé("garde");
    lexèmes.ajoute_mot_clé("importe");
    lexèmes.ajoute_mot_clé("info_de");
    lexèmes.ajoute_mot_clé("init_de");
    lexèmes.ajoute_mot_clé("mémoire");
    lexèmes.ajoute_mot_clé("n16", EST_IDENTIFIANT_TYPE);
    lexèmes.ajoute_mot_clé("n32", EST_IDENTIFIANT_TYPE);
    lexèmes.ajoute_mot_clé("n64", EST_IDENTIFIANT_TYPE);
    lexèmes.ajoute_mot_clé("n8", EST_IDENTIFIANT_TYPE);
    lexèmes.ajoute_mot_clé("nonatteignable");
    lexèmes.ajoute_mot_clé("nonsûr");
    lexèmes.ajoute_mot_clé("nul");
    lexèmes.ajoute_mot_clé("octet", EST_IDENTIFIANT_TYPE);
    lexèmes.ajoute_mot_clé("opérateur");
    lexèmes.ajoute_mot_clé("piège");
    lexèmes.ajoute_mot_clé("pour");
    lexèmes.ajoute_mot_clé("pousse_contexte");
    lexèmes.ajoute_mot_clé("r16", EST_IDENTIFIANT_TYPE);
    lexèmes.ajoute_mot_clé("r32", EST_IDENTIFIANT_TYPE);
    lexèmes.ajoute_mot_clé("r64", EST_IDENTIFIANT_TYPE);
    lexèmes.ajoute_mot_clé("reprends");
    lexèmes.ajoute_mot_clé("retiens");
    lexèmes.ajoute_mot_clé("retourne");
    lexèmes.ajoute_mot_clé("rien", EST_IDENTIFIANT_TYPE);
    lexèmes.ajoute_mot_clé("répète");
    lexèmes.ajoute_mot_clé("sansarrêt");
    lexèmes.ajoute_mot_clé("saufsi");
    lexèmes.ajoute_mot_clé("si");
    lexèmes.ajoute_mot_clé("sinon");
    lexèmes.ajoute_mot_clé("struct");
    lexèmes.ajoute_mot_clé("taille_de");
    lexèmes.ajoute_mot_clé("tantque");
    lexèmes.ajoute_mot_clé("tente");
    lexèmes.ajoute_mot_clé("type_de", EST_SPÉCIFIANT_TYPE);
    lexèmes.ajoute_mot_clé("type_de_données", EST_IDENTIFIANT_TYPE);
    lexèmes.ajoute_mot_clé("union");
    lexèmes.ajoute_mot_clé("vrai");
    lexèmes.ajoute_mot_clé("z16", EST_IDENTIFIANT_TYPE);
    lexèmes.ajoute_mot_clé("z32", EST_IDENTIFIANT_TYPE);
    lexèmes.ajoute_mot_clé("z64", EST_IDENTIFIANT_TYPE);
    lexèmes.ajoute_mot_clé("z8", EST_IDENTIFIANT_TYPE);
    lexèmes.ajoute_mot_clé("énum");
    lexèmes.ajoute_mot_clé("énum_drapeau");
    lexèmes.ajoute_ponctuation("!", "EXCLAMATION", EST_OPÉRATEUR_UNAIRE | EST_OPÉRATEUR_BOOL);
    lexèmes.ajoute_ponctuation("\"", "GUILLEMET");
    lexèmes.ajoute_ponctuation("#", "DIRECTIVE");
    lexèmes.ajoute_ponctuation("$", "DOLLAR", EST_SPÉCIFIANT_TYPE);
    lexèmes.ajoute_ponctuation("%", "POURCENT");
    lexèmes.ajoute_ponctuation("&", "ESPERLUETTE", EST_SPÉCIFIANT_TYPE);
    lexèmes.ajoute_ponctuation("'", "APOSTROPHE");
    lexèmes.ajoute_ponctuation("(", "PARENTHESE_OUVRANTE");
    lexèmes.ajoute_ponctuation(")", "PARENTHESE_FERMANTE");
    lexèmes.ajoute_ponctuation("*", "FOIS", EST_SPÉCIFIANT_TYPE);
    lexèmes.ajoute_ponctuation("+", "PLUS");
    lexèmes.ajoute_ponctuation(",", "VIRGULE");
    lexèmes.ajoute_ponctuation("-", "MOINS");
    lexèmes.ajoute_ponctuation(".", "POINT");
    lexèmes.ajoute_ponctuation("/", "DIVISE");
    lexèmes.ajoute_ponctuation(":", "DOUBLE_POINTS");
    lexèmes.ajoute_ponctuation(";", "POINT_VIRGULE");
    lexèmes.ajoute_ponctuation("<", "INFERIEUR", EST_OPÉRATEUR_COMPARAISON | EST_OPÉRATEUR_BOOL);
    lexèmes.ajoute_ponctuation("=", "EGAL");
    lexèmes.ajoute_ponctuation(">", "SUPERIEUR", EST_OPÉRATEUR_COMPARAISON | EST_OPÉRATEUR_BOOL);
    lexèmes.ajoute_ponctuation("@", "AROBASE");
    lexèmes.ajoute_ponctuation("[", "CROCHET_OUVRANT", EST_OPÉRATEUR_UNAIRE | EST_SPÉCIFIANT_TYPE);
    lexèmes.ajoute_ponctuation("]", "CROCHET_FERMANT");
    lexèmes.ajoute_ponctuation("^", "CHAPEAU");
    lexèmes.ajoute_ponctuation("{", "ACCOLADE_OUVRANTE");
    lexèmes.ajoute_ponctuation("|", "BARRE");
    lexèmes.ajoute_ponctuation("}", "ACCOLADE_FERMANTE");
    lexèmes.ajoute_ponctuation("~", "TILDE", EST_OPÉRATEUR_UNAIRE);
    lexèmes.ajoute_ponctuation("!=", "DIFFÉRENCE", EST_OPÉRATEUR_COMPARAISON | EST_OPÉRATEUR_BOOL);
    lexèmes.ajoute_ponctuation("%=", "MODULO_EGAL", EST_ASSIGNATION_COMPOSÉE);
    lexèmes.ajoute_ponctuation("&&", "ESP_ESP", EST_OPÉRATEUR_BOOL);
    lexèmes.ajoute_ponctuation("&&=", "ESP_ESP_EGAL", EST_ASSIGNATION_COMPOSÉE);
    lexèmes.ajoute_ponctuation("&=", "ET_EGAL", EST_ASSIGNATION_COMPOSÉE);
    lexèmes.ajoute_ponctuation("*/", "FIN_BLOC_COMMENTAIRE");
    lexèmes.ajoute_ponctuation("*=", "MULTIPLIE_EGAL", EST_ASSIGNATION_COMPOSÉE);
    lexèmes.ajoute_ponctuation("+=", "PLUS_EGAL", EST_ASSIGNATION_COMPOSÉE);
    lexèmes.ajoute_ponctuation("-=", "MOINS_EGAL", EST_ASSIGNATION_COMPOSÉE);
    lexèmes.ajoute_ponctuation("->", "RETOUR_TYPE");
    lexèmes.ajoute_ponctuation("/*", "DEBUT_BLOC_COMMENTAIRE");
    lexèmes.ajoute_ponctuation("//", "DEBUT_LIGNE_COMMENTAIRE");
    lexèmes.ajoute_ponctuation("/=", "DIVISE_EGAL", EST_ASSIGNATION_COMPOSÉE);
    lexèmes.ajoute_ponctuation("::", "DECLARATION_CONSTANTE");
    lexèmes.ajoute_ponctuation(":=", "DECLARATION_VARIABLE");
    lexèmes.ajoute_ponctuation("`", "ACCENT_GRAVE", EST_OPÉRATEUR_UNAIRE);
    lexèmes.ajoute_ponctuation("<<", "DECALAGE_GAUCHE");
    lexèmes.ajoute_ponctuation(
        "<=", "INFERIEUR_EGAL", EST_OPÉRATEUR_COMPARAISON | EST_OPÉRATEUR_BOOL);
    lexèmes.ajoute_ponctuation("==", "EGALITE", EST_OPÉRATEUR_COMPARAISON | EST_OPÉRATEUR_BOOL);
    lexèmes.ajoute_ponctuation(
        ">=", "SUPERIEUR_EGAL", EST_OPÉRATEUR_COMPARAISON | EST_OPÉRATEUR_BOOL);
    lexèmes.ajoute_ponctuation(">>", "DECALAGE_DROITE");
    lexèmes.ajoute_ponctuation("^=", "OUX_EGAL", EST_ASSIGNATION_COMPOSÉE);
    lexèmes.ajoute_ponctuation("|=", "OU_EGAL", EST_ASSIGNATION_COMPOSÉE);
    lexèmes.ajoute_ponctuation("||", "BARRE_BARRE", EST_OPÉRATEUR_BOOL);
    lexèmes.ajoute_ponctuation("||=", "BARRE_BARRE_EGAL", EST_ASSIGNATION_COMPOSÉE);
    lexèmes.ajoute_ponctuation("---", "NON_INITIALISATION");
    lexèmes.ajoute_ponctuation("...", "TROIS_POINTS", EST_SPÉCIFIANT_TYPE);
    lexèmes.ajoute_ponctuation("..", "DEUX_POINTS", EST_SPÉCIFIANT_TYPE);
    lexèmes.ajoute_ponctuation("<<=", "DEC_GAUCHE_EGAL", EST_ASSIGNATION_COMPOSÉE);
    lexèmes.ajoute_ponctuation(">>=", "DEC_DROITE_EGAL", EST_ASSIGNATION_COMPOSÉE);
    lexèmes.ajoute_extra("", "NOMBRE_REEL");
    lexèmes.ajoute_extra("", "NOMBRE_ENTIER");
    lexèmes.ajoute_extra("-", "PLUS_UNAIRE", EST_OPÉRATEUR_UNAIRE);
    lexèmes.ajoute_extra("+", "MOINS_UNAIRE", EST_OPÉRATEUR_UNAIRE);
    lexèmes.ajoute_extra("*", "FOIS_UNAIRE", EST_OPÉRATEUR_UNAIRE);
    lexèmes.ajoute_extra("&", "ESP_UNAIRE", EST_OPÉRATEUR_UNAIRE);
    lexèmes.ajoute_extra("", "CHAINE_CARACTERE", EST_IDENTIFIANT_TYPE);
    lexèmes.ajoute_extra("", "CHAINE_LITTERALE", EST_CHAINE_LITTÉRALE);
    lexèmes.ajoute_extra("", "CARACTÈRE", EST_CHAINE_LITTÉRALE);
    lexèmes.ajoute_extra("*", "POINTEUR");
    lexèmes.ajoute_extra("[..]", "TABLEAU", EST_OPÉRATEUR_UNAIRE);
    lexèmes.ajoute_extra("&", "REFERENCE");
    lexèmes.ajoute_extra("", "CARACTÈRE_BLANC");
    lexèmes.ajoute_extra("// commentaire", "COMMENTAIRE");
    lexèmes.ajoute_extra("...", "EXPANSION_VARIADIQUE");
    lexèmes.ajoute_extra("...", "INCONNU");
}

static void construit_nom_énums(ListeLexèmes &lexèmes)
{
    POUR (lexèmes.lexèmes) {
        if (it.nom_énum == "") {
            it.nom_énum_final = en_majuscule(it.chaine);
        }
        else {
            it.nom_énum_final = it.nom_énum;
        }
    }
}

static void génère_enum(const ListeLexèmes &lexèmes, std::ostream &os)
{
    os << "enum class GenreLexème : uint32_t {\n";
    POUR (lexèmes.lexèmes) {
        os << "\t" << it.nom_énum_final << ",\n";
    }
    os << "};\n";
}

static void génère_fonction_cpp_pour_drapeau(const ListeLexèmes &lexèmes,
                                             kuri::chaine_statique nom,
                                             uint32_t drapeau,
                                             std::ostream &os)
{
    os << "bool " << nom << "(GenreLexème genre)\n";
    os << "{\n";
    os << "\tswitch (genre) {\n";
    os << "\t\tdefault:\n";
    os << "\t\t{\n";
    os << "\t\t\treturn false;\n";
    os << "\t\t}\n";
    POUR (lexèmes.lexèmes) {
        if ((it.drapeaux & drapeau) == 0) {
            continue;
        }
        os << "\t\tcase GenreLexème::" << it.nom_énum_final << ":\n";
    }
    os << "\t\t{\n";
    os << "\t\t\treturn true;\n";
    os << "\t\t}\n";
    os << "\t}\n";
    os << "}\n\n";

    os << "bool " << nom << "(const Lexème &lexème)\n";
    os << "{\n";
    os << "\treturn " << nom << "(lexème.genre);\n";
    os << "}\n\n";
}

static void génère_impression_lexème(const ListeLexèmes &lexèmes, std::ostream &os)
{
    os << "static kuri::chaine_statique noms_genres_lexèmes[" << lexèmes.lexèmes.taille()
       << "] = {\n";
    POUR (lexèmes.lexèmes) {
        os << "\t" << '"' << it.nom_énum_final << '"' << ",\n";
    }
    os << "};\n\n";

    os << "static kuri::chaine_statique chaines_lexèmes[" << lexèmes.lexèmes.taille() << "] = {\n";
    POUR (lexèmes.lexèmes) {
        if (it.chaine == "\"") {
            os << "\t\"\\\"\",\n";
        }
        else {
            os << "\t" << '"' << it.chaine << '"' << ",\n";
        }
    }
    os << "};\n\n";

    os << "std::ostream &operator<<(std::ostream &os, GenreLexème genre)\n";
    os << "{\n";
    os << "\treturn os << noms_genres_lexèmes[static_cast<int>(genre)];\n";
    os << "}\n\n";

    os << "kuri::chaine_statique chaine_du_genre_de_lexème(GenreLexème genre)\n";
    os << "{\n";
    os << "\treturn noms_genres_lexèmes[static_cast<int>(genre)];\n";
    os << "}\n\n";

    os << "kuri::chaine_statique chaine_du_lexème(GenreLexème genre)\n";
    os << "{\n";
    os << "\treturn chaines_lexèmes[static_cast<int>(genre)];\n";
    os << "}\n\n";
}

static void génère_fichier_entête(const ListeLexèmes &lexèmes, std::ostream &os)
{
    os << "#pragma once\n";
    os << '\n';
    inclus_système(os, "iosfwd");
    inclus(os, "biblinternes/structures/chaine.hh");
    os << '\n';
    prodéclare_struct(os, "IdentifiantCode");
    os << '\n';
    génère_enum(lexèmes, os);

    const char *declarations = R"(
#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#endif
struct Lexème {
    dls::vue_chaine_compacte chaine{};

	union {
		uint64_t valeur_entiere;
		double valeur_reelle;
		int64_t index_chaine;
		IdentifiantCode *ident;
	};

    GenreLexème genre{};
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

bool est_mot_clé(GenreLexème genre);
bool est_mot_clé(const Lexème &lexème);
bool est_assignation_composée(GenreLexème genre);
bool est_assignation_composée(const Lexème &lexème);
bool est_opérateur_bool(GenreLexème genre);
bool est_opérateur_bool(const Lexème &lexème);
bool est_opérateur_comparaison(GenreLexème genre);
bool est_opérateur_comparaison(const Lexème &lexème);
bool est_chaine_littérale(GenreLexème genre);
bool est_chaine_littérale(const Lexème &lexème);
bool est_spécifiant_type(GenreLexème genre);
bool est_spécifiant_type(const Lexème &lexème);
bool est_identifiant_type(GenreLexème genre);
bool est_identifiant_type(const Lexème &lexème);
bool est_opérateur_unaire(GenreLexème genre);
bool est_opérateur_unaire(const Lexème &lexème);
std::ostream &operator<<(std::ostream &os, GenreLexème genre);
kuri::chaine_statique chaine_du_genre_de_lexème(GenreLexème id);
kuri::chaine_statique chaine_du_lexème(GenreLexème genre);
)";

    os << declarations;
}

static void génère_fichier_source(const ListeLexèmes &lexèmes, std::ostream &os)
{
    inclus(os, "lexemes.hh");
    inclus_système(os, "iostream");
    inclus(os, "structures/chaine_statique.hh");
    génère_impression_lexème(lexèmes, os);
    génère_fonction_cpp_pour_drapeau(lexèmes, "est_mot_clé", EST_MOT_CLÉ, os);
    génère_fonction_cpp_pour_drapeau(
        lexèmes, "est_assignation_composée", EST_ASSIGNATION_COMPOSÉE, os);
    génère_fonction_cpp_pour_drapeau(lexèmes, "est_opérateur_bool", EST_OPÉRATEUR_BOOL, os);
    génère_fonction_cpp_pour_drapeau(
        lexèmes, "est_opérateur_comparaison", EST_OPÉRATEUR_COMPARAISON, os);
    génère_fonction_cpp_pour_drapeau(lexèmes, "est_chaine_littérale", EST_CHAINE_LITTÉRALE, os);
    génère_fonction_cpp_pour_drapeau(lexèmes, "est_spécifiant_type", EST_SPÉCIFIANT_TYPE, os);
    génère_fonction_cpp_pour_drapeau(lexèmes, "est_identifiant_type", EST_IDENTIFIANT_TYPE, os);
    génère_fonction_cpp_pour_drapeau(lexèmes, "est_opérateur_unaire", EST_OPÉRATEUR_UNAIRE, os);
}

static void génère_fonction_kuri_pour_drapeau(const ListeLexèmes &lexèmes,
                                              kuri::chaine_statique nom,
                                              uint32_t drapeau,
                                              std::ostream &os)
{
    os << nom << " :: fonc (genre: GenreLexème) -> bool\n";
    os << "{\n";
    os << "\tdiscr genre {\n";
    auto virgule = "\t\t";
    POUR (lexèmes.lexèmes) {
        if ((it.drapeaux & drapeau) == 0) {
            continue;
        }

        os << virgule << it.nom_énum_final;
        virgule = ",\n\t\t";
    }
    os << " { retourne vrai; }\n";
    os << "\t\tsinon { retourne faux; }\n";
    os << "\t}\n";
    os << "}\n\n";
}

static void génère_fichier_kuri(const ListeLexèmes &lexèmes, std::ostream &os)
{
    os << "/* Fichier générer automatiquement, NE PAS ÉDITER ! */\n\n";
    os << "GenreLexème :: énum n32 {\n";
    POUR (lexèmes.lexèmes) {
        os << "\t" << it.nom_énum_final << '\n';
    }
    os << "}\n\n";
    os << "Lexème :: struct {\n";
    os << "\tgenre: GenreLexème\n";
    os << "\ttexte: chaine\n";
    os << "}\n\n";
    génère_fonction_kuri_pour_drapeau(lexèmes, "est_mot_clé", EST_MOT_CLÉ, os);
    génère_fonction_kuri_pour_drapeau(
        lexèmes, "est_assignation_composée", EST_ASSIGNATION_COMPOSÉE, os);
    génère_fonction_kuri_pour_drapeau(lexèmes, "est_opérateur_bool", EST_OPÉRATEUR_BOOL, os);
    génère_fonction_kuri_pour_drapeau(
        lexèmes, "est_opérateur_comparaison", EST_OPÉRATEUR_COMPARAISON, os);
    génère_fonction_kuri_pour_drapeau(lexèmes, "est_chaine_littérale", EST_CHAINE_LITTÉRALE, os);
    génère_fonction_kuri_pour_drapeau(lexèmes, "est_spécifiant_type", EST_SPÉCIFIANT_TYPE, os);
    génère_fonction_kuri_pour_drapeau(lexèmes, "est_identifiant_type", EST_IDENTIFIANT_TYPE, os);
    génère_fonction_kuri_pour_drapeau(lexèmes, "est_opérateur_unaire", EST_OPÉRATEUR_UNAIRE, os);
}

static int génère_empreinte_parfaite(const ListeLexèmes &lexèmes, std::ostream &os)
{
    const char *début_fichier = R"(
%compare-lengths
%compare-strncmp
%define class-name EmpreinteParfaite
%define hash-function-name calcule_empreinte
%define initializer-suffix ,GenreLexème::CHAINE_CARACTERE
%define lookup-function-name lexème_pour_chaine
%define slot-name nom
%enum
%global-table
%language=C++
%readonly-tables
%struct-type

%{
#include "lexemes.hh"
%}

struct EntreeTable {  const char *nom; GenreLexème genre;  };
)";

    const char *fin_fichier = R"(
inline GenreLexème lexème_pour_chaine(dls::vue_chaine_compacte chn)
{
  return EmpreinteParfaite::lexème_pour_chaine(chn.pointeur(), static_cast<size_t>(chn.taille()));
}
)";

    auto empreinte_parfaite_txt = kuri::chemin_systeme::chemin_temporaire(
        "empreinte_parfaite.txt");
    auto empreinte_parfaite_tmp_hh = kuri::chemin_systeme::chemin_temporaire(
        "empreinte_parfaite_tmp.hh");

    std::ofstream fichier_tmp(vers_std_path(empreinte_parfaite_txt));

    fichier_tmp << début_fichier;
    fichier_tmp << "%%\n";

    POUR (lexèmes.lexèmes) {
        if ((it.drapeaux & EST_MOT_CLÉ) != 0) {
            fichier_tmp << "\"" << it.chaine << "\", GenreLexème::" << it.nom_énum_final << '\n';
        }
    }

    fichier_tmp << "%%\n";
    fichier_tmp << fin_fichier;

    fichier_tmp.close();

    std::stringstream ss;
    ss << CHEMIN_GPERF << " ";
    ss << "-m100 ";
    ss << empreinte_parfaite_txt << " ";
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

        if (remplace(ligne, "const struct EntreeTable *", "inline GenreLexème ")) {
            os << ligne << '\n';
            continue;
        }

        if (remplace(ligne, "return &wordlist[key];", "return wordlist[key].genre;")) {
            os << ligne << '\n';
            continue;
        }

        if (remplace(ligne, "return 0;", "return GenreLexème::CHAINE_CARACTERE;")) {
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

    auto lexèmes = ListeLexèmes{};
    construit_lexèmes(lexèmes);
    construit_nom_énums(lexèmes);

    auto nom_fichier_tmp = kuri::chemin_systeme::chemin_temporaire(
        nom_fichier_sortie.nom_fichier());

    if (nom_fichier_sortie.nom_fichier() == "lexemes.cc") {
        {
            std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
            génère_fichier_source(lexèmes, fichier_sortie);
        }
        {
            // Génère le fichier de lexèmes pour le module Compilatrice
            // Apparemment, ce n'est pas possible de le faire via CMake
            nom_fichier_sortie.remplace_nom_fichier("../modules/Compilatrice/lexèmes.kuri");
            std::ofstream fichier_sortie(vers_std_path(nom_fichier_sortie));
            génère_fichier_kuri(lexèmes, fichier_sortie);
        }
    }
    else if (nom_fichier_sortie.nom_fichier() == "lexemes.hh") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        génère_fichier_entête(lexèmes, fichier_sortie);
    }
    else if (nom_fichier_sortie.nom_fichier() == "empreinte_parfaite.hh") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        if (génère_empreinte_parfaite(lexèmes, fichier_sortie) != 0) {
            return 1;
        }
    }
    else {
        std::cerr << "Fichier de sortie « " << argv[1] << " » inconnu !\n";
        return 1;
    }

    if (!remplace_si_différent(nom_fichier_tmp, argv[1])) {
        return 1;
    }

    return 0;
}
