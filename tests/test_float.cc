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

#include "tests.hh"

#include "../nombre_decimaux/ordre.h"

void test_float_total(dls::test_unitaire::Controleuse &controleur)
{
	using namespace dls::nombre_decimaux;
	float f0 = 0.0f;
	float f1 = 4.2038954e-045f;  /* *(float *)&(*(int *)&f0 + 3) */

	float fn0 = -0.0f;
	float fn1 = -2.8025969e-045f;  /* *(float *)&(*(int *)&fn0 - 2) */

	CU_VERIFIE_CONDITION(controleur, total_less(f0, f1) == true);
	CU_VERIFIE_CONDITION(controleur, total_less(f1, f0) == false);

	CU_VERIFIE_CONDITION(controleur, total_equal(f0, f1) == false);
	CU_VERIFIE_CONDITION(controleur, total_equal(f1, f0) == false);

	CU_VERIFIE_CONDITION(controleur, total_less(fn0, fn1) == false);
	CU_VERIFIE_CONDITION(controleur, total_less(fn1, fn0) == true);

	CU_VERIFIE_CONDITION(controleur, total_less(f0, f1) == true);
	CU_VERIFIE_CONDITION(controleur, total_less(f1, f0) == false);

	CU_VERIFIE_CONDITION(controleur, total_less(fn0, f0) == true);
	CU_VERIFIE_CONDITION(controleur, total_less(f0, fn0) == false);

	CU_VERIFIE_CONDITION(controleur, total_less(f0, fn1) == false);
	CU_VERIFIE_CONDITION(controleur, total_less(fn1, f0) == true);

	CU_VERIFIE_CONDITION(controleur, total_equal(fn0, f0) == false);
	CU_VERIFIE_CONDITION(controleur, total_equal(f0, fn0) == false);

	CU_VERIFIE_CONDITION(controleur, total_equal(fn0, f1) == false);
	CU_VERIFIE_CONDITION(controleur, total_equal(f1, fn0) == false);
}

void test_float_ULP(dls::test_unitaire::Controleuse &controleur)
{
	using namespace dls::nombre_decimaux;
	float f0 = 0.0f;
	float f1 = 4.2038954e-045f;  /* *(float *)&(*(int *)&f0 + 3) */

	float fn0 = -0.0f;
	float fn1 = -2.8025969e-045f;  /* *(float *)&(*(int *)&fn0 - 2) */

	CU_VERIFIE_CONDITION(controleur, is_ulp_equal(f0, f1, 3) == true);
	CU_VERIFIE_CONDITION(controleur, is_ulp_equal(f1, f0, 3) == true);

	CU_VERIFIE_CONDITION(controleur, is_ulp_equal(f0, f1, 1) == false);
	CU_VERIFIE_CONDITION(controleur, is_ulp_equal(f1, f0, 1) == false);

	CU_VERIFIE_CONDITION(controleur, is_ulp_equal(fn0, fn1, 8) == true);
	CU_VERIFIE_CONDITION(controleur, is_ulp_equal(fn1, fn0, 8) == true);

	CU_VERIFIE_CONDITION(controleur, is_ulp_equal(fn0, f0, 1) == true);
	CU_VERIFIE_CONDITION(controleur, is_ulp_equal(f0, fn0, 1) == true);

	/* REMARQUE : en théorie, cela devrait retourner false puisque 0.0f et -0.0f
	 * ont une différence de 0x80000000, mais un débordement dans la
	 * soustraction semble casser quelque chose ici
	 * (abs(*(int *)&fn0 - *(int *)&f0) == 0x80000000 == fn0), probablement un
	 * int32 ne peut pas contenir cette valeur absolue.
	 *
	 * Ceci est encore une illustration de pourquoi nous ne devrions jamais
	 * utiliser des floats équivalente ou proche de zéro dans des comparaisons
	 * ne se basant uniquement que sur l'ULP. */
	CU_VERIFIE_CONDITION(controleur, is_ulp_equal(fn0, f0, 1024) == true); // échec
	CU_VERIFIE_CONDITION(controleur, is_ulp_equal(f0, fn0, 1024) == true); // échec

	CU_VERIFIE_CONDITION(controleur, is_ulp_equal(fn0, f1, 1024) == true); // échec
	CU_VERIFIE_CONDITION(controleur, is_ulp_equal(f1, fn0, 1024) == true); // échec
}

void test_nombre_decimaux(dls::test_unitaire::Controleuse &controleur)
{
	test_float_ULP(controleur);
	test_float_total(controleur);
}
