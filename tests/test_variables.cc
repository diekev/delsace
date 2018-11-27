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

static void test_variable_redefinie(dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
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

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une variable ne peut prendre le nom d'une autre variable locale");
	{
		const char *texte =
				R"(
				fonction principale() : z32
				{
					soit x = 0;
					soit x = 0;
					retourne 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une variable ne peut prendre le nom d'une variable globale");
	{
		const char *texte =
				R"(
				soit PI = 3.14159;
				fonction principale() : z32
				{
					soit PI = 3.14159;
					retourne 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une variable ne peut prendre le nom d'une valeur énumérée");
	{
		const char *texte =
				R"(
				énum {
					LUMIÈRE_DISTANTE = 0,
				}
				fonction principale() : rien
				{
					soit LUMIÈRE_DISTANTE = 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_variable_indefinie(dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une variable indéfinie ne peut être utilisée");
	{
		const char *texte =
				R"(
				fonction principale(compte : n32, arguments : n8) : z32
				{
				soit a = comte;
				retourne 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_INCONNUE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_portee_variable(dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Les paramètres des fonctions peuvent être accéder dans toutes les portées filles de la fonction");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : z32
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

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_INCONNUE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une variable dans une portée fille ne peut avoir le même nom"
				" qu'une variable dans une portée mère");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : z32
				{
					soit a = compte;

					si compte == 5 {
						soit a = compte;
					}

					retourne 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une variable définie après une portée peut avoir le même nom"
				" qu'une variable dans la portée.");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : z32
				{
					soit a = compte;

					si compte == 5 {
						soit b = compte;
					}

					soit b = compte;
					retourne 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une variable définie avant un contrôle de flux peut être utilisée en dedans.");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : z32
				{
					soit a = compte;

					si compte == 5 {
						soit b = a;
					}

					retourne 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une variable définie avant un contrôle de flux peut être utilisée après lui.");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : z32
				{
					soit a = compte;

					si compte == 5 {
						soit b = a;
					}

					soit b = a + compte;

					retourne 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une variable définie avant une boucle 'pour' peut être utilisée en dedans.");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : z32
				{
					soit a = compte;

					pour i dans 0...10 {
						soit b = a;
					}

					retourne 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une variable définie avant une boucle 'pour' peut être utilisée après.");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : z32
				{
					soit a = compte;

					pour i dans 0...10 {
						soit b = a + i;
					}

					soit b = a;

					retourne 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"La variable itérable d'une boucle 'pour' ne peut avoir le même nom qu'une variable prédéfinie.");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : z32
				{
					soit a = compte;

					pour a dans 0...10 {
						soit b = a;
					}

					retourne 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"La variable itérable d'une boucle 'pour' ne peut être utilisée après la boucle.");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : z32
				{
					pour i dans 0...10 {
						soit a = i;
					}

					soit a = i;

					retourne 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_INCONNUE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Plusieurs boucles 'pour' de même portée racine peuvent avoir des variables de même nom.");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : z32
				{
					pour i dans 0...10 {
						soit a = i;
					}

					pour i dans 0...10 {
						soit a = i;
					}

					pour i dans 0...10 {
						soit a = i;
					}

					retourne 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut définir sans problème des variables entre des boucles 'pour'.");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : z32
				{
					soit a = compte;

					pour i dans 0...10 {
						soit ai = i;
					}

					soit b = compte;
					soit ba = a;

					pour i dans 0...10 {
						soit ai = i;
					}

					soit c = compte;
					soit ca = b;
					soit cb = a;
					soit cba = ba;

					pour i dans 0...10 {
						soit ai = i;
					}

					soit d = compte;
					soit da = b;
					soit db = a;
					soit dba = ba;
					soit dca = cb;
					soit dcb = ca;
					soit dcba = cba;

					retourne 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut définir sans problème des variables entre des contrôles de flux.");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : z32
				{
					soit a = compte;

					si a == 10 {
						soit ai = a;
					}
					sinon {
						soit ai = a;
					}

					soit b = compte;
					soit ba = a;

					si b == 10 {
						soit bi = b;
					}
					sinon {
						soit bi = a;
					}

					soit c = compte;
					soit ca = b;
					soit cb = a;
					soit cba = ba;

					si c == 10 {
						soit ci = c;
					}
					sinon {
						soit ci = a;
					}

					soit d = compte;
					soit da = b;
					soit db = a;
					soit dba = ba;
					soit dca = cb;
					soit dcb = ca;
					soit dcba = cba;

					retourne 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut réassigner une variable définie avant un contrôle de flux.");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : z32
				{
					dyn a = compte;

					si a == 10 {
						soit ai = a;
					}
					sinon {
						soit ai = a;
					}

					a = 5;

					retourne 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}

	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut réassigner une variable définie avant une boucle 'pour'.");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : z32
				{
					dyn a = compte;

					pour x dans 0...10 {

					}

					a = 5;

					retourne 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut réassigner une variable définie avant une boucle 'infinie'.");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : z32
				{
					dyn a = compte;

					boucle {

					}

					a = 5;

					retourne 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut retourner une variable définie avant un contrôle de flux.");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : z32
				{
					soit a = compte;

					si a == 10 {
						soit ai = a;
					}
					sinon {
						soit ai = a;
					}

					retourne a;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}

	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut retourner une variable définie avant une boucle 'pour'.");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : z32
				{
					soit a = compte;

					pour x dans 0...10 {

					}

					retourne a;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut retourner une variable définie avant une boucle 'infinie'.");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : n8) : z32
				{
					soit a = compte;

					boucle {

					}

					retourne a;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::VARIABLE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

void test_variables(dls::test_unitaire::Controleuse &controleuse)
{
	test_variable_redefinie(controleuse);
	test_variable_indefinie(controleuse);
	test_portee_variable(controleuse);
}
