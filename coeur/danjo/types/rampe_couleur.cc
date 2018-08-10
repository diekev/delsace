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

#include "rampe_couleur.h"

#include <algorithm>

void cree_rampe_defaut(RampeCouleur &rampe)
{
	ajoute_point_rampe(rampe, 0.0f, glm::vec4{0.0f, 0.0f, 0.0f, 1.0f});
	ajoute_point_rampe(rampe, 0.5f, glm::vec4{0.0f, 1.0f, 0.0f, 1.0f});
	ajoute_point_rampe(rampe, 1.0f, glm::vec4{1.0f, 1.0f, 1.0f, 1.0f});
}

void tri_points_rampe(RampeCouleur &rampe)
{
	std::sort(rampe.points.begin(), rampe.points.end(),
			  [](const PointRampeCouleur &a, const PointRampeCouleur &b)
	{
		return a.position < b.position;
	});
}

void ajoute_point_rampe(RampeCouleur &rampe, float x, const glm::vec4 &couleur)
{
	PointRampeCouleur p;
	p.position = x;
	p.couleur = couleur;

	rampe.points.push_back(p);

	tri_points_rampe(rampe);
}

PointRampeCouleur *trouve_point_selectionne(const RampeCouleur &rampe)
{
	for (const auto &point : rampe.points) {
		if (point.selectionne) {
			return const_cast<PointRampeCouleur *>(&point);
		}
	}

	return nullptr;
}

template <typename T>
static auto restreint(const T &a, const T &min, const T &max)
{
	if (a < min) {
		return min;
	}

	if (a > max) {
		return max;
	}

	return a;
}

void rgb_to_hsv(float r, float g, float b, float *lh, float *ls, float *lv)
{
	float k = 0.0f;
	float chroma;
	float min_gb;

	if (g < b) {
		std::swap(g, b);
		k = -1.0f;
	}
	min_gb = b;
	if (r < g) {
		std::swap(r, g);
		k = -2.0f / 6.0f - k;
		min_gb = std::min(g, b);
	}

	chroma = r - min_gb;

	*lh = std::abs(k + (g - b) / (6.0f * chroma + 1e-20f));
	*ls = chroma / (r + 1e-20f);
	*lv = r;
}

void hsv_to_rgb(float h, float s, float v, float *r, float *g, float *b)
{
	auto nr =        std::abs(h * 6.0f - 3.0f) - 1.0f;
	auto ng = 2.0f - std::abs(h * 6.0f - 2.0f);
	auto nb = 2.0f - std::abs(h * 6.0f - 4.0f);

	nr = restreint(nr, 0.0f, 1.0f);
	nb = restreint(nb, 0.0f, 1.0f);
	ng = restreint(ng, 0.0f, 1.0f);

	*r = ((nr - 1.0f) * s + 1.0f) * v;
	*g = ((ng - 1.0f) * s + 1.0f) * v;
	*b = ((nb - 1.0f) * s + 1.0f) * v;
}

glm::vec4 evalue_rampe_couleur(const RampeCouleur &rampe, const float valeur)
{
	auto v = restreint(valeur, 0.0f, 1.0f);

	if (rampe.points.size() == 1) {
		return rampe.points[0].couleur;
	}

	if (v <= rampe.points[0].position) {
		return rampe.points[0].couleur;
	}

	if (v >= rampe.points[rampe.points.size() - 1].position) {
		return rampe.points[rampe.points.size() - 1].couleur;
	}

	glm::vec4 res;

	for (size_t i = 0; i < rampe.points.size() - 1; ++i) {
		const auto &p0 = rampe.points[i];
		const auto &p1 = rampe.points[i + 1];

		if (p0.position <= v && v <= p1.position) {
			const auto &c0 = p0.couleur;
			const auto &c1 = p1.couleur;
			auto fac = (v - p0.position) / (p1.position - p0.position);

			if (rampe.interpolation == INTERPOLATION_HSV) {
				float h0, s0, v0, h1, s1, v1;
				rgb_to_hsv(c0[0], c0[1], c0[2], &h0, &s0, &v0);
				rgb_to_hsv(c1[0], c1[1], c1[2], &h1, &s1, &v1);


				float nh;
				float d = h1 - h0;
				auto t = fac;

				if (h0 > h1) {
					std::swap(h0, h1);
					d = -d;
					t = 1 - t;
				}

				/* 180 degrés */
				if (d > 0.5) {
					/* 360 degrés */
					h0 = h0 + 1.0f;
					/* 360 degrés */
					nh = (1.0f - fac) * h0 + fac * h1;

					if (nh > 1.0f) {
						nh -= 1.0f;
					}
				}
				/* 180 degrés */
				else if (d <= 0.5) {
					nh = h0 + t * d;
				}

				auto ns = (1.0f - fac) * s0 + fac * s1;
				auto nv = (1.0f - fac) * v0 + fac * v1;
				res[3] = (1.0f - fac) * c0.a + fac * c1.a;

				hsv_to_rgb(nh, ns, nv, &res[0], &res[1], &res[2]);
			}
			else {
				for (int i = 0; i < 4; ++i) {
					res[i] = (1.0f - fac) * c0[i] + fac * c1[i];
				}
			}

			return res;
		}
	}

	/* ne devrait pas arriver */
	return glm::vec4(0.0f);
}
