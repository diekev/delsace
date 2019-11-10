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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <functional>

struct Corps;
struct Sphere;
class Polygone;
class Primitive;
class Volume;

using type_fonc_rap_prim = std::function<void(Corps &, Primitive *)>;
using type_fonc_rap_poly = std::function<void(Corps &, Polygone *)>;
using type_fonc_rap_volume = std::function<void(Corps &, Volume *)>;
using type_fonc_rap_sphere = std::function<void(Corps &, Sphere *)>;

void pour_chaque_polygone(Corps &corps, type_fonc_rap_poly fonction_rappel);

void pour_chaque_polygone_ferme(Corps &corps, type_fonc_rap_poly fonction_rappel);

void pour_chaque_polygone_ouvert(Corps &corps, type_fonc_rap_poly fonction_rappel);

void pour_chaque_primitive(Corps &corps, type_fonc_rap_prim fonction_rappel);

void pour_chaque_volume(Corps &corps, type_fonc_rap_volume fonction_rappel);

void pour_chaque_sphere(Corps &corps, type_fonc_rap_sphere fonction_rappel);

/* version const */

using type_fonc_rap_prim_const = std::function<void(Corps const &, Primitive *)>;
using type_fonc_rap_poly_const = std::function<void(Corps const &, Polygone *)>;
using type_fonc_rap_volume_const = std::function<void(Corps const &, Volume *)>;
using type_fonc_rap_sphere_const = std::function<void(Corps const &, Sphere *)>;

void pour_chaque_polygone(Corps const &corps, type_fonc_rap_poly_const fonction_rappel);

void pour_chaque_polygone_ferme(Corps const &corps, type_fonc_rap_poly_const fonction_rappel);

void pour_chaque_polygone_ouvert(Corps const &corps, type_fonc_rap_poly_const fonction_rappel);

void pour_chaque_primitive(Corps const &corps, type_fonc_rap_prim_const fonction_rappel);

void pour_chaque_volume(Corps const &corps, type_fonc_rap_volume_const fonction_rappel);

void pour_chaque_sphere(Corps const &corps, type_fonc_rap_sphere_const fonction_rappel);
