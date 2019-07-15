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

#include "chaine.hh"

#include "biblinternes/structures/flux_chaine.hh"

namespace dls {

tableau<chaine> morcelle(chaine const &texte, char const delimiteur)
{
	tableau<chaine> morceaux;
	std::string morceau;
	std::stringstream ss(texte.c_str());

	while (std::getline(ss, morceau, delimiteur)) {
		if (morceau.empty()) {
			continue;
		}

		morceaux.pousse(morceau);
	}

	return morceaux;
}

tableau<chaine> morcelle(chaine const &texte, chaine const &delimitrice)
{
	auto results = tableau<chaine>{};
	auto cstr = new char[static_cast<size_t>(texte.taille() + 1)];

	std::strcpy (cstr, texte.c_str());
	auto p = std::strtok(cstr, delimitrice.c_str());

	while (p != nullptr) {
		results.pousse(p);
		p = strtok(nullptr, delimitrice.c_str());
	}

	delete [] cstr;
	delete [] p;

	return results;
}

chaine garnis_chaine(chaine const &chn, long taille)
{
	auto garniture = chaine(taille, '0');
	garniture += chn;

	return garniture;
}

bool remplace_chaine(chaine &chn, chaine const &de, chaine const &vers)
{
	auto start_pos = chn.trouve(de);

	if (start_pos == chaine::npos) {
		return false;
	}

	chn.remplace(start_pos, de.taille(), vers);

	return true;
}

chaine chaine_depuis_entier(int nombre)
{
	flux_chaine ss;
	ss << nombre;
	return ss.chn();
}

chaine premier_n_caracteres(chaine const &chn, long n)
{
	return chn.sous_chaine(0, n);
}

chaine dernier_n_caracteres(chaine const &chn, long n)
{
	return chn.sous_chaine(chn.taille() - n, n);
}

}  /* namespace dls */
