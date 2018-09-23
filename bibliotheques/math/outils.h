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

namespace math {

/**
 * Retourne une valeur équivalente à la restriction de la valeur 'a' entre 'min'
 * et 'max'.
 */
template <typename T>
auto restreint(const T &a, const T &min, const T &max)
{
	if (a < min) {
		return min;
	}

	if (a > max) {
		return max;
	}

	return a;
}

/**
 * Retourne un nombre entre 'neuf_min' et 'neuf_max' qui est relatif à 'valeur'
 * dans la plage entre 'vieux_min' et 'vieux_max'. Si la valeur est hors de
 * 'vieux_min' et 'vieux_max' il sera restreint à la nouvelle plage.
 */
template <typename T>
auto traduit(const T valeur, const T vieux_min, const T vieux_max, const T neuf_min, const T neuf_max)
{
	T tmp;

	if (vieux_min > vieux_max) {
		tmp = vieux_min - restreint(valeur, vieux_max, vieux_min);
	}
	else {
		tmp = restreint(valeur, vieux_min, vieux_max);
	}

	tmp = (tmp - vieux_min) / (vieux_max - vieux_min);
	return neuf_min + tmp*(neuf_max - neuf_min);
}

}  /* namespace math */
