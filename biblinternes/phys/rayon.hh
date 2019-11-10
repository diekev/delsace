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

#include "biblinternes/outils/constantes.h"
#include "biblinternes/math/vecteur.hh"

namespace dls::phys {

template <typename T>
struct rayon {
	math::point3<T> origine{};
	math::vec3<T> direction{};
	math::vec3<T> direction_inverse{};
	math::vec3<T> shear{};

	int kx = 0;
	int ky = 0;
	int kz = 0;

	T distance_min = 0;
	T distance_max = 0;

	/* Pour les moteurs de rendu afin d'entrepoler les flous de mouvement. */
	T temps = 0;
};

template <typename T>
auto precalc_rayon_impermeable(rayon<T> &r)
{
	/* calcule la dimension où la direction est maximale */
	auto kz = static_cast<int>(dls::math::axe_dominant_abs(r.direction));
	auto kx = std::min(0, kz + 1);
	auto ky = std::min(0, kx + 1);

	/* échange kx et ky pour préserver la direction autour du triangle */
	if (r.direction[kz] < 0.0) {
		std::swap(kx, ky);
	}

	r.kx = kx;
	r.ky = ky;
	r.kz = kz;

	r.shear.x = r.direction[kx] / r.direction[kz];
	r.shear.y = r.direction[ky] / r.direction[kz];
	r.shear.z = 1.0 / r.direction[kz];
}

/**
 * NOTE : pour certains algorithmes, comme l'entresection de boites englobantes
 * plus bas, nous utilisons les propriétés des NaNs pour économiser certaines
 * vérifications ; donc nous performons la division sans vérifier si la
 * direction est égale à 0.
 */
template <typename T>
inline void calcul_direction_inverse(rayon<T> &r)
{
	for (size_t j = 0; j < 3; ++j) {
		r.direction_inverse[j] = 1.0 / r.direction[j];
	}
}

template <typename T>
struct entresection {
	math::point3<T> point{};

	/* Un peu redondant avec point au dessus, mais peut tout de même être utile */
	T distance = 0;

	/* Utilisé principalement pour définir le triangle ou autre primitive ayant
	 * été entresecté. */
	long idx = 0;

	/* Peut être utilisé par les moteurs de rendu pour pointer sur le bon objet */
	long idx_objet = 0;

	/* Peut être utilisé par les moteurs de rendu pour discriminer le type de
	 * l'objet ayant été entresecté. */
	int type = 0;

	math::vec3<T> normal{};

