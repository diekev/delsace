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

static void test_plage_pour(dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
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

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
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

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::TYPE_DIFFERENTS);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
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

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::TYPE_DIFFERENTS);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"La plage d'une boucle 'pour' ne peut être de type booléenne");
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

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::TYPE_DIFFERENTS);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"La plage d'une boucle 'pour' ne peut avoir des types définis par l'utilisateur");
	{
		const char *texte =
				R"(
				structure Demo {
					demo : z32;
				}

				fonction foo() : rien
				{
					dyn debut : Demo;
					dyn fin   : Demo;

					pour x dans debut...fin {
					}
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::TYPE_DIFFERENTS);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_continue_arrete(dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(controleuse, "Le mot clé 'continue' peut apparaître dans une boucle");
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

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(controleuse, "Le mot clé 'arrête' peut apparaître dans une boucle");
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

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Il est possible d'associer la variable d'une boucle 'pour' à un contrôle 'arrête' ou 'continue'");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					pour i dans 0 ... 10 {
						arrête i;
					}
					pour i dans 0 ... 10 {
						continue i;
					}
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une boucle 'boucle' ne peut avoir une variable associée au contrôle 'arrête' ou 'continue'");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					boucle {
						continue a;
					}
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::VARIABLE_INCONNUE);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"La variable d'un contrôle 'arrête' ou 'continue' doit être connue");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					pour i dans 0 ... 10 {
						arrête j;
					}
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::VARIABLE_INCONNUE);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"La variable d'un contrôle 'arrête' ou 'continue' doit être celle d'une boucle");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					soit a = 5;

					pour i dans 0 ... 10 {
						arrête a;
					}
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::VARIABLE_INCONNUE);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(controleuse, "Le mot clé 'continue' ne peut apparaître hors d'une boucle");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					continue;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::CONTROLE_INVALIDE);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(controleuse, "Le mot clé 'arrête' ne peut apparaître hors d'une boucle");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					arrête;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::CONTROLE_INVALIDE);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
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

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::CONTROLE_INVALIDE);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut avoir des contrôles de flux dans des boucles 'boucle' sans problème");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					dyn i = 0;
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

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::CONTROLE_INVALIDE);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

void test_boucle(dls::test_unitaire::Controleuse &controleuse)
{
	test_plage_pour(controleuse);
	test_continue_arrete(controleuse);
}
