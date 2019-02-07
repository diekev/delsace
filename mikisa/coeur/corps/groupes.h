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

#include <string>
#include <vector>

#include "bibliotheques/outils/iterateurs.h"

/* ************************************************************************** */

class GroupePoint {
	std::vector<size_t> m_points{};

public:
	using plage_points = plage_iterable<std::vector<size_t>::iterator>;
	using plage_points_const = plage_iterable<std::vector<size_t>::const_iterator>;

	std::string nom{};

	void ajoute_point(size_t index_point);

	void reserve(long const nombre);

	void reinitialise();

	long taille() const;

	bool contiens(size_t index_point) const;

	plage_points index();

	plage_points_const index() const;
};

/* ************************************************************************** */

class GroupePrimitive {
	std::vector<size_t> m_primitives{};

public:
	using plage_prims = plage_iterable<std::vector<size_t>::iterator>;
	using plage_prims_const = plage_iterable<std::vector<size_t>::const_iterator>;

	std::string nom{};

	void ajoute_primitive(size_t index_poly);

	void reserve(long const nombre);

	void reinitialise();

	long taille() const;

	plage_prims index();

	plage_prims_const index() const;
};

/* ************************************************************************** */

/**
 * Classe servant à itérer sur les index des points ou des primitives contenus
 * dans un groupe ou sur les index de l'ensemble des points ou primitives d'un
 * corps.
 *
 * Le problème que l'on essaie de régler est de pouvoir avoir des algorithmes
 * fonctionnant soit sur tout le corps, soit un groupe sans avoir à avoir une
 * duplication des boucles. Un autre solution serait d'avoir des groupes par
 * défaut contenant les index de tous les points ou toutes les primitives, mais
 * cela augmenterait la consommation de mémoire.
 */
class iteratrice_index {
	long m_nombre = 0;
	long m_courant = 0;
	std::vector<size_t>::iterator m_iter_groupe{};
	std::vector<size_t>::iterator m_iter_fin{};
	bool m_est_groupe = false;
	bool m_pad[7];

public:
	/**
	 * Classe implémentant l'incrémentation, la déréférence, et l'égalité des
	 * iteratrices.
	 */
	class iteratrice {
		bool m_est_groupe = false;
		long m_etat_nombre = 0;
		std::vector<size_t>::iterator m_etat_iter{};

	public:
		iteratrice(long nombre);

		iteratrice(std::vector<size_t>::iterator iter);

		long operator*();

		iteratrice &operator++();

		bool est_egal(iteratrice it);
	};

	iteratrice_index() = default;

	/**
	 * Construction à partir d'un nombre d'index, cela est pour itérer sur tout
	 * le corps.
	 *
	 * L'itération se fait sur l'ensemble [0, 1, 2, ..., nombre - 1], où nombre
	 * est soit le nombre de points, soit le nombre de primitives.
	 */
	explicit iteratrice_index(long nombre);

	/**
	 * Construction à partir d'un groupe de points.
	 *
	 * L'itération se fait sur les index contenu dans le groupe.
	 */
	explicit iteratrice_index(GroupePoint *groupe_point);

	/**
	 * Construction à partir d'un groupe de primitive.
	 *
	 * L'itération se fait sur les index contenu dans le groupe.
	 */
	explicit iteratrice_index(GroupePrimitive *groupe_primitive);

	iteratrice begin();

	iteratrice end();
};

inline bool operator==(iteratrice_index::iteratrice ita, iteratrice_index::iteratrice itb)
{
	return ita.est_egal(itb);
}

inline bool operator!=(iteratrice_index::iteratrice ita, iteratrice_index::iteratrice itb)
{
	return !(ita == itb);
}
