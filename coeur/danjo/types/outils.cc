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

#include "outils.h"


QColor converti_couleur(float r, float v, float b, float a)
{
	r = restreint(r, 0.0f, 1.0f);
	v = restreint(v, 0.0f, 1.0f);
	b = restreint(b, 0.0f, 1.0f);
	a = restreint(a, 0.0f, 1.0f);

	return QColor(static_cast<int>(r * 255.0f),
				  static_cast<int>(v * 255.0f),
				  static_cast<int>(b * 255.0f),
				  static_cast<int>(a * 255.0f));
}

QColor converti_couleur(const float *rvba)
{
	return converti_couleur(rvba[0], rvba[1], rvba[2], rvba[3]);
}

QColor converti_couleur(const dls::phys::couleur32 &rvba)
{
	return converti_couleur(rvba[0], rvba[1], rvba[2], rvba[3]);
}