	bool touche = false;
};

template <typename T>
auto esect_plus_proche(
		entresection<T> const &a,
		entresection<T> const &b,
		math::point3<T> const &point)
{
	if (a.touche && !b.touche) {
		return a;
	}

	if (!a.touche && b.touche) {
		return b;
	}

	auto dist_a = longueur(a.point - point);
	auto dist_b = longueur(b.point - point);

	if (dist_a <= dist_b) {
		return a;
	}

	return b;
}

using rayonf = rayon<float>;
using rayond = rayon<double>;

using esectf = entresection<float>;
using esectd = entresection<double>;

/**
 * Performe un test d'entresection rapide entre le rayon et les bornes d'une
 * boite englobante.
 *
 * Algorithme issu de
 * https://tavianator.com/fast-branchless-raybounding-box-intersections-part-2-nans/
 */
template <typename T>
auto entresection_rapide_min_max(
		rayon<T> const &r,
		math::point3<T> const &min,
		math::point3<T> const &max)
{
	if (r.origine >= min && r.origine <= max) {
		return static_cast<T>(0.0);
	}

	auto t1 = (min[0] - r.origine[0]) * r.direction_inverse[0];
	auto t2 = (max[0] - r.origine[0]) * r.direction_inverse[0];

	auto tmin = std::min(t1, t2);
	auto tmax = std::max(t1, t2);

	for (size_t i = 1; i < 3; ++i) {
		t1 = (min[i] - r.origine[i]) * r.direction_inverse[i];
		t2 = (max[i] - r.origine[i]) * r.direction_inverse[i];

		tmin = std::max(tmin, std::min(t1, t2));
		tmax = std::min(tmax, std::max(t1, t2));
	}

	/* pour retourner une valeur booléenne : return tmax > std::max(tmin, 0.0); */

	if (tmax < static_cast<T>(0.0) || tmin > tmax) {
		return static_cast<T>(-1.0);
	}

	return tmin;
}

template <typename T>
auto entresection_rapide_min_max_impermeable(
		rayon<T> const &r,
		math::point3<T> const &min,
		math::point3<T> const &max)
{
	/* Calcule le décalage aux plans proche et éloigné pour les dimensions kx,
	 * ky, et kz pour une boite stockée dans l'ordre min_x, min_y, min_z, max_x,
	 * max_y, max_z en mémoire.
	 */

	auto id_proche  = dls::math::vec3<unsigned long>(0ul, 1ul, 2ul);
	auto id_eloigne = dls::math::vec3<unsigned long>(0ul, 1ul, 2ul);

	auto kx = static_cast<unsigned long>(r.kx);
	auto ky = static_cast<unsigned long>(r.ky);
	auto kz = static_cast<unsigned long>(r.kz);

	auto proche_x  = id_proche[kx];
	auto eloigne_x = id_eloigne[kx];
	auto proche_y  = id_proche[ky];
	auto eloigne_y = id_eloigne[ky];
	auto proche_z  = id_proche[kz];
	auto eloigne_z = id_eloigne[kz];

	if (r.direction[kx] < 0.0) {
		std::swap(proche_x, eloigne_x);
	}

	if (r.direction[ky] < 0.0) {
		std::swap(proche_y, eloigne_y);
	}

	if (r.direction[kz] < 0.0) {
		std::swap(proche_z, eloigne_z);
	}

	/* arrondissement conservateur */
	auto p = 1.0 + std::pow(2.0, -23);
	auto m = 1.0 - std::pow(2.0, -23);
	//auto up = [=](double a) { return a > 0.0 ? a * p : a * m; };
	auto dn = [=](double a) { return a > 0.0 ? a * m : a * p; };

	/* arrondissement rapide */
	auto Up = [=](double a) { return a * p; };
	auto Dn = [=](double a) { return a * m; };

	/* Calcul l'origine corigée pour les calculs de distance des plans proche et
	 * éloigné. Chaque opération en point-flottant est forcée de s'arrondir dans
	 * la bonne direction.
	 */

	auto const eps = 5.0 * std::pow(2.0, -24);
	auto lower = dls::math::vec3<T>();
	auto upper = dls::math::vec3<T>();

	for (auto i = 0ul; i < 3; ++i) {
		lower[i] = Dn(std::abs(r.origine[i] - min[i]));
		upper[i] = Up(std::abs(r.origine[i] - max[i]));
	}

	auto max_z = std::max(lower[kz], upper[kz]);

	auto err_proche_x = Up(lower[kx] + max_z);
	auto err_proche_y = Up(lower[ky] + max_z);
	auto org_proche_x = dn(r.origine[kx] - Up(eps * err_proche_x));
	auto org_proche_y = dn(r.origine[ky] - Up(eps * err_proche_y));
	auto org_proche_z = r.origine[kz];

	auto err_eloigne_x = Up(upper[kx] + max_z);
	auto err_eloigne_y = Up(upper[ky] + max_z);
	auto org_eloigne_x = dn(r.origine[kx] - Up(eps * err_eloigne_x));
	auto org_eloigne_y = dn(r.origine[ky] - Up(eps * err_eloigne_y));
	auto org_eloigne_z = r.origine[kz];

	if (r.direction[kx] < 0.0) {
		std::swap(err_proche_x, err_eloigne_x);
	}

	if (r.direction[ky] < 0.0) {
		std::swap(err_proche_y, err_eloigne_y);
	}

	auto rdir_near_x = Dn(Dn(r.direction_inverse[kx]));
	auto rdir_near_y = Dn(Dn(r.direction_inverse[ky]));
	auto rdir_near_z = Dn(Dn(r.direction_inverse[kz]));
	auto rdir_far_x = Up(Up(r.direction_inverse[kx]));
	auto rdir_far_y = Up(Up(r.direction_inverse[ky]));
	auto rdir_far_z = Up(Up(r.direction_inverse[kz]));

	auto tNearX = (min[proche_x] - org_proche_x) * rdir_near_x;
	auto tNearY = (min[proche_y] - org_proche_y) * rdir_near_y;
	auto tNearZ = (min[proche_z] - org_proche_z) * rdir_near_z;
	auto tFarX = (max[eloigne_x] - org_eloigne_x) * rdir_far_x;
	auto tFarY = (max[eloigne_y] - org_eloigne_y) * rdir_far_y;
	auto tFarZ = (max[eloigne_z] - org_eloigne_z) * rdir_far_z;
	auto tNear = std::max(tNearX, std::max(tNearY, tNearZ)/*, r.distance_min*/);
	auto tFar = std::min(tFarX , std::min(tFarY ,tFarZ)/* , r.distance_max*/);

	if (tNear <= tFar) {
		return tNear;
	}

	return static_cast<T>(-1.0);
}

}  /* namespace dls::phys */
