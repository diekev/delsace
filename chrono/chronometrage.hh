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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "outils.hh"

#include <cstddef>  /* pour std::size_t */
#include <limits>
#include <utility>  /* pour std::forward */

namespace dls {
namespace chronometrage {

namespace __privee {

/**
 * Empêche l'optimisation d'une boucle dont le corps est vide.
 */
static inline void empeche_optimisation()
{
	asm volatile("");
}

/**
 * Empêche l'optimisation d'une boucle dont le corps n'est pas utilisé.
 */
template <typename Fonction>
static inline void empeche_optimisation(Fonction &&fonction)
{
	asm volatile("" : "+r" (fonction));
}

inline void clobber()
{
	asm volatile("" : : : "memory");
}

}  /* namespace __privee */

/**
 * Chronomètre l'exécution d'une fonction.
 */
template <typename Fonction, typename... Parametres>
double chronometre_fonction(Fonction &&fonction, Parametres &&... params)
{
	auto debut = maintenant();

	fonction(std::forward<Parametres>(params)...);

	return maintenant() - debut;
}

/**
 * Chronomètre l'exécution d'une fonction à l'intérieur d'une boucle.
 */
template <typename Fonction, typename... Parametres>
double chronometre_boucle(
		const std::size_t iterations,
		bool normalise,
		Fonction &&fonction,
		Parametres &&... params)
{
	auto i = iterations;

	/* Chronomètre une boucle vide. */
	auto debut = maintenant();

	while (i-- > 0) {
		__privee::empeche_optimisation();
	}

	auto temps_iterations = maintenant() - debut;

	i = iterations;

	/* Chronomètre une boucle avec la fonction. */
	debut = maintenant();

	while (i-- > 0) {
		__privee::empeche_optimisation(
					fonction(std::forward<Parametres>(params)...));
	}

	/* Retourne la différence entre la durée d'une boucle vide et d'une boucle
	 * avec boucle. */

	if (normalise) {
		return ((maintenant() - debut) - temps_iterations) / iterations;
	}

	return ((maintenant() - debut) - temps_iterations);
}

/**
 * Chronomètre l'exécution d'une fonction à l'intérieur d'une boucle pour un
 * nombre d'époques donné.
 */
template <typename Fonction, typename... Parametres>
double chronometre_boucle_epoque(
		const std::size_t iterations,
		const std::size_t epoques,
		bool normalise,
		Fonction &&fonction,
		Parametres &&... params)
{
	/* Nous calculons le temps qu'il faut pour exécuter une fonction en
	 * utilisant le temps minimun qu'il aura fallu à exécuter la fonction
	 * pendant un certain nombre d'époques. Le temps minimun est utilisé car le
	 * temps d'exécution total d'une fonction contient, et le temps exact
	 * d'exécution, et le temps du bruit causé par d'autres évènements survenant
	 * lors de l'exécution de la fonction.
	 */

	auto temps_iterations_min = std::numeric_limits<double>::max();
	auto e = epoques;

	while (e-- > 0) {
		auto i = iterations;

		/* Chronomètre une boucle vide. */
		auto debut = maintenant();

		while (i-- > 0) {
			__privee::empeche_optimisation();
		}

		auto temps_iterations = maintenant() - debut;

		if (temps_iterations_min > temps_iterations) {
			temps_iterations_min = temps_iterations;
		}
	}

	auto temps_fonction_min = std::numeric_limits<double>::max();
	e = epoques;

	while (e-- > 0) {
		auto i = iterations;

		/* Chronomètre une boucle avec la fonction. */
		auto debut = maintenant();

		while (i-- > 0) {
			__privee::empeche_optimisation(
						fonction(std::forward<Parametres>(params)...));
		}

		auto temps_fonction = maintenant() - debut;

		if (temps_fonction_min > temps_fonction) {
			temps_fonction_min = temps_fonction;
		}
	}

	/* Retourne la différence entre la durée d'une boucle vide et d'une boucle
	 * avec boucle. */

	if (normalise) {
		return (temps_fonction_min - temps_iterations_min) / iterations;
	}

	return (temps_fonction_min - temps_iterations_min);
}

}  /* namespace chronometrage */
}  /* namespace dls */
