/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

/* Fichier de génération du code pour l'IPA de la Compilatrice. */

#include <fstream>
#include <iostream>

#include "biblinternes/moultfilage/synchrone.hh"

#include "parsage/base_syntaxeuse.hh"
#include "parsage/gerante_chaine.hh"
#include "parsage/identifiant.hh"
#include "parsage/lexeuse.hh"
#include "parsage/modules.hh"

#include "structures/chemin_systeme.hh"
#include "structures/tableau.hh"

#include "adn.hh"
#include "outils_dependants_sur_lexemes.hh"

static void génère_code_kuri(const kuri::tableau<Protéine *> &protéines, FluxSortieKuri &os)
{
    os << "// ----------------------------------------------------------------------------\n";
    os << "// Déclaration des fonctions intrinsèques de la compilatrice.\n";
    os << "\n";

    POUR (protéines) {
        if (auto protéine_énum = it->comme_enum()) {
            if (protéine_énum->nom().nom() == "GenreIntrinsèque") {
                continue;
            }
        }

        it->génère_code_kuri(os);
    }
}

static void génère_code_cpp(const kuri::tableau<Protéine *> &protéines,
                            FluxSortieCPP &os,
                            bool pour_entête)
{
    os << "// Fichier généré automatiquement, NE PAS ÉDITER\n\n";

    if (pour_entête) {
        os << "#pragma once\n\n";

        inclus_système(os, "cstdint");
        inclus_système(os, "iosfwd");
        inclus_système(os, "optional");

        inclus(os, "structures/chaine_statique.hh");

        prodéclare_struct(os, "IdentifiantCode");

        os << "\n";

        genere_déclaration_identifiants_code(protéines, os, pour_entête, "intrinsèques");

        POUR (protéines) {
            auto protéine_énum = it->comme_enum();
            if (!protéine_énum) {
                continue;
            }

            it->génère_code_cpp(os, true);
        }

        os << "\n";

        os << "std::optional<kuri::chaine_statique> "
              "donne_nom_intrinsèque_gcc_pour_identifiant(IdentifiantCode const *ident);\n";

        os << "\n";

        os << "std::optional<GenreIntrinsèque> "
              "donne_genre_intrinsèque_pour_identifiant(IdentifiantCode const *ident);\n";

        return;
    }

    inclus(os, "intrinseques.hh");
    inclus_système(os, "iostream");

    genere_déclaration_identifiants_code(protéines, os, pour_entête, "intrinsèques");

    POUR (protéines) {
        auto protéine_énum = it->comme_enum();
        if (!protéine_énum) {
            continue;
        }

        it->génère_code_cpp(os, false);
    }

    os << "\n";
    os << "std::optional<kuri::chaine_statique> "
          "donne_nom_intrinsèque_gcc_pour_identifiant(IdentifiantCode const *ident)\n";
    os << "{\n";

    POUR (protéines) {
        auto protéine_fonction = it->comme_fonction();
        if (!protéine_fonction) {
            continue;
        }

        if (!protéine_fonction->est_marquée_intrinsèque()) {
            continue;
        }

        if (protéine_fonction->donne_symbole_gcc() == "") {
            continue;
        }

        os << "    if (ident == ID::" << protéine_fonction->donne_genre_intrinsèque() << ") {\n";
        os << "        return \"" << protéine_fonction->donne_symbole_gcc() << "\";\n";
        os << "    }\n";
    }

    os << "    return {};\n";
    os << "}\n";

    ProtéineEnum *enum_genre_intrinsèque = nullptr;
    POUR (protéines) {
        auto protéine_enum = it->comme_enum();
        if (!protéine_enum) {
            continue;
        }

        if (protéine_enum->nom().nom() != "GenreIntrinsèque") {
            continue;
        }

        enum_genre_intrinsèque = protéine_enum;
        break;
    }

    os << "\n";
    os << "std::optional<GenreIntrinsèque> "
          "donne_genre_intrinsèque_pour_identifiant(IdentifiantCode const *ident)\n";
    os << "{\n";

    for (auto const &membre : enum_genre_intrinsèque->donne_membres()) {
        os << "    if (ident == ID::" << membre.nom.nom() << ") {\n";
        os << "        return GenreIntrinsèque::" << membre.nom.nom() << ";\n";
        os << "    }\n";
    }

    os << "    return {};\n";
    os << "}\n";
}

static bool est_type_rien(Type const *type)
{
    if (!type) {
        /* Si le type est nul, nous n'avons « rien ». */
        return true;
    }

    if (!type->est_nominal()) {
        return false;
    }

    return type->comme_nominal()->accede_nom().nom() == "void";
}

static void génère_code_appel_intrinsèque_défaut(FluxSortieCPP &os, ProtéineFonction *fonction)
{
    auto symbole = fonction->donne_symbole_gcc();
    auto type_sortie = fonction->type_sortie();
    auto ne_retourne_rien = est_type_rien(type_sortie);

    for (auto const &params : fonction->donne_paramètres()) {
        os << "        auto " << params.nom << " = dépile<" << *params.type << ">();\n";
    }

    os << "        ";
    if (!ne_retourne_rien) {
        os << "auto résultat = ";
    }

    os << symbole;

    auto virgule = "(";
    for (auto const &params : fonction->donne_paramètres()) {
        os << virgule << params.nom;
        virgule = ", ";
    }

    if (fonction->donne_paramètres().taille() == 0) {
        os << "(";
    }
    os << ");\n";

    if (!est_type_rien(type_sortie)) {
        os << "        empile<" << *type_sortie << ">(résultat);\n";
    }
}

