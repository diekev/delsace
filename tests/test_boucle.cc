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
				"La plage d'une boucle 'pour' peut avoir des types réguliers");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					pour x dans 'a'...'z' {
					}
					pour x dans 0...10 {
					}
					pour x dans 0.0...10.0 {
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
				"La plage d'une boucle 'pour' ne être de type booléenne");
	{
		const char *texte =
				R"(
				structure Demo {
					demo : z32;
				}

				fonction foo() : rien
				{
					pour x dans vrai...faux {
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
				"La plage d'une boucle 'pour' ne peut avoir des types définis par l'utilisateur");
	{
		const char *texte =
				R"(
				structure Demo {
					demo : z32;
				}

				fonction foo() : rien
				{
					soit variable debut : Demo;
					soit variable fin   : Demo;

					pour x dans debut...fin {
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

	CU_DEBUTE_PROPOSITION(
				controleur,
				"On peut avoir des contrôles de flux dans des boucles 'pour' sans problème");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					pour i dans 0 ... 10 {
						si i > 5 {
							si i > 8 {

							}
							sinon si i < 6 {

							}
							sinon {

							}
						}
						sinon si i < 3 {

						}
						sinon {

						}
					}
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::CONTROLE_INVALIDE);

		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"On peut avoir des contrôles de flux dans des boucles 'boucle' sans problème");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					soit variable i = 0;
					boucle {

						si i > 5 {
							si i > 8 {

							}
							sinon si i < 6 {

							}
							sinon {

							}
						}
						sinon si i < 3 {

						}
						sinon {

						}

						i = i + 1;
					}
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::CONTROLE_INVALIDE);

		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleur);
}

void test_boucle(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	test_plage_pour(controleur);
	test_continue_arrete(controleur);
}
