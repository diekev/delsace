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

static void genere_code_kuri(const kuri::tableau<Proteine *> &proteines, FluxSortieKuri &os)
{
    os << "// ----------------------------------------------------------------------------\n";
    os << "// Déclaration des fonctions intrinsèques de la compilatrice.\n";
    os << "\n";

    POUR (proteines) {
        it->genere_code_kuri(os);
    }
}

static void genere_code_cpp(const kuri::tableau<Proteine *> &proteines,
                            FluxSortieCPP &os,
                            bool pour_entete)
{
    os << "// Fichier généré automatiquement, NE PAS ÉDITER\n\n";

    if (pour_entete) {
        os << "#pragma once\n\n";

        inclus_systeme(os, "cstdint");
        inclus_systeme(os, "iosfwd");

        prodeclare_struct(os, "IdentifiantCode");

        os << "\n";

        genere_déclaration_identifiants_code(proteines, os, pour_entete, "intrinsèques");

        POUR (proteines) {
            auto protéine_énum = it->comme_enum();
            if (!protéine_énum) {
                continue;
            }

            it->genere_code_cpp(os, true);
        }

        return;
    }

    inclus(os, "intrinseques.hh");
    inclus_systeme(os, "iostream");

    genere_déclaration_identifiants_code(proteines, os, pour_entete, "intrinsèques");

    POUR (proteines) {
        auto protéine_énum = it->comme_enum();
        if (!protéine_énum) {
            continue;
        }

        it->genere_code_cpp(os, false);
    }
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

    return type->comme_nominal()->accede_nom().nom_cpp() == "void";
}

static void genere_code_appel_intrinsèque_défaut(FluxSortieCPP &os, ProteineFonction *fonction)
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

static void genere_code_appel_intrinsèque_ignorée(FluxSortieCPP &os,
                                                  ProteineFonction *fonction,
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
        if (params.nom.nom_cpp() == argument_résultat) {
            continue;
        }
        os << "        static_cast<void>(" << params.nom << ");\n";
    }

    if (!est_type_rien(type_sortie)) {
        os << "        empile<" << *type_sortie << ">(résultat);\n";
    }
}

static void genere_code_appel_intrinsèque(FluxSortieCPP &os, ProteineFonction *fonction)
{
    auto symbole = fonction->donne_symbole_gcc();

    if (symbole == "__builtin___clear_cache" || symbole == "__builtin_prefetch") {
        genere_code_appel_intrinsèque_ignorée(os, fonction, "{}");
    }
    else if (symbole == "__builtin_expect" || symbole == "__builtin_expect_with_probability") {
        genere_code_appel_intrinsèque_ignorée(
            os, fonction, fonction->donne_paramètres()[0].nom.nom_cpp());
    }
    else {
        genere_code_appel_intrinsèque_défaut(os, fonction);
    }
}

static void genere_code_machine_virtuelle(const kuri::tableau<Proteine *> &proteines,
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

    POUR (proteines) {
        if (!it->est_fonction()) {
            continue;
        }
        auto fonction = dynamic_cast<ProteineFonction *>(it);
        auto symbole = fonction->donne_symbole_gcc();

        os << "    if (decl->nom_symbole == \"" << symbole << "\") {\n";
        genere_code_appel_intrinsèque(os, fonction);
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

    auto gerante_chaine = dls::outils::Synchrone<GeranteChaine>();
    auto table_identifiants = dls::outils::Synchrone<TableIdentifiant>();
    auto contexte_lexage = ContexteLexage{gerante_chaine, table_identifiants, imprime_erreur};

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
        genere_code_cpp(syntaxeuse.proteines, flux, true);
    }
    else if (nom_fichier_sortie.nom_fichier() == "intrinseques.cc") {
        {
            std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
            auto flux = FluxSortieCPP(fichier_sortie);
            genere_code_cpp(syntaxeuse.proteines, flux, false);
        }
        {
            // Génère le fichier pour le module Kuri
            // Apparemment, ce n'est pas possible de le faire via CMake
            nom_fichier_sortie.remplace_nom_fichier("../modules/Kuri/intrinseques.kuri");
            std::ofstream fichier_sortie(vers_std_path(nom_fichier_sortie));
            auto flux = FluxSortieKuri(fichier_sortie);
            genere_code_kuri(syntaxeuse.proteines, flux);
        }
    }
    else if (nom_fichier_sortie.nom_fichier() == "machine_virtuelle_intrinseques.cc") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        auto flux = FluxSortieCPP(fichier_sortie);
        genere_code_machine_virtuelle(syntaxeuse.proteines, flux);
    }

    if (!remplace_si_different(nom_fichier_tmp, argv[1])) {
        return 1;
    }

    return 0;
}
