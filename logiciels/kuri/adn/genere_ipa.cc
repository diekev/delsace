/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

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
    os << "// Prodéclarations de types opaques pour certains types non manipulable directement\n";
    os << "\n";
    os << "EspaceDeTravail :: struct #externe;\n";
    os << "Module :: struct #externe;\n";
    os << "\n";
    os << "// ----------------------------------------------------------------------------\n";
    os << "// Interface de métaprogrammation pour controler ou communiquer avec la Compilatrice\n";
    os << "\n";

    génère_code_kuri(os, protéines);
}

static void génère_code_cpp(const kuri::tableau<Protéine *> &protéines,
                            FluxSortieCPP &os,
                            bool pour_entête)
{
    os << "// Fichier généré automatiquement, NE PAS ÉDITER\n\n";

    if (pour_entête) {
        os << "#pragma once\n\n";
        inclus(os, "parsage/lexemes.hh");
        inclus(os, "structures/tableau.hh");
        os << "\n";
        prodéclare_struct(os, "EspaceDeTravail");
        prodéclare_struct(os, "Message");
        prodéclare_struct(os, "Module");
        prodéclare_struct(os, "OptionsDeCompilation");
        prodéclare_struct(os, "NoeudCode");
        prodéclare_struct(os, "NoeudCodeEntêteFonction");
        prodéclare_struct(os, "InfoType");
        os << "\n";
        prodéclare_struct_espace(os, "chaine_statique", "kuri", "");
        os << "\n";
    }
    else {
        inclus(os, "ipa.hh");
        os << "\n";
        inclus_système(os, "cassert");
        os << "\n\n";

        os << "// -----------------------------------------------------------------------------\n";
        os << "// Implémentation « symbolique » des fonctions d'interface afin d'éviter les "
              "erreurs\n";
        os << "// de liaison des programmes. Ces fonctions sont soit implémentées via "
              "Compilatrice,\n";
        os << "// soit via EspaceDeTravail, ou directement dans la MachineVirtuelle.\n";
        os << "\n";
    }

    genere_déclaration_identifiants_code(protéines, os, pour_entête, "ipa");

    génère_code_cpp(os, protéines, pour_entête);

    if (pour_entête) {
        os << "using type_fonction_compilatrice = void(*)();\n\n";
    }

    os << "type_fonction_compilatrice fonction_compilatrice_pour_ident(IdentifiantCode *ident)";

    if (pour_entête) {
        os << ";\n\n";
    }
    else {
        os << "\n{\n";
        POUR (protéines) {
            if (!it->est_fonction()) {
                continue;
            }
            os << "\tif (ident == ID::" << it->nom() << ") {\n";
            os << "\t\treturn reinterpret_cast<type_fonction_compilatrice>(" << it->nom()
               << ");\n";
            os << "\t}\n\n";
        }

        os << "\treturn nullptr;\n";
        os << "}\n\n";
    }

    os << "bool est_fonction_compilatrice(IdentifiantCode *ident)";

    if (pour_entête) {
        os << ";\n\n";
    }
    else {
        os << "\n{\n";
        POUR (protéines) {
            if (!it->est_fonction()) {
                continue;
            }
            os << "\tif (ident == ID::" << it->nom() << ") {\n";
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

    if (nom_fichier_sortie.nom_fichier() == "ipa.hh") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        auto flux = FluxSortieCPP(fichier_sortie);
        génère_code_cpp(syntaxeuse.protéines, flux, true);
    }
    else if (nom_fichier_sortie.nom_fichier() == "ipa.cc") {
        {
            std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
            auto flux = FluxSortieCPP(fichier_sortie);
            génère_code_cpp(syntaxeuse.protéines, flux, false);
        }
        {
            // Génère le fichier de lexèmes pour le module Compilatrice
            // Apparemment, ce n'est pas possible de le faire via CMake
            nom_fichier_sortie.remplace_nom_fichier("../modules/Compilatrice/ipa.kuri");
            std::ofstream fichier_sortie(vers_std_path(nom_fichier_sortie));
            auto flux = FluxSortieKuri(fichier_sortie);
            génère_code_kuri(syntaxeuse.protéines, flux);
        }
    }

    if (!remplace_si_différent(nom_fichier_tmp, argv[1])) {
        return 1;
    }

    return 0;
}
