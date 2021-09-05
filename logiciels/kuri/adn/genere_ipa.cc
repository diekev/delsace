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

#include "adn.hh"

static void genere_code_kuri(const kuri::tableau<Proteine *> &proteines, FluxSortieKuri &os)
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
        inclus(os, "parsage/lexemes.hh");
        inclus(os, "structures/tableau.hh");
        os << "\n";
        prodeclare_struct(os, "EspaceDeTravail");
        prodeclare_struct(os, "IdentifiantCode");
        prodeclare_struct(os, "Message");
        prodeclare_struct(os, "Module");
        prodeclare_struct(os, "OptionsDeCompilation");
        prodeclare_struct(os, "TableIdentifiant");
        prodeclare_struct(os, "NoeudCode");
        prodeclare_struct(os, "NoeudCodeEnteteFonction");
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
        os << "// Implémentation « symbolique » des fonctions d'interface afin d'éviter les "
              "erreurs\n";
        os << "// de liaison des programmes. Ces fonctions sont soit implémentées via "
              "Compilatrice,\n";
        os << "// soit via EspaceDeTravail, ou directement dans la MachineVirtuelle.\n";
        os << "\n";
    }

    POUR (proteines) {
        it->genere_code_cpp(os, pour_entete);
    }

    // identifiants
    os << "namespace ID {\n";
    if (pour_entete) {
        POUR (proteines) {
            if (!it->est_fonction()) {
                continue;
            }
            os << "extern IdentifiantCode *" << it->nom().nom_cpp() << ";\n";
        }
    }
    else {
        POUR (proteines) {
            if (!it->est_fonction()) {
                continue;
            }
            os << "IdentifiantCode *" << it->nom().nom_cpp() << ";\n";
        }
    }
    os << "}\n\n";

    os << "void initialise_identifiants_ipa(TableIdentifiant &table)";

    if (pour_entete) {
        os << ";\n\n";
    }
    else {
        os << "\n{\n";
        POUR (proteines) {
            if (!it->est_fonction()) {
                continue;
            }
            os << "\tID::" << it->nom().nom_cpp() << " = table.identifiant_pour_chaine(\""
               << it->nom().nom_kuri() << "\");\n";
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
        POUR (proteines) {
            if (!it->est_fonction()) {
                continue;
            }
            os << "\tif (ident == ID::" << it->nom().nom_cpp() << ") {\n";
            os << "\t\treturn reinterpret_cast<type_fonction_compilatrice>(" << it->nom().nom_cpp()
               << ");\n";
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
        POUR (proteines) {
            if (!it->est_fonction()) {
                continue;
            }
            os << "\tif (ident == ID::" << it->nom().nom_cpp() << ") {\n";
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

    auto fichier = Fichier();
    fichier.tampon_ = lng::tampon_source(texte.c_str());
    fichier.chemin_ = chemin_adn_ipa;

    auto gerante_chaine = dls::outils::Synchrone<GeranteChaine>();
    auto table_identifiants = dls::outils::Synchrone<TableIdentifiant>();
    auto rappel_erreur = [](kuri::chaine message) { std::cerr << message << '\n'; };

    auto contexte_lexage = ContexteLexage{gerante_chaine, table_identifiants, rappel_erreur};

    auto lexeuse = Lexeuse(contexte_lexage, &fichier);
    lexeuse.performe_lexage();

    if (lexeuse.possede_erreur()) {
        return 1;
    }

    auto syntaxeuse = SyntaxeuseADN(&fichier);
    syntaxeuse.analyse();

    if (syntaxeuse.possede_erreur()) {
        return 1;
    }

    if (nom_fichier_sortie.filename() == "ipa.hh") {
        std::ofstream fichier_sortie(argv[1]);
        auto flux = FluxSortieCPP(fichier_sortie);
        genere_code_cpp(syntaxeuse.proteines, flux, true);
    }
    else if (nom_fichier_sortie.filename() == "ipa.cc") {
        {
            std::ofstream fichier_sortie(argv[1]);
            auto flux = FluxSortieCPP(fichier_sortie);
            genere_code_cpp(syntaxeuse.proteines, flux, false);
        }
        {
            // Génère le fichier de lexèmes pour le module Compilatrice
            // Apparemment, ce n'est pas possible de le faire via CMake
            nom_fichier_sortie.replace_filename("../modules/Compilatrice/ipa.kuri");
            std::ofstream fichier_sortie(nom_fichier_sortie);
            auto flux = FluxSortieKuri(fichier_sortie);
            genere_code_kuri(syntaxeuse.proteines, flux);
        }
    }

    return 0;
}
