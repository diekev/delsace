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

#include "outils.h"

void test_expression(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	const char *texte =
R"(fonction foo()
{
	soit x = a + b;
	soit x = a - b;
	soit x = a * b;
	soit x = a / b;
	soit x = a << b;
	soit x = a >> b;
	soit x = a == b;
	soit x = a != b;
	soit x = a <= b;
	soit x = a >= b;
	soit x = 0x80 <= a <= 0xBF;
	soit x = a < b;
	soit x = a > b;
	soit x = a && b;
	soit x = a & b;
	soit x = a || b;
	soit x = a | b;
	soit x = a ^ b;
	soit x = !a;
	soit x = ~a;
	soit x = @a;
	soit x = a;
}
)";

	/* À FAIRE : passage du test avec la génération du code. */
	const auto erreur_lancee = retourne_erreur_lancee(texte, true, false);
	CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
}
