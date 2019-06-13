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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1201, USA.
 *
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "../../math/matrice/matrice.h"
#include "../pixel.h"

namespace dls {
namespace image {
namespace operation {

/**
 * Extrait le canal spécifié d'une matrice de pixel. Le canal est copié dans le
 * tableau pointé par `donnees`. La taille du tableau pointé n'est pas vérifié
 * et il est supposé que sa taille est suffisante et que le pointeur n'est pas
 * nul.
 */
template <ConceptNombre nombre>
void extrait_canal(const math::matrice<Pixel<nombre>> &image, nombre *donnees, int canal)
{
	for (int l = 0; l < image.nombre_lignes(); ++l) {
		for (int c = 0; c < image.nombre_colonnes(); ++c) {
			switch (canal) {
				case CANAL_R:
					*donnees++ = image[l][c].r;
					break;
				case CANAL_G:
					*donnees++ = image[l][c].g;
					break;
				case CANAL_B:
					*donnees++ = image[l][c].b;
					break;
				case CANAL_A:
					*donnees++ = image[l][c].a;
					break;
			}
		}
	}
}


/**
 * Retourne la valeur maximale d'une matrice de pixel.
 */
template <ConceptNombre nombre>
Pixel<nombre> valeur_maximale(const math::matrice<Pixel<nombre>> &image)
{
	Pixel<nombre> resultat;
	resultat.r = std::numeric_limits<nombre>::min();
	resultat.g = std::numeric_limits<nombre>::min();
	resultat.b = std::numeric_limits<nombre>::min();
	resultat.a = std::numeric_limits<nombre>::min();

	for (int l = 0; l < image.nombre_lignes(); ++l) {
		for (int c = 0; c < image.nombre_colonnes(); ++c) {
			resultat.r = std::max(resultat.r, image[l][c].r);
			resultat.g = std::max(resultat.g, image[l][c].g);
			resultat.b = std::max(resultat.b, image[l][c].b);
			resultat.a = std::max(resultat.a, image[l][c].a);
		}
	}

	return resultat;
}

/**
 * Retourne la valeur minimale d'une matrice de pixel.
 */
template <ConceptNombre nombre>
Pixel<nombre> valeur_minimale(const math::matrice<Pixel<nombre>> &image)
{
	Pixel<nombre> resultat;
	resultat.r = std::numeric_limits<nombre>::max();
	resultat.g = std::numeric_limits<nombre>::max();
	resultat.b = std::numeric_limits<nombre>::max();
	resultat.a = std::numeric_limits<nombre>::max();

	for (int l = 0; l < image.nombre_lignes(); ++l) {
		for (int c = 0; c < image.nombre_colonnes(); ++c) {
			resultat.r = std::min(resultat.r, image[l][c].r);
			resultat.g = std::min(resultat.g, image[l][c].g);
			resultat.b = std::min(resultat.b, image[l][c].b);
			resultat.a = std::min(resultat.a, image[l][c].a);
		}
	}

	return resultat;
}

}  /* namespace operation */
}  /* namespace image */
}  /* namespace dls */
