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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "outils.hh"

#include "structures/chaine.hh"

bool remplace(std::string &std_string, std::string_view motif, std::string_view remplacement)
{
	bool remplacement_effectue = false;
	size_t index = 0;
	while (true) {
		/* Locate the substring to replace. */
		index = std_string.find(motif, index);

		if (index == std::string::npos) {
			break;
		}

		/* Make the replacement. */
		std_string.replace(index, motif.size(), remplacement);

		/* Advance index forward so the next iteration doesn't pick it up as well. */
		index += motif.size();
		remplacement_effectue = true;
	}

	return remplacement_effectue;
}

kuri::chaine supprime_accents(kuri::chaine_statique avec_accent)
{
	auto std_string = std::string(avec_accent.pointeur(), static_cast<size_t>(avec_accent.taille()));

	remplace(std_string, "à", "a");
	remplace(std_string, "é", "e");
	remplace(std_string, "è", "e");
	remplace(std_string, "ê", "e");
	remplace(std_string, "û", "u");
	remplace(std_string, "É", "E");
	remplace(std_string, "È", "E");
	remplace(std_string, "Ê", "E");

	return kuri::chaine(std_string.c_str(), static_cast<long>(std_string.size()));
}

void inclus_systeme(std::ostream &os, kuri::chaine_statique fichier)
{
	os << "#include <" << fichier << ">\n";
}

void inclus(std::ostream &os, kuri::chaine_statique fichier)
{
	os << "#include \"" << fichier << "\"\n";
}

void prodeclare_struct(std::ostream &os, kuri::chaine_statique nom)
{
	os << "struct " << nom << ";\n";
}

void prodeclare_struct_espace(std::ostream &os, kuri::chaine_statique nom, kuri::chaine_statique espace, kuri::chaine_statique param_gabarit)
{
	os << "namespace " << espace << " {\n";
	if (param_gabarit != "") {
		os << "template <" << param_gabarit << ">\n";
	}
	os << "struct " << nom << ";\n";
	os << "}\n";
}
