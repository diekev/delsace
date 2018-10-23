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

#include "test_assignation.hh"

#include "decoupage/erreur.h"

#include "outils.h"

void test_assignation(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une variable 'variable' peut être réassignée");
	{
		const char *texte =
				R"(
				fonction foo()
				{
					soit variable a = 5;
					a = 6;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::ASSIGNATION_INVALIDE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une variable constante ne peut être réassignée");
	{
		const char *texte =
				R"(
				fonction foo()
				{
					soit a = 5;
					a = 6;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::ASSIGNATION_INVALIDE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"L'addresse d'une variable ne peut être assignée");
	{
		const char *texte =
				R"(
				fonction foo()
				{
					soit variable a = 5;
					@a = 6;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::ASSIGNATION_INVALIDE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"On peut assigner une valeur à un élément d'un tableau");
	{
		const char *texte =
				R"(
				fonction foo(variable a : [2]z32)
				{
					a[0] = 5;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::ASSIGNATION_INVALIDE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"On ne peut pas assigner une valeur à un élément d'un tableau"
				" d'un type différent");
	{
		const char *texte =
				R"(
				fonction foo(variable a : [2]z32)
				{
					a = 6;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::ASSIGNATION_MAUVAIS_TYPE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"On ne peut assigner une valeur scalaire à une variable de type"
				" tableau");
	{
		const char *texte =
				R"(
				fonction foo(a : [2]n32)
				{
					a = 6;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::ASSIGNATION_INVALIDE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"On peut réassigner une valeur d'un type différent à une"
				" variable");
	{
		const char *texte =
				R"(
				fonction foo()
				{
					soit variable a = 5.0;
					a = 6;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::ASSIGNATION_MAUVAIS_TYPE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"On ne peut avoir une assignation dans une expression droite");
	{
		const char *texte =
				R"(
				fonction foo()
				{
					soit a = 5.0;
					soit b = a = 6;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::ASSIGNATION_INVALIDE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"On peut assigner une valeur à un membre d'une structure");
	{
		const char *texte =
				R"(
				structure Vecteur3D {
					x : z32;
				}
				fonction foo(variable v : Vecteur3D)
				{
					x de v = 5;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::ASSIGNATION_INVALIDE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"On peut assigner une valeur à un élément d'un membre de type"
				" tableau d'une structure");
	{
		const char *texte =
				R"(
				structure Vecteur3D {
					x : [2]z32;
				}
				fonction foo(variable v : Vecteur3D)
				{
					x[0] de v = 5;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::ASSIGNATION_INVALIDE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"On ne peut pas assigner une valeur scalaire à un membre de type"
				" tableau d'une structure");
	{
		const char *texte =
				R"(
				structure Vecteur3D {
					x : [2]n32;
				}
				fonction foo(variable v : Vecteur3D)
				{
					x de v = 5;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::ASSIGNATION_MAUVAIS_TYPE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);
}
