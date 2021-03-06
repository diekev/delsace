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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

extern "C" {

enum ResultatOperation {
    OK,
    IMAGE_INEXISTANTE,
    TYPE_IMAGE_NON_SUPPORTE,
};

struct Image {
    float *donnees;
    long taille_donnees;

    int largeur;
    int hauteur;
    int nombre_composants;
};

ResultatOperation IMG_ouvre_image(const char *chemin, Image *image);

ResultatOperation IMG_ecris_image(const char *chemin, Image *image);

void IMG_detruit_image(Image *image);

void IMG_calcul_empreinte_floue(
    const char *chemin, int composant_x, int composant_y, char *resultat, long *taille_resultat);
}
