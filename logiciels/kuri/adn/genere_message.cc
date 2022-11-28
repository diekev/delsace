/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

/* Fichier de génération du code pour les options de compilation. */

#include <filesystem>
#include <fstream>

#include "biblinternes/outils/conditions.h"

#include "parsage/gerante_chaine.hh"
#include "parsage/identifiant.hh"
#include "parsage/lexeuse.hh"
#include "parsage/modules.hh"

#include "structures/tableau.hh"

#include "adn.hh"
#include "outils_dependants_sur_lexemes.hh"

static void genere_code_cpp(FluxSortieCPP &os,
                            kuri::tableau<Proteine *> const &proteines,
                            bool pour_entete)
{
    os << "// Fichier généré automatiquement, NE PAS ÉDITER !\n\n";

    if (pour_entete) {
        os << "#pragma once\n\n";
        inclus_systeme(os, "iostream");
        os << '\n';
        inclus(os, "biblinternes/outils/definitions.h");
        os << '\n';
        inclus(os, "structures/chaine_statique.hh");
        os << '\n';
        prodeclare_struct(os, "EspaceDeTravail");
        prodeclare_struct(os, "Module");
        prodeclare_struct(os, "NoeudCode");
        os << '\n';
    }
    else {
        inclus(os, "message.hh");
        os << '\n';
    }

    POUR (proteines) {
        it->genere_code_cpp(os, pour_entete);
    }
}

static void genere_code_kuri(FluxSortieKuri &os, kuri::tableau<Proteine *> const &proteines)
{
    POUR (proteines) {
        it->genere_code_kuri(os);
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

    auto fichier = Fichier();
    fichier.tampon_ = lng::tampon_source(texte.c_str());
    fichier.chemin_ = chemin_adn_ipa;

    auto gerante_chaine = dls::outils::Synchrone<GeranteChaine>();
    auto table_identifiants = dls::outils::Synchrone<TableIdentifiant>();
    auto contexte_lexage = ContexteLexage{gerante_chaine, table_identifiants, imprime_erreur};

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

    auto nom_fichier_tmp = chemin_temporaire(nom_fichier_sortie.filename());

    if (nom_fichier_sortie.filename() == "message.hh") {
        std::ofstream fichier_sortie(nom_fichier_tmp);
        auto flux = FluxSortieCPP(fichier_sortie);
        genere_code_cpp(flux, syntaxeuse.proteines, true);
    }
    else if (nom_fichier_sortie.filename() == "message.cc") {
        {
            std::ofstream fichier_sortie(nom_fichier_tmp);
            auto flux = FluxSortieCPP(fichier_sortie);
            genere_code_cpp(flux, syntaxeuse.proteines, false);
        }
        {
            // Génère le fichier de message pour le module Compilatrice
            // Apparemment, ce n'est pas possible de le faire via CMake
            nom_fichier_sortie.replace_filename("../modules/Compilatrice/message.kuri");
            std::ofstream fichier_sortie(nom_fichier_sortie);
            auto flux = FluxSortieKuri(fichier_sortie);
            genere_code_kuri(flux, syntaxeuse.proteines);
        }
    }

    remplace_si_different(nom_fichier_tmp.c_str(), argv[1]);

    return 0;
}
