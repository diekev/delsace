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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

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

    fichier_entree.read(donnees, static_cast<long>(taille_fichier));

#if 1
    {
        auto compilatrice = Compilatrice("");
        auto fichier = Fichier();
        auto vue_donnees = dls::vue_chaine(donnees, taille_fichier);
        fichier.charge_tampon(lng::tampon_source(dls::chaine(vue_donnees)));

        auto lexeuse = Lexeuse(compilatrice.contexte_lexage(), &fichier);
        lexeuse.performe_lexage();
    }
#else
    auto donnees_morceaux = reinterpret_cast<const id_morceau *>(donnees);
    auto nombre_morceaux = taille_fichier / static_cast<long>(sizeof(id_morceau));

    kuri::tableau<Lexeme> morceaux;
    morceaux.reserve(nombre_morceaux);

    for (auto i = 0; i < nombre_morceaux; ++i) {
        auto dm = Lexeme{};
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
        auto module = compilatrice.cree_module("", "");
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
