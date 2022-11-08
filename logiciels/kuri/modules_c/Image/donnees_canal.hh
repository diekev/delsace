/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

#include "biblinternes/structures/tableau.hh"

#include "image.h"

namespace image {

template <typename TypeParametres>
struct DonneesCanal {
    int largeur = 0;
    int hauteur = 0;
    const float *donnees_entree = nullptr;
    float *donnees_sortie = nullptr;

    TypeParametres params;

    IMG_Fenetre fenetre{};
};

template <typename TypeParametres>
inline long calcule_index(DonneesCanal<TypeParametres> &image, int i, int j)
{
    return j * image.largeur + i;
}

template <typename TypeParametres>
inline float valeur_entree(DonneesCanal<TypeParametres> &image, int i, int j)
{
    if (i < 0 || i >= image.largeur) {
        return 0.0f;
    }

    if (j < 0 || j >= image.hauteur) {
        return 0.0f;
    }

    auto index = calcule_index(image, i, j);
    return image.donnees_entree[index];
}

template <typename TypeParametres>
inline float valeur_sortie(DonneesCanal<TypeParametres> &image, int i, int j)
{
    if (i < 0 || i >= image.largeur) {
        return 0.0f;
    }

    if (j < 0 || j >= image.hauteur) {
        return 0.0f;
    }

    auto index = calcule_index(image, i, j);
    return image.donnees_sortie[index];
}

template <typename TypeParametres>
auto extrait_canaux_et_cree_sorties(const AdaptriceImage &entree, AdaptriceImage &sortie)
{
    dls::tableau<DonneesCanal<TypeParametres>> canaux;

    DescriptionImage desc;
    entree.decris_image(&entree, &desc);

    IMG_Fenetre fenetre;
    entree.fenetre_image(&entree, &fenetre);

    auto const nombre_de_calques = entree.nombre_de_calques(&entree);

    /* Crée les calques de sorties. */
    for (auto i = 0; i < nombre_de_calques; i++) {
        auto const calque_entree = entree.calque_pour_index(&entree, i);

        char *ptr_nom;
        long taille_nom;
        entree.nom_calque(&entree, calque_entree, &ptr_nom, &taille_nom);

        auto calque_sortie = sortie.cree_calque(&sortie, ptr_nom, taille_nom);

        auto const nombre_de_canaux = entree.nombre_de_canaux(&entree, calque_entree);

        for (auto j = 0; j < nombre_de_canaux; j++) {
            auto const canal_entree = entree.canal_pour_index(&entree, calque_entree, j);
            entree.nom_canal(&entree, canal_entree, &ptr_nom, &taille_nom);

            auto const donnees_canal_entree = entree.donnees_canal_pour_lecture(&entree,
                                                                                canal_entree);

            auto canal_sortie = sortie.ajoute_canal(&sortie, calque_sortie, ptr_nom, taille_nom);

            auto donnees_canal_sortie = sortie.donnees_canal_pour_ecriture(&sortie, canal_sortie);

            DonneesCanal<TypeParametres> donnees;
            donnees.hauteur = desc.hauteur;
            donnees.largeur = desc.largeur;
            donnees.fenetre = fenetre;
            donnees.donnees_entree = donnees_canal_entree;
            donnees.donnees_sortie = donnees_canal_sortie;

            canaux.ajoute(donnees);
        }
    }

    return canaux;
}

}  // namespace image
