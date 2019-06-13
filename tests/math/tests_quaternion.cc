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

#include "tests_quaternion.hh"

#include "math/quaternion.hh"

static void test_addition(dls::test_unitaire::Controleuse &controleuse)
{
	using namespace dls::math;

	CU_DEBUTE_PROPOSITION(controleuse, "Les additions de quaternions sont correctes.");

	quaternion<int> q1(vec3<int>(0, 2, 4), 6);
	quaternion<int> q2(vec3<int>(4, 5, 6), 7);

	CU_VERIFIE_CONDITION(controleuse, (q1 + q2) == quaternion<int>(vec3<int>(4, 7, 10), 13));
	CU_VERIFIE_CONDITION(controleuse, (q1 - q2) == quaternion<int>(vec3<int>(-4, -3, -2), -1));
	CU_VERIFIE_CONDITION(controleuse, (q2 - q1) == quaternion<int>(vec3<int>(4, 3, 2), 1));

	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_multiplication(dls::test_unitaire::Controleuse &controleuse)
{
	using namespace dls::math;

	CU_DEBUTE_PROPOSITION(controleuse, "Les multiplications de quaternions sont correctes.");

	quaternion<float> q1(vec3<float>(0.0f, 1.0f, 2.0f), 3.0f);
	quaternion<float> q2(vec3<float>(4.0f, 5.0f, 6.0f), 7.0f);
	quaternion<float> q3(vec3<float>(4.0f, 5.0f, 6.0f), 7.0f);

	CU_VERIFIE_CONDITION(
				controleuse,
				(q1 * 2.0f) == quaternion<float>(vec3<float>(0.0f, 2.0f, 4.0f), 6.0f));

	CU_VERIFIE_CONDITION(
				controleuse,
				(3.0f * q2) == quaternion<float>(vec3<float>(12.0f, 15.0f, 18.0f), 21.0f));

	CU_VERIFIE_CONDITION(
				controleuse,
				(q1 / 2.0f) == quaternion<float>(vec3<float>(0.0f, 0.5f, 1.0f), 1.5f));

	CU_VERIFIE_CONDITION(
				controleuse,
				(2.0f / q2) == quaternion<float>(vec3<float>(0.5f, 0.4f, 1.0f / 3.0f), 2.0f / 7.0f));

	q1 = quaternion<float>(vec3<float>(3.0f, 0.0f, -1.0f), 0.0f);
	q2 = quaternion<float>(vec3<float>(0.0f, 1.0f, 1.0f), 2.0f);

	CU_VERIFIE_CONDITION(
				controleuse,
				(q1 * q2) == quaternion<float>(vec3<float>(7.0f, -3.0f, 1.0f), 1.0f));

	CU_VERIFIE_CONDITION(
				controleuse,
				(q1 * q2) != (q2 * q1));

	CU_VERIFIE_CONDITION(
				controleuse,
				((q1 * q2) * q3) == (q1 * (q2 * q3)));

	CU_VERIFIE_CONDITION(
				controleuse,
				((q1 + q2) * q3) == (q1 * q3 + q2 * q3));

	CU_VERIFIE_CONDITION(
				controleuse,
				((q1 + q2) * 0.5f) == (q1 * 0.5f + q2 * 0.5f));

	CU_VERIFIE_CONDITION(
				controleuse,
				((q1 * 0.5f) * q2) == ((q1 * q2) * 0.5f));

	CU_VERIFIE_CONDITION(
				controleuse,
				((q1 * 0.5f) * q2) == (q1 * (q2 * 0.5f)));

	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_produit_scalaire(dls::test_unitaire::Controleuse &controleuse)
{
	using namespace dls::math;

	CU_DEBUTE_PROPOSITION(controleuse, "Les produits scalaires de quaternions sont corrects.");

	quaternion<float> q1(vec3<float>(1.0f, 0.0f, 0.0f), 0.0f);
	quaternion<float> q2(vec3<float>(0.0f, 1.0f, 0.0f), 0.0f);

	CU_VERIFIE_EGALITE(controleuse, produit_scalaire(q1, q1), 1.0f);
	CU_VERIFIE_EGALITE(controleuse, produit_scalaire(q2, q2), 1.0f);
	CU_VERIFIE_EGALITE(controleuse, produit_scalaire(q2, q1), 0.0f);
	CU_VERIFIE_EGALITE(controleuse, produit_scalaire(q1, q2), 0.0f);

	q1 = quaternion<float>(vec3<float>(1.0f, 1.0f, 0.0f), 0.0f);

	CU_VERIFIE_EGALITE(controleuse, produit_scalaire(q1, q2), 1.0f);

	q1 = quaternion<float>(vec3<float>(-1.0f, 0.0f, 0.0f), 0.0f);

	CU_VERIFIE_EGALITE(controleuse, produit_scalaire(q1, q2), 0.0f);

	q1 = quaternion<float>(vec3<float>(-2.0f, 0.0f, 0.0f), 0.0f);
	q2 = quaternion<float>(vec3<float>(2.0f, 0.0f, 0.0f), 0.0f);

	/* Vérifie que le produit scalaire soit égal à |q1|*|q2|*cos(theta). */
	CU_VERIFIE_EGALITE(controleuse, produit_scalaire(q1, q2), -4.0f);

	CU_TERMINE_PROPOSITION(controleuse);
}

void tests_quaternion(dls::test_unitaire::Controleuse &controleuse)
{
	test_addition(controleuse);
	test_multiplication(controleuse);
	test_produit_scalaire(controleuse);
}
