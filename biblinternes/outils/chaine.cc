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

#include "iterateurs.h"

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

long compte(const chaine &str, char c)
{
	auto count = 0l;
	auto index = 0l;
	auto prev =  0l;

	while ((index = str.trouve(c, prev)) != dls::chaine::npos) {
		++count;
		prev = index + 1;
	}

	return count;
}

long compte_commun(const chaine &rhs, const chaine &lhs)
{
	if (rhs.taille() != lhs.taille()) {
		return 0;
	}

	auto match = 0;

	for (const auto &i : outils::plage(rhs.taille())) {
		if (rhs[i] == lhs[i]) {
			++match;
		}
	}

	return match;
}

void remplace_souschaine(chaine &str, const chaine &substr, const chaine &rep)
{
	long index = 0;

	while (true) {
		/* Locate the substring to replace. */
		index = str.trouve(substr, index);

		if (index == dls::chaine::npos) {
			break;
		}

		/* Make the replacement. */
		str.remplace(index, substr.taille(), rep);

		/* Advance index forward so the next iteration doesn't pick it up as well. */
		index += rep.taille();
	}
}

long distance_levenshtein(
			dls::vue_chaine_compacte const &chn1,
			dls::vue_chaine_compacte const &chn2)
{
	auto const m = chn1.taille();
	auto const n = chn2.taille();

	if (m == 0) {
		return n;
	}

	if (n == 0) {
		return m;
	}

	auto couts = dls::tableau<long>(n + 1);

	for (auto k = 0; k <= n; k++) {
		couts[k] = k;
	}

	for (auto i = 0l; i < chn1.taille(); ++i) {
		couts[0] = i + 1;
		auto coin = i;

		for (auto j = 0; j < chn2.taille(); ++j) {
			auto enhaut = couts[j + 1];

			if (chn1[i] == chn2[j]) {
				couts[j + 1] = coin;
			}
			else {
				auto t = enhaut < coin ? enhaut : coin;
				couts[j + 1] = (couts[j] < t ? couts[j] : t) + 1;
			}

			coin = enhaut;
		}
	}

	return couts[n];
}

char caractere_echappe(const char *sequence)
{
	switch (sequence[0]) {
		case '\\':
			switch (sequence[1]) {
				case '\\':
					return '\\';
				case '\'':
					return '\'';
				case 'a':
					return '\a';
				case 'b':
					return '\b';
				case 'f':
					return '\f';
				case 'n':
					return '\n';
				case 'r':
					return '\r';
				case 't':
					return '\t';
				case 'v':
					return '\v';
				case '0':
					return '\0';
				default:
					return sequence[1];
			}
		default:
			return sequence[0];
	}
}

}  /* namespace dls */
