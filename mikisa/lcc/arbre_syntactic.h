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

#include <any>
#include <list>

#include "morceaux.hh"

struct compileuse_lng;

struct ContexteGenerationCode;

namespace lcc {
enum class type_var : unsigned short;
}

namespace lcc::noeud {

enum class type_noeud : char {
	RACINE,
	PROPRIETE,
	VARIABLE,
	VALEUR,
	FONCTION,
	ATTRIBUT,
	OPERATION_UNAIRE,
	OPERATION_BINAIRE,
	ASSIGNATION,
	ACCES_MEMBRE_POINT,
	SI,
	BLOC,
	POUR,
	PLAGE,
};

/* ************************************************************************** */

struct base final {
	/* propriétés */
	std::list<base *> enfants{};
	DonneesMorceaux const &donnees;
	int pointeur_donnees = 0;
	type_noeud type = type_noeud::RACINE;
	bool calcule = false;
	type_var donnees_type{};
	std::any valeur_calculee{};

	/* entreface */
	base(ContexteGenerationCode &contexte, DonneesMorceaux const &donnees_, type_noeud type_);

	void ajoute_noeud(base *noeud);

	const DonneesMorceaux &donnees_morceau() const;

	id_morceau identifiant() const;

	std::string_view chaine() const;

	void imprime_code(std::ostream &os, int profondeur);
};

/* ************************************************************************** */

int genere_code(
		base *b,
		ContexteGenerationCode &contexte_generation,
		compileuse_lng &compileuse,
		bool expr_gauche);

}
