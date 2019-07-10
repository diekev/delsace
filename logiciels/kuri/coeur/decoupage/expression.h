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

#include "morceaux.hh"

namespace noeud {
struct base;
}

class assembleuse_arbre;

struct ContexteGenerationCode;

bool precedence_faible(id_morceau identifiant1, id_morceau identifiant2);

noeud::base *calcul_expression_double(
		assembleuse_arbre &assembleuse,
		ContexteGenerationCode &contexte,
		noeud::base *op,
		noeud::base *n1,
		noeud::base *n2);

noeud::base *calcul_expression_simple(
		assembleuse_arbre &assembleuse,
		noeud::base *op,
		noeud::base *n1);
