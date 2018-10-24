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

#include "test_boucle.hh"

#include "erreur.h"
#include "outils.h"

static void test_plage_pour(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	CU_DEBUTE_PROPOSITION(
				controleur,
				"La plage d'une boucle 'pour' avec des types entiers identiques est correcte");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					pour x dans 0...10 {
					}
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"La plage d'une boucle 'pour' doit avoir des types identiques");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					pour x dans 0...10.0 {
					}
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
				"La plage d'une boucle 'pour' doit avoir des types entiers");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					pour x dans 0.0...10.0 {
					}
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::TYPE_DIFFERENTS);

		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);
}

static void test_continue_arrete(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	CU_DEBUTE_PROPOSITION(controleur, "Le mot clé 'continue' peut apparaître dans une boucle");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					boucle {
						continue;
					}
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(controleur, "Le mot clé 'arrête' peut apparaître dans une boucle");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					boucle {
						arrête;
					}
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(controleur, "Le mot clé 'continue' ne peut apparaître hors d'une boucle");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					continue;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::CONTROLE_INVALIDE);

		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(controleur, "Le mot clé 'arrête' ne peut apparaître hors d'une boucle");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					arrête;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::CONTROLE_INVALIDE);

		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);
}

void test_boucle(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	test_plage_pour(controleur);
	test_continue_arrete(controleur);
}
