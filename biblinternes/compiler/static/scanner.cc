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

#include "scanner.h"

#include <cstdlib>
#include <ctype.h>

Scanner::Scanner(const char *begin, const char *end) noexcept
    : m_next(begin)
    , m_end(end)
    , m_value(0.0f)
    , m_line(0)
    , m_token(Token::Invalid)
{}

void Scanner::read() noexcept
{
	skipWhiteSpace();

	if (m_next == m_end) {
		m_token = Token::EndOfFile;
		return;
	}

	const char c = *m_next;

	if (c == '+') {
		++m_next;
		m_token = Token::Add;
	}
	else if (c == '-') {
		++m_next;
		m_token = Token::Subtract;
	}
	else if (c == '*') {
		++m_next;
		m_token = Token::Multiply;
	}
	else if (c == '/') {
		++m_next;
		m_token = Token::Divide;
	}
	else if (c == '(') {
		++m_next;
		m_token = Token::LeftParenthesis;
	}
	else if (c == ')') {
		++m_next;
		m_token = Token::RightParenthesis;
	}
	else if (c == '.' || (c >= '0' && c <= '9')) {
		m_token = readNumber();
	}
	else {
		m_token = Token::Invalid;
	}
}

float Scanner::number() const noexcept
{
	return m_value;
}

Token Scanner::token() const noexcept
{
	return m_token;
}

void Scanner::skipWhiteSpace() noexcept
{
	while (m_next != m_end) {
		if (*m_next == '\n') {
			++m_next;
			++m_line;
		}
		else if (::isspace(*m_next)) {
			++m_next;
		}
		else {
			break;
		}
	}
}

Token Scanner::readNumber() noexcept
{
	auto digit = false;
	auto point = false;
	auto divide = 1.0f;
	m_value = 0.0f;

	for (char c = *m_next; m_next != m_end; c = *++m_next) {
		if (c >= '0' && c <= '9') {
			digit = true;
			m_value = 10.0f * m_value + static_cast<float>(c - '0');

			if (point) {
				divide *= 10.0f;
			}
		}
		else if (c == '.') {
			point = true;
		}
		else {
			break;
		}
	}

	if (digit) {
		m_value /= divide;
		return Token::Number;
	}

	return Token::Invalid;
}

float scan_expression(Scanner &scanner)
{
	auto value = scan_term(scanner);

	for (;;) {
		const auto &token = scanner.token();

		if (token == Token::Add) {
			scanner.read();
			value += scan_term(scanner);
		}
		else if (token == Token::Subtract) {
			scanner.read();
			value -= scan_term(scanner);
		}
		else if (token == Token::Invalid) {
			throw SyntaxException(SyntaxError::InvalidToken,
			                      scanner.line());
		}
		else {
			break;
		}
	}

	return value;
}

float scan_primary(Scanner &scanner)
{
	const auto &token = scanner.token();

	if (token == Token::Number) {
		const auto value = scanner.number();
		scanner.read();
		return value;
	}

	if (token == Token::LeftParenthesis) {
		scanner.read();

		const auto value = scan_expression(scanner);

		if (scanner.token() != Token::RightParenthesis) {
			throw SyntaxException(SyntaxError::RightParenthesisExpected,
			                      scanner.line());
		}

		scanner.read();
		return value;
	}

	throw SyntaxException(SyntaxError::PrimaryExpected,
	                      scanner.line());
}

float scan_term(Scanner &scanner)
{
	auto value = scan_primary(scanner);

	for (;;) {
		const auto &token = scanner.token();

		if (token == Token::Multiply) {
			scanner.read();
			value *= scan_primary(scanner);
		}
		else if (token == Token::Divide) {
			scanner.read();

			const auto other = scan_primary(scanner);
			if (other == 0.0f)
				throw SyntaxException(SyntaxError::DivideByZero,
			                          scanner.line());

			value /= other;
		}
		else {
			break;
		}
	}

	return value;
}
