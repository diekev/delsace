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

#include "biblinternes/outils/gna.hh"
#include "biblinternes/tests/test_unitaire.hh"

#include "../structures/ensemble.hh"
#include "../structures/file.hh"

static void iteration_test_ensemble(dls::test_unitaire::Controleuse &controleuse, uint iteration)
{
    auto gna = GNA(iteration);
    const auto nombre_d_elements = gna.uniforme(0, 1024);

    auto elements = kuri::tableau<int>(nombre_d_elements);
    for (auto i = 0; i < elements.taille(); ++i) {
        elements[i] = i;
    }

    auto e = kuri::ensemble<int>();

    /* Insertion. */

    for (auto i = 0; i < nombre_d_elements; i++) {
        CU_VERIFIE_CONDITION(controleuse, !e.possede(elements[i]));

        e.insere(elements[i]);
        CU_VERIFIE_EGALITE(controleuse, e.taille(), static_cast<long>(i) + 1);
        CU_VERIFIE_CONDITION(controleuse, e.possede(elements[i]));
    }
    CU_VERIFIE_EGALITE(controleuse, e.taille(), static_cast<long>(nombre_d_elements));

    e.efface();
    CU_VERIFIE_EGALITE(controleuse, e.taille(), 0l);

    auto element_unique = gna.uniforme(std::numeric_limits<int>::min(),
                                       std::numeric_limits<int>::max());

    for (auto i = 0; i < elements.taille(); ++i) {
        elements[i] = element_unique;
    }
    CU_VERIFIE_CONDITION(controleuse, !e.possede(element_unique));

    for (auto i = 0; i < nombre_d_elements; i++) {
        e.insere(elements[i]);
        CU_VERIFIE_EGALITE(controleuse, e.taille(), 1l);
        CU_VERIFIE_CONDITION(controleuse, e.possede(elements[i]));
    }

    /* Suppression. */
    e.supprime(element_unique);
    CU_VERIFIE_CONDITION(controleuse, !e.possede(element_unique));
    CU_VERIFIE_CONDITION(controleuse, e.est_vide());

    for (auto i = 0; i < nombre_d_elements; i++) {
        e.insere(i);
    }
    CU_VERIFIE_EGALITE(controleuse, e.taille(), static_cast<long>(nombre_d_elements));

    for (auto i = 0; i < nombre_d_elements; i++) {
        e.supprime(i);
        CU_VERIFIE_CONDITION(controleuse, !e.possede(i));
        CU_VERIFIE_EGALITE(controleuse, e.taille(), static_cast<long>(nombre_d_elements - i - 1));
    }
    CU_VERIFIE_CONDITION(controleuse, e.est_vide());
}

void test_ensemble(dls::test_unitaire::Controleuse &controleuse)
{
    for (uint i = 0; i < 1000; ++i) {
        iteration_test_ensemble(controleuse, i);
    }
}

static void iteration_test_file(dls::test_unitaire::Controleuse &controleuse, uint iteration)
{
    auto gna = GNA(iteration);
    const auto nombre_d_elements = gna.uniforme(0, 1024);

    auto tableau_controle = kuri::tableau<long>(nombre_d_elements);
    for (int i = 0; i < nombre_d_elements; i++) {
        tableau_controle[i] = gna.uniforme(std::numeric_limits<long>::min(),
                                           std::numeric_limits<long>::max());
    }

    auto file = kuri::file<long>();

    for (int i = 0; i < 2; i++) {
        for (auto v : tableau_controle) {
            file.enfile(v);
        }

        CU_VERIFIE_CONDITION(controleuse, !file.est_vide());
        CU_VERIFIE_EGALITE(controleuse, file.taille(), tableau_controle.taille());
        CU_VERIFIE_EGALITE(controleuse, file.taille(), static_cast<long>(nombre_d_elements));

        for (auto v : tableau_controle) {
            auto valeur_defilee = file.defile();
            CU_VERIFIE_EGALITE(controleuse, v, valeur_defilee);
        }

        CU_VERIFIE_CONDITION(controleuse, file.est_vide());
    }
}

void test_file(dls::test_unitaire::Controleuse &controleuse)
{
    for (uint i = 0; i < 1000; ++i) {
        iteration_test_file(controleuse, i);
    }
}

int main()
{
    dls::test_unitaire::Controleuse controleuse;
    controleuse.ajoute_fonction(test_ensemble);
    controleuse.ajoute_fonction(test_file);
    controleuse.performe_controles();

    controleuse.imprime_resultat();

    if (controleuse.possede_erreur()) {
        return 1;
    }

    return 0;
}
