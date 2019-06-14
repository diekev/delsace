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

enum class Token {
	Invalid,
	EndOfFile,

	Number,
	Add,
	Subtract,
	Multiply,
	Divide,
	LeftParenthesis,
	RightParenthesis,
};

enum class SyntaxError {
	InvalidToken,
	PrimaryExpected,
	DivideByZero,
	RightParenthesisExpected,
	UnexpectedToken,
};

struct SyntaxException {
	SyntaxError error;
    unsigned line;

     SyntaxException(const SyntaxError &err, const unsigned l)
	     : error(err)
	     , line(l)
     {}
};

class Scanner {
	const char *m_next;
	const char * const m_end;
	float m_value;
	unsigned m_line;
	Token m_token;

public:
	Scanner(const char *begin, const char *end) noexcept;
	~Scanner() = default;

	Scanner(const Scanner &) = delete;
	Scanner &operator=(const Scanner &) = delete;

	void read() noexcept;

	float number() const noexcept;
	unsigned line() const noexcept { return m_line + 1; }

	Token token() const noexcept;

private:
	void skipWhiteSpace() noexcept;
	Token readNumber() noexcept;
};

float scan_expression(Scanner &scanner);
float scan_primary(Scanner &scanner);
float scan_term(Scanner &scanner);
