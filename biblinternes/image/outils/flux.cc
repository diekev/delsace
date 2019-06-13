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

#include "flux.h"

namespace dls {
namespace image {
namespace outils {

namespace detail {

/**
 * Retourne vrai si les éléments spécifiés sont égaux.
 */
template<typename T1, typename T2>
auto fait_parti(T1 &&a, T2 &&b) -> bool
{
	return a == b;
}

/**
 * Retourne vrai si le premier élément est égal à l'un des éléments spécifiés
 * dans la liste variable de paramètre.
 */
template<typename T1, typename T2, typename... Ts>
auto fait_parti(T1 &&a, T2 &&b, Ts &&... t) -> bool
{
	return a == b || fait_parti(a, t...);
}

}  /* namespace detail */

bool est_extension_exr(const std::string &extension)
{
	return detail::fait_parti(extension, ".exr");
}

bool est_extension_jpeg(const std::string &extension)
{
	return detail::fait_parti(extension, ".jpg", ".jpeg");
}

bool est_extension_pnm(const std::string &extension)
{
	return detail::fait_parti(extension, ".ppm", ".pgm", ".pbm", ".pnm");
}

}  /* namespace outils */
}  /* namespace image */
}  /* namespace dls */
