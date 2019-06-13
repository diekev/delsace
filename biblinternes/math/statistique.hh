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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <numeric>

#include "concepts.hh"

namespace dls {
namespace math {

namespace interne {

/**
 * Implémentation de la fonction médiane.
 */
template <ConceptNombre N, typename InputIterator>
auto mediane(InputIterator debut, size_t size)
{
	const auto &half = static_cast<long>(size >> 1);

	if ((size & 1) == 0) {
		return (*(debut + half - 1) + *(debut + half)) / N(2);
	}

	return *(debut + half);
}

/**
 * Implémentation de la fonction de calcul de l'écart type.
 */
template <ConceptNombre N, typename InputIterator>
auto ecart_type(InputIterator debut, InputIterator fin, N moyenne)
{
	const auto &size = std::distance(debut, fin);
	const auto &inner_product = std::inner_product(debut, fin, debut, N(0));
	return std::sqrt(inner_product / static_cast<N>(size) - moyenne * moyenne);
}

}  /* namespace interne */

/**
 * Calcule la médiane d'un ensemble de données d'une taille spécifiée, à partir
 * de l'itérateur début donné. La fonction ne vérifie pas que la taille soit
 * dans les bornes de l'itérateur.
 */
template <ConceptNombre N, typename InputIterator>
auto mediane(InputIterator debut, size_t taille)
{
	return interne::mediane<N>(debut, taille);
}

/**
 * Calcule la médiane d'un ensemble de données, entre l'itérateur début et
 * l'itérateur fin spécifiés.
 */
template <ConceptNombre N, typename InputIterator>
auto mediane(InputIterator debut, InputIterator fin)
{
	const auto &taille = std::distance(debut, fin);
	return interne::mediane<N>(debut, static_cast<size_t>(taille));
}

/**
 * Calcule la moyenne d'un ensemble de données, entre l'itérateur début et
 * l'itérateur fin spécifiés.
 */
template <ConceptNombre N, typename InputIterator>
auto moyenne(InputIterator debut, InputIterator fin)
{
	const auto &somme = std::accumulate(debut, fin, N(0));
	return somme / static_cast<typename InputIterator::value_type>(std::distance(debut, fin));
}

/**
 * Calcule l'écart type d'un ensemble de données, entre l'itérateur début et
 * l'itérateur fin spécifiés.
 */
template <ConceptNombre N, typename InputIterator>
auto ecart_type(InputIterator debut, InputIterator fin)
{
	const auto &m = moyenne<N>(debut, fin);
	return interne::ecart_type(debut, fin, m);
}

/**
 * Calcule l'écart type d'un ensemble de données, entre l'itérateur début et
 * l'itérateur fin spécifiés, avec la moyenne donnée.
 */
template <ConceptNombre N, typename InputIterator>
auto ecart_type(InputIterator debut, InputIterator fin, N moyenne)
{
	return interne::ecart_type(debut, fin, moyenne);
}

}  /* namespace math */
}  /* namespace dls */
