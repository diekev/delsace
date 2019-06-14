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

#include "depsparser.h"

#if 0
void test_words()
{
	word("canis", category::noun, gender::masculine, number::singular, gcase::nominative, 3);
	word("canem", category::noun, gender::masculine, number::singular, gcase::accusative, 3);
	word("canes", category::noun, gender::masculine, number::plural, gcase::nominative, 3);
	word("canes", category::noun, gender::masculine, number::plural, gcase::accusative, 3);

	word("felis", category::noun, gender::masculine, number::singular, gcase::nominative, 3);
	word("felem", category::noun, gender::masculine, number::singular, gcase::accusative, 3);
	word("feles", category::noun, gender::masculine, number::plural, gcase::nominative, 3);
	word("feles", category::noun, gender::masculine, number::plural, gcase::accusative, 3);

	word("video",   category::verb, gender::none, number::singular, gcase::none, 1);
	word("videmus", category::verb, gender::none, number::plural, gcase::none, 1);
	word("videt",   category::verb, gender::none, number::singular, gcase::none, 3);
	word("vident",  category::verb, gender::none, number::plural, gcase::none, 3);

	word("parvus", category::adjective, gender::masculine, number::singular, gcase::nominative, 0);
	word("parvum", category::adjective, gender::masculine, number::singular, gcase::accusative, 0);
	word("parvi",  category::adjective, gender::masculine, number::plural, gcase::nominative, 0);
	word("parvos", category::adjective, gender::masculine, number::plural, gcase::accusative, 0);
}
#endif

static std::ostream &operator<<(std::ostream &os, const word &w)
{
	os << "word: " << w.label;

	if (w.head) {
		os << ", head: " << w.head->label;
	}

	return os;
}

void test_dependency_parser(std::ostream &os)
{
	std::vector<word *> sentence;

	word canis = word("canis", category::noun, gender::masculine, number::singular, gcase::nominative, 3);
	word parvum = word("parvum", category::adjective, gender::masculine, number::singular, gcase::accusative, 0);
	word videt = word("videt",   category::verb, gender::none, number::singular, gcase::none, 3);
	word felis = word("felis", category::noun, gender::masculine, number::singular, gcase::nominative, 3);

	sentence.push_back(&canis);
	sentence.push_back(&parvum);
	sentence.push_back(&videt);
	sentence.push_back(&felis);

	lsup(sentence);

	for (word *w : sentence) {
		os << *w << '\n';
	}
}
