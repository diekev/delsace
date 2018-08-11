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

#include "courbe_bezier.h"

#include <algorithm>
#include <cmath>

auto operator+(const Point &p1, const Point &p2)
{
	return Point{
		p1.x + p2.x,
		p1.y + p2.y
	};
}

auto operator-(const Point &p1, const Point &p2)
{
	return Point{
		p1.x - p2.x,
		p1.y - p2.y
	};
}

auto operator*(const Point &p1, const float f)
{
	return Point{
		p1.x * f,
		p1.y * f
	};
}

auto longueur(const Point &p)
{
	return std::sqrt(p.x * p.x + p.y * p.y);
}

auto longueur(const Point &p1, const Point &p2)
{
	return longueur(p1 - p2);
}

/* ************************************************************************** */

CourbeCouleur::CourbeCouleur()
{
	cree_courbe_defaut(courbe_m);
	cree_courbe_defaut(courbe_r);
	cree_courbe_defaut(courbe_v);
	cree_courbe_defaut(courbe_b);
	cree_courbe_defaut(courbe_a);
}

/* ************************************************************************** */

void cree_courbe_defaut(CourbeBezier &courbe)
{
	courbe.points.clear();

	ajoute_point_courbe(courbe, 0.0f, 0.0f);
	ajoute_point_courbe(courbe, 1.0f, 1.0f);
	calcule_controles_courbe(courbe);

	courbe.point_courant = &courbe.points[0];
}

void ajoute_point_courbe(CourbeBezier &courbe, float x, float y)
{
	PointBezier point;
	point.co[POINT_CONTROLE1].x = x - 0.1f;
	point.co[POINT_CONTROLE1].y = y;
	point.co[POINT_CENTRE].x = x;
	point.co[POINT_CENTRE].y = y;
	point.co[POINT_CONTROLE2].x = x + 0.1f;
	point.co[POINT_CONTROLE2].y = y;

	courbe.points.push_back(point);

	std::sort(courbe.points.begin(), courbe.points.end(),
			  [](const PointBezier &p1, const PointBezier &p2)
	{
		return p1.co[POINT_CENTRE].x < p2.co[POINT_CENTRE].x;
	});

	courbe.extension_min.co[POINT_CENTRE].x = -1.0f;
	courbe.extension_min.co[POINT_CENTRE].y = courbe.points[0].co[POINT_CENTRE].y;
	courbe.extension_max.co[POINT_CENTRE].x = 2.0f;
	courbe.extension_max.co[POINT_CENTRE].y = courbe.points[courbe.points.size() - 1].co[POINT_CENTRE].y;

	construit_table_courbe(courbe);
}

void construit_table_courbe(CourbeBezier &courbe)
{
	const auto res_courbe = 32;
	const auto facteur = 1.0f / res_courbe;

	courbe.table.clear();
	courbe.table.reserve(res_courbe + 1);

	for (size_t i = 0; i < courbe.points.size() - 1; ++i) {
		const auto &p1 = courbe.points[i];
		const auto &p2 = courbe.points[i + 1];

		/* formule
		 *  x0 * (1.0 - t)^3
		 *  + 3 * x1 * t * (1.0 - t)^2
		 *  + 3 * x2 * t^2 * (1.0 - t)
		 *  + x3 * t^3
		 */

		const auto &x0 =     p1.co[POINT_CENTRE].x;
		const auto &y0 =     p1.co[POINT_CENTRE].y;
		const auto &x1 = 3 * p1.co[POINT_CONTROLE2].x;
		const auto &y1 = 3 * p1.co[POINT_CONTROLE2].y;
		const auto &x2 = 3 * p2.co[POINT_CONTROLE1].x;
		const auto &y2 = 3 * p2.co[POINT_CONTROLE1].y;
		const auto &x3 =     p2.co[POINT_CENTRE].x;
		const auto &y3 =     p2.co[POINT_CENTRE].y;

		courbe.table.push_back(Point{x0, y0});

		for (int i = 1; i <= res_courbe; ++i) {
			const auto fac_i = facteur * i;
			const auto mfac_i = 1.0f - fac_i;

			const auto p0x = x0                         * std::pow(mfac_i, 3.0f);
			const auto p0y = y0                         * std::pow(mfac_i, 3.0f);
			const auto p1x = x1 * fac_i                 * std::pow(mfac_i, 2.0f);
			const auto p1y = y1 * fac_i                 * std::pow(mfac_i, 2.0f);
			const auto p2x = x2 * std::pow(fac_i, 2.0f) * mfac_i;
			const auto p2y = y2 * std::pow(fac_i, 2.0f) * mfac_i;
			const auto p3x = x3 * std::pow(fac_i, 3.0f);
			const auto p3y = y3 * std::pow(fac_i, 3.0f);

			courbe.table.push_back(Point{p0x + p1x + p2x + p3x, p0y + p1y + p2y + p3y});
		}
	}
}

