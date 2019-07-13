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

#include "biblinternes/math/matrice/matrice.hh"
#include "biblinternes/math/matrice/operations.hh"

static void test_dimensions(dls::test_unitaire::Controleuse &controleur)
{
	using namespace dls::math;

	matrice_dyn<int> mat1({ { 0, 1, 2 }, { 3, 4, 5 } });

	CU_VERIFIE_CONDITION(controleur, mat1.dimensions() == Dimensions(Hauteur(2), Largeur(3)));

	matrice_dyn<int> mat2({ { 0, 1 }, { 2, 3 }, { 4, 5 } });

	CU_VERIFIE_CONDITION(controleur, mat2.dimensions() == Dimensions(Hauteur(3), Largeur(2)));

	CU_VERIFIE_CONDITION(controleur, mat1 != mat2);

	mat1.redimensionne(Dimensions(Hauteur(3), Largeur(2)));

	CU_VERIFIE_CONDITION(controleur, mat1 == mat2);

	mat1.redimensionne(Dimensions(Hauteur(2), Largeur(3)));

	auto redimension_exception = [&]()
	{
		mat1.redimensionne(Dimensions(Hauteur(3), Largeur(3)));
	};

	CU_VERIFIE_EXCEPTION_JETEE(controleur, redimension_exception);
}

static void test_identite(dls::test_unitaire::Controleuse &controleur)
{
	using namespace dls::math;

	matrice_dyn<int> mat1 = matrice_dyn<int>::identite(Dimensions(Hauteur(2), Largeur(2)));
	matrice_dyn<int> mat2({ { 1, 0 }, { 0, 1 } });

	CU_VERIFIE_CONDITION(controleur, mat1 == mat2);

	mat1 = matrice_dyn<int>::identite(Dimensions(Hauteur(3), Largeur(3)));
	mat2 = matrice_dyn<int>({ { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } });

	CU_VERIFIE_CONDITION(controleur, mat1 == mat2);

	auto exception_identite = [&]()
	{
		mat1 = matrice_dyn<int>::identite(Dimensions(Hauteur(3), Largeur(2)));
	};

	CU_VERIFIE_EXCEPTION_JETEE(controleur, exception_identite);
}

static void test_addition(dls::test_unitaire::Controleuse &controleur)
{
	using namespace dls::math;

	matrice_dyn<int> mat1({ { 1, 3 }, { 1, 0 }, { 1, 2 } });
	matrice_dyn<int> mat2({ { 0, 0 }, { 7, 5 }, { 2, 1 } });

	/* Résultat de mat1 + mat2 */
	matrice_dyn<int> res_attendu({ { 1, 3 }, { 8, 5 }, { 3, 3 } });

	auto res = mat1 + mat2;

	CU_VERIFIE_CONDITION(controleur, res == res_attendu);

	auto exception_addition = [&]()
	{
		auto m1 = matrice_dyn<int>(Dimensions(Hauteur(3), Largeur(3)));
		auto m2 = matrice_dyn<int>(Dimensions(Hauteur(2), Largeur(2)));
		m1 += m2;
	};

	CU_VERIFIE_EXCEPTION_JETEE(controleur, exception_addition);
}

static void test_multiplication(dls::test_unitaire::Controleuse &controleur)
{
	using namespace dls::math;

	matrice_dyn<int> mat1({ { 1, 0 }, { -1, 3 } });
	matrice_dyn<int> mat2({ { 3, 1 }, {  2, 1 } });

	/* Résultat de mat1 * mat2 */
	matrice_dyn<int> res({ { 3, 1 }, {  3, 2 } });

	auto res1 = mat1 * mat2;

	CU_VERIFIE_CONDITION(controleur, res == res1);

	/* Test commutativité 1 */

	mat1 = matrice_dyn<int>({ { 1, 2, 0 }, { 4, 3, -1 } });
	mat2 = matrice_dyn<int>({ { 5, 1 }, { 2, 3 }, { 3, 4 } });

	/* Résultat de mat1 * mat2. */
	matrice_dyn<int> res_m1m2_attendu({ { 9, 7 }, { 23, 9 } });

	/* Résultat de mat2 * mat1. */
	matrice_dyn<int> res_m2m1_attendu({ { 9, 13, -1 }, { 14, 13, -3 }, { 19, 18, -4 } });

	auto res_m1m2 = mat1 * mat2;
	auto res_m2m1 = mat2 * mat1;

	CU_VERIFIE_CONDITION(controleur, res_m1m2 == res_m1m2_attendu);
	CU_VERIFIE_CONDITION(controleur, res_m2m1 == res_m2m1_attendu);

	/* Simplement au cas où. */
	CU_VERIFIE_CONDITION(controleur, res_m2m1 != res_m1m2);

	mat1 = matrice_dyn<int>({ { 3, 4, 2 } });
	mat2 = matrice_dyn<int>({ { 13, 9, 7, 15 }, { 8, 7, 4, 6 }, { 6, 4, 0, 3 } });

	/* Résultat de mat1 * mat2. */
	res_m1m2_attendu = matrice_dyn<int>({ { 83, 63, 37, 75 } });

	res_m1m2 = mat1 * mat2;

	CU_VERIFIE_CONDITION(controleur, res_m1m2 == res_m1m2_attendu);

	auto exception_multiplication = [&]()
	{
		auto m1 = matrice_dyn<int>(Dimensions(Hauteur(3), Largeur(3)));
		auto m2 = matrice_dyn<int>(Dimensions(Hauteur(2), Largeur(2)));
		m1 *= m2;
	};

	CU_VERIFIE_EXCEPTION_JETEE(controleur, exception_multiplication);
}

