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

#include "test_structures.hh"

#include "outils.h"

static void test_structure_redefinie(dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une structure doit avoir un nom unique.");
	{
		const char *texte =
				R"(
				struct Vecteur2D {
					x : n32;
					y : n32;
				}
				struct Vecteur3D {
					x : n32;
					y : n32;
					z : n32;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::STRUCTURE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une structure ne peut avoir le nom d'une autre structure.");
	{
		const char *texte =
				R"(
				struct Vecteur2D {
					x : n32;
					y : n32;
				}
				struct Vecteur2D {
					x : n32;
					y : n32;
					z : n32;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::STRUCTURE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_structure_inconnue(dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une structure peut avoir une autre structure comme membre.");
	{
		const char *texte =
				R"(
				struct Vecteur2D {
					x : n32;
					y : n32;
				}
				struct Vecteur3D {
					xy : Vecteur2D;
					z : n32;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::STRUCTURE_INCONNUE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une structure ne peut avoir une structure inconnue comme membre.");
	{
		const char *texte =
				R"(
				struct Vecteur3D {
					xy : Vecteur2D;
					z : n32;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::STRUCTURE_INCONNUE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une fonction ne peut avoir une structure inconnue comme argument.");
	{
		const char *texte =
				R"(
				fonc foo(v : Vecteur3D) : rien
				{
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::STRUCTURE_INCONNUE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_acces_membre(dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On ne peut accèder qu'aux membres connus des structures.");
	{
		const char *texte =
				R"(
				struct Vecteur2D {
					x : n32;
					y : n32;
				}
				fonc accès_x(v : Vecteur2D) : n32
				{
					retourne x de v;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::MEMBRE_INCONNU);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On ne peut pas accèder aux membres inconnus des structures.");
	{
		const char *texte =
				R"(
				struct Vecteur2D {
					x : n32;
					y : n32;
				}
				fonc accès_x(v : Vecteur2D) : rien
				{
					retourne z de v;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::MEMBRE_INCONNU);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On ne peut pas accèder aux membres d'une variable inconnue.");
	{
		const char *texte =
				R"(
				struct Vecteur2D {
					x : n32;
					y : n32;
				}
				fonc accès_x(v : Vecteur2D) : rien
				{
					retourne x de w;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_INCONNUE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On ne peut pas accèder aux membres d'une variable qui n'est pas une structure.");
	{
		const char *texte =
				R"(
				fonc accès_x() : rien
				{
					soit a = 0;
					soit b = x de a;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::TYPE_DIFFERENTS);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_membre_unique(dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Les membres des structures ne peut avoir les mêmes noms.");
	{
		const char *texte =
				R"(
				struct Vecteur2D {
					x : n32;
					x : n32;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::MEMBRE_REDEFINI);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_expression_defaut(dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Il est possible de fournir des valeurs par défaut aux membres des structures.");
	{
		const char *texte =
				R"(
				struct Vecteur2D {
					x : n32 = 0;
					y : n32 = 1;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_construction(dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Il est possible de construire les structures dans une expression en fournisant le nom des variables.");
	{
		const char *texte =
				R"(
				struct Vecteur2D {
					x : n32;
					y : n32;
				}

				fonc foo() : rien
				{
					soit v = Vecteur2D{x = 0, y = 1};
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR, false);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Le constructeur des structures doit avoir le nom de ces membres.");
	{
		const char *texte =
				R"(
				struct Vecteur2D {
					x : n32;
					y : n32;
				}

				fonc foo() : rien
				{
					soit v = Vecteur2D{0, 1};
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::MEMBRE_INCONNU);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

void test_structures(dls::test_unitaire::Controleuse &controleuse)
{
	test_structure_redefinie(controleuse);
	test_structure_inconnue(controleuse);
	test_acces_membre(controleuse);
	test_membre_unique(controleuse);
	test_expression_defaut(controleuse);
	test_construction(controleuse);
}
