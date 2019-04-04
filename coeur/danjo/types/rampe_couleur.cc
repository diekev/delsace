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

#include "types/outils.h"

void cree_rampe_defaut(RampeCouleur &rampe)
{
	ajoute_point_rampe(rampe, 0.0f, dls::phys::couleur32{0.0f, 0.0f, 0.0f, 1.0f});
	ajoute_point_rampe(rampe, 0.5f, dls::phys::couleur32{0.0f, 1.0f, 0.0f, 1.0f});
	ajoute_point_rampe(rampe, 1.0f, dls::phys::couleur32{1.0f, 1.0f, 1.0f, 1.0f});
}

void tri_points_rampe(RampeCouleur &rampe)
{
	std::sort(rampe.points.begin(), rampe.points.end(),
			  [](const PointRampeCouleur &a, const PointRampeCouleur &b)
	{
		return a.position < b.position;
	});
}

void ajoute_point_rampe(RampeCouleur &rampe, float x, const dls::phys::couleur32 &couleur)
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

dls::phys::couleur32 evalue_rampe_couleur(const RampeCouleur &rampe, const float valeur)
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

	dls::phys::couleur32 res;

	for (size_t i = 0; i < rampe.points.size() - 1; ++i) {
		const auto &p0 = rampe.points[i];
		const auto &p1 = rampe.points[i + 1];

		if (p0.position <= v && v <= p1.position) {
			const auto &c0 = p0.couleur;
			const auto &c1 = p1.couleur;
			auto fac = (v - p0.position) / (p1.position - p0.position);

			if (rampe.entrepolation == ENTREPOLATION_HSV) {
				float h0, s0, v0, h1, s1, v1;
				dls::phys::rvb_vers_hsv(c0[0], c0[1], c0[2], &h0, &s0, &v0);
				dls::phys::rvb_vers_hsv(c1[0], c1[1], c1[2], &h1, &s1, &v1);

				/* Les teintes doivent être entrepolées selon leurs angles, donc
				 * on doit définir la distance entre les angles et leur
				 * orientation anti-horaire (dist_a) ou horaire (dist_h).
				 */
				const auto dist_a = (h0 >= h1) ? h0 - h1 : 1 + h0 - h1;
				const auto dist_h = (h0 >= h1) ? 1 + h1 - h0 : h1 - h0;
				auto nh = (dist_h <= dist_a) ? h0 + (dist_h * fac)
											 : h0 - (dist_a * fac);

				if (nh < 0.0f) {
					nh = 1 + nh;
				}

				if (nh > 1.0f) {
					nh = nh - 1.0f;
				}

				auto ns = (1.0f - fac) * s0 + fac * s1;
				auto nv = (1.0f - fac) * v0 + fac * v1;
				res[3] = (1.0f - fac) * c0.a + fac * c1.a;

				dls::phys::hsv_vers_rvb(nh, ns, nv, &res[0], &res[1], &res[2]);
			}
			else {
				for (int j = 0; j < 4; ++j) {
					res[j] = (1.0f - fac) * c0[j] + fac * c1[j];
				}
			}

			return res;
		}
	}

	/* ne devrait pas arriver */
	return dls::phys::couleur32(0.0f);
}
