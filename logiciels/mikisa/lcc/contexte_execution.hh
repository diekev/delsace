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

#include "biblinternes/parallèle/synchronisé.hh"
#include "biblinternes/structures/tableau.hh"

struct Corps;

namespace lcc {

enum class ctx_script : unsigned short {
	invalide  = 0,
	pixel     = (1 << 0),
	voxel     = (1 << 1),
	fichier   = (1 << 2),
	nuanceur  = (1 << 3),
	primitive = (1 << 4),
	point     = (1 << 5),

	detail    = (8 << 0),
	topologie = (8 << 1),

	/* outils */
	tous = (pixel | voxel | fichier | nuanceur | primitive | detail | topologie),
};

inline auto operator~(ctx_script ctx)
{
	return static_cast<ctx_script>(~static_cast<unsigned short>(ctx));
}

inline auto operator&(ctx_script ctx0, ctx_script ctx1)
{
	return static_cast<ctx_script>(
				static_cast<unsigned short>(ctx0) & static_cast<unsigned short>(ctx1));
}

inline auto operator|(ctx_script ctx0, ctx_script ctx1)
{
	return static_cast<ctx_script>(
				static_cast<unsigned short>(ctx0) | static_cast<unsigned short>(ctx1));
}

template <typename T>
auto possede_drapeau(T v, T d)
{
	return (v & d) != static_cast<T>(0);
}

/* ************************************************************************** */

struct magasin_tableau {
	dls::tableau<dls::tableau<int>> tableaux;

	std::pair<dls::tableau<int> &, long> cree_tableau()
	{
		tableaux.pousse({});
		return { tableaux.back(), tableaux.taille() - 1};
	}

	dls::tableau<int> &tableau(long idx)
	{
		return tableaux[idx];
	}

	void reinitialise()
	{
		tableaux.clear();
	}
};

/* ************************************************************************** */

/* Le contexte local est pour les données locales à chaque thread et chaque
 * itération. */
struct ctx_local {
	magasin_tableau tableaux;

	void reinitialise()
	{
		tableaux.reinitialise();
	}
};

/* ************************************************************************** */

struct ctx_exec {
	/* Le corps dans notre contexte. */
	dls::synchronise<Corps *> ptr_corps;

	/* Si contexte topologie primitive. */
	//dls::tableau<Corps const *> corps_entrees;
};

}  /* namespace lcc */
