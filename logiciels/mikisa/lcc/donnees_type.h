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

#include "biblinternes/structures/tableau.hh"

namespace lcc {

enum class type_var : unsigned short {
	TABLEAU       = (1 << 0),
	ENT32         = (1 << 1),
	DEC           = (1 << 2),
	VEC2          = (1 << 3),
	VEC3          = (1 << 4),
	VEC4          = (1 << 5),
	MAT3          = (1 << 6),
	MAT4          = (1 << 7),
	CHAINE        = (1 << 8),
	POLYMORPHIQUE = (1 << 9),
	COULEUR       = (1 << 10),

	INVALIDE      = (1 << 15),
};

/* ************************************************************************** */

const char *chaine_type_var(type_var type);

/* ************************************************************************** */

template <int N>
struct donnees_type {
	dls::tableau<type_var> types{};

	donnees_type() = default;

	donnees_type(type_var type0)
	{
		types.pousse(type0);
	}

	template <typename... Ts>
	donnees_type(type_var type0, Ts... reste)
	{
		types.redimensionne(1 + static_cast<long>(sizeof...(reste)));
		types[0] = type0;

		accumule(1, reste...);
	}

	void ajoute(type_var type)
	{
		types.pousse(type);
	}

private:
	template <typename... Ts>
	void accumule(long idx, type_var type0, Ts... reste)
	{
		types[idx] = type0;
		accumule(idx + 1, reste...);
	}

	void accumule(long idx, type_var type0)
	{
		types[idx] = type0;
	}
};

/* ************************************************************************** */

int taille_type(type_var type);

/* ************************************************************************** */

enum class code_inst : int;

code_inst code_inst_conversion(type_var type1, type_var type2);

/* ************************************************************************** */

using types_entrees = donnees_type<0>;
using types_sorties = donnees_type<1>;

}  /* namespace lcc  */
