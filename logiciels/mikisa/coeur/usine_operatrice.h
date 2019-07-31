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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <functional>

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/tableau.hh"

class Graphe;
class Noeud;
class OperatriceImage;

struct DescOperatrice {
	using fonction_construction = std::function<OperatriceImage *(Graphe &, Noeud *)>;
	using fonction_destruction = std::function<void(OperatriceImage *)>;

	DescOperatrice() = default;

	DescOperatrice(
			dls::chaine const &opname,
			dls::chaine const &ophelp,
			fonction_construction func,
			fonction_destruction func_supp)
	    : name(opname)
	    , tooltip(ophelp)
	    , build_operator(func)
		, supprime_operatrice(func_supp)
	{}

	dls::chaine name = "";
	dls::chaine tooltip = "";
	fonction_construction build_operator = nullptr;
	fonction_destruction supprime_operatrice = nullptr;
};

template <typename T>
inline DescOperatrice cree_desc()
{
	return DescOperatrice(
				T::NOM,
				T::AIDE,
				[](Graphe &graphe_parent, Noeud *noeud) -> OperatriceImage*
	{
		return memoire::loge<T>(T::NOM, graphe_parent, noeud);
	},
	[](OperatriceImage *operatrice) -> void
	{
		auto derivee = dynamic_cast<T *>(operatrice);
		memoire::deloge(T::NOM, derivee);
	});
}

class UsineOperatrice final {
public:
	/**
	 * @brief register_type Register a new element in this factory.
	 *
	 * @param key The key associate @ func to.
	 * @param func A function pointer with signature 'ImageNode *(void)'.
	 *
	 * @return The number of entries after registering the new element.
	 */
	long enregistre_type(DescOperatrice const &desc);

	/**
	 * @brief operator() Create a ImageNode based on the given key.
	 *
	 * @param key The key to lookup.
	 * @return A new ImageNode object corresponding to the given key.
	 */
	OperatriceImage *operator()(dls::chaine const &name, Graphe &graphe_parent, Noeud *noeud);

	void deloge(OperatriceImage *operatrice);

	/**
	 * @brief num_entries The number of entries registered in this factory.
	 *
	 * @return The number of entries registered in this factory, 0 if empty.
	 */
	inline long num_entries() const
	{
		return m_map.taille();
	}

	/**
	 * @brief keys dls::chaines registered in this factory.
	 *
	 * @return An unsorted vector containing the keys registered in this factory.
	 */
	dls::tableau<DescOperatrice> keys() const;

	/**
	 * @brief registered Check whether or not a key has been registered in this
	 *                   factory.
	 *
	 * @param key The key to lookup.
	 * @return True if the key is found, false otherwise.
	 */
	bool registered(dls::chaine const &key) const;

private:
	dls::dico_desordonne<dls::chaine, DescOperatrice> m_map{};
};