static void génère_code_appel_intrinsèque_ignorée(FluxSortieCPP &os,
                                                  ProtéineFonction *fonction,
                                                  kuri::chaine_statique argument_résultat)
{
    auto type_sortie = fonction->type_sortie();
    auto ne_retourne_rien = est_type_rien(type_sortie);

    for (auto const &params : fonction->donne_paramètres()) {
        os << "        auto " << params.nom << " = dépile<" << *params.type << ">();\n";
    }

    if (!ne_retourne_rien) {
        os << "        " << *type_sortie << " résultat = " << argument_résultat << ";\n";
    }

    for (auto const &params : fonction->donne_paramètres()) {
        if (params.nom.nom() == argument_résultat) {
            continue;
        }
        os << "        static_cast<void>(" << params.nom << ");\n";
    }

    if (!est_type_rien(type_sortie)) {
        os << "        empile<" << *type_sortie << ">(résultat);\n";
    }
}

static void génère_code_appel_intrinsèque(FluxSortieCPP &os, ProtéineFonction *fonction)
{
    auto symbole = fonction->donne_symbole_gcc();

    if (symbole == "__builtin___clear_cache" || symbole == "__builtin_prefetch" ||
        symbole == "__atomic_thread_fence" || symbole == "__atomic_fetch_add") {
        génère_code_appel_intrinsèque_ignorée(os, fonction, "{}");
    }
    else if (symbole == "__builtin_expect" || symbole == "__builtin_expect_with_probability") {
        génère_code_appel_intrinsèque_ignorée(
            os, fonction, fonction->donne_paramètres()[0].nom.nom());
    }
    else {
        génère_code_appel_intrinsèque_défaut(os, fonction);
    }
}

static void génère_code_machine_virtuelle(const kuri::tableau<Protéine *> &protéines,
                                          FluxSortieCPP &os)
{
    os << "// Fichier généré automatiquement, NE PAS ÉDITER\n\n";
    inclus(os, "machine_virtuelle.hh");
    inclus(os, "instructions.hh");
    inclus(os, "arbre_syntaxique/noeud_expression.hh");
    inclus(os, "compilation/intrinseques.hh");
    os << "\n";

    os << "void MachineVirtuelle::appel_fonction_intrinsèque(AtomeFonction *ptr_fonction)\n";
    os << "{\n";

    os << "    auto decl = ptr_fonction->decl;\n";

    POUR (protéines) {
        if (!it->est_fonction()) {
            continue;
        }
        auto fonction = dynamic_cast<ProtéineFonction *>(it);
        auto symbole = fonction->donne_symbole_gcc();

        if (symbole == "") {
            continue;
        }

        os << "    if (decl->données_externes->nom_symbole == \"" << symbole << "\") {\n";
        génère_code_appel_intrinsèque(os, fonction);
        os << "        return;\n";
        os << "    }\n";
    }

    os << "}\n";
}

int main(int argc, const char **argv)
{
    if (argc != 4) {
        std::cerr << "Utilisation: " << argv[0] << " nom_fichier_sortie -i fichier_adn\n";
        return 1;
    }

    auto nom_fichier_sortie = kuri::chemin_systeme(argv[1]);

    const auto chemin_adn_ipa = argv[3];

    auto texte = charge_contenu_fichier(chemin_adn_ipa);

    auto fichier = Fichier();
    fichier.tampon_ = lng::tampon_source(texte.c_str());
    fichier.chemin_ = chemin_adn_ipa;

    auto gérante_chaine = dls::outils::Synchrone<GeranteChaine>();
    auto table_identifiants = dls::outils::Synchrone<TableIdentifiant>();
    auto contexte_lexage = ContexteLexage{gérante_chaine, table_identifiants, imprime_erreur};

    auto lexeuse = Lexeuse(contexte_lexage, &fichier);
    lexeuse.performe_lexage();

    if (lexeuse.possède_erreur()) {
        return 1;
    }

    auto syntaxeuse = SyntaxeuseADN(&fichier);
    syntaxeuse.analyse();

    if (syntaxeuse.possède_erreur()) {
        return 1;
    }

    auto nom_fichier_tmp = kuri::chemin_systeme::chemin_temporaire(
        nom_fichier_sortie.nom_fichier());

    if (nom_fichier_sortie.nom_fichier() == "intrinseques.hh") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        auto flux = FluxSortieCPP(fichier_sortie);
        génère_code_cpp(syntaxeuse.protéines, flux, true);
    }
    else if (nom_fichier_sortie.nom_fichier() == "intrinseques.cc") {
        {
            std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
            auto flux = FluxSortieCPP(fichier_sortie);
            génère_code_cpp(syntaxeuse.protéines, flux, false);
        }
        {
            // Génère le fichier pour le module Kuri
            // Apparemment, ce n'est pas possible de le faire via CMake
            nom_fichier_sortie.remplace_nom_fichier("../modules/Kuri/intrinseques.kuri");
            std::ofstream fichier_sortie(vers_std_path(nom_fichier_sortie));
            auto flux = FluxSortieKuri(fichier_sortie);
            génère_code_kuri(syntaxeuse.protéines, flux);
        }
    }
    else if (nom_fichier_sortie.nom_fichier() == "machine_virtuelle_intrinseques.cc") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        auto flux = FluxSortieCPP(fichier_sortie);
        génère_code_machine_virtuelle(syntaxeuse.protéines, flux);
    }

    if (!remplace_si_différent(nom_fichier_tmp, argv[1])) {
        return 1;
    }

    return 0;
}
