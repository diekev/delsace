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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <type_traits>

#include "../../math/concepts.hh"

namespace dls {
namespace image {
namespace outils {

/**
 * Retourne la moyenne des nombres passés en paramètre.
 */
template <ConceptNombre nombre>
auto moyenne(nombre r, nombre g, nombre b)
{
	return (r + b + g) / 3;
}

/**
 * Retourne la luminance calculée selon la recommandation UIT-R BT 709, ou Rec.
 * 709, des valeurs passées en paramètre.
 */
template <ConceptNombre nombre>
auto luminance_709(nombre r, nombre g, nombre b)
{
	/* cppcheck-suppress syntaxError */
	if constexpr (std::is_floating_point<nombre>::value) {
		return r * 0.2126f + g * 0.7152f + b * 0.0722f;
	}
	else {
		return (r * 54 + g * 182 + b * 19) / 255;
	}
}

/**
 * Retourne la luminance calculée selon la Rec. 601, des valeurs passées en
 * paramètre.
 */
template <ConceptNombre nombre>
auto luminance_601(nombre r, nombre g, nombre b)
{
	/* cppcheck-suppress syntaxError */
	if constexpr (std::is_floating_point<nombre>::value) {
		return r * 0.299f + g * 0.587f + b * 0.114f;
	}
	else {
		return (r * 76 + g * 149 + b * 36) / 255;
	}
}

}  /* namespace outils */
}  /* namespace image */
}  /* namespace dls */
