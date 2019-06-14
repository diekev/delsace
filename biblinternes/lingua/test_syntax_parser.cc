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

#include "syntax_parser.h"

#include <vector>

std::vector<std::string> determiners;
std::vector<std::string> adjectives;
std::vector<std::string> nouns;
std::vector<std::string> verbs;

static void init_dicts()
{
	determiners.push_back("a");
	determiners.push_back("an");
	determiners.push_back("the");
	determiners.push_back("this");
	determiners.push_back("those");

	adjectives.push_back("beautiful");
	adjectives.push_back("small");
	adjectives.push_back("chirping");
	adjectives.push_back("perching");

	nouns.push_back("bird");
	nouns.push_back("birds");
	nouns.push_back("grain");
	nouns.push_back("grains");

	verbs.push_back("peck");
	verbs.push_back("pecks");
	verbs.push_back("pecking");
}

void test_syntax_parser(std::ostream &os)
{
	init_dicts();

	SyntaxParser parser;
	parser("the beautiful bird pecks the grains");

	os << '\n';
}
