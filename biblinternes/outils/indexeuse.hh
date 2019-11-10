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

#include "parametres.hh"

namespace otl {

/**
 * Structures auxilliaires pour calculer des index séquentiels depuis des
 * coordonnées N-dimensionnelles.
 *
 * Pour une coordonnée 2D (x, y), sur une grille de résolution Rx, Ry, l'index
 * sera de x + y * Rx.
 *
 * Pour une coordonnée 2D (x, y, z), sur une grille de résolution Rx, Ry, Rz,
 * l'index sera de x + y * Rx + z * Rx * Ry.
 *
 * Nous pouvons généraliser le calcul avec x * Px + y * Py + ... + n * Pn, où Pn
 * est un poids appliqué pour chaque dimension. Le calcul de ce poids est tout
 * simplement le produit des résolutions Rx ... R(n - 1), avec Px = 1.
 */

/**
 * Une indexeuse_statique ne garde pas trace du dernier index calculé, et peut
 * donc être utilisée par des différents fils d'exécution.
 */
template <unsigned long NDIMS>
struct indexeuse_statique {
	long dalles[NDIMS];
	long nombre_elements = 0;

	/**
	 * Construit une indexeuse_statique pour les résolutions rs de type Rs. Rs
	 * doit être un type entier convertible en long.
	 */
	template <typename... Rs>
	indexeuse_statique(Rs... rs)
	{
		static_assert(sizeof...(Rs) <= NDIMS);

		long res[sizeof...(Rs)];
		accumule(0, res, rs...);

		dalles[0] = 1;
		nombre_elements = 1;

		for (auto i = 1ul; i < sizeof...(Rs); ++i) {
			dalles[i] = dalles[i - 1] * res[i - 1];
			nombre_elements *= res[i];
		}
	}

	/**
	 * Cacul l'index pour les positions ps données. Si le nombre de positions
	 * est inférieure au nombre de résolutions définies via le constructeur, les
	 * positions pour les dimensions restantes sont implicitement considérées
	 * comme étant égales à 0.
	 */
	template <typename... Ps>
	long operator()(Ps... ps)
	{
		static_assert(sizeof...(Ps) <= NDIMS);

		long pos[sizeof...(Ps)];
		accumule(0, pos, ps...);

		auto r = 0;
		for (auto i = 0ul; i < sizeof...(Ps); ++i) {
			r += pos[i] * dalles[i];
		}

		return r;
	}
};

/**
 * Crée une indexeuse_statique avec les résolutions passées en paramètres. Cette
 * fonction nous évite de toujours avoir à préciser le nombre de dimensions :
 *
 * auto idx = indexeuse_statique<3>(rx, ry, rz);
 *
 * devient :
 *
 * auto idx = cree_indexeuse_statique(rx, ry, rz);
 *
 * ce qui peut nous éviter des bugs.
 */
template <typename... Rs>
auto cree_indexeuse_statique(Rs... rs)
{
	return indexeuse_statique<sizeof...(rs)>(rs...);
}

/**
 * Une indexeuse_dynamique garde trace du dernier index calculé, ce qui peut
 * être utile pour les différences finies, et de ce fait ne peut pas être
 * utilisé par différents fils d'exécution sans problème de concurrence
 * critique.
 */
template <unsigned long NDIMS>
struct indexeuse_dynamique {
	long dalles[NDIMS];
	long index = 0;
	long nombre_elements = 0;

	/**
	 * Construit une indexeuse_dynamique pour les résolutions rs de type Rs. Rs
	 * doit être un type entier convertible en long.
	 */
	template <typename... Rs>
	indexeuse_dynamique(Rs... rs)
	{
		static_assert(sizeof...(Rs) <= NDIMS);

		long res[sizeof...(Rs)];
		accumule(0, res, rs...);

		dalles[0] = 1;
		nombre_elements = 1;

		for (auto i = 1ul; i < sizeof...(Rs); ++i) {
			dalles[i] = dalles[i - 1] * res[i - 1];
			nombre_elements *= res[i];
		}
	}

	/**
	 * Cacul l'index pour les positions ps données. Si le nombre de positions
	 * est inférieure au nombre de résolutions définies via le constructeur, les
	 * positions pour les dimensions restantes sont implicitement considérées
	 * comme étant égales à 0.
	 *
	 * L'index est gardé en mémoire et retourné.
	 */
	template <typename... Ps>
	long operator()(Ps... ps)
	{
		static_assert(sizeof...(Ps) <= NDIMS);

		long pos[sizeof...(Ps)];
		accumule(0, pos, ps...);

		index = 0;
		for (auto i = 0ul; i < sizeof...(Ps); ++i) {
			index += pos[i] * dalles[i];
		}

		return index;
	}

	/**
	 * Retourne un nouvel index basé sur le dernier index et les valeurs de
	 * décalage données. Le calcul est similaire à celui d'un index normal. Le
	 * but de la fonction est de générer des index autour d'un autre pour des
	 * différences finies.
	 *
	 * Par exemple, avec une indexeuse_statique :
	 *
	 * auto idx = cree_indexeuse_statique(rx, ry);
	 * auto centre = idx(x    , y);
	 * auto gauche = idx(x - 1, y);
	 * auto droite = idx(x + 1, y);
	 *
	 * et avec une indexeuse_dynamique :
	 *
	 * auto idx = cree_indexeuse_dynamique(rx, ry);
	 * auto centre = idx(x, y);
	 * auto gauche = idx.decale(-1, 0);
	 * auto droite = idx.decale( 1, 0);
	 */
	template <typename... Ds>
	long decale(Ds... ds) const
	{
		static_assert(sizeof...(Ds) <= NDIMS);

		long decs[sizeof...(Ds)];
		accumule(0, decs, ds...);

		auto tmp = index;
		for (auto i = 0ul; i < sizeof...(Ds); ++i) {
			tmp += decs[i] * dalles[i];
		}

		return tmp;
	}
};

/**
 * Crée une cree_indexeuse_dynamique avec les résolutions passées en paramètres.
 * Cette fonction nous évite de toujours avoir à préciser le nombre de
 * dimensions :
 *
 * auto idx = indexeuse_dynamique<3>(rx, ry, rz);
 *
 * devient :
 *
 * auto idx = cree_indexeuse_dynamique(rx, ry, rz);
 *
 * ce qui peut nous éviter des bugs.
 */
template <typename... Rs>
auto cree_indexeuse_dynamique(Rs... rs)
{
	return indexeuse_dynamique<sizeof...(rs)>(rs...);
}

}  /* namespace otl */
