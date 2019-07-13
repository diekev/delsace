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

#include "learner.h"

#include <fstream>
#include <iostream>

void learner::respond(const dls::chaine &phrase)
{
	std::ostream &os = std::cout;

	std::fstream memory;
	memory.open("memory.txt", std::ios::in);

	while (memory.is_open() && !memory.eof()) {
		std::string identifier;
		std::getline(memory, identifier);

		if (identifier == phrase) {
			std::string response;
			std::getline(memory, response);
			say(response);
			return;
		}
	}

	memory.close();

	/* If we don't have the phrase in the database, repeat it to the user to get
	 * the answer from them.
	 */

	memory.open("memory.txt", std::ios::out | std::ios::app);
	memory << phrase << '\n';

	say(phrase);
	os << "YOU: ";

	std::string response;
	std::getline(std::cin, response);

	memory << response << '\n';
	memory.close();
}

void learner::say(const dls::chaine &phrase)
{
	std::ostream &os = std::cout;
	os << phrase << '\n';
}
