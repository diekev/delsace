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

#include "biblinternes/structures/tableau.hh"

dls::tableau<dls::chaine> determiners;
dls::tableau<dls::chaine> adjectives;
dls::tableau<dls::chaine> nouns;
dls::tableau<dls::chaine> verbs;

static void init_dicts()
{
	determiners.pousse("a");
	determiners.pousse("an");
	determiners.pousse("the");
	determiners.pousse("this");
	determiners.pousse("those");

	adjectives.pousse("beautiful");
	adjectives.pousse("small");
	adjectives.pousse("chirping");
	adjectives.pousse("perching");

	nouns.pousse("bird");
	nouns.pousse("birds");
	nouns.pousse("grain");
	nouns.pousse("grains");

	verbs.pousse("peck");
	verbs.pousse("pecks");
	verbs.pousse("pecking");
}

void test_syntax_parser(std::ostream &os)
{
	init_dicts();

	SyntaxParser parser;
	parser("the beautiful bird pecks the grains");

	os << '\n';
}
