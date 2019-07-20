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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "tests.h"

#include "language.h"

//#define FIND_DIFFERENCE

#ifdef FIND_DIFFERENCE
/* longest common substrings */
static auto find_diff(const dls::chaine &lhs, const dls::chaine &rhs)
{
	dls::tableau<int> L(lhs.taille() * rhs.taille());
	dls::tableau<dls::chaine> ret{};
	auto z = 0;
	auto index = 0;

	for (auto i = 0ul; i < lhs.taille(); ++i) {
		for (auto j = 0ul; j < rhs.taille(); ++j, ++index) {
			if (lhs[i] != rhs[j]) {
				L[index] = 0;
				continue;
			}

			if (i == 0 || j == 0) {
				L[index] = 1;
			}
			else {
				L[index] = L[(i - 1) + rhs.taille() * (j - 1)] + 1;
			}

			if (L[index] > z) {
				z = L[index];
				ret.efface();
				ret.pousse(rhs.substr(i - z + 1, i));
			}
			else if (L[index] == z) {
				ret.pousse(rhs.substr(i - z + 1, i));
			}
		}
	}

	return ret;
}
#endif

void test_langage(std::ostream &os)
{
#if 0
	table_t old_french_phon = {
	    { "ʦ", "s" },
	    { "ʧ", "ʃ" },
	    { "ʤ", "ʒ" },
	};
#endif

	table_t sample = {
	    { "chasteau", "château" },
	    { "chastier", "châtier" },
	    { "chesne", "chêne" },
	    { "estoile", "étoile" },
	    { "escuier", "écuyer" },
	    { "estois", "étais" },
	    { "françois", "français" }
	};

#ifndef FIND_DIFFERENCE
	LangageTree tree;

	Language::Ptr old_french = Language::create("Old French");
	old_french->setTable({{ "â", "as" },
	                      { "é", "es" },
	                      { "ê", "es" },
	                      { "ais", "ois"}});

	tree.addLanguage(old_french);

	Language::Ptr middle_french = Language::create("Middle French");
	middle_french->setTable({{ "ch", "c" },
	                         { "oi", "ei" }});

	tree.addLanguage(middle_french);

	Language::Ptr high_french = Language::create("High French");
	high_french->setTable({{ "es", "s" },
	                       { "ei", "e" }});

	tree.addLanguage(high_french);

	tree.connect(old_french, middle_french);
	tree.connect(middle_french, high_french);

	for (const auto &pair : sample) {
		dls::chaine word = pair.second;
		tree.walk(word);

		os << word << '\n';
	}
#else
	for (const auto &pair : sample) {
		auto diff = find_diff(pair.first, pair.second);

		for (const auto &d : diff) {
			os << d << ' ';
		}

		os << '\n';
	}
#endif
}
