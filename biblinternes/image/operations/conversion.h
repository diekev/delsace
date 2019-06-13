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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "../pixel.h"

#include "../outils/couleurs.h"

#include "../../math/matrice/matrice.hh"

namespace dls {
namespace image {
namespace operation {

/**
 * Retourne une matrice dont les valeurs sont le résultat de la conversion des
 * valeurs décimales (type float) en valeurs entières (type unsigned char) de la
 * matrice spécifiée en paramètre.
 */
math::matrice<PixelChar> converti_en_char(const math::matrice<PixelFloat> &image);

/**
 * Retourne une matrice dont les valeurs sont le résultat de la conversion des
 * valeurs entières (type unsigned char) en valeurs décimales (type float) de la
 * matrice spécifiée en paramètre.
 */
math::matrice<PixelFloat> converti_en_float(const math::matrice<PixelChar> &image);

/**
 * Converti une image avec un seul canal (par exemple luminance) vers une image
 * avec quatre canaux.
 */
template <ConceptNombre nombre>
static auto converti_float_pixel(const math::matrice<nombre> &image)
{
	math::matrice<Pixel<nombre>> resultat(image.dimensions());

	for (auto l = 0; l < image.nombre_lignes(); ++l) {
		for (auto c = 0; c < image.nombre_colonnes(); ++c) {
			resultat[l][c] = Pixel<nombre>(image[l][c]);
		}
	}

	return resultat;
}

/**
 * Retourne une matrice dont les valeurs sont le résultat de la conversion des
 * valeurs colorimétriques en valeurs de luminance de la matrice spécifiée.
 *
 * La fonction utilisée pour performer la conversion est basé sur la définition
 * Rec. 709 de la luminance.
 */
template <ConceptNombre nombre>
auto luminance(const math::matrice<Pixel<nombre>> &image)
{
	math::matrice<nombre> luma(image.dimensions());

	for (int x = 0; x < image.nombre_lignes(); ++x) {
		for (int y = 0; y < image.nombre_colonnes(); ++y) {
			luma[x][y] = outils::luminance_709(image[x][y].r, image[x][y].g, image[x][y].b);
		}
	}

	return luma;
}

}  /* namespace operation */
}  /* namespace image */
}  /* namespace dls */
