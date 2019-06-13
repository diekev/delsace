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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "empreinte.h"

#include "conversion.h"
#include "echantillonage.h"

#include "../flux/lecture.h"

namespace dls {
namespace image {
namespace operation {

size_t empreinte_image(const filesystem::path &chemin_image)
{
	auto image = flux::lecture_float(chemin_image);

	/* 1. Réduit les couleurs. */
	auto luma = luminance(image);

	/* 2. Réduit la taille. */
	//std::cerr << "Redimensionnage image....\n";
	auto resultat = math::matrice<float>(math::Hauteur(8), math::Largeur(9));
	redimensionne(luma, resultat);

	/* 3. Calcule l'empreinte. */
	size_t empreinte = 0;

	for (auto l = 0; l < 8; ++l) {
		for (auto c = 0; c < 8; ++c) {
			const auto difference = resultat[l][c] < resultat[l][c + 1];
			empreinte |= static_cast<size_t>(difference << (l + c * 8));
		}
	}

	return empreinte;
}

}  /* namespace operation */
}  /* namespace image */
}  /* namespace dls */
