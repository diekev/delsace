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

#include "statistiques.h"

namespace kdo {

Statistiques statistiques;

void init_statistiques()
{
	statistiques.nombre_rayons_primaires = 0;
	statistiques.test_entresections_triangles = 0;
	statistiques.nombre_entresections_triangles = 0;
	statistiques.test_entresections_boites = 0;
	statistiques.test_entresections_volumes = 0;
}

void imprime_statistiques(std::ostream &os)
{
	os << "Nombre de rayons primaires                : " << statistiques.nombre_rayons_primaires << '\n';
	os << "Nombre de tests d'entresections boites    : " << statistiques.test_entresections_boites << '\n';
	os << "Nombre de tests d'entresections volumes   : " << statistiques.test_entresections_volumes << '\n';
	os << "Nombre de tests d'entresections triangles : " << statistiques.test_entresections_triangles << '\n';
	os << "Nombre d'entresections triangles          : " << statistiques.nombre_entresections_triangles << '\n';
	os << "Pourcentage d'entresections               : " << statistiques.nombre_entresections_triangles * 100.0 / statistiques.test_entresections_triangles << '\n';
	os << std::endl;
}

}  /* namespace kdo */
