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

#pragma once

#include "biblinternes/structures/tableau.hh"

namespace kdo {

/**
 * Structure auxilliaire nous permettant d'économiser de la mémoire en stockant
 * des index avec la précision suffisante pour les représenter. Si nous avons
 * au plus 256 valeurs, les index seront stockés chacun sur 1 octet, au plus
 * 65 536 valeurs, sur 2 octets, et sur 4 octets sinon.
 */
struct tableau_index {
	dls::tableau<unsigned char> donnees{};

	/* La taille du nombres de données (donnees.taille() >> octets). */
	int taille = 0;

	/* Exposant pour le nombre d'octets utilisés pour stocker les données
	 * 0 = char  (2^0 = 1)
	 * 1 = short (2^1 = 2)
	 * 2 = int   (2^2 = 4)
	 */
	int octets = 0;

	tableau_index() = default;

	explicit tableau_index(int oct);

	static int octets_pour_taille(long taille);

	inline int operator[](long i) const
	{
		switch (octets) {
			case 0:
			{
				return donnees[i];
			}
			case 1:
			{
				return *reinterpret_cast<unsigned short const *>(&donnees[i << 1]);
			}
			case 2:
			{
				return *reinterpret_cast<int const *>(&donnees[i << 2]);
			}
		}

		return 0;
	}

	void pousse(int v);

private:
	void pousse_impl(unsigned char v);

	void pousse_impl(unsigned short v);

	void pousse_impl(int v);
};

}  /* namespace kdo */
