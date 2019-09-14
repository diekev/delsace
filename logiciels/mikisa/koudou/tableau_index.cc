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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "tableau_index.hh"

namespace kdo {

tableau_index::tableau_index(int oct)
	: octets(oct)
{}

int tableau_index::octets_pour_taille(long taille)
{
	if (taille <= 256) {
		return 0;
	}

	if (taille <= 65536) {
		return 1;
	}

	return 2;
}

void tableau_index::pousse(int v)
{
	switch (octets) {
		case 0:
		{
			pousse_impl(static_cast<unsigned char>(v));
			break;
		}
		case 1:
		{
			pousse_impl(static_cast<unsigned short>(v));
			break;
		}
		case 2:
		{
			pousse_impl(v);
			break;
		}
	}
}

void tableau_index::pousse_impl(unsigned char v)
{
	assert(octets == 0);
	donnees.pousse(v);
	assert((*this)[taille] >= 0 && (*this)[taille] < 256);
	taille += 1;
}

void tableau_index::pousse_impl(unsigned short v)
{
	assert(octets == 1);
	auto curseur = donnees.taille();
	donnees.redimensionne(curseur + 2);
	*reinterpret_cast<unsigned short *>(&donnees[curseur]) = v;
	assert((*this)[curseur / 2] >= 0 && (*this)[curseur / 2] < 65536);
	taille += 1;
}

void tableau_index::pousse_impl(int v)
{
	assert(octets == 2);
	auto curseur = donnees.taille();
	donnees.redimensionne(curseur + 4);
	*reinterpret_cast<int *>(&donnees[curseur]) = v;
	assert((*this)[curseur / 4] >= 0);
	taille += 1;
}

}  /* namespace kdo */
