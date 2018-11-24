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

#include "test_expression.h"

#include "erreur.h"
#include "outils.h"

static void test_expression_flux_si(dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(controleuse, "L'expression d'un contrôle 'si' doit être booléenne.");
	{
		const char *texte =
	R"(fonction foo() : rien
	{
		soit a = 5;
		soit b = 6;
		si a == b {
		}
	}
	)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::AUCUNE_ERREUR);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(controleuse, "L'expression d'un contrôle 'si' ne peut être d'un autre type que booléen.");
	{
		const char *texte =
	R"(fonction foo() : rien
	{
		soit a = 5;

		si a {
		}
	}
	)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::TYPE_DIFFERENTS);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

static void test_expression_general(dls::test_unitaire::Controleuse &controleuse)
{
	const char *texte =
R"(fonction foo() : rien
{
	soit a = 5;
	soit b = 5;
	soit x00 = a + b;
	soit x01 = a - b;
	soit x02 = a * b;
	soit x03 = a / b;
	soit x04 = a << b;
	soit x05 = a >> b;
	soit x06 = a == b;
	soit x07 = a != b;
	soit x08 = a <= b;
	soit x09 = a >= b;
	soit x10 = 0x80 <= a && a <= 0xBF;
	soit x11 = a < b;
	soit x12 = a > b;
	soit x13 = a && b;
	soit x14 = a & b;
	soit x15 = a || b;
	soit x16 = a | b;
	soit x17 = a ^ b;
	soit x18 = !(a == b);
	soit x19 = ~a;
	soit x20 = @a;
	soit x21 = a;
	soit x22 = -a + -b;
	soit x23 = +b - +a;
}
)";

	{
		/* Passage du test sans la génération du code. */
		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::AUCUNE_ERREUR, false);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	{
		/* Passage du test avec la génération du code. */
		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::AUCUNE_ERREUR, true);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
}

static void test_expression_constante_reelle(dls::test_unitaire::Controleuse &controleuse)
{
	/* comparaison réussie */
	{
		const char *texte =
	R"(fonction foo() : rien
	{
		soit t0 = 5.0 == 2.0;
		soit t1 = 5.0 != 2.0;
		soit t2 = 5.0 <= 2.0;
		soit t3 = 5.0 >= 2.0;
		soit t4 = 5.0 < 2.0;
		soit t5 = 5.0 > 2.0;
	}
	)";

		/* Passage du test sans la génération du code. */
		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::NORMAL, false);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	/* comparaison échouée */
	{
		const char *texte =
	R"(fonction foo() : rien
	{
		soit t0 = 5.0 == 2;
	}
	)";

		/* Passage du test sans la génération du code. */
		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::NORMAL, false);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	/* comparaison échouée */
	{
		const char *texte =
	R"(fonction foo() : rien
	{
		soit t0 = 5.0 && 2.0;
	}
	)";

		/* Passage du test sans la génération du code. */
		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::NORMAL, false);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	/* arithmétique réussie */
	{
		const char *texte =
	R"(fonction foo() : rien
	{
		soit t0 = 5.0 + 2.0;
		soit t1 = 5.0 - 2.0;
		soit t2 = 5.0 * 2.0;
		soit t3 = 5.0 / 2.0;
	}
	)";

		/* Passage du test sans la génération du code. */
		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::NORMAL, false);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	/* arithmétique échouée */
	{
		const char *texte =
	R"(fonction foo() : rien
	{
		soit t0 = 5.0 % 2.0;
	}
	)";

		/* Passage du test sans la génération du code. */
		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::NORMAL, false);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	/* binaire échouée */
	{
		const char *texte =
	R"(fonction foo() : rien
	{
		soit t0 = 5.0 & 2.0;
	}
	)";

		/* Passage du test sans la génération du code. */
		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::NORMAL, false);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
}

static void test_expression_constante_entiere(dls::test_unitaire::Controleuse &controleuse)
{
	/* comparaison réussie */
	{
		const char *texte =
	R"(fonction foo() : rien
	{
		soit t0 = 5 == 2;
		soit t1 = 5 != 2;
		soit t2 = 5 <= 2;
		soit t3 = 5 >= 2;
		soit t4 = 5 < 2;
		soit t5 = 5 > 2;
	}
	)";

		/* Passage du test sans la génération du code. */
		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::NORMAL, false);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	/* arithmétique réussie */
	{
		const char *texte =
	R"(fonction foo() : rien
	{
		soit t0 = 5 + 2;
		soit t1 = 5 - 2;
		soit t2 = 5 * 2;
		soit t3 = 5 / 2;
		soit t4 = 5 % 2;
		soit t5 = 5 >> 2;
		soit t6 = 5 << 2;
		soit t7 = 5 & 2;
		soit t8 = 5 | 2;
		soit t9 = 5 ^ t0;
	}
	)";

		/* Passage du test sans la génération du code. */
		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::NORMAL, false);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	/* binaire réussie */
	{
		const char *texte =
	R"(fonction foo() : rien
	{
		soit t0 = 5 & 2;
		soit t1 = 5 | 2;
	}
	)";

		/* Passage du test sans la génération du code. */
		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(texte, false, erreur::type_erreur::NORMAL, false);
		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
}

void test_expression(dls::test_unitaire::Controleuse &controleuse)
{
	test_expression_general(controleuse);
	test_expression_constante_reelle(controleuse);
	test_expression_constante_entiere(controleuse);
	test_expression_flux_si(controleuse);
}