/**
 * Voir http://whizkidtech.redprince.net/bezier/circle/
 * et rB290361776e5858b3903a83c0cddf722b8340e699
 */
static constexpr auto KAPPA = 2.5614f;

void calcule_controles_courbe(CourbeBezier &courbe)
{
	const auto nombre_points = courbe.points.size();

	/* fais pointer les controles vers le centre des points environnants */
	for (size_t i = 0; i < nombre_points; ++i) {
		auto &p = courbe.points[i];

		if (i == 0) {
			p.co[POINT_CONTROLE2] = courbe.points[i + 1].co[POINT_CENTRE];
			p.co[POINT_CONTROLE1] = p.co[POINT_CENTRE] * 2.0f - p.co[POINT_CONTROLE2];
		}
		else if (i == nombre_points - 1) {
			p.co[POINT_CONTROLE1] = courbe.points[i - 1].co[POINT_CENTRE];
			p.co[POINT_CONTROLE2] = p.co[POINT_CENTRE] * 2.0f - p.co[POINT_CONTROLE1];
		}
		else {
			p.co[POINT_CONTROLE1] = courbe.points[i - 1].co[POINT_CENTRE];
			p.co[POINT_CONTROLE2] = courbe.points[i + 1].co[POINT_CENTRE];
		}
	}

	/* corrige les controles pour qu'ils soient tangeants à la courbe */
	for (auto &p : courbe.points) {
		auto &p0 = p.co[POINT_CONTROLE1];
		const auto &p1 = p.co[POINT_CENTRE];
		auto &p2 = p.co[POINT_CONTROLE2];

		auto vec_a = Point{p1.x - p0.x, p1.y - p0.y};
		auto vec_b = Point{p2.x - p1.x, p2.y - p1.y};

		auto len_a = longueur(vec_a);
		auto len_b = longueur(vec_b);

		if (len_a == 0.0f) {
			len_a = 1.0f;
		}

		if (len_b == 0.0f) {
			len_b = 1.0f;
		}

		Point tangeante;
		tangeante.x = vec_b.x / len_b + vec_a.x / len_a;
		tangeante.y = vec_b.y / len_b + vec_a.y / len_a;

		auto len_t = longueur(tangeante) * KAPPA;

		if (len_t != 0.0f) {
			// point a
			len_a /= len_t;
			p0 = p1 + tangeante * -len_a;

			// point b
			len_b /= len_t;
			p2 = p1 + tangeante *  len_b;
		}
	}

	/* corrige premier et dernier point pour pointer vers le controle le plus proche */
	if (nombre_points > 2) {
		// premier
		{
			auto &p0 = courbe.points[0];

			auto hlen = longueur(p0.co[POINT_CENTRE], p0.co[POINT_CONTROLE2]); /* original handle length */
			/* clip handle point */
			auto vec = courbe.points[1].co[POINT_CONTROLE1];

			if (vec.x < p0.co[POINT_CENTRE].x) {
				vec.x = p0.co[POINT_CENTRE].x;
			}

			vec = vec - p0.co[POINT_CENTRE];
			auto nlen = longueur(vec);

			if (nlen > 1e-6f) {
				vec = vec * (hlen / nlen);
				p0.co[POINT_CONTROLE2] = vec + p0.co[POINT_CENTRE];
				p0.co[POINT_CONTROLE1] = p0.co[POINT_CENTRE] - vec;
			}
		}

		// dernier
		{
			auto &p0 = courbe.points[nombre_points - 1];

			auto hlen = longueur(p0.co[POINT_CENTRE], p0.co[POINT_CONTROLE1]); /* original handle length */
			/* clip handle point */
			auto vec = courbe.points[nombre_points - 2].co[POINT_CONTROLE2];

			if (vec.x > p0.co[POINT_CENTRE].x) {
				vec.x = p0.co[POINT_CENTRE].x;
			}

			vec = vec - p0.co[POINT_CENTRE];
			auto nlen = longueur(vec);

			if (nlen > 1e-6f) {
				vec = vec * (hlen / nlen);
				p0.co[POINT_CONTROLE1] = p0.co[POINT_CENTRE] + vec;
				p0.co[POINT_CONTROLE2] = p0.co[POINT_CENTRE] - vec;
			}
		}
	}

	construit_table_courbe(courbe);
}