static void test_hadamard(dls::test_unitaire::Controleuse &controleur)
{
	using namespace dls::math;

	matrice_dyn<int> mat1({ { 1, 3, 2 }, { 1, 0, 0 }, { 1, 2, 2 } });
	matrice_dyn<int> mat2({ { 0, 0, 2 }, { 7, 5, 0 }, { 2, 1, 1 } });

	/* Résultat de mat1 * mat2 */
	matrice_dyn<int> res_attendu({ { 0, 0, 4 }, { 7, 0, 0 }, { 2, 2, 2 } });

	auto res = hadamard(mat1, mat2);

	CU_VERIFIE_CONDITION(controleur, res == res_attendu);

	auto exception_hadamard = [&]()
	{
		auto m1 = matrice_dyn<int>(Dimensions(Hauteur(3), Largeur(3)));
		auto m2 = matrice_dyn<int>(Dimensions(Hauteur(2), Largeur(2)));
		return hadamard(m1, m2);
	};

	CU_VERIFIE_EXCEPTION_JETEE(controleur, exception_hadamard);
}

static void test_kronecker(dls::test_unitaire::Controleuse &controleur)
{
	using namespace dls::math;

	matrice_dyn<int> mat1({ { 1, 2 }, { 3, 1 } });
	matrice_dyn<int> mat2({ { 0, 3 }, { 2, 1 } });

	matrice_dyn<int> res_attendu({
			{ 0, 3, 0, 6 },
			{ 2, 1, 4, 2 },
			{ 0, 9, 0, 3 },
			{ 6, 3, 2, 1 }
	});

	auto res = kronecker(mat1, mat2);

	CU_VERIFIE_CONDITION(controleur, res == res_attendu);

	mat1 = matrice_dyn<int>({ { 1, 3, 2 }, { 1, 0, 0 }, { 1, 2, 2 } });
	mat2 = matrice_dyn<int>({ { 0, 5 }, { 5, 0 }, { 1, 1 } });

	res_attendu = matrice_dyn<int>({
			{  0,  5,  0, 15,  0, 10 },
			{  5,  0, 15,  0, 10,  0 },
			{  1,  1,  3,  3,  2,  2 },
			{  0,  5,  0,  0,  0,  0 },
			{  5,  0,  0,  0,  0,  0 },
			{  1,  1,  0,  0,  0,  0 },
			{  0,  5,  0, 10,  0, 10 },
			{  5,  0, 10,  0, 10,  0 },
			{  1,  1,  2,  2,  2,  2 }
	});

	res = kronecker(mat1, mat2);

	CU_VERIFIE_CONDITION(controleur, res == res_attendu);
}

static void test_concatenation(dls::test_unitaire::Controleuse &controleur)
{
	using namespace dls::math;

	matrice_dyn<int> mat1({ { 1, 2 }, { 3, 1 } });
	matrice_dyn<int> mat2({ { 0, 3 }, { 2, 1 } });

	matrice_dyn<int> res_attendu({
			{ 1, 2, 0, 3 },
			{ 3, 1, 2, 1 },
	});

	auto res = contenation_horizontale(mat1, mat2);

	CU_VERIFIE_CONDITION(controleur, res == res_attendu);

	auto exception_horizontale = [&]()
	{
		auto m1 = matrice_dyn<int>(Dimensions(Hauteur(3), Largeur(3)));
		auto m2 = matrice_dyn<int>(Dimensions(Hauteur(2), Largeur(2)));
		return contenation_horizontale(m1, m2);
	};

	CU_VERIFIE_EXCEPTION_JETEE(controleur, exception_horizontale);

	res = contenation_veritcale(mat1, mat2);

	res_attendu = matrice_dyn<int>({
			{ 1, 2 },
			{ 3, 1 },
			{ 0, 3 },
			{ 2, 1 },
	});

	CU_VERIFIE_CONDITION(controleur, res == res_attendu);

	auto exception_verticale = [&]()
	{
		auto m1 = matrice_dyn<int>(Dimensions(Hauteur(3), Largeur(3)));
		auto m2 = matrice_dyn<int>(Dimensions(Hauteur(2), Largeur(2)));
		return contenation_veritcale(m1, m2);
	};

	CU_VERIFIE_EXCEPTION_JETEE(controleur, exception_verticale);
}

