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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <algorithm>

namespace dls {
namespace outils {

/**
 * Retourne vrai si le premier élément est à l'un ou l'autre des éléments
 * suivants.
 */
template <typename T, typename... Ts>
inline auto est_element(T &&a, Ts &&... ts)
{
	return ((a == ts) || ...);
}

/**
 * Retourne vrai si tous les éléments sont égaux les uns aux autres.
 */
template <typename T, typename... Ts>
inline auto sont_egaux(T &&a, Ts &&... ts)
{
	return ((a == ts) && ...);
}

/**
 * Retourne vrai si 'v' possède l'un ou l'autre des bits contenu dans 'd'.
 */
template <typename T>
inline auto possede_drapeau(T v, T d)
{
	return (v & d) != static_cast<T>(0);
}

}  /* namespace outils */
}  /* namespace dls */
