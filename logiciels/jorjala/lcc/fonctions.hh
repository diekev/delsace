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

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/ensemble.hh"

#include "donnees_type.h"

namespace lcc {

/* ************************************************************************** */

struct signature {
	param_entrees entrees{};
	param_sorties sorties{};

	signature() = default;

	signature(param_entrees _entrees_, param_sorties _sorties_);
};

/* ************************************************************************** */

enum class code_inst : int;
enum class ctx_script : unsigned short;

enum class req_fonc : char {
	polyedre,
	arbre_kd,
};

struct donnees_fonction {
	signature seing{};
	lcc::code_inst type{};
	lcc::ctx_script ctx{};
	req_fonc requete{};
	char pad{};
};

/* ************************************************************************** */

struct donnees_fonction_generation {
	donnees_fonction const *donnees;
	types_entrees entrees;
	types_sorties sorties;
	type_var type;
};

struct magasin_fonctions {
	donnees_fonction *ajoute_fonction(
			dls::chaine const &nom,
			lcc::code_inst type,
			signature const &seing,
			lcc::ctx_script ctx);

	donnees_fonction_generation meilleure_candidate(
			dls::chaine const &nom,
			types_entrees const &type_params);

	dls::chaine categorie = "";

	dls::dico_desordonne<dls::chaine, dls::tableau<donnees_fonction>> table{};
	dls::dico_desordonne<dls::chaine, dls::ensemble<dls::chaine>> table_categories{};
};

/* ************************************************************************** */

void enregistre_fonctions_base(magasin_fonctions &magasin);

}  /* namespace lcc */
