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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "rmsd.hh"
#include "vecteur.hh"

#include "biblinternes/outils/constantes.h"

namespace dls::math {

template <typename T>
auto calcule_rotation_kabsch(
		vec3<T> const &mov0,
		vec3<T> const &mov1,
		vec3<T> const &mov2,
		vec3<T> const &ref0,
		vec3<T> const &ref1,
		vec3<T> const &ref2)
{
	double liste_rotationne[3][3];
	liste_rotationne[0][0] = static_cast<double>(mov0.x);
	liste_rotationne[0][1] = static_cast<double>(mov0.y);
	liste_rotationne[0][2] = static_cast<double>(mov0.z);

	liste_rotationne[1][0] = static_cast<double>(mov1.x);
	liste_rotationne[1][1] = static_cast<double>(mov1.y);
	liste_rotationne[1][2] = static_cast<double>(mov1.z);

	liste_rotationne[2][0] = static_cast<double>(mov2.x);
	liste_rotationne[2][1] = static_cast<double>(mov2.y);
	liste_rotationne[2][2] = static_cast<double>(mov2.z);

	double liste_reference[3][3];
	liste_reference[0][0] = static_cast<double>(ref0.x);
	liste_reference[0][1] = static_cast<double>(ref0.y);
	liste_reference[0][2] = static_cast<double>(ref0.z);

	liste_reference[1][0] = static_cast<double>(ref1.x);
	liste_reference[1][1] = static_cast<double>(ref1.y);
	liste_reference[1][2] = static_cast<double>(ref1.z);

	liste_reference[2][0] = static_cast<double>(ref2.x);
	liste_reference[2][1] = static_cast<double>(ref2.y);
	liste_reference[2][2] = static_cast<double>(ref2.z);

	auto const taille_liste = 3;
	auto const movcom = (mov0 + mov1 + mov2) / 3.0f;

	double mov_com[3];
	mov_com[0] = static_cast<double>(movcom.x);
	mov_com[1] = static_cast<double>(movcom.y);
	mov_com[2] = static_cast<double>(movcom.z);

	auto const refcom = (ref0 + ref1 + ref2) / 3.0f;
	auto const movversref = refcom - movcom;

	double mov_vers_ref[3];
	mov_vers_ref[0] = static_cast<double>(movversref[0]);
	mov_vers_ref[1] = static_cast<double>(movversref[1]);
	mov_vers_ref[2] = static_cast<double>(movversref[2]);

	double U[3][3];
	double rmsd;

	calculate_rotation_rmsd(liste_reference, liste_rotationne, taille_liste, mov_com, mov_vers_ref, U, &rmsd);

	auto x = std::atan2( U[1][2], U[2][2]  ) * constantes<double>::POIDS_RAD_DEG;
	auto y = std::atan2(-U[0][2], std::sqrt(U[1][2]*U[1][2] + U[2][2]*U[2][2])) * constantes<double>::POIDS_RAD_DEG;
	auto z = std::atan2( U[0][1], U[0][0]) * constantes<double>::POIDS_RAD_DEG;

	return vec3<T>(
				static_cast<T>(x - 180.0),
				static_cast<T>(180.0 - y),
				static_cast<T>(180.0 - z));
}

}  /* namespace dls::math */
