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

#include "tests_vecteur.hh"

#include "biblinternes/math/vecteur.hh"

static void test_ordre(dls::test_unitaire::Controleuse &controleuse)
{
	using namespace dls::math;

	CU_DEBUTE_PROPOSITION(controleuse, "Les opérateurs de comparaisons sont corrects.");

	vec3<int> v1(0, 1, 2);
	vec3<int> v2(3, 4, 2);
	vec3<int> v3 = v1;
	vec3<int> v4 = v2;

	CU_VERIFIE_CONDITION(controleuse, v1 != v2);
	CU_VERIFIE_CONDITION(controleuse, v1 == v3);
	CU_VERIFIE_CONDITION(controleuse, v1 < v2);
	CU_VERIFIE_CONDITION(controleuse, v2 > v1);
	CU_VERIFIE_CONDITION(controleuse, v3 <= v1);
	CU_VERIFIE_CONDITION(controleuse, v3 >= v1);
	CU_VERIFIE_CONDITION(controleuse, v4 <= v2);
	CU_VERIFIE_CONDITION(controleuse, v4 >= v2);

	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_addition(dls::test_unitaire::Controleuse &controleuse)
{
	using namespace dls::math;

	CU_DEBUTE_PROPOSITION(controleuse, "Les opérateurs d'addition et de soustractions sont corrects.");

	vec3<int> v1(0, 1, 2);
	vec3<int> v2(4, 2, 3);

	CU_VERIFIE_EGALITE(controleuse, (v1 + v2), vec3<int>( 4,  3,  5));
	CU_VERIFIE_EGALITE(controleuse, (v1 - v2), vec3<int>(-4, -1, -1));
	CU_VERIFIE_EGALITE(controleuse, (v2 - v1), vec3<int>( 4,  1,  1));

	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(controleuse, "L'opérateur moins unaire retourne l'inverse du vecteur passé.");

	CU_VERIFIE_EGALITE(controleuse, -v1,  vec3<int>( 0, -1, -2));
	CU_VERIFIE_EGALITE(controleuse, -v2,  vec3<int>(-4, -2, -3));
	CU_VERIFIE_EGALITE(controleuse,  v2, -vec3<int>(-4, -2, -3));

	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_multiplication(dls::test_unitaire::Controleuse &controleuse)
{
	using namespace dls::math;

	CU_DEBUTE_PROPOSITION(controleuse, "Les opérateurs de multiplications et de divisions sont corrects.");

	vec3<float> v1(0.0f, 1.0f, 2.0f);
	vec3<float> v2(3.0f, 4.0f, 5.0f);

	CU_VERIFIE_EGALITE(controleuse, (v1 * v2), vec3<float>(0.0f, 4.0f, 10.0f));
	CU_VERIFIE_EGALITE(controleuse, (v2 * v1), vec3<float>(0.0f, 4.0f, 10.0f));
	CU_VERIFIE_EGALITE(controleuse, (v1 / v2), vec3<float>(0.0f, 0.25f, 0.4f));
	CU_VERIFIE_EGALITE(controleuse, (v2 / v1), vec3<float>(0.0f, 4.0f, 2.5f));

	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_produit_scalaire(dls::test_unitaire::Controleuse &controleuse)
{
	using namespace dls::math;

	CU_DEBUTE_PROPOSITION(controleuse, "Les produits scalaires sont corrects.");

	vec3<float> v1(0.0f, 1.0f, 0.0f);
	vec3<float> v2(1.0f, 0.0f, 0.0f);

	CU_VERIFIE_EGALITE(controleuse, produit_scalaire(v1, v1), 1.0f);
	CU_VERIFIE_EGALITE(controleuse, produit_scalaire(v2, v2), 1.0f);
	CU_VERIFIE_EGALITE(controleuse, produit_scalaire(v2, v1), 0.0f);
	CU_VERIFIE_EGALITE(controleuse, produit_scalaire(v1, v2), 0.0f);

	v1 = vec3<float>(1.0f, 1.0f, 0.0f);

	CU_VERIFIE_EGALITE(controleuse, produit_scalaire(v1, v2), 1.0f);

	v1 = vec3<float>(-1.0f, 0.0f, 0.0f);

	CU_VERIFIE_EGALITE(controleuse, produit_scalaire(v1, v2), -1.0f);

	v1 = vec3<float>(-2.0f, 0.0f, 0.0f);
	v2 = vec3<float>(2.0f, 0.0f, 0.0f);

	/* Vérifie que le produit scalaire soit égal à |v1|*|v2|*cos(theta). */
	CU_VERIFIE_EGALITE(controleuse, produit_scalaire(v1, v2), -4.0f);

	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_produit_croix(dls::test_unitaire::Controleuse &controleuse)
{
	using namespace dls::math;

	CU_DEBUTE_PROPOSITION(controleuse, "Les produits en croix sont corrects.");

	/* Test du produit en croix en vérifie l'aire entre les vecteurs. */

	vec3<float> v1(0.0f, 1.0f, 0.0f);
	vec3<float> v2(1.0f, 0.0f, 0.0f);

	CU_VERIFIE_EGALITE(controleuse, longueur(produit_vectorielle(v1, v2)), 1.0f);

	v1 = vec3<float>(0.0f, 2.0f, 0.0f);
	v2 = vec3<float>(2.0f, 0.0f, 0.0f);

	CU_VERIFIE_EGALITE(controleuse, longueur(produit_vectorielle(v1, v2)), 4.0f);

	v1 = vec3<float>(0.0f, -0.5f, 0.0f);
	v2 = vec3<float>(-0.5f, 0.0f, 0.0f);

	CU_VERIFIE_EGALITE(controleuse, longueur(produit_vectorielle(v1, v2)), 0.25f);

	/* Si les vecteurs sont colinéaires, le produit vectorielle sera égale à 0. */

	CU_VERIFIE_EGALITE(controleuse, longueur(produit_vectorielle(v1, v1)), 0.0f);

	/* Si les vecteurs ne sont pas colinéaires, le vector w issu du produit
	 * vectorielle sera :
	 * - orthogonal à v1 et v2
	 * - sa longueur sera égale à |v1||v2||sin(v1, v2)|
	 */

	v1 = vec3<float>(1.0f, 0.0f, 0.0f);
	v2 = vec3<float>(0.0f, 1.0f, 0.0f);

	const auto w = produit_vectorielle(v1, v2);

	CU_VERIFIE_CONDITION(controleuse, w == vec3<float>(0.0f, 0.0f, 1.0f));
	CU_VERIFIE_EGALITE(controleuse, produit_scalaire(w, v1), 0.0f);
	CU_VERIFIE_EGALITE(controleuse, produit_scalaire(w, v2), 0.0f);
	CU_VERIFIE_EGALITE(controleuse, longueur(w), 1.0f);

	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_fonction_absolu(dls::test_unitaire::Controleuse &controleuse)
{
	using namespace dls::math;

	CU_DEBUTE_PROPOSITION(controleuse, "La fonction absolue est correcte.");

	vec3f v1(-1.0f, -2.0f, -3.0f);

	CU_VERIFIE_EGALITE(controleuse, absolu(v1), vec3f(1.0f, 2.0f, 3.0f));

	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_fonction_exp(dls::test_unitaire::Controleuse &controleuse)
{
	using namespace dls::math;

	CU_DEBUTE_PROPOSITION(controleuse, "La fonction exponentiel est correcte.");

	vec3f v1(1.0f, 2.0f, 3.0f);

	CU_VERIFIE_EGALITE(controleuse, exponentiel(v1), vec3f(std::exp(1.0f), std::exp(2.0f), std::exp(3.0f)));

	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_finitude(dls::test_unitaire::Controleuse &controleuse)
{
	using namespace dls::math;

	CU_DEBUTE_PROPOSITION(controleuse, "Les fonctions de finitudes sont correctes.");

	vec3f v1(-1.0f, -2.0f, -3.0f);

	CU_VERIFIE_CONDITION(controleuse, est_fini(v1));
	CU_VERIFIE_CONDITION(controleuse, !est_infini(v1));

	v1.x = std::numeric_limits<float>::infinity();

	CU_VERIFIE_CONDITION(controleuse, !est_fini(v1));
	CU_VERIFIE_CONDITION(controleuse, est_infini(v1));

	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_swizzler(dls::test_unitaire::Controleuse &controleuse)
{
	using namespace dls::math;

	CU_DEBUTE_PROPOSITION(controleuse, "Le swizzling est correcte.");

	vec2i v(11, 22);

	CU_VERIFIE_EGALITE(controleuse, v.xy.dechoie(), v);
	CU_VERIFIE_EGALITE(controleuse, v.xx.dechoie(), vec2i(11, 11));
	CU_VERIFIE_EGALITE(controleuse, v.yy.dechoie(), vec2i(22, 22));
	CU_VERIFIE_EGALITE(controleuse, v.yx.dechoie(), vec2i(22, 11));

	vec3i v1 = v.xxx;
	CU_VERIFIE_EGALITE(controleuse, v1, vec3i(11));

	v1 = v.yyy;
	CU_VERIFIE_EGALITE(controleuse, v1, vec3i(22));

	CU_TERMINE_PROPOSITION(controleuse);
}

void tests_vecteur(dls::test_unitaire::Controleuse &controleuse)
{
	test_ordre(controleuse);
	test_addition(controleuse);
	test_multiplication(controleuse);
	test_produit_scalaire(controleuse);
	test_produit_croix(controleuse);
	test_fonction_absolu(controleuse);
	test_fonction_exp(controleuse);
	test_finitude(controleuse);
	test_swizzler(controleuse);
}
