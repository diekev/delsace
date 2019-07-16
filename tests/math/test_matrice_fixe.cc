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

#include "tests_math.hh"

#if 0
#include "biblinternes/math/mat4.h"

static void test_matrice_identite(dls::test_unitaire::Controleuse &controleur)
{
	auto A = dls::math::mat4d::identity();
	auto B = dls::math::mat4d::identity();

	auto C = A * B;

	CU_VERIFIE_CONDITION(controleur, C == dls::math::mat4d::identity());

	A *= B;

	CU_VERIFIE_CONDITION(controleur, A == dls::math::mat4d::identity());

	B = dls::math::mat4d({1.0, 0.0, 0.0, 0.0,
							  0.0, 0.1, 0.9, 0.0,
							  0.0, -0.9, 0.1, 0.0,
							  0.0, 0.0, 0.0, 1.0});

	A *= B;

	CU_VERIFIE_CONDITION(controleur, A == B);
}

static void test_matrice_inverse(dls::test_unitaire::ControleurUnitaire &controleur)
{
	auto A = dls::math::mat4d({1.0, 0.0, 0.0, 0.0,
								   0.0, 1.0, 0.0, 0.0,
								   0.0, 0.0, 1.0, 0.0,
								   0.0, 0.0, 0.0, 1.0});


	auto I = dls::math::inverse(A);

	CU_VERIFIE_CONDITION(controleur, I == dls::math::mat4d::identity());
	CU_VERIFIE_CONDITION(controleur, A*I == dls::math::mat4d::identity());

	A = dls::math::mat4d({1.0, 0.0, 0.0, 1.0,
							  0.0, 1.0, 0.0, 1.0,
							  0.0, 0.0, 1.0, 1.0,
							  1.0, 1.0, 1.0, 1.0});


	I = dls::math::inverse(A);

	CU_VERIFIE_CONDITION(controleur, A*I == dls::math::mat4d::identity());

	A = dls::math::mat4d({2.0, 0.0, 0.0, 6.0,
							  0.0, 3.0, 0.0, -5.0,
							  0.0, 0.0, 4.0, 1.0,
							  2.0, 2.0, 2.0, 1.0});


	I = dls::math::inverse(A);

	CU_VERIFIE_CONDITION(
				controleur,
				sont_environ_egales(A*I, dls::math::mat4d::identity()));

	/* Vérifie que les inverses de matrices de rotations soient bien leurs
	 * transposées.
	 */

	/* Rotation axe X. */
	A = dls::math::mat4d({1.0, 0.0, 0.0, 0.0,
							  0.0, 0.0, -1.0, 0.0,
							  0.0, 1.0, 0.0, 0.0,
							  0.0, 0.0, 0.0, 1.0});


	I = dls::math::inverse(A);

	CU_VERIFIE_CONDITION(
				controleur,
				sont_environ_egales(I, dls::math::transpose(A)));

	CU_VERIFIE_CONDITION(
				controleur,
				sont_environ_egales(A*I, dls::math::mat4d::identity()));

	/* Rotation axe Y. */
	A = dls::math::mat4d({0.0, 0.0, 1.0, 0.0,
							  0.0, 1.0, 0.0, 0.0,
							  -1.0, 0.0, 0.0, 0.0,
							  0.0, 0.0, 0.0, 1.0});


	I = dls::math::inverse(A);

	CU_VERIFIE_CONDITION(
				controleur,
				sont_environ_egales(I, dls::math::transpose(A)));

	CU_VERIFIE_CONDITION(
				controleur,
				sont_environ_egales(A*I, dls::math::mat4d::identity()));

	/* Rotation axe Z. */
	A = dls::math::mat4d({0.0, 1.0, 0.0, 0.0,
							  -1.0, 0.0, 0.0, 0.0,
							  0.0, 0.0, 1.0, 0.0,
							  0.0, 0.0, 0.0, 1.0});


	I = dls::math::inverse(A);

	CU_VERIFIE_CONDITION(
				controleur,
				sont_environ_egales(I, dls::math::transpose(A)));

	CU_VERIFIE_CONDITION(
				controleur,
				sont_environ_egales(A*I, dls::math::mat4d::identity()));
}

void test_matrice_fixe(dls::test_unitaire::ControleurUnitaire &controleur)
{
	test_matrice_identite(controleur);
	test_matrice_inverse(controleur);
}
#endif
