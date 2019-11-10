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

#include <atomic>
#include <iostream>

#undef STATISTIQUES

namespace kdo {

struct Statistiques {
	std::atomic_uint nombre_rayons_primaires;
	std::atomic_uint test_entresections_triangles;
	std::atomic_uint nombre_entresections_triangles;
	std::atomic_uint test_entresections_boites;
	std::atomic_uint test_entresections_volumes;
};

extern Statistiques statistiques;

void init_statistiques();

void imprime_statistiques(std::ostream &os);

}  /* namespace kdo */
