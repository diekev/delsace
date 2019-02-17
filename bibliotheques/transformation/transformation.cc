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

#include "transformation.h"

namespace math {

transformation::transformation(dls::math::mat4x4d const &matrice, dls::math::mat4x4d const &inverse)
	: m_matrice(matrice)
	, m_inverse(inverse)
{}

transformation::transformation(const double matrice[4][4])
{
	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			m_matrice[i][j] = matrice[i][j];
		}
	}

	m_inverse = dls::math::inverse(m_matrice);
}

transformation::transformation(dls::math::mat4x4d const &matrice)
	: m_matrice(matrice)
	, m_inverse(dls::math::inverse(m_matrice))
{}

dls::math::mat4x4d transformation::matrice() const
{
	return m_matrice;
}

dls::math::mat4x4d transformation::inverse() const
{
	return m_inverse;
}

dls::math::vec3d transformation::operator()(dls::math::vec3d const &vecteur) const
{
	auto const x = vecteur[0];
	auto const y = vecteur[1];
	auto const z = vecteur[2];

	auto const px = m_matrice[0][0] * x + m_matrice[1][0] * y + m_matrice[2][0] * z;
	auto const py = m_matrice[0][1] * x + m_matrice[1][1] * y + m_matrice[2][1] * z;
	auto const pz = m_matrice[0][2] * x + m_matrice[1][2] * y + m_matrice[2][2] * z;

	return dls::math::vec3d(px, py, pz);
}

void transformation::operator()(dls::math::vec3d const &vecteur, dls::math::vec3d *vecteur_retour) const
{
	auto const x = vecteur[0];
	auto const y = vecteur[1];
	auto const z = vecteur[2];

	auto const px = m_matrice[0][0] * x + m_matrice[1][0] * y + m_matrice[2][0] * z;
	auto const py = m_matrice[0][1] * x + m_matrice[1][1] * y + m_matrice[2][1] * z;
	auto const pz = m_matrice[0][2] * x + m_matrice[1][2] * y + m_matrice[2][2] * z;

	vecteur_retour->x = px;
	vecteur_retour->y = py;
	vecteur_retour->z = pz;
}

dls::math::point3d transformation::operator()(dls::math::point3d const &point) const
{
	auto const x = point[0];
	auto const y = point[1];
	auto const z = point[2];

	auto const px = m_matrice[0][0] * x + m_matrice[1][0] * y + m_matrice[2][0] * z + m_matrice[3][0];
	auto const py = m_matrice[0][1] * x + m_matrice[1][1] * y + m_matrice[2][1] * z + m_matrice[3][1];
	auto const pz = m_matrice[0][2] * x + m_matrice[1][2] * y + m_matrice[2][2] * z + m_matrice[3][2];
	auto const pw = m_matrice[0][3] * x + m_matrice[1][3] * y + m_matrice[2][3] * z + m_matrice[3][3];

	if (pw == 1.0) {
		return dls::math::point3d(px, py, pz);
	}

	return dls::math::point3d(px, py, pz) / pw;
}

void transformation::operator()(dls::math::point3d const &point, dls::math::point3d *point_retour) const
{
	auto const x = point[0];
	auto const y = point[1];
	auto const z = point[2];

	auto const px = m_matrice[0][0] * x + m_matrice[1][0] * y + m_matrice[2][0] * z + m_matrice[3][0];
	auto const py = m_matrice[0][1] * x + m_matrice[1][1] * y + m_matrice[2][1] * z + m_matrice[3][1];
	auto const pz = m_matrice[0][2] * x + m_matrice[1][2] * y + m_matrice[2][2] * z + m_matrice[3][2];
	auto const pw = m_matrice[0][3] * x + m_matrice[1][3] * y + m_matrice[2][3] * z + m_matrice[3][3];

	point_retour->x = px;
	point_retour->y = py;
	point_retour->z = pz;

	if (pw != 1.0) {
		*point_retour /= pw;
	}
}

bool transformation::possede_echelle() const
{
	auto const a1 = dls::math::longueur((*this)(dls::math::vec3d(1.0, 0.0, 0.0)));
	auto const a2 = dls::math::longueur((*this)(dls::math::vec3d(0.0, 1.0, 0.0)));
	auto const a3 = dls::math::longueur((*this)(dls::math::vec3d(0.0, 0.0, 1.0)));

	auto pas_un = [](const double valeur)
	{
		return valeur < 0.999 || valeur > 1.001;
	};

	return pas_un(a1) || pas_un(a2) || pas_un(a3);
}

transformation &transformation::operator*=(transformation const &transforme)
{
	m_matrice *= transforme.matrice();

	auto inv2 = transforme.inverse();
	auto inv1 = m_inverse;
	m_inverse = inv2 * inv1;

	return *this;
}