static void test_transpose(dls::test_unitaire::Controleuse &controleur)
{
	using namespace dls::math;

	matrice_dyn<int> mat1({ { 0, 1, 2, 3 }, { 4, 5, 6, 7 }, { 8, 9, 10, 11 } });

	matrice_dyn<int> res_attendu({
			{ 0, 4, 8 },
			{ 1, 5, 9 },
			{ 2, 6, 10 },
			{ 3, 7, 11 },
	});

	auto res = transpose(mat1);

	CU_VERIFIE_CONDITION(controleur, res == res_attendu);
}

static void test_maximum_local(dls::test_unitaire::Controleuse &controleur)
{
	using namespace dls::math;

	matrice_dyn<int> mat1({
			{  0,  1,  2,  3 },
			{  4,  5,  6,  7 },
			{  8,  9, 10, 11 },
			{ 12, 13, 14, 15 }
	});

	matrice_dyn<int> res_attendu({
			{  5,  7 },
			{ 13, 15 },
	});

	auto res = maximum_local(mat1, 2);

	CU_VERIFIE_CONDITION(controleur, res == res_attendu);

	mat1 = matrice_dyn<int>({
			{  0,  1,  2,  3,  4,  5 },
			{  6,  7,  8,  9, 10, 11 },
			{ 12, 13, 14, 15, 16, 17 },
			{ 18, 19, 20, 21, 22, 23 },
			{ 24, 25, 26, 27, 28, 29 },
			{ 30, 31, 32, 33, 34, 35 }
	});

	res_attendu = matrice_dyn<int>({
			{  7,  9, 11 },
			{ 19, 21, 23 },
			{ 31, 33, 35 },
	});

	res = maximum_local(mat1, 2);

	CU_VERIFIE_CONDITION(controleur, res == res_attendu);

	res_attendu = matrice_dyn<int>({
			{ 14, 17 },
			{ 32, 35 }
	});

	res = maximum_local(mat1, 3);

	CU_VERIFIE_CONDITION(controleur, res == res_attendu);
}

static void test_minimum_local(dls::test_unitaire::Controleuse &controleur)
{
	using namespace dls::math;

	matrice_dyn<int> mat1({
			{  0,  1,  2,  3 },
			{  4,  5,  6,  7 },
			{  8,  9, 10, 11 },
			{ 12, 13, 14, 15 }
	});

	matrice_dyn<int> res_attendu({
			{  0,  2 },
			{  8, 10 },
	});

	auto res = minimum_local(mat1, 2);

	CU_VERIFIE_CONDITION(controleur, res == res_attendu);

	mat1 = matrice_dyn<int>({
			{  0,  1,  2,  3,  4,  5 },
			{  6,  7,  8,  9, 10, 11 },
			{ 12, 13, 14, 15, 16, 17 },
			{ 18, 19, 20, 21, 22, 23 },
			{ 24, 25, 26, 27, 28, 29 },
			{ 30, 31, 32, 33, 34, 35 }
	});

	res_attendu = matrice_dyn<int>({
			{  0,  2,  4 },
			{ 12, 14, 16 },
			{ 24, 26, 28 },
	});

	res = minimum_local(mat1, 2);

	CU_VERIFIE_CONDITION(controleur, res == res_attendu);

	res_attendu = matrice_dyn<int>({
			{  0,  3 },
			{ 18, 21 }
	});

	res = minimum_local(mat1, 3);

	CU_VERIFIE_CONDITION(controleur, res == res_attendu);
}

void test_matrice(dls::test_unitaire::Controleuse &controleur)
{
	test_dimensions(controleur);
	test_identite(controleur);
	test_addition(controleur);
	test_multiplication(controleur);
	test_hadamard(controleur);
	test_kronecker(controleur);
	test_concatenation(controleur);
	test_transpose(controleur);
	test_maximum_local(controleur);
	test_minimum_local(controleur);
}
