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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "tests.hh"

#include "biblinternes/types/pystring.h"

void test_pystring(dls::test_unitaire::Controleuse &controleur)
{
	{
		dls::chaine stdstr("stdstr");
		dls::types::pystring str = dls::types::pystring(stdstr.c_str());

		CU_VERIFIE_CONDITION(controleur, !str.empty());
		CU_VERIFIE_CONDITION(controleur, str.taille() == 6);

		CU_VERIFIE_CONDITION(controleur, str.isalpha());
		CU_VERIFIE_CONDITION(controleur, str.isalnum());
		CU_VERIFIE_CONDITION(controleur, str.islower());
		CU_VERIFIE_CONDITION(controleur, !str.isspace());
		CU_VERIFIE_CONDITION(controleur, !str.isupper());
		CU_VERIFIE_CONDITION(controleur, !str.istitle());
		CU_VERIFIE_CONDITION(controleur, !str.isdecimal());
		CU_VERIFIE_CONDITION(controleur, !str.isnumeric());

		CU_VERIFIE_CONDITION(controleur, str.capacity() == 6);
		str.reserve(1000);
		CU_VERIFIE_CONDITION(controleur, str.capacity() == 1000);
		CU_VERIFIE_CONDITION(controleur, str.capacity() != str.taille());
		CU_VERIFIE_CONDITION(controleur, str.front() == str[0]);
		CU_VERIFIE_CONDITION(controleur, str.back() == str[str.taille() - 1]);

		CU_VERIFIE_CONDITION(controleur, str == dls::types::pystring("stdstr"));
	}
}
