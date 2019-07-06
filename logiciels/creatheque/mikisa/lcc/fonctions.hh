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

#include <unordered_map>

#include "donnees_type.h"

namespace lcc {

/* ************************************************************************** */

struct signature {
	types_entrees entrees{};
	types_sorties sorties{};

	signature() = default;

	signature(types_entrees _entrees_, types_sorties _sorties_);
};

/* ************************************************************************** */

enum class code_inst : int;
enum class ctx_script : unsigned short;

struct donnees_fonction {
	signature seing{};
	lcc::code_inst type{};
	lcc::ctx_script ctx{};
	short pad{};

	donnees_fonction() = default;
};

/* ************************************************************************** */

struct donnees_fonction_generation {
	donnees_fonction const *donnees;
	types_entrees entrees;
	types_sorties sorties;
	type_var type;
};

struct magasin_fonctions {
	void ajoute_fonction(
			std::string const &nom,
			lcc::code_inst type,
			signature const &seing,
			lcc::ctx_script ctx);

	donnees_fonction_generation meilleure_candidate(
			std::string const &nom,
			types_entrees const &type_params);

private:
	std::unordered_map<std::string, dls::tableau<donnees_fonction>> table{};
};

/* ************************************************************************** */

void enregistre_fonctions_base(magasin_fonctions &magasin);

}  /* namespace lcc */
