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

#include "couleur.h"

#include <algorithm>

#include "outils.h"

/**
 * Pris depuis http://lolengine.net/blog/2013/01/13/fast-rgb-to-hsv.
 */
void rvb_vers_hsv(float r, float g, float b, float *lh, float *ls, float *lv)
{
	auto k = 0.0f;

	if (g < b) {
		std::swap(g, b);
		k = -1.0f;
	}

	auto min_gb = b;

	if (r < g) {
		std::swap(r, g);
		k = -2.0f / 6.0f - k;
		min_gb = std::min(g, b);
	}

	auto chroma = r - min_gb;

	*lh = std::abs(k + (g - b) / (6.0f * chroma + 1e-20f));
	*ls = chroma / (r + 1e-20f);
	*lv = r;
}

void rvb_vers_hsv(const couleur32 &rvb, couleur32 &hsv)
{
	rvb_vers_hsv(rvb.r, rvb.v, rvb.b, &hsv.r, &hsv.v, &hsv.b);
}

void hsv_vers_rvb(float h, float s, float v, float *r, float *g, float *b)
{
	auto nr =        std::abs(h * 6.0f - 3.0f) - 1.0f;
	auto ng = 2.0f - std::abs(h * 6.0f - 2.0f);
	auto nb = 2.0f - std::abs(h * 6.0f - 4.0f);

	nr = restreint(nr, 0.0f, 1.0f);
	ng = restreint(ng, 0.0f, 1.0f);
	nb = restreint(nb, 0.0f, 1.0f);

	*r = ((nr - 1.0f) * s + 1.0f) * v;
	*g = ((ng - 1.0f) * s + 1.0f) * v;
	*b = ((nb - 1.0f) * s + 1.0f) * v;
}

void hsv_vers_rvb(const couleur32 &hsv, couleur32 &rvb)
{
	hsv_vers_rvb(hsv.r, hsv.v, hsv.b, &rvb.r, &rvb.v, &rvb.b);
}
