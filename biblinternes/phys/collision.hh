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

#include "rayon.hh"

/**
 * Retourne vrai s'il y a entresection entre le triangle et le rayon spécifiés.
 * Si oui, la distance spécifiée est mise à jour.
 *
 * Algorithme de Möller-Trumbore.
 * https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
 */
template <typename T>
auto entresecte_triangle(
		dls::math::point3<T> const &vertex0,
		dls::math::point3<T> const &vertex1,
		dls::math::point3<T> const &vertex2,
		dls::phys::rayon<T> const &rayon,
		T &distance)
{
	constexpr auto epsilon = static_cast<T>(0.000001);

	auto const &cote1 = vertex1 - vertex0;
	auto const &cote2 = vertex2 - vertex0;
	auto const &h = dls::math::produit_croix(rayon.direction, cote2);
	auto const angle = dls::math::produit_scalaire(cote1, h);

	if (angle > -epsilon && angle < epsilon) {
		return false;
	}

	auto const f = static_cast<T>(1.0) / angle;
	auto const &s = (rayon.origine - vertex0);
	auto const angle_u = f * dls::math::produit_scalaire(s, h);

	if (angle_u < static_cast<T>(0.0) || angle_u > static_cast<T>(1.0)) {
		return false;
	}

	auto const q = dls::math::produit_croix(s, cote1);
	auto const angle_v = f * dls::math::produit_scalaire(rayon.direction, q);

	if (angle_v < static_cast<T>(0.0) || angle_u + angle_v > static_cast<T>(1.0)) {
		return false;
	}

	/* À cette étape on peut calculer t pour trouver le point d'entresection sur
	 * la ligne. */
	auto const t = f * dls::math::produit_scalaire(cote2, q);

	/* Entresection avec le rayon. */
	if (t > epsilon) {
		distance = t;
		return true;
	}

	/* Cela veut dire qu'il y a une entresection avec une ligne, mais pas avec
	 * le rayon. */
	return false;
}

/* Retourne le point dans le triangle (a, b, c) le plus proche de 'p'.
 *
 * Adapté de "Real-Time Collision Detection" par Christer Ericson,
 * publié par Morgan Kaufmann Publishers, copyright 2005 Elsevier Inc. */
template <typename T>
auto plus_proche_point_triangle(
		dls::math::vec3<T> const &p,
		dls::math::vec3<T> const &a,
		dls::math::vec3<T> const &b,
		dls::math::vec3<T> const &c)
{
	constexpr auto UN = static_cast<T>(1);
	constexpr auto ZERO = static_cast<T>(0);

	/* Vérifie si P est dans la région en dehors de A */
	auto ab = b - a;
	auto ac = c - a;
	auto ap = p - a;

	auto d1 = produit_scalaire(ab, ap);
	auto d2 = produit_scalaire(ac, ap);

	if (d1 <= ZERO && d2 <= ZERO) {
		/* coordonnées barycentriques (1, 0, 0) */
		return a;
	}

	/* Vérifie si P est dans la région en dehors de B */
	auto bp = p - b;
	auto d3 = produit_scalaire(ab, bp);
	auto d4 = produit_scalaire(ac, bp);
	if (d3 >= ZERO && d4 <= d3) {
		/* coordonnées barycentriques (0,1,0) */
		return b;
	}

	/* Vérifie si P est dans la région de AB, si oui retourne sa projection sur AB */
	auto vc = d1 * d4 - d3 * d2;
	if (vc <= ZERO && d1 >= ZERO && d3 <= ZERO) {
		auto v = d1 / (d1 - d3);
		/* coordonnées barycentriques (1-v,v,0) */
		return a + ab * v;
	}

	/* Vérifie si P est dans la région en dehors de C */
	auto cp = p - c;
	auto d5 = produit_scalaire(ab, cp);
	auto d6 = produit_scalaire(ac, cp);
	if (d6 >= ZERO && d5 <= d6) {
		/* coordonnées barycentriques (0,0,1) */
		return c;
	}

	/* Vérifie si P est dans la région de AC, si oui retourne sa projection sur AC */
	auto vb = d5 * d2 - d1 * d6;
	if (vb <= ZERO && d2 >= ZERO && d6 <= ZERO) {
		auto w = d2 / (d2 - d6);
		/* coordonnées barycentriques (1-w,0,w) */
		return a + ac * w;
	}

	/* Vérifie si P est dans la région de BC, si oui retourne sa projection sur BC */
	auto va = d3 * d6 - d5 * d4;
	if (va <= ZERO && (d4 - d3) >= ZERO && (d5 - d6) >= ZERO) {
		auto w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
		/* coordonnées barycentriques (0,1-w,w) */
		auto r = c - b;
		r *= w;
		r += b;
		return r;
	}

	/* P est dans la région de triangle.
	 * Calcul Q via ses coordonnées barycentriques (u,v,w) */
	auto denom = UN / (va + vb + vc);
	auto v = vb * denom;
	auto w = vc * denom;

	/* = u*a + v*b + w*c, u = va * denom = 1.0f - v - w */
	/* ac * w */
	ac *= w;
	/* a + ab * v */
	auto r = a + ab * v;
	/* a + ab * v + ac * w */
	r += ac;

	return r;
}