float evalue_courbe_bezier(const CourbeBezier &courbe, float valeur)
{
	const auto nombre_points = courbe.table.size();

	if (valeur <= courbe.valeur_min) {
		return courbe.table[0].y;
	}

	if (valeur >= courbe.valeur_max) {
		return courbe.table[nombre_points - 1].y;
	}

	if (courbe.utilise_table) {
		for (size_t i = 0; i < nombre_points - 1; ++i) {
			const auto &p0 = courbe.table[i];
			const auto &p1 = courbe.table[i + 1];

			if (p0.x <= valeur && valeur <= p1.x) {
				auto fac = (valeur - p0.x) / (p1.x - p0.x);
				return (1.0f - fac) * p0.y + fac * p1.y;
			}
		}
	}
	else {
		for (size_t i = 0; i < courbe.points.size() - 1; ++i) {
			const auto &p0 = courbe.points[i];
			const auto &p1 = courbe.points[i + 1];

			if (p0.co[POINT_CENTRE].x <= valeur && valeur <= p1.co[POINT_CENTRE].x) {
				auto fac = (valeur - p0.co[POINT_CENTRE].x) / (p1.co[POINT_CENTRE].x - p0.co[POINT_CENTRE].x);
				auto mfac = 1.0f - fac;
				const auto &y0 =     p0.co[POINT_CENTRE].y;
				const auto &y1 = 3 * p0.co[POINT_CONTROLE2].y;
				const auto &y2 = 3 * p1.co[POINT_CONTROLE1].y;
				const auto &y3 =     p1.co[POINT_CENTRE].y;

				const auto p0y = y0                       * std::pow(mfac, 3.0f);
				const auto p1y = y1 * fac                 * std::pow(mfac, 2.0f);
				const auto p2y = y2 * std::pow(fac, 2.0f) * mfac;
				const auto p3y = y3 * std::pow(fac, 3.0f);

				return p0y + p1y + p2y + p3y;
			}
		}
	}

	/* ne devrais pas arriver */
	return 0.0f;
}

couleur32 evalue_courbe_couleur(const CourbeCouleur &courbe, const couleur32 &valeur)
{
	couleur32 res;

	/* applique la courbe maitresse, sauf pour l'alpha */
	res[0] = evalue_courbe_bezier(courbe.courbe_m, valeur[0]);
	res[1] = evalue_courbe_bezier(courbe.courbe_m, valeur[1]);
	res[2] = evalue_courbe_bezier(courbe.courbe_m, valeur[2]);
	res[3] = valeur[3];

	/* applique les courbes individuelles */
	res[0] = evalue_courbe_bezier(courbe.courbe_r, res[0]);
	res[1] = evalue_courbe_bezier(courbe.courbe_v, res[1]);
	res[2] = evalue_courbe_bezier(courbe.courbe_b, res[2]);
	res[3] = evalue_courbe_bezier(courbe.courbe_a, res[3]);

	return res;
}
