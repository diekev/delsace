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

#include "test_transtype.hh"

#include "erreur.h"
#include "outils.h"

static void test_transtype_litterales(dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Les nombres littéraux entiers peuvent être convertis en types relatifs.");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					soit x : z8  = 0;
					soit y : z16 = 0;
					soit z : z32 = 0;
					soit w : z64 = 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Les nombres littéraux entiers peuvent être convertis en types naturels.");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					soit x : n8  = 0;
					soit y : n16 = 0;
					soit z : n32 = 0;
					soit w : n64 = 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Les nombres littéraux entiers ne peuvent pas être convertis en types réels.");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					soit x : r16 = 0;
					soit y : r32 = 0;
					soit z : r64 = 0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::ASSIGNATION_MAUVAIS_TYPE);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Les nombres littéraux réels peuvent être convertis en types réels.");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					soit x : r16 = 0.0;
					soit y : r32 = 0.0;
					soit z : r64 = 0.0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Les nombres littéraux réels ne peuvent pas être convertis en types relatifs.");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					soit x : z8  = 0.0;
					soit y : z16 = 0.0;
					soit z : z32 = 0.0;
					soit w : z64 = 0.0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::ASSIGNATION_MAUVAIS_TYPE);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Les nombres littéraux réels ne peuvent pas être convertis en types naturels.");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					soit x : n8  = 0.0;
					soit y : n16 = 0.0;
					soit z : n32 = 0.0;
					soit w : n64 = 0.0;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::ASSIGNATION_MAUVAIS_TYPE);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_transtype_variable(dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(controleuse,
						  "On peut transtyper entre types entiers relatif et naturel");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					soit x : z32 = 0;
					soit y : n32 = transtype(x : n32);
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(controleuse,
						  "On peut transtyper entre types entiers de tailles binaires différentes");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					soit x : z32 = 0;
					soit y : z8  = transtype(x : z8);
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut transtyper entre types entiers naturel et relatif de"
				" tailles binaires différentes");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					soit x : z32 = 0;
					soit y : n8  = transtype(x : n8);
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(controleuse,
						  "On peut transtyper entre types entiers et réels");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					soit x : z32 = 0;
					soit y : r32 = transtype(x : r32);
					soit z : z32 = transtype(y : z32);
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut transtyper entre types entiers et réels de tailles"
				" binaires différentes");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					soit x : z32 = 0;
					soit y : r64 = transtype(x : r64);
					soit z : z32 = transtype(y : z32);
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_transtype_expression(dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut transtyper le résultat des expressions");
	{
		const char *texte =
				R"(
				fonction foo() : rien
				{
					soit x : z32 = 1;
					soit y : z32 = 5;
					soit z : z32 = transtype(x + y * 5 : z32);
					soit w : z32 = transtype(x - y : z32);
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

void test_transtype(dls::test_unitaire::Controleuse &controleuse)
{
	test_transtype_litterales(controleuse);
	test_transtype_variable(controleuse);
	test_transtype_expression(controleuse);
}
