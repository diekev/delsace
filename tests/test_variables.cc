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

#include "test_variables.h"

#include "erreur.h"
#include "outils.h"

static void test_variable_redefinie(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une variable ne peut prendre le nom d'un argument de la fonction");
	{
		const char *texte =
				R"(
				fonction principale(compte : n32, arguments : n8) : n32
				{
					soit compte = 0;
					retourne 0;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une variable ne peut prendre le nom d'une autre variable locale");
	{
		const char *texte =
				R"(
				fonction principale()
				{
					soit x = 0;
					soit x = 0;
					retourne 0;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une variable ne peut prendre le nom d'une variable globale");
	{
		const char *texte =
				R"(
				soit constante PI = 3.14159;
				fonction principale()
				{
					soit PI = 3.14159;
					retourne 0;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une variable ne peut prendre le nom d'une valeur énumérée");
	{
		const char *texte =
				R"(
				énum {
					LUMIÈRE_DISTANTE = 0,
				}
				fonction principale()
				{
					soit LUMIÈRE_DISTANTE = 0;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);
}

static void test_variable_indefinie(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une variable indéfinie ne peut être utilisée");
	{
		const char *texte =
				R"(
				fonction principale(compte : n32, arguments : n8) : n32
				{
				soit a = comte;
				retourne 0;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_INCONNUE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);
}

static void test_portee_variable(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	CU_DEBUTE_PROPOSITION(
				controleur,
				"Les paramètres des fonctions peuvent être accéder dans toutes les portées filles de la fonction");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : n32
				{
					soit a = compte;

					si compte == 5 {
						soit b = compte;
					}

					# Dans le code généré, après 'si' nous avons un nouveau bloc
					# donc nous devons vérifier que les variables sont toujours
					# accessibles.
					soit c = compte;

					retourne 0;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_INCONNUE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une variable dans une portée fille ne peut avoir le même nom"
				" qu'une variable dans une portée mère");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : n32
				{
					soit a = compte;

					si compte == 5 {
						soit a = compte;
					}

					retourne 0;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une variable définie après une portée peut avoir le même nom"
				" qu'une variable dans la portée.");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : n32
				{
					soit a = compte;

					si compte == 5 {
						soit b = compte;
					}

					soit b = compte;
					retourne 0;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleur);
}

void test_variables(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	test_variable_redefinie(controleur);
	test_variable_indefinie(controleur);
	test_portee_variable(controleur);
}
