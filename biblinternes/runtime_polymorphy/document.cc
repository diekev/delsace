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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "document.h"

#include <cassert>

// draw a document
void draw(const document_t &x, std::ostream &out, size_t position)
{
	out << std::string(position, ' ') <<  "\033[0;32m" << "<document>\n";

	for (const auto &e : x) {
		draw(e, out, position + 4);
	}

	out << std::string(position, ' ') << "\033[0;32m" << "</document>\n";
}

// history

void commit(history_t &x)
{
	assert(x.size());
	x.push_back(x.back());
}

void undo(history_t &x)
{
	x.pop_back();
}

document_t &current(history_t &x)
{
	assert(x.size());
	return x.back();
}

// custom draw for a my_class_t
void draw(const my_class_t &, std::ostream &out, size_t position)
{
	out << std::string(position, ' ') << "\033[0m" << "my_class_t\n";
}

void test_history()
{
	history_t h(1);

	current(h).emplace_back(0);
	current(h).emplace_back(std::string("Hello !"));

	draw(current(h), std::cout, 0);
	std::cout << "\033[0;33m" << "<--------------------------\n";

	commit(h);

	current(h).emplace_back(current(h));
	current(h).emplace_back(my_class_t());
	current(h)[1] = object_t(std::string("World"));

	draw(current(h), std::cout, 0);
	std::cout << "\033[0;33m" << "<--------------------------\n";

	undo(h);

	draw(current(h), std::cout, 0);

	std::cout << "\033[0m";
}
