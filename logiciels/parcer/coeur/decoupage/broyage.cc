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

#include "broyage.hh"

#include "biblinternes/langage/unicode.hh"

static char char_depuis_hex(char hex)
{
	return "0123456789ABCDEF"[static_cast<int>(hex)];
}

dls::chaine broye_nom_simple(dls::vue_chaine const &nom)
{
	auto ret = dls::chaine{};

	auto debut = &nom[0];
	auto fin   = &nom[nom.taille()];

	while (debut < fin) {
		auto no = lng::nombre_octets(debut);

		switch (no) {
			case 0:
			{
				debut += 1;
				break;
			}
			case 1:
			{
				ret += *debut;
				break;
			}
			default:
			{
				for (int i = 0; i < no; ++i) {
					ret += 'x';
					ret += char_depuis_hex(static_cast<char>((debut[i] & 0xf0) >> 4));
					ret += char_depuis_hex(static_cast<char>(debut[i] & 0x0f));
				}

				break;
			}
		}

		debut += no;
	}

	return ret;
}

dls::chaine broye_nom_fonction(
		dls::vue_chaine const &nom_fonction,
		const dls::chaine &nom_module,
		size_t index_type)
{
	/* pour l'instant, nous ne broyons que le nom pour supprimer les accents
	 * dans le future, quand nous aurons des surcharges de fonctions et des
	 * génériques nous incluerons également les types
	 */

	auto ret = dls::chaine("KR_");
	ret += broye_nom_simple(nom_module);
	ret += '_';
	ret += broye_nom_simple(nom_fonction);
	ret += std::to_string(index_type);

	return ret;
}
