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
	std::vector<int> m_points;

public:
	using plage_points = plage_iterable<std::vector<int>::iterator>;
	using plage_points_const = plage_iterable<std::vector<int>::const_iterator>;

	std::string nom;

	void ajoute_point(int index_point);

	void reserve(const size_t nombre);

	plage_points index();

	plage_points_const index() const;
};

/* ************************************************************************** */

class GroupePolygone {
	std::vector<int> m_polygones;

public:
	using plage_points = plage_iterable<std::vector<int>::iterator>;
	using plage_points_const = plage_iterable<std::vector<int>::const_iterator>;

	std::string nom;

	void ajoute_primitive(int index_poly);

	void reserve(const size_t nombre);

	plage_points index();

	plage_points_const index() const;
};
