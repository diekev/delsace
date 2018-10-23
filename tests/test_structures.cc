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

#include "decoupage/erreur.h"

#include "outils.h"

static void test_structure_redefinie(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une structure doit avoir un nom unique.");
	{
		const char *texte =
				R"(
				structure Vecteur2D {
					x : n32;
					y : n32;
				}
				structure Vecteur3D {
					x : n32;
					y : n32;
					z : n32;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::STRUCTURE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une structure ne peut avoir le nom d'une autre structure.");
	{
		const char *texte =
				R"(
				structure Vecteur2D {
					x : n32;
					y : n32;
				}
				structure Vecteur2D {
					x : n32;
					y : n32;
					z : n32;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::STRUCTURE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);
}

static void test_structure_inconnue(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une structure peut avoir une autre structure comme membre.");
	{
		const char *texte =
				R"(
				structure Vecteur2D {
					x : n32;
					y : n32;
				}
				structure Vecteur3D {
					xy : Vecteur2D;
					z : n32;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::STRUCTURE_INCONNUE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une structure ne peut avoir une structure inconnue comme membre.");
	{
		const char *texte =
				R"(
				structure Vecteur3D {
					xy : Vecteur2D;
					z : n32;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::STRUCTURE_INCONNUE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une fonction ne peut avoir une structure inconnue comme argument.");
	{
		const char *texte =
				R"(
				fonction foo(v : Vecteur3D) : rien
				{
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::STRUCTURE_INCONNUE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);
}

static void test_acces_membre(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	CU_DEBUTE_PROPOSITION(
				controleur,
				"On ne peut accèder qu'aux membres connus des structures.");
	{
		const char *texte =
				R"(
				structure Vecteur2D {
					x : n32;
					y : n32;
				}
				fonction accès_x(v : Vecteur2D) : rien
				{
					retourne x de v;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::MEMBRE_INCONNU);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"On ne peut pas accèder aux membres inconnus des structures.");
	{
		const char *texte =
				R"(
				structure Vecteur2D {
					x : n32;
					y : n32;
				}
				fonction accès_x(v : Vecteur2D) : rien
				{
					retourne z de v;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::MEMBRE_INCONNU);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"On ne peut pas accèder aux membres d'une variable inconnue.");
	{
		const char *texte =
				R"(
				structure Vecteur2D {
					x : n32;
					y : n32;
				}
				fonction accès_x(v : Vecteur2D) : rien
				{
					retourne x de w;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_INCONNUE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);
}

static void test_membre_unique(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	CU_DEBUTE_PROPOSITION(
				controleur,
				"Les membres des structures ne peut avoir les mêmes noms.");
	{
		const char *texte =
				R"(
				structure Vecteur2D {
					x : n32;
					x : n32;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::MEMBRE_REDEFINI);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);
}

void test_structures(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	test_structure_redefinie(controleur);
	test_structure_inconnue(controleur);
	test_acces_membre(controleur);
	test_membre_unique(controleur);
}
