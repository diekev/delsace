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

#pragma once

#include "biblinternes/structures/chaine.hh"

class InvalidNumberExcetion : public std::exception {
	dls::chaine m_what;

public:
	InvalidNumberExcetion() = delete;

	explicit InvalidNumberExcetion(int n);

	~InvalidNumberExcetion() = default;

	const char *what() const noexcept override;
};

enum class hangman_player : char {
	computer = 0,
	human    = 1
};

auto to_words(int number) -> dls::chaine;
auto hangman(std::ostream &os, std::istream &is, hangman_player player) -> void;
