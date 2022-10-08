/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum ResultatOperation {
    OK,
    IMAGE_INEXISTANTE,
    TYPE_IMAGE_NON_SUPPORTE,
};

struct ImageIO {
    float *donnees;
    long taille_donnees;

    int largeur;
    int hauteur;
    int nombre_composants;
};

enum ResultatOperation IMG_ouvre_image(const char *chemin, struct ImageIO *image);

enum ResultatOperation IMG_ecris_image(const char *chemin, struct ImageIO *image);

void IMG_detruit_image(struct ImageIO *image);

void IMG_calcul_empreinte_floue(
    const char *chemin, int composant_x, int composant_y, char *resultat, long *taille_resultat);

#ifdef __cplusplus
}
#endif