bool operator==(transformation const &a, transformation const &b)
{
	return a.matrice() == b.matrice();
}

bool operator!=(transformation const &a, transformation const &b)
{
	return !(a == b);
}

transformation inverse(transformation const &transforme)
{
	return transformation(transforme.inverse(), transforme.matrice());
}

transformation translation(const double x, const double y, const double z)
{
	auto matrice = dls::math::mat4x4d(1.0);
	auto matrice_inverse = dls::math::mat4x4d(1.0);

	matrice[3][0] = x;
	matrice[3][1] = y;
	matrice[3][2] = z;

	matrice_inverse[3][0] = -x;
	matrice_inverse[3][1] = -y;
	matrice_inverse[3][2] = -z;

	return transformation(matrice, matrice_inverse);
}

transformation translation(dls::math::vec3d const &vecteur)
{
	return translation(vecteur[0], vecteur[1], vecteur[2]);
}

transformation echelle(const double x, const double y, const double z)
{
	auto matrice = dls::math::mat4x4d(1.0);
	auto matrice_inverse = dls::math::mat4x4d(1.0);

	matrice[0][0] = x;
	matrice[1][1] = y;
	matrice[2][2] = z;

	matrice_inverse[0][0] = 1.0 / x;
	matrice_inverse[1][1] = 1.0 / y;
	matrice_inverse[2][2] = 1.0 / z;

	return transformation(matrice, matrice_inverse);
}

transformation echelle(dls::math::vec3d const &vecteur)
{
	return echelle(vecteur[0], vecteur[1], vecteur[2]);
}

/**
 * Soit le système de coordonnées suivant :
 *    | y
 *    |
 *    |
 *    |______ x
 *   /
 *  /
 * / z
 *
 * Une rotation de 90° autour de l'axe des X donne la transformation suivante :
 * | z
 * |  / y
 * | /
 * |/_____ x
 *
 * Donc les coordonnées [0, 1, 0] et [0, 0, 1] deviennent [0, 0, -1] et [0, 1, 0]
 *
 * La matrice est donc :
 * [1, 0,  0, 0]
 * [0, 0, -1, 0]
 * [0, 1,  0, 0]
 * [0, 0,  0, 1]
 *
 * Puisque cos 90° = 0, et sin 90° = 1, la matrice de rotation est :
 * [1,     0,      0, 0]
 * [0, cos t, -sin t, 0]
 * [0, sin t,  cos t, 0]
 * [0,     0,      0, 1]
 *
 * Puisque les vecteurs de bases sont orthogonaux, l'inverse de la matrice est
 * simplement sa transposée.
 */
transformation rotation_x(const double angle)
{
	auto const sin_angle = std::sin(angle);
	auto const cos_angle = std::cos(angle);

	dls::math::mat4x4d matrice(
		1.0,       0.0,        0.0, 0.0,
		0.0, cos_angle, -sin_angle, 0.0,
		0.0, sin_angle,  cos_angle, 0.0,
		0.0,       0.0,        0.0, 1.0
	);

	return transformation(matrice, dls::math::transpose(matrice));
}

/**
 * Soit le système de coordonnées suivant :
 *    | y
 *    |
 *    |
 *    |______ x
 *   /
 *  /
 * / z
 *
 * Une rotation de 90° autour de l'axe des Y donne la transformation suivante :
 *        | y
 *        |
 *        |
 * z______|
 *       /
 *      /
 *     / x
 *
 * Donc les coordonnées [1, 0, 0] et [0, 0, 1] deviennent [0, 0, 1] et [-1, 0, 0]
 *
 * La matrice est donc :
 * [0, 0, -1, 0]
 * [0, 1,  0, 0]
 * [1, 0,  0, 0]
 * [0, 0,  0, 1]
 *
 * Puisque cos 90° = 0, et sin 90° = 1, la matrice de rotation est :
 * [ cos t, 0, sin t, 0]
 * [     0, 1,     0, 0]
 * [-sin t, 0, cos t, 0]
 * [     0, 0,     0, 1]
 *
 * Puisque les vecteurs de bases sont orthogonaux, l'inverse de la matrice est
 * simplement sa transposée.
 */
transformation rotation_y(const double angle)
{
	auto const sin_angle = std::sin(angle);
	auto const cos_angle = std::cos(angle);

	dls::math::mat4x4d matrice(
		cos_angle,  0.0, sin_angle, 0.0,
		0.0,        1.0,       0.0, 0.0,
		-sin_angle, 0.0, cos_angle, 0.0,
		0.0,        0.0,       0.0, 1.0
	);

	return transformation(matrice, dls::math::transpose(matrice));
}

