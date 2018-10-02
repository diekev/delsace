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

#include "test_fonctions.h"

#include <cstring>

#include "analyseuse_grammaire.h"
#include "decoupeuse.h"
#include "erreur.h"

static bool retourne_erreur_lancee(const char *texte, const bool imprime_message)
{
	auto tampon = TamponSource(texte);

	try {
		decoupeuse_texte decoupeuse(tampon);
		decoupeuse.genere_morceaux();

		auto assembleuse = assembleuse_arbre();
		auto analyseuse = analyseuse_grammaire(tampon, &assembleuse);
		analyseuse.lance_analyse(decoupeuse.morceaux());

		assembleuse.genere_code_llvm();
	}
	catch (const erreur::frappe &e) {
		if (imprime_message) {
			std::cerr << e.message() << '\n';
		}

		return true;
	}
	catch (...) {
		return true;
	}

	return false;
}

static void test_fonction_general(
		numero7::test_unitaire::ControleurUnitaire &controleur)
{
	const char *texte =
			R"(
			fonction ne_retourne_rien() : rien
			{
				retourne;
			}

			fonction ajouter(a : e32, b : e32) : e32
			{
				retourne a + b;
			}

			fonction principale(compte : e32, arguments : e8) : e32
			{
				ne_retourne_rien();
				soit a = ajouter(5, 8);
				soit b = ajouter(8, 5);
				soit c = ajouter(ajouter(a + b, b), ajouter(b + a, a));
				retourne c != 5;
			}
			)";

	const auto erreur_lancee = retourne_erreur_lancee(texte, false);
	CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
}

static void test_fonction_inconnue(
		numero7::test_unitaire::ControleurUnitaire &controleur)
{
	const char *texte =
			R"(
			fonction principale(compte : e32, arguments : e8) : e32
			{
				retourne sortie(0);
			}
			)";

	const auto erreur_lancee = retourne_erreur_lancee(texte, false);
	CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
}

static void test_argument_nomme_succes(
		numero7::test_unitaire::ControleurUnitaire &controleur)
{
	const char *texte =
			R"(
			fonction ajouter(a : e32, b : e32) : e32
			{
				retourne a + b;
			}

			fonction principale(compte : e32, arguments : e8) : e32
			{
				soit x = ajouter(a=5, b=6);
				soit y = ajouter(b=5, a=6);
				retourne x - y;
			}
			)";

	const auto erreur_lancee = retourne_erreur_lancee(texte, false);
	CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
}

static void test_argument_nomme_echec(
		numero7::test_unitaire::ControleurUnitaire &controleur)
{
	/* argument redondant */
	{
		const char *texte =
				R"(
				fonction ajouter(a : e32, b : e32) : e32
				{
					retourne a + b;
				}

				fonction principale(compte : e32, arguments : e8) : e32
				{
					soit x = ajouter(a=5, a=6);
					retourne x != 5;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
	/* argument inconnu */
	{
		const char *texte =
				R"(
				fonction ajouter(a : e32, b : e32) : e32
				{
					retourne a + b;
				}

				fonction principale(compte : e32, arguments : e8) : e32
				{
					soit x = ajouter(a=5, c=6);
					retourne x != 5;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
}

static void test_type_argument_echec(
		numero7::test_unitaire::ControleurUnitaire &controleur)
{
	const char *texte =
			R"(
			fonction ajouter(a : e32, b : e32) : e32
			{
				retourne a + b;
			}

			fonction principale(compte : e32, arguments : e8) : e32
			{
				soit x = ajouter(a=5.0, b=6.0);
				retourne x != 5;
			}
			)";

	const auto erreur_lancee = retourne_erreur_lancee(texte, false);
	CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
}

void test_fonctions(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	test_fonction_general(controleur);
	test_fonction_inconnue(controleur);
	test_argument_nomme_succes(controleur);
	test_argument_nomme_echec(controleur);
	test_type_argument_echec(controleur);
}
