/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#include <fstream>
#include <iostream>

#include "compilation/compilatrice.hh"
#include "compilation/erreur.h"
#include "compilation/syntaxeuse.hh"

#include "parsage/lexeuse.hh"
#include "parsage/modules.hh"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "Usage : charge_fichier_aleatoire FICHIER\n";
        return 1;
    }

    std::ifstream fichier_entree(argv[1]);

    fichier_entree.seekg(0, fichier_entree.end);
    auto const taille_fichier = fichier_entree.tellg();
    fichier_entree.seekg(0, fichier_entree.beg);

    char *donnees = new char[static_cast<size_t>(taille_fichier)];

    fichier_entree.read(donnees, static_cast<int64_t>(taille_fichier));

#if 1
    {
        auto compilatrice = Compilatrice("", {});
        auto fichier = Fichier();
        auto vue_donnees = dls::vue_chaine(donnees, taille_fichier);
        fichier.charge_tampon(lng::tampon_source(dls::chaine(vue_donnees)));

        auto lexeuse = Lexeuse(compilatrice.contexte_lexage(nullptr), &fichier);
        lexeuse.performe_lexage();
    }
#else
    auto donnees_morceaux = reinterpret_cast<const id_morceau *>(donnees);
    auto nombre_morceaux = taille_fichier / static_cast<int64_t>(sizeof(id_morceau));

    kuri::tableau<Lexème> morceaux;
    morceaux.reserve(nombre_morceaux);

    for (auto i = 0; i < nombre_morceaux; ++i) {
        auto dm = Lexème{};
        dm.genre = donnees_morceaux[i];
        /* rétabli une chaine car nous une décharge de la mémoire, donc les
         * pointeurs sont mauvais. */
        dm.chaine = "texte_test";
        dm.ligne_pos = 0ul;
        dm.module = 0;
        morceaux.ajoute(dm);
    }

    std::cerr << "Il y a " << nombre_morceaux << " morceaux.\n";

    {
        auto compilatrice = Compilatrice{};
        auto module = compilatrice.crée_module("", "");
        module->tampon = lng::tampon_source("texte_test");
        module->morceaux = morceaux;
        auto assembleuse = AssembleuseArbre(compilatrice);
        compilatrice.assembleuse = &assembleuse;
        auto analyseuse = Syntaxeuse(compilatrice, module, "");

        std::ostream os(nullptr);
        analyseuse.lance_analyse(os);
    }

    delete[] donnees;
#endif

    return 0;
}
