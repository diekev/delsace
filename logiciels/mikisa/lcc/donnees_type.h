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

#include "biblinternes/outils/parametres.hh"
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

const char *type_var_opengl(type_var type);

/* ************************************************************************** */

struct donnees_parametre {
	type_var type{};
	float valeur_defaut = 0.0f;
	const char *nom = "";

	donnees_parametre() = default;

	donnees_parametre(char const *n, type_var t, float v = 0.0f)
		: type(t)
		, valeur_defaut(v)
		, nom(n)
	{}
};

/* Le paramètre de gabarit est utilisé pour définir des types différents. */
template <int N>
struct parametres_fonction {
private:
	dls::tableau<donnees_parametre> m_donnees{};

public:
	parametres_fonction() = default;

	parametres_fonction(donnees_parametre type0)
	{
		m_donnees.pousse(type0);
	}

	template <typename... Ts>
	parametres_fonction(donnees_parametre type0, Ts... reste)
	{
		m_donnees.redimensionne(1 + static_cast<long>(sizeof...(reste)));
		otl::accumule(0, &m_donnees[0], type0, reste...);
	}

	type_var type(int idx) const
	{
		return m_donnees[idx].type;
	}

	char const *nom(int idx) const
	{
		return m_donnees[idx].nom;
	}

	float valeur_defaut(int idx) const
	{
		return m_donnees[idx].valeur_defaut;
	}

	long taille() const
	{
		return m_donnees.taille();
	}
};

/* ************************************************************************** */

using param_entrees = parametres_fonction<0>;
using param_sorties = parametres_fonction<1>;

/* ************************************************************************** */

/* Le paramètre de gabarit est utilisé pour définir des types différents.
 * Cela semble redondant avec parametres_fonction, mais on l'utilise dans la
 * résolution de fonctions surchargés puisque nous n'avons que les types. */
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
		otl::accumule(1, &types[0], type0, reste...);
	}

	void ajoute(type_var type)
	{
		types.pousse(type);
	}
};

/* ************************************************************************** */

int taille_type(type_var type);

/* ************************************************************************** */

enum class code_inst : int;

code_inst code_inst_conversion(type_var type1, type_var type2);

/* ************************************************************************** */

template <int N>
auto extrait_types(parametres_fonction<N> const &params)
{
	auto dt = donnees_type<N>();
	dt.types.reserve(params.taille());

	for (auto i = 0; i < params.taille(); ++i) {
		dt.types.pousse(params.type(i));
	}

	return dt;
}

/* ************************************************************************** */

using types_entrees = donnees_type<0>;
using types_sorties = donnees_type<1>;

}  /* namespace lcc  */
