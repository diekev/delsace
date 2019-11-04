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

#include "biblinternes/math/concepts.hh"
#include "biblinternes/math/statistique.hh"
#include "biblinternes/outils/gna.hh"

template <ConceptDecimal T>
void test_marsaglia(dls::test_unitaire::Controleuse &controleur)
{
	using namespace dls::math;

	auto rng = GNA(137);
	auto v = dls::math::vec2<T>();
	dls::math::vec2<T> seq[100];
	const auto sigma = static_cast<T>(0.5);
	const auto mean  = static_cast<T>(0.2);

	/* Generate a sequence of random number and verify that they all fall in the
	 * [-1, 1] interval, taking the standard deviation into account */
	for (int i(0); i < 100; ++i) {
		v = seq[i] = echantillone_disque_normale(rng, mean, sigma);
		CU_VERIFIE_CONDITION(controleur, (v.x >= T(-1 - sigma)) && (v.x <= T(1 + sigma)));
		CU_VERIFIE_CONDITION(controleur, (v.y >= T(-1 - sigma)) && (v.y <= T(1 + sigma)));
	}

	/* Verify that generators with the same seed produce the same sequence. */
	rng = GNA(137);
	for (int i(0); i < 100; ++i) {
		CU_VERIFIE_CONDITION(controleur, seq[i] == echantillone_disque_normale(rng, mean, sigma));
	}

	/* Verify that generators with different seeds produce different sequences. */
	rng = GNA(157);
	for (int i(0); i < 100; ++i) {
		CU_VERIFIE_CONDITION(controleur, seq[i] != echantillone_disque_normale(rng, mean, sigma));
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
		const dls::tableau<real> v = { 1, 2, 3, 4 };

		auto m = moyenne<real>(v.debut(), v.fin());
		CU_VERIFIE_EGALITE_DECIMALE(controleur, m, static_cast<real>(2.5));

		auto md = mediane<real>(v.debut(), v.fin());
		CU_VERIFIE_EGALITE_DECIMALE(controleur, md, static_cast<real>(2.5));
		CU_VERIFIE_EGALITE_DECIMALE(controleur, md, m);
	}

	{
		const dls::tableau<real> v = { 1, 1, 1, 1 };

		auto m = moyenne<real>(v.debut(), v.fin());
		CU_VERIFIE_EGALITE_DECIMALE(controleur, m, static_cast<real>(1));

		auto md = mediane<real>(v.debut(), v.fin());
		CU_VERIFIE_EGALITE_DECIMALE(controleur, md, static_cast<real>(1));
		CU_VERIFIE_EGALITE_DECIMALE(controleur, md, m);

		auto sigma = ecart_type<real>(v.debut(), v.fin(), m);
		CU_VERIFIE_EGALITE_DECIMALE(controleur, sigma, static_cast<real>(0));
	}

	{
		const dls::tableau<real> v = { 1, 2, 3, 4, 5 };

		auto m = moyenne<real>(v.debut(), v.fin());
		CU_VERIFIE_EGALITE_DECIMALE(controleur, m, static_cast<real>(3));

		auto md = mediane<real>(v.debut(), v.fin());
		CU_VERIFIE_EGALITE_DECIMALE(controleur, md, static_cast<real>(3));
		CU_VERIFIE_EGALITE_DECIMALE(controleur, md, m);
	}
}

void test_statistique(dls::test_unitaire::Controleuse &controleur)
{
	test_statistique_impl<float>(controleur);
	test_statistique_impl<double>(controleur);
}
