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

#include "echantillonage.h"

namespace dls {
namespace image {
namespace operation {

tuple_vecteurs calcul_index_et_poids(
		const long taille_soure,
		const long taille_dest)
{
	dls::tableau<int>   index_gauche(taille_soure);
	dls::tableau<int>   index_droite(taille_soure);
	dls::tableau<float> poids(taille_soure);

	const auto echelle = static_cast<float>(taille_dest) / static_cast<float>(taille_soure);

	/* On ne calcule les index que jusqu'à l'avant-dernier puisqu'il faut
	 * nécessairement que les derniers éléments de la source et de la
	 * destination soient les mêmes. */
	for (long i = 0; i < taille_soure - 1; i++) {
		const auto i0 = static_cast<float>(i) * echelle;
		const auto i1 = static_cast<float>(i + 1) * echelle;

		const auto ii0 = static_cast<int>(i0);
		const auto ii1 = static_cast<int>(i1);

		index_gauche[i] = ii0;
		index_droite[i] = ii1;

		if (ii0 == ii1) {
			/* Une portion entière du pixel de l'image source va vers un seul
			 * pixel de la destination. */
			poids[i] = echelle;
		}
		else {
			/* Le pixel sur l'image source va vers deux pixels adjacents sur
			 * la destination. */
			poids[i] = (static_cast<float>(ii1) - i0);
		}
	}

	/* Les derniers éléments de la source et de la destinations doivent être les
	 * mêmes. */
	index_gauche[taille_soure - 1] = static_cast<int>(taille_dest - 1);
	index_droite[taille_soure - 1] = static_cast<int>(taille_dest - 1);
	poids[taille_soure - 1] = echelle;

	return std::tuple{ index_gauche, index_droite, poids };
}

}  /* namespace operation */
}  /* namespace image */
}  /* namespace dls */
