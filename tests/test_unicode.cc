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

#include "test_unicode.h"

#include "../coeur/decoupage/unicode.h"

void test_unicode(dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Des séquences UTF-8 peuvent prendre entre 1 et 4 octets");
	{
		CU_VERIFIE_EGALITE(controleuse, nombre_octets("a"), 1);
		CU_VERIFIE_EGALITE(controleuse, nombre_octets("é"), 2);
		CU_VERIFIE_EGALITE(controleuse, nombre_octets("ア"), 3);
		CU_VERIFIE_EGALITE(controleuse, nombre_octets("𠮟"), 4);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Aucune séquence UTF-8 ne peut commencer par les valeurs hexadécimales 0xC0 ou 0xC1");
	{
		CU_VERIFIE_EGALITE(controleuse, nombre_octets("\xC0"), 0);
		CU_VERIFIE_EGALITE(controleuse, nombre_octets("\xC1"), 0);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Les points allant de u+D800 à u+DFFF sont interdits");
	{
		CU_VERIFIE_EGALITE(controleuse, nombre_octets("\xD8\x00"), 0);
		CU_VERIFIE_EGALITE(controleuse, nombre_octets("\xDF\xFF"), 0);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Si le premier octet est 0xED, les suivants ne peuvent être en 0xA0 et 0xBF");
	{
		CU_VERIFIE_EGALITE(controleuse, nombre_octets("\xED\xA0"), 0);
		CU_VERIFIE_EGALITE(controleuse, nombre_octets("\xED\xBF"), 0);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Si le premier octet est 0xF4, les suivants ne peuvent être en 0x90 et 0xBF");
	{
		CU_VERIFIE_EGALITE(controleuse, nombre_octets("\xF4\x90"), 0);
		CU_VERIFIE_EGALITE(controleuse, nombre_octets("\xF4\xBF"), 0);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"Une séquence ne peut commencer par des octets allant de 0xF5 à 0xFF");
	{
		CU_VERIFIE_EGALITE(controleuse, nombre_octets("\xF5"), 0);
		CU_VERIFIE_EGALITE(controleuse, nombre_octets("\xFF"), 0);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}
