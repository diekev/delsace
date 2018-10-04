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

void test_expression(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	const char *texte =
R"(fonction foo()
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
	soit x10 = 0x80 <= a <= 0xBF;
	soit x11 = a < b;
	soit x12 = a > b;
	soit x13 = a && b;
	soit x14 = a & b;
	soit x15 = a || b;
	soit x16 = a | b;
	soit x17 = a ^ b;
	soit x18 = !a;
	soit x19 = ~a;
	soit x20 = @a;
	soit x21 = a;
}
)";

	/* Passage du test sans la génération du code. */
	auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::AUCUNE_ERREUR, false);
	CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);

	/* Passage du test avec la génération du code. */
	erreur_lancee = retourne_erreur_lancee(texte, false, erreur::AUCUNE_ERREUR, true);
	CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
}
