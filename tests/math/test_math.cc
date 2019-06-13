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

#include "tests_math.hh"

#include "../math/aleatoire.hh"
#include "../math/concepts.hh"
#include "../math/statistique.hh"

#include <tuple>

template <ConceptDecimal T>
void test_marsaglia(dls::test_unitaire::Controleuse &controleur)
{
	using namespace dls::math;

	std::mt19937 rng(137);
	T x, y;
	std::pair<T, T> seq[100];
	const auto sigma = static_cast<T>(0.5);
	const auto mean  = static_cast<T>(0.2);

	/* Generate a sequence of random number and verify that they all fall in the
	 * [-1, 1] interval, taking the standard deviation into account */
	for (int i(0); i < 100; ++i) {
		std::tie(x, y) = seq[i] = marsaglia(rng, mean, sigma);
		CU_VERIFIE_CONDITION(controleur, (x >= T(-1 - sigma)) && (x <= T(1 + sigma)));
		CU_VERIFIE_CONDITION(controleur, (y >= T(-1 - sigma)) && (y <= T(1 + sigma)));
	}

	/* Verify that generators with the same seed produce the same sequence. */
	rng = std::mt19937(137);
	for (int i(0); i < 100; ++i) {
		CU_VERIFIE_CONDITION(controleur, seq[i] == marsaglia(rng, mean, sigma));
	}

	/* Verify that generators with different seeds produce different sequences. */
	rng = std::mt19937(207);
	for (int i(0); i < 100; ++i) {
		CU_VERIFIE_CONDITION(controleur, seq[i] != marsaglia(rng, mean, sigma));
	}
}

void test_math(dls::test_unitaire::Controleuse &controleur)
{
	test_marsaglia<float>(controleur);
	test_marsaglia<double>(controleur);
}

template <ConceptDecimal real>
void test_statistique_impl(dls::test_unitaire::Controleuse &controleur)
{
	using namespace dls::math;

	{
		const std::vector<real> v = { 1, 2, 3, 4 };

		auto m = moyenne<real>(v.begin(), v.end());
		CU_VERIFIE_EGALITE_DECIMALE(controleur, m, static_cast<real>(2.5));

		auto md = mediane<real>(v.begin(), v.end());
		CU_VERIFIE_EGALITE_DECIMALE(controleur, md, static_cast<real>(2.5));
		CU_VERIFIE_EGALITE_DECIMALE(controleur, md, m);
	}

	{
		const std::vector<real> v = { 1, 1, 1, 1 };

		auto m = moyenne<real>(v.begin(), v.end());
		CU_VERIFIE_EGALITE_DECIMALE(controleur, m, static_cast<real>(1));

		auto md = mediane<real>(v.begin(), v.end());
		CU_VERIFIE_EGALITE_DECIMALE(controleur, md, static_cast<real>(1));
		CU_VERIFIE_EGALITE_DECIMALE(controleur, md, m);

		auto sigma = ecart_type<real>(v.begin(), v.end(), m);
		CU_VERIFIE_EGALITE_DECIMALE(controleur, sigma, static_cast<real>(0));
	}

	{
		const std::vector<real> v = { 1, 2, 3, 4, 5 };

		auto m = moyenne<real>(v.begin(), v.end());
		CU_VERIFIE_EGALITE_DECIMALE(controleur, m, static_cast<real>(3));

		auto md = mediane<real>(v.begin(), v.end());
		CU_VERIFIE_EGALITE_DECIMALE(controleur, md, static_cast<real>(3));
		CU_VERIFIE_EGALITE_DECIMALE(controleur, md, m);
	}
}

void test_statistique(dls::test_unitaire::Controleuse &controleur)
{
	test_statistique_impl<float>(controleur);
	test_statistique_impl<double>(controleur);
}
