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

#include "tests_matrice.hh"

#include "math/matrice.hh"

static void test_matrice_identite(dls::test_unitaire::Controleuse &controleuse)
{
	using namespace dls::math;

	auto A = mat4x4d(1.0);
	auto B = mat4x4d(1.0);

	auto C = A * B;

	CU_DEBUTE_PROPOSITION(controleuse, "Multiplier des matrices par l'indentité ne les change pas.");

	CU_VERIFIE_CONDITION(controleuse, C == mat4x4d(1.0));

	A *= B;

	CU_VERIFIE_CONDITION(controleuse, A == mat4x4d(1.0));

	B = mat4x4d(1.0, 0.0, 0.0, 0.0,
				0.0, 0.1, 0.9, 0.0,
				0.0, -0.9, 0.1, 0.0,
				0.0, 0.0, 0.0, 1.0);

	A *= B;

	CU_VERIFIE_CONDITION(controleuse, A == B);

	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_transpose(dls::test_unitaire::Controleuse &controleuse)
{
	using namespace dls::math;

	CU_DEBUTE_PROPOSITION(controleuse, "Transposer une matrice est correcte pour les matrices carrées.");
	{
		auto A = mat3x3d(1.0, 2.0, 3.0,
						 4.0, 5.0, 6.0,
						 7.0, 8.0, 9.0);

		auto B = mat3x3d(1.0, 4.0, 7.0,
						 2.0, 5.0, 8.0,
						 3.0, 6.0, 9.0);

		CU_VERIFIE_CONDITION(controleuse, transpose(A) == B);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	/* À FAIRE : construction de mat2x3 semble incorrecte. */
//	CU_DEBUTE_PROPOSITION(controleuse, "Transposer une matrice est correcte pour les matrices rectangulaires.");
//	{
//		using mat3x2d = matrice<double, vecteur, paquet_index<0, 1>, paquet_index<0, 1, 2>>;
//		using mat2x3d = matrice<double, vecteur, paquet_index<0, 1, 2>, paquet_index<0, 1>>;

//		auto A = mat3x2d(1.0, 2.0, 4.0,
//						 5.0, 7.0, 8.0);

//		auto B = mat2x3d(1.0, 5.0,
//						 2.0, 7.0,
//						 4.0, 8.0);

//		CU_VERIFIE_CONDITION(controleuse, transpose(A) == B);
//	}
//	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_matrice_inverse(dls::test_unitaire::Controleuse &controleuse)
{
	using namespace dls::math;

	auto A = mat4x4d(1.0, 0.0, 0.0, 0.0,
					 0.0, 1.0, 0.0, 0.0,
					 0.0, 0.0, 1.0, 0.0,
					 0.0, 0.0, 0.0, 1.0);


	auto I = inverse(A);

	CU_VERIFIE_CONDITION(controleuse, I == mat4x4d(1.0));
	CU_VERIFIE_CONDITION(controleuse, A*I == mat4x4d(1.0));

	A = mat4x4d(1.0, 0.0, 0.0, 1.0,
				0.0, 1.0, 0.0, 1.0,
				0.0, 0.0, 1.0, 1.0,
				1.0, 1.0, 1.0, 1.0);


	I = inverse(A);

	CU_VERIFIE_CONDITION(controleuse, A*I == mat4x4d(1.0));

	A = mat4x4d(2.0, 0.0, 0.0, 6.0,
				0.0, 3.0, 0.0, -5.0,
				0.0, 0.0, 4.0, 1.0,
				2.0, 2.0, 2.0, 1.0);


	I = inverse(A);

	CU_VERIFIE_CONDITION(
				controleuse,
				sont_environ_egales(A*I, mat4x4d(1.0)));

	/* Vérifie que les inverses de matrices de rotations soient bien leurs
	 * transposées.
	 */

	/* Rotation axe X. */
	A = mat4x4d(1.0, 0.0, 0.0, 0.0,
				0.0, 0.0, -1.0, 0.0,
				0.0, 1.0, 0.0, 0.0,
				0.0, 0.0, 0.0, 1.0);


	I = inverse(A);

	CU_VERIFIE_CONDITION(
				controleuse,
				sont_environ_egales(I, transpose(A)));

	CU_VERIFIE_CONDITION(
				controleuse,
				sont_environ_egales(A*I, mat4x4d(1.0)));

	/* Rotation axe Y. */
	A = mat4x4d(0.0, 0.0, 1.0, 0.0,
				0.0, 1.0, 0.0, 0.0,
				-1.0, 0.0, 0.0, 0.0,
				0.0, 0.0, 0.0, 1.0);


	I = inverse(A);

	CU_VERIFIE_CONDITION(
				controleuse,
				sont_environ_egales(I, transpose(A)));

	CU_VERIFIE_CONDITION(
				controleuse,
				sont_environ_egales(A*I, mat4x4d(1.0)));

	/* Rotation axe Z. */
	A = mat4x4d(0.0, 1.0, 0.0, 0.0,
				-1.0, 0.0, 0.0, 0.0,
				0.0, 0.0, 1.0, 0.0,
				0.0, 0.0, 0.0, 1.0);


	I = inverse(A);

	CU_VERIFIE_CONDITION(
				controleuse,
				sont_environ_egales(I, transpose(A)));

	CU_VERIFIE_CONDITION(
				controleuse,
				sont_environ_egales(A*I, mat4x4d(1.0)));
}

void tests_matrice(dls::test_unitaire::Controleuse &controleuse)
{
	test_matrice_identite(controleuse);
	test_matrice_inverse(controleuse);
	test_transpose(controleuse);
}
