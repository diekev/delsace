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

#include "test_retour.hh"

#include "erreur.h"
#include "outils.h"

void test_retour(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une fonction dont le type de retour est 'rien' peut ommettre"
				" une instruction de retour");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::TYPE_DIFFERENTS);

		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une fonction dont le type de retour est 'rien' peut ommettre"
				" une instruction de retour même après une branche");
	{
		/* NOTE : ce genre de cas causait un crash à cause de la manière dont on
		 * vérifiait la dernière instruction, donc on le test. */
		const char *texte =
				R"(
				fonction foo() : rien
				{
					pour i dans 0 ... 10 {
					}
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::TYPE_DIFFERENTS);

		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une fonction dont le type de retour est 'rien' ne peut pas "
				"retourner de valeur");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					retourne 0;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::TYPE_DIFFERENTS);

		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une fonction dont le type de retour est de différent 'rien' ne"
				" peut ommettre une instruction de retour");
	{
		const char *texte =
				R"(
				fonction foo() : z32
				{
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::TYPE_DIFFERENTS);

		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une fonction dont le type de retour est égal au type de son "
				"instruction de retour est correcte");
	{
		const char *texte =
				R"(
				fonction foo() : z32
				{
					retourne 0;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::TYPE_DIFFERENTS);

		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une fonction ne peut avoir des types de retour et "
				"d'instructions de retour différents");
	{
		const char *texte =
				R"(
				fonction foo() : z32
				{
					retourne 0.0;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::TYPE_DIFFERENTS);

		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"Une fonction ne peut avoir des types de retour et "
				"d'instructions de retour différents");
	{
		const char *texte =
				R"(
				fonction foo() : z32
				{
					retourne 0.0;
					soit a = 5;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::NORMAL);

		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);
}
