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

#include <string>
#include <vector>

#include "bibliotheques/outils/iterateurs.h"

/* ************************************************************************** */

class GroupePoint {
	std::vector<size_t> m_points;

public:
	using plage_points = plage_iterable<std::vector<size_t>::iterator>;
	using plage_points_const = plage_iterable<std::vector<size_t>::const_iterator>;

	std::string nom;

	void ajoute_point(size_t index_point);

	void reserve(long const nombre);

	plage_points index();

	plage_points_const index() const;
};

/* ************************************************************************** */

class GroupePrimitive {
	std::vector<size_t> m_primitives{};

public:
	using plage_prims = plage_iterable<std::vector<size_t>::iterator>;
	using plage_prims_const = plage_iterable<std::vector<size_t>::const_iterator>;

	std::string nom{};

	void ajoute_primitive(size_t index_poly);

	void reserve(long const nombre);

	plage_prims index();

	plage_prims_const index() const;
};
