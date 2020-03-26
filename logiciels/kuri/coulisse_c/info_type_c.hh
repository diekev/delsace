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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/flux_chaine.hh"
#include "biblinternes/structures/vue_chaine_compacte.hh"

struct ConstructriceCodeC;
struct ContexteGenerationCode;
struct NoeudEnum;
struct Type;

dls::chaine predeclare_info_type_C(
		ConstructriceCodeC &constructrice,
		Type *type);

dls::chaine cree_info_type_C(
		ContexteGenerationCode &contexte,
		ConstructriceCodeC &constructrice,
		Type *type);

dls::chaine chaine_valeur_enum(
		NoeudEnum *noeud_enum,
		dls::vue_chaine_compacte const &nom);
