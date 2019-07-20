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

#pragma once

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

/* The folowing enums were derived from
 * "A Free-Word-Order Dependency Parser in Prolog", Michael A. Covington, 2003
 */

enum class category {
	noun,
	verb,
	adjective,
};

enum class gender {
	masculine,
	feminine,
	neutral,

	none,
};

enum class number {
	singular,
	plural,
};

enum class gcase {
	nominative,
	accusative,

	none,
};

struct word {
	word *head = nullptr;
	dls::tableau<word *> dependents{};

	dls::chaine label;
	int count = 0;
	category cat;
	gender gend;
	number num;
	gcase cas;
	int person;

	word(const dls::chaine &l, category c, gender g, number n, gcase gc, int pers)
	    : label(l)
	    , cat(c)
	    , gend(g)
	    , num(n)
	    , cas(gc)
	    , person(pers)
	{}

	word(const word &) = default;
	word &operator=(const word &) = default;
};

void link(word *head, word *dependent);

void esh_heads_first(const dls::tableau<word *> &sentence);
void esh_dependents_first(const dls::tableau<word *> &sentence);

void esh_heads_first_unique(const dls::tableau<word *> &sentence);
void esh_dependents_first_unique(const dls::tableau<word *> &sentence);

void lsup(const dls::tableau<word *> &words);
