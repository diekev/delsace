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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "numerique.hh"

#include "biblinternes/outils/definitions.h"

namespace dls::num {

unsigned nombre_chiffre_base_10(unsigned long v)
{
	unsigned resultat = 0;

	do {
		++resultat;
		v /= 10;
	} while (v);

	return resultat;
}

unsigned nombre_chiffre_base_10_opt(unsigned long v)
{
	unsigned resultat = 1;

	for (;;) {
		if (v < 10) {
			return resultat;
		}

		if (v < 100) {
			return resultat + 1;
		}

		if (v < 1000) {
			return resultat + 2;
		}

		if (v < 10000) {
			return resultat + 3;
		}

		resultat += 4;
		v /= 1000U;
	}
}

unsigned nombre_chiffre_base_10_pro(unsigned long v)
{
	unsigned resultat = 1;

	for (;;) {
		if (PROBABLE(v < 10)) {
			return resultat;
		}

		if (PROBABLE(v < 100)) {
			return resultat + 1;
		}

		if (PROBABLE(v < 1000)) {
			return resultat + 2;
		}

		if (PROBABLE(v < 10000)) {
			return resultat + 3;
		}

		resultat += 4;
		v /= 1000U;
	}
}

int nombre_de_chiffres(long nombre)
{
	if (nombre == 0) {
		return 1;
	}

	auto compte = 0;

	while (nombre > 0) {
		nombre /= 10;
		compte += 1;
	}

	return compte;
}

}
