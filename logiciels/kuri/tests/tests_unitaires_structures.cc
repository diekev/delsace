/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "biblinternes/outils/gna.hh"
#include "biblinternes/tests/test_unitaire.hh"

#include "../structures/ensemble.hh"
#include "../structures/file.hh"

static void iteration_test_ensemble(dls::test_unitaire::Controleuse &controleuse, uint32_t iteration)
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
        CU_VERIFIE_EGALITE(controleuse, e.taille(), static_cast<int64_t>(i) + 1);
        CU_VERIFIE_CONDITION(controleuse, e.possede(elements[i]));
    }
    CU_VERIFIE_EGALITE(controleuse, e.taille(), static_cast<int64_t>(nombre_d_elements));

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
    CU_VERIFIE_EGALITE(controleuse, e.taille(), static_cast<int64_t>(nombre_d_elements));

    for (auto i = 0; i < nombre_d_elements; i++) {
        e.supprime(i);
        CU_VERIFIE_CONDITION(controleuse, !e.possede(i));
        CU_VERIFIE_EGALITE(controleuse, e.taille(), static_cast<int64_t>(nombre_d_elements - i - 1));
    }
    CU_VERIFIE_CONDITION(controleuse, e.est_vide());
}

void test_ensemble(dls::test_unitaire::Controleuse &controleuse)
{
    for (uint32_t i = 0; i < 1000; ++i) {
        iteration_test_ensemble(controleuse, i);
    }
}

static void iteration_test_file(dls::test_unitaire::Controleuse &controleuse, uint32_t iteration)
{
    auto gna = GNA(iteration);
    const auto nombre_d_elements = gna.uniforme(0, 1024);

    auto tableau_controle = kuri::tableau<int64_t>(nombre_d_elements);
    for (int i = 0; i < nombre_d_elements; i++) {
        tableau_controle[i] = gna.uniforme(std::numeric_limits<int64_t>::min(),
                                           std::numeric_limits<int64_t>::max());
    }

    auto file = kuri::file<int64_t>();

    for (int i = 0; i < 2; i++) {
        for (auto v : tableau_controle) {
            file.enfile(v);
        }

        CU_VERIFIE_CONDITION(controleuse, !file.est_vide());
        CU_VERIFIE_EGALITE(controleuse, file.taille(), tableau_controle.taille());
        CU_VERIFIE_EGALITE(controleuse, file.taille(), static_cast<int64_t>(nombre_d_elements));

        for (auto v : tableau_controle) {
            auto valeur_defilee = file.defile();
            CU_VERIFIE_EGALITE(controleuse, v, valeur_defilee);
        }

        CU_VERIFIE_CONDITION(controleuse, file.est_vide());
    }
}

void test_file(dls::test_unitaire::Controleuse &controleuse)
{
    for (uint32_t i = 0; i < 1000; ++i) {
        iteration_test_file(controleuse, i);
    }
}

static void test_tableau(dls::test_unitaire::Controleuse &controleuse)
{
    /* Construction d'un tableau via un std::initializer_list. */
    auto tableau = kuri::tableau<int>({0, 1, 2, 3, 4, 5});

    CU_VERIFIE_EGALITE(controleuse, tableau.taille(), 6l);

    for (int i = 0; i < 6; i++) {
        CU_VERIFIE_EGALITE(controleuse, tableau[i], i);
    }
}

int main()
{
    dls::test_unitaire::Controleuse controleuse;
    controleuse.ajoute_fonction(test_tableau);
    controleuse.ajoute_fonction(test_ensemble);
    controleuse.ajoute_fonction(test_file);
    controleuse.performe_controles();

    controleuse.imprime_resultat();

    if (controleuse.possede_erreur()) {
        return 1;
    }

    return 0;
}
