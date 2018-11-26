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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <delsace/math/matrice.hh>
#include <delsace/math/vecteur.hh>

namespace math {

class transformation {
	dls::math::mat4x4d m_matrice = dls::math::mat4x4d(1.0);
	dls::math::mat4x4d m_inverse = dls::math::mat4x4d(1.0);

public:
	transformation() = default;

	transformation(
			const dls::math::mat4x4d &matrice,
			const dls::math::mat4x4d &inverse);

	explicit transformation(const double matrice[4][4]);

	explicit transformation(const dls::math::mat4x4d &matrice);

	dls::math::mat4x4d matrice() const;

	dls::math::mat4x4d inverse() const;

	dls::math::vec3d operator()(const dls::math::vec3d &vecteur) const;

	void operator()(const dls::math::vec3d &vecteur, dls::math::vec3d *vecteur_retour) const;

	dls::math::point3d operator()(const dls::math::point3d &point) const;

	void operator()(const dls::math::point3d &point, dls::math::point3d *point_retour) const;

	bool possede_echelle() const;

	transformation &operator*=(const transformation &transforme);
};

bool operator==(const transformation &a, const transformation &b);

bool operator!=(const transformation &a, const transformation &b);

inline transformation operator*(const transformation &a, const transformation &b)
{
	const auto matrice = a.matrice() * b.matrice();
	const auto inverse = b.inverse() * a.inverse();

	return transformation(matrice, inverse);
}

transformation inverse(const transformation &transforme);

transformation translation(const double x, const double y, const double z);

transformation translation(const dls::math::vec3d &vecteur);

transformation echelle(const double x, const double y, const double z);

transformation echelle(const dls::math::vec3d &vecteur);

transformation rotation_x(const double angle);

transformation rotation_y(const double angle);

transformation rotation_z(const double angle);

transformation rotation(const double angle, const dls::math::vec3d &vecteur);

transformation vise(
		const dls::math::vec3d &position,
		const dls::math::vec3d &mire,
		const dls::math::vec3d &haut);

}  /* namespace math */
