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

#include "erreur.h"
#include "outils.h"

static void test_pointeur_fonction(
		dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une variable ou un membre d'une structure peut être un pointeur vers une fonction.");
	{
		const char *texte =
				R"(
				structure Foo {
					x : fonction(z32,*z8,z32)z32;
				}

				fonction bar(a : fonction(r32, r64)rien) : rien
				{
					soit x = a;
					soit y : fonction(r32, r64)rien = a;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::ARGUMENT_REDEFINI);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On ne peut appeler une variable qui n'est pas un pointeur fonction.");
	{
		const char *texte =
				R"(
				fonction bar() : rien
				{
					soit x = 5;
					soit y = x();
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::FONCTION_INCONNUE);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut appeler sans problème un pointeur vers une fonction.");
	{
		const char *texte =
				R"(
				fonction bar(a : fonction(r64, r64)r32) : r32
				{
					retourne a(0.2, 0.4);
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::ARGUMENT_REDEFINI);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On ne peut pas nommer les arguments d'un pointeur vers une fonction.");
	{
		const char *texte =
				R"(
				fonction bar(a : fonction(r32, r64)r32) : r32
				{
					retourne a(x=0.2, z=0.4);
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::ARGUMENT_INCONNU);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Les pointeurs fonction ont un typage strict (succès).");
	{
		const char *texte =
				R"(
				fonction ajoute_cb(x : r64, z : r64, cb : fonction(r64, r64) r64) : r64
				{
					retourne cb(x, z);
				}

				fonction ajoute_deux(x : r64, y : r64) : r64
				{
					retourne x + y;
				}

				fonction bar() : r64
				{
					retourne ajoute_cb(1.0, 2.0, ajoute_deux);
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::TYPE_DIFFERENTS);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Les pointeurs fonction ont un typage strict (échec).");
	{
		const char *texte =
				R"(
				fonction ajoute_cb(x : r64, z : r64, cb : fonction(r64, r64) r64) : r64
				{
					retourne cb(x, z);
				}

				fonction ajoute_trois(x : r64, y : r64, z : r64) : r64
				{
					retourne x + y + z;
				}

				fonction bar() : r64
				{
					retourne ajoute_cb(1.0, 2.0, ajoute_trois);
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::TYPE_ARGUMENT);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Les arguments des pointeurs fonction ont un typage strict.");
	{
		const char *texte =
				R"(
				fonction ajoute_cb(x : z32, z : z32, cb : fonction(r32, r32) r32) : r32
				{
					retourne cb(x, z);
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::TYPE_ARGUMENT);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_appel_fonction_variadique_args_nommes(
		dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"L'appel d'une fonction variadique n'a pas besoin de nommer les arguments.");
	{
		const char *texte =
				R"(
				fonction externe foo(a : z32, b : z32, c : ...z32) : rien;

				fonction bar() : rien
				{
					foo(0, 1, 2, 3, 4, 5);
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::ARGUMENT_REDEFINI);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"L'appel d'une fonction variadique peut nommer tous les arguments.");
	{
		const char *texte =
				R"(
				fonction externe foo(a : z32, b : z32, c : ...z32) : rien;

				fonction bar() : rien
				{
					foo(a=0, b=1, c=2, c=3, c=4, c=5);
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::ARGUMENT_REDEFINI);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"L'appel d'une fonction variadique peut éviter de renommer l'argument variadique.");
	{
		const char *texte =
				R"(
				fonction externe foo(a : z32, b : z32, c : ...z32) : rien;

				fonction bar() : rien
				{
					foo(a=0, b=1, c=2, 3, 4, 5);
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::ARGUMENT_REDEFINI);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"L'appel d'une fonction variadique peut nommer les arguments dans le désordre.");
	{
		const char *texte =
				R"(
				fonction externe foo(a : z32, b : z32, c : ...z32) : rien;

				fonction bar() : rien
				{
					foo(a=0, c=1, 2, 3, 4, b=5);
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::ARGUMENT_REDEFINI);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_appel_fonction_variadique(
		dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une fonction variadique dont l'argument variadic n'est pas typé"
				" peut prendre n'importe quel type.");
	{
		const char *texte =
				R"(
				fonction externe printf(format : *z8, arguments : ...) : rien;

				fonction foo() : rien
				{
					printf("%d%c%f%s", 0, 'z', 2.5, "chaine");
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::TYPE_ARGUMENT);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une fonction variadique dont l'argument variadic est typé"
				" ne peut prendre n'importe quel type.");
	{
		const char *texte =
				R"(
				fonction externe printf(format : *z8, arguments : ...z32) : rien;

				fonction foo() : rien
				{
					printf("%d%c%f%s", 0, 'z', 2.5, "chaine");
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::TYPE_ARGUMENT);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une fonction variadique dont l'argument variadic est typé est"
				" correct si les types passés correspondent au type défini.");
	{
		const char *texte =
				R"(
				fonction externe printf(format : *z8, arguments : ...z32) : rien;

				fonction foo() : rien
				{
					printf("%d%c%f%s", 0, 1, 2, 3, 4);
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::TYPE_ARGUMENT);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_declaration_fonction_variadique(
		dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Ce n'est pas obliger de typer la liste d'arguments d'une fonction variable.");
	{
		const char *texte =
				R"(
				fonction externe principale(compte : z32, arguments : ...) : rien;
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::NORMAL);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"C'est possible de typer la liste d'arguments d'une fonction variable.");
	{
		const char *texte =
				R"(
				fonction externe principale(compte : z32, arguments : ...z32) : rien;
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::NORMAL);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On ne peut pas avoir d'autres arguments après un argument variadique.");
	{
		const char *texte =
				R"(
				fonction externe principale(arguments : ...*z32, compte : z32) : rien;
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::NORMAL);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On ne peut pas avoir plusieurs arguments variadiques.");
	{
		const char *texte =
				R"(
				fonction externe principale(arguments : ...*z32, comptes : ...z32) : rien;
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::NORMAL);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_fonction_general(
		dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut appeler des fonctions connue avec les bon nombre et"
				" types d'arguments.");
	{
		const char *texte =
				R"(
				fonction ne_retourne_rien() : rien
				{
					retourne;
				}

				fonction ajouter(a : z32, b : z32) : z32
				{
					retourne a + b;
				}

				fonction principale(compte : z32, arguments : z8) : bool
				{
					ne_retourne_rien();
					soit a = ajouter(5, 8);
					soit b = ajouter(8, 5);
					soit c = ajouter(ajouter(a + b, b), ajouter(b + a, a));
					retourne c != 5;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::AUCUNE_ERREUR);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_fonction_inconnue(
		dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On ne peut appeler une fonction inconnue.");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : z8) : z32
				{
					retourne sortie(0);
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::FONCTION_INCONNUE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_argument_nomme_succes(
		dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Il est possible de nommer les arguments passés à une fonction"
				" selon le noms des arguments de sa défition.");
	{
		const char *texte =
				R"(
				fonction ajouter(a : z32, b : z32) : z32
				{
					retourne a + b;
				}

				fonction principale(compte : z32, arguments : z8) : z32
				{
					soit x = ajouter(a=5, b=6);
					soit y = ajouter(b=5, a=6);
					retourne x - y;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::AUCUNE_ERREUR);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_argument_nomme_echec(
		dls::test_unitaire::Controleuse &controleuse)
{
	/* argument redondant */
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Un argument nommé ne peut avoir le nom d'un argument déjà nommé.");
	{
		const char *texte =
				R"(
				fonction ajouter(a : z32, b : z32) : z32
				{
					retourne a + b;
				}

				fonction principale(compte : z32, arguments : z8) : z32
				{
					soit x = ajouter(a=5, a=6);
					retourne x != 5;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::ARGUMENT_REDEFINI);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Un argument nommé ne peut avoir le nom d'un argument inconnu.");
	{
		const char *texte =
				R"(
				fonction ajouter(a : z32, b : z32) : z32
				{
					retourne a + b;
				}

				fonction principale(compte : z32, arguments : z8) : z32
				{
					soit x = ajouter(a=5, c=6);
					retourne x != 5;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::ARGUMENT_INCONNU);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Si un argument d'une fonction appelée est nommé, tous les arguments doivent l'être (premier).");
	{
		const char *texte =
				R"(
				fonction ajouter(a : z32, b : z32) : z32
				{
					retourne a + b;
				}

				fonction principale(compte : z32, arguments : z8) : z32
				{
					soit x = ajouter(a=5, 6);
					retourne x != 5;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::ARGUMENT_INCONNU);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Si un argument d'une fonction appelée est nommé, les précédents"
				" peuvent être anonymes si l'argument nommé n'a pas le nom d'un"
				" précédent argument.");
	{
		const char *texte =
				R"(
				fonction ajouter(a : z32, b : z32) : z32
				{
					retourne a + b;
				}

				fonction principale(compte : z32, arguments : z8) : z32
				{
					soit x = ajouter(5, b=6);
					retourne 0;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::ARGUMENT_INCONNU);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Si un argument d'une fonction appelée est nommé, il ne peut pas"
				" prendre d'un argument précédent anonyme.");
	{
		const char *texte =
				R"(
				fonction ajouter(a : z32, b : z32) : z32
				{
					retourne a + b;
				}

				fonction principale(compte : z32, arguments : z8) : z32
				{
					soit x = ajouter(5, a=6);
					retourne 0;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::ARGUMENT_REDEFINI);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_type_argument_echec(
		dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Les types des arguments passés à une fonction ne peuvent être"
				" différents de ceux de sa définition.");
	{
		const char *texte =
				R"(
				fonction ajouter(a : z32, b : z32) : z32
				{
				retourne a + b;
				}

				fonction principale(compte : z32, arguments : z8) : z32
				{
				soit x = ajouter(a=5.0, b=6.0);
				retourne 0;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::TYPE_ARGUMENT);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_nombre_argument(
		dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Le nombre d'arguments (nommés) passé à une fonction lors de son appel "
				"doit être le même que le nombre d'arguments de sa définition.");
	{
		const char *texte =
				R"(
				fonction ajouter(a : z32, b : z32) : z32
				{
					retourne a + b;
				}

				fonction principale(compte : z32, arguments : z8) : z32
				{
					soit x = ajouter(a=5);
					retourne x != 5;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::NOMBRE_ARGUMENT);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Le nombre d'arguments (anonymes) passé à une fonction lors de son appel "
				"doit être le même que le nombre d'arguments de sa définition.");
	{
		const char *texte =
				R"(
				fonction ajouter(a : z32, b : z32) : z32
				{
					retourne a + b;
				}

				fonction principale(compte : z32, arguments : z8) : z32
				{
					soit x = ajouter(5, 6, 7);
					retourne x != 5;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::NOMBRE_ARGUMENT);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_argument_unique(
		dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Deux arguments d'une fonction ne peuvent avoir les même nom.");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, compte : z8) : z32
				{
					retourne x != 5;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::ARGUMENT_REDEFINI);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_fonction_redinie(
		dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Deux fonctions ne peuvent avoir les même nom.");
	{
		const char *texte =
				R"(
				fonction principale(compte : z32, arguments : z8) : z32
				{
					retourne 0;
				}

				fonction principale(compte : z32, arguments : z8) : z32
				{
					retourne 0;
				}
				)";

		const auto [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::FONCTION_REDEFINIE);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

void test_fonctions(dls::test_unitaire::Controleuse &controleuse)
{
	test_fonction_general(controleuse);
	test_fonction_inconnue(controleuse);
	test_argument_nomme_succes(controleuse);
	test_argument_nomme_echec(controleuse);
	test_type_argument_echec(controleuse);
	test_nombre_argument(controleuse);
	test_argument_unique(controleuse);
	test_fonction_redinie(controleuse);
	test_declaration_fonction_variadique(controleuse);
	test_appel_fonction_variadique(controleuse);
	test_appel_fonction_variadique_args_nommes(controleuse);
	test_pointeur_fonction(controleuse);
}
