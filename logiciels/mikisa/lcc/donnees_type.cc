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

#include "donnees_type.h"

#include "code_inst.hh"

namespace lcc {

/* ************************************************************************** */

int taille_type(type_var type)
{
	switch (type) {
		default:
		{
			return 0;
		}
		case type_var::TABLEAU:
		case type_var::ENT32:
		case type_var::DEC:
			return 1;
		case type_var::VEC2:
			return 2;
		case type_var::VEC3:
			return 3;
		case type_var::COULEUR:
		case type_var::VEC4:
			return 4;
		case type_var::MAT3:
			return 9;
		case type_var::MAT4:
			return 16;
	}
}

/* ************************************************************************** */

const char *chaine_type_var(type_var type)
{
	switch (type) {
		case type_var::DEC:
			return "type_var::DEC";
		case type_var::VEC2:
			return "type_var::VEC2";
		case type_var::VEC3:
			return "type_var::VEC3";
		case type_var::VEC4:
			return "type_var::VEC4";
		case type_var::MAT3:
			return "type_var::MAT3";
		case type_var::MAT4:
			return "type_var::MAT4";
		case type_var::ENT32:
			return "type_var::ENT";
		case type_var::CHAINE:
			return "type_var::CHAINE";
		case type_var::INVALIDE:
			return "type_var::INVALIDE";
		case type_var::TABLEAU:
			return "type_var::TABLEAU";
		case type_var::POLYMORPHIQUE:
			return "type_var::POLYMORPHIQUE";
		case type_var::COULEUR:
			return "type_var::COULEUR";
	}

	return "erreur";
}

/* ************************************************************************** */

struct donnees_conversions {
	code_inst code_conversion{};
	type_var type_entree{};
	type_var type_sortie{};
};

static donnees_conversions table_conversions[] = {
	{ code_inst::ENT_VERS_DEC, type_var::ENT32, type_var::DEC },
	{ code_inst::DEC_VERS_ENT, type_var::DEC, type_var::ENT32 },
	{ code_inst::ENT_VERS_VEC2, type_var::ENT32, type_var::VEC2 },
	{ code_inst::DEC_VERS_VEC2, type_var::DEC, type_var::VEC2 },
	{ code_inst::ENT_VERS_VEC3, type_var::ENT32, type_var::VEC3 },
	{ code_inst::DEC_VERS_VEC3, type_var::DEC, type_var::VEC3 },
	{ code_inst::ENT_VERS_VEC4, type_var::ENT32, type_var::VEC4 },
	{ code_inst::DEC_VERS_VEC4, type_var::DEC, type_var::VEC4 },
	{ code_inst::DEC_VERS_COULEUR, type_var::DEC, type_var::COULEUR },
	{ code_inst::VEC3_VERS_COULEUR, type_var::VEC3, type_var::COULEUR },
	{ code_inst::COULEUR_VERS_VEC3, type_var::COULEUR, type_var::VEC3 },
};

code_inst code_inst_conversion(type_var type1, type_var type2)
{
	for (auto &conv : table_conversions) {
		if (conv.type_entree == type1 && conv.type_sortie == type2) {
			return conv.code_conversion;
		}
	}

	return code_inst::TERMINE;
}

}  /* namespace lcc */