/**
 * Soit le système de coordonnées suivant :
 *     | y
 *     |
 *     |
 *     |______ x
 *    /
 *   /
 *  / z
 *
 * Une rotation de 90° autour de l'axe des Z donne la transformation suivante :
 *        | x
 *        |
 *        |
 * y______|
 *       /
 *      /
 *     / z
 *
 * Donc les coordonnées [1, 0, 0] et [0, 1, 0] deviennent [0, 1, 0] et [-1, 0, 0]
 *
 * La matrice est donc :
 *
 * [ 0, 1, 0, 0]
 * [-1, 0, 0, 0]
 * [ 0, 0, 1, 0]
 * [ 0, 0, 0, 1]
 *
 * Puisque cos 90° = 0, et sin 90° = 1, la matrice de rotation est :
 *
 * [ cos t, sin t, 0, 0]
 * [-sin t, cos t, 0, 0]
 * [     0,     0, 1, 0]
 * [     0,     0, 0, 1]
 *
 * Puisque les vecteurs de bases sont orthogonaux, l'inverse de la matrice est
 * simplement sa transposée.
 */
transformation rotation_z(const double angle)
{
	auto const sin_angle = std::sin(angle);
	auto const cos_angle = std::cos(angle);

	dls::math::mat4x4d matrice(
		 cos_angle, sin_angle, 0.0, 0.0,
		-sin_angle, cos_angle, 0.0, 0.0,
		0.0,        0.0, 1.0, 0.0,
		0.0,        0.0, 0.0, 1.0
	);

	return transformation(matrice, dls::math::transpose(matrice));
}

transformation rotation(const double angle, dls::math::vec3d const &vecteur)
{
	auto const a = dls::math::normalise(vecteur);
	auto const s = std::sin(angle);
	auto const c = std::cos(angle);

	dls::math::mat4x4d matrice;

	matrice[0][0] = a[0] * a[0] + (1.0 - a[0] * a[0]) * c;
	matrice[0][1] = a[1] * a[1] + (1.0 - c) - a[2] * s;
	matrice[0][2] = a[2] * a[2] + (1.0 - c) + a[1] * s;
	matrice[0][3] = 0.0;

	matrice[1][0] = a[0] * a[0] + (1.0 - c) + a[2] * s;
	matrice[1][1] = a[1] * a[1] + (1.0 - a[1] * a[1]) * c;
	matrice[1][2] = a[2] * a[2] + (1.0 - c) - a[0] * s;
	matrice[1][3] = 0.0;

	matrice[2][0] = a[0] * a[0] + (1.0 - c) - a[1] * s;
	matrice[2][1] = a[1] * a[1] + (1.0 - c) + a[0] * s;
	matrice[2][2] = a[2] * a[2] + (1.0 - a[2] * a[2]) * c;
	matrice[2][3] = 0.0;

	matrice[3][0] = 0.0;
	matrice[3][1] = 0.0;
	matrice[3][2] = 0.0;
	matrice[3][3] = 1.0;

	return transformation(matrice, dls::math::transpose(matrice));
}

transformation vise(
		dls::math::vec3d const &position,
		dls::math::vec3d const &mire,
		dls::math::vec3d const &haut)
{
	std::cout << "============================\n";
	std::cout << "LookAt\n";
	std::cout << "Position : " << position[0] << ", " << position[1] << ", " << position[2] << '\n';
	std::cout << "Mire : " << mire[0] << ", " << mire[1] << ", " << mire[2] << '\n';
	std::cout << "Haut : " << haut[0] << ", " << haut[1] << ", " << haut[2] << '\n';

	auto const direction = dls::math::normalise(mire - position);
	auto const droite = dls::math::normalise(dls::math::produit_croix(dls::math::normalise(haut), direction));
	auto const nouveau_haut = dls::math::produit_croix(direction, droite);

	dls::math::mat4x4d matrice;

	matrice[0][0] = droite[0];
	matrice[1][0] = droite[1];
	matrice[2][0] = droite[2];
	matrice[3][0] = 0.0;

	matrice[0][1] = nouveau_haut[0];
	matrice[1][1] = nouveau_haut[1];
	matrice[2][1] = nouveau_haut[2];
	matrice[3][1] = 0.0;

	matrice[0][2] = direction[0];
	matrice[1][2] = direction[1];
	matrice[2][2] = direction[2];
	matrice[3][2] = 0.0;

	matrice[0][3] = position[0];
	matrice[1][3] = position[1];
	matrice[2][3] = position[2];
	matrice[3][3] = 1.0;

	std::cout << "Matrice : " << matrice << '\n';
	std::cout << "Inverse : " << dls::math::inverse(matrice) << '\n';
	std::cout << "============================\n";

	return transformation(dls::math::inverse(matrice), matrice);
}

}  /* namespace math */
