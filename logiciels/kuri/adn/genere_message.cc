/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

/* Fichier de génération du code pour les options de compilation. */

#include <fstream>
#include <iostream>

#include "parsage/gerante_chaine.hh"
#include "parsage/identifiant.hh"
#include "parsage/lexeuse.hh"
#include "parsage/modules.hh"

#include "structures/chemin_systeme.hh"
#include "structures/tableau.hh"

#include "adn.hh"
#include "outils_dependants_sur_lexemes.hh"

static void génère_code_messages_cpp(FluxSortieCPP &os,
                                     kuri::tableau<Protéine *> const &protéines,
                                     bool pour_entête)
{
    os << "// Fichier généré automatiquement, NE PAS ÉDITER !\n\n";

    if (pour_entête) {
        os << "#pragma once\n\n";
        inclus_système(os, "iosfwd");
        os << '\n';
        inclus(os, "utilitaires/macros.hh");
        os << '\n';
        inclus(os, "structures/chaine_statique.hh");
        os << '\n';
        prodéclare_struct(os, "EspaceDeTravail");
        prodéclare_struct(os, "Module");
        prodéclare_struct(os, "NoeudCode");
        os << '\n';
    }
    else {
        inclus(os, "message.hh");
        inclus_système(os, "iostream");
        os << '\n';
    }

    génère_code_cpp(os, protéines, pour_entête);
}

int main(int argc, const char **argv)
{
    if (argc != 4) {
        std::cerr << "Utilisation: " << argv[0] << " nom_fichier_sortie -i fichier_adn\n";
        return 1;
    }

    const auto chemin_adn_ipa = argv[3];
    auto nom_fichier_sortie = kuri::chemin_systeme(argv[1]);

    auto texte = charge_contenu_fichier(chemin_adn_ipa);

    auto fichier = Fichier();
    fichier.tampon_ = TamponSource(texte);
    fichier.chemin_ = chemin_adn_ipa;

    auto gérante_chaine = kuri::Synchrone<GeranteChaine>();
    auto table_identifiants = kuri::Synchrone<TableIdentifiant>();
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

    if (nom_fichier_sortie.nom_fichier() == "message.hh") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        auto flux = FluxSortieCPP(fichier_sortie);
        génère_code_messages_cpp(flux, syntaxeuse.protéines, true);
    }
    else if (nom_fichier_sortie.nom_fichier() == "message.cc") {
        {
            std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
            auto flux = FluxSortieCPP(fichier_sortie);
            génère_code_messages_cpp(flux, syntaxeuse.protéines, false);
        }
        {
            // Génère le fichier de message pour le module Compilatrice
            // Apparemment, ce n'est pas possible de le faire via CMake
            nom_fichier_sortie.remplace_nom_fichier("../modules/Compilatrice/message.kuri");
            std::ofstream fichier_sortie(vers_std_path(nom_fichier_sortie));
            auto flux = FluxSortieKuri(fichier_sortie);
            génère_code_kuri(flux, syntaxeuse.protéines);
        }
    }

    if (!remplace_si_différent(nom_fichier_tmp, argv[1])) {
        return 1;
    }

    return 0;
}
