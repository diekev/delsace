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
	/* redéfinition argument fonction */
	{
		const char *texte =
				R"(
				fonction principale(compte : n32, arguments : n8) : n32
				{
					soit compte = 0;
					retourne 0;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
	/* redéfinition variable local */
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

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
	/* redéfinition variable globale */
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

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
	/* redéfinition variable énum */
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

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
}

static void test_variable_indefinie(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	const char *texte =
			R"(
			fonction principale(compte : n32, arguments : n8) : n32
			{
				soit a = comte;
				retourne 0;
			}
			)";

	const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_INCONNUE);
	CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
}

void test_variables(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	test_variable_redefinie(controleur);
	test_variable_indefinie(controleur);
}
