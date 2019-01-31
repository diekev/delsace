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

#include <string>
#include <unordered_map>
#include <vector>

class Graphe;
class Noeud;
class OperatriceImage;

struct DescOperatrice {
	typedef OperatriceImage *(*factory_func)(Graphe &, Noeud *);

	DescOperatrice() = default;

	DescOperatrice(const std::string &opname, const std::string &ophelp, factory_func func)
	    : name(opname)
	    , tooltip(ophelp)
	    , build_operator(func)
	{}

	std::string name = "";
	std::string tooltip = "";
	factory_func build_operator = nullptr;
};

template <typename T>
inline DescOperatrice cree_desc()
{
	return DescOperatrice(
				T::NOM,
				T::AIDE,
	            [](Graphe &graphe_parent, Noeud *noeud) -> OperatriceImage* { return new T(graphe_parent, noeud); });
}

class UsineOperatrice final {
public:
	typedef OperatriceImage *(*factory_func)(Graphe &, Noeud *);

	/**
	 * @brief register_type Register a new element in this factory.
	 *
	 * @param key The key associate @ func to.
	 * @param func A function pointer with signature 'ImageNode *(void)'.
	 *
	 * @return The number of entries after registering the new element.
	 */
	size_t enregistre_type(const DescOperatrice &desc);

	/**
	 * @brief operator() Create a ImageNode based on the given key.
	 *
	 * @param key The key to lookup.
	 * @return A new ImageNode object corresponding to the given key.
	 */
	OperatriceImage *operator()(const std::string &name, Graphe &graphe_parent, Noeud *noeud);

	/**
	 * @brief num_entries The number of entries registered in this factory.
	 *
	 * @return The number of entries registered in this factory, 0 if empty.
	 */
	inline size_t num_entries() const
	{
		return m_map.size();
	}

	/**
	 * @brief keys std::strings registered in this factory.
	 *
	 * @return An unsorted vector containing the keys registered in this factory.
	 */
	std::vector<DescOperatrice> keys() const;

	/**
	 * @brief registered Check whether or not a key has been registered in this
	 *                   factory.
	 *
	 * @param key The key to lookup.
	 * @return True if the key is found, false otherwise.
	 */
	bool registered(const std::string &key) const;

private:
	std::unordered_map<std::string, DescOperatrice> m_map{};
};
