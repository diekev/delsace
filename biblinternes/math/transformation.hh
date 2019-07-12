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

#include "matrice.hh"
#include "vecteur.hh"

namespace math {

class transformation {
	dls::math::mat4x4d m_matrice = dls::math::mat4x4d(1.0);
	dls::math::mat4x4d m_inverse = dls::math::mat4x4d(1.0);

public:
	transformation() = default;

	transformation(
			dls::math::mat4x4d const &matrice,
			dls::math::mat4x4d const &inverse);

	explicit transformation(const double matrice[4][4]);

	explicit transformation(dls::math::mat4x4d const &matrice);

	dls::math::mat4x4d matrice() const;

	dls::math::mat4x4d inverse() const;

	dls::math::vec3d operator()(dls::math::vec3d const &vecteur) const;

	void operator()(dls::math::vec3d const &vecteur, dls::math::vec3d *vecteur_retour) const;

	dls::math::point3d operator()(dls::math::point3d const &point) const;

	void operator()(dls::math::point3d const &point, dls::math::point3d *point_retour) const;

	bool possede_echelle() const;

	transformation &operator*=(transformation const &transforme);
};

bool operator==(transformation const &a, transformation const &b);

bool operator!=(transformation const &a, transformation const &b);

inline transformation operator*(transformation const &a, transformation const &b)
{
	auto const matrice = a.matrice() * b.matrice();
	auto const inverse = b.inverse() * a.inverse();

	return transformation(matrice, inverse);
}

transformation inverse(transformation const &transforme);

transformation translation(const double x, const double y, const double z);

transformation translation(dls::math::vec3d const &vecteur);

transformation echelle(const double x, const double y, const double z);

transformation echelle(dls::math::vec3d const &vecteur);

transformation rotation_x(const double angle);

transformation rotation_y(const double angle);

transformation rotation_z(const double angle);

transformation rotation(const double angle, dls::math::vec3d const &vecteur);

transformation vise(
		dls::math::vec3d const &position,
		dls::math::vec3d const &mire,
		dls::math::vec3d const &haut);

}  /* namespace math */
