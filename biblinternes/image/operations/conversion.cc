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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "conversion.h"

#include <iostream>

namespace dls {
namespace image {
namespace operation {

math::matrice_dyn<PixelChar> converti_en_char(const math::matrice_dyn<PixelFloat> &image)
{
	math::matrice_dyn<PixelChar> resultat(image.dimensions());

	for (int x = 0; x < image.nombre_lignes(); ++x) {
		for (int y = 0; y < image.nombre_colonnes(); ++y) {
			resultat[x][y].r = static_cast<unsigned char>(image[x][y].r * 255.0f);
			resultat[x][y].g = static_cast<unsigned char>(image[x][y].g * 255.0f);
			resultat[x][y].b = static_cast<unsigned char>(image[x][y].b * 255.0f);
			resultat[x][y].a = static_cast<unsigned char>(image[x][y].a * 255.0f);
		}
	}

	return resultat;
}

math::matrice_dyn<PixelFloat> converti_en_float(const math::matrice_dyn<PixelChar> &image)
{
	math::matrice_dyn<PixelFloat> resultat(image.dimensions());

	static constexpr auto echelle = 1.0f / 255.0f;

	for (int x = 0; x < image.nombre_lignes(); ++x) {
		for (int y = 0; y < image.nombre_colonnes(); ++y) {
			resultat[x][y].r = static_cast<float>(image[x][y].r) * echelle;
			resultat[x][y].g = static_cast<float>(image[x][y].g) * echelle;
			resultat[x][y].b = static_cast<float>(image[x][y].b) * echelle;
			resultat[x][y].a = static_cast<float>(image[x][y].a) * echelle;
		}
	}

	return resultat;
}

}  /* namespace operation */
}  /* namespace image */
}  /* namespace dls */
