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

#include "postfix.h"

#include <iostream>

#include "biblinternes/structures/pile.hh"

#include "../../outils/conditions.h"

class Operator {
	int associativity;
	int precedence;

	enum {
		LEFT = 0,
		RIGHT = 1,
	};

public:
	explicit Operator(const char *op)
	    : associativity(LEFT)
	    , precedence(0)
	{
		switch (*op) {
			case '+':
			case '-':
				precedence = 0;
				associativity = LEFT;
				break;
			case 'x':
			case '*':
			case '/':
			case '%':
				precedence = 1;
				associativity = LEFT;
				break;
			case '^':
				precedence = 2;
				associativity = RIGHT;
				break;
		}
	}

	bool has_lower_precedence(const Operator &other) const
	{
		return (associativity == LEFT && precedence <= other.precedence)
		        || ((other.associativity == RIGHT) && (precedence < other.precedence));
	}
};

auto has_lower_precedence(const dls::chaine &o1, const dls::chaine &o2)
{
	const auto &op1 = Operator(o1.c_str());
	const auto &op2 = Operator(o2.c_str());

	return op1.has_lower_precedence(op2);
}

bool is_operator(const dls::chaine &token)
{
	return dls::outils::est_element(token, "+", "-", "/", "*", "^");
}

auto is_function(const dls::chaine &token)
{
	return dls::outils::est_element(token, "sqrt", "sin", "cos", "tan", "!");
}

auto is_operator(const char token)
{
	return dls::outils::est_element(token, '+', '-', '/', '*', '^');
}

auto split(const dls::chaine &source)
{
	dls::tableau<dls::chaine> result;

	for (auto i(0); i < source.taille(); ++i) {
		if (source[i] == ' ') {
			continue;
		}

		if (source[i] == '!') {
			result.emplace_back(1, source[i]);
			continue;
		}

		if (dls::outils::est_element(source[i], '(',  ')') || is_operator(source[i])) {
			result.emplace_back(1, source[i]);
			continue;
		}

		if (isdigit(source[i]) || source[i] == '.' || isalpha(source[i])) {
			auto next = i + 1;

			while (isdigit(source[next]) || source[next] == '.' || isalpha(source[next])) {
				++next;
			}

			result.pousse(source.sous_chaine(i, next - i));
			i += (next - i - 1);
			continue;
		}
	}

	return result;
}

auto postfix(const dls::chaine &expression) -> dls::file<dls::chaine>
{
	dls::file<dls::chaine> output;
	dls::pile<dls::chaine> stack;

	const auto &tokens = split(expression);

	for (const auto &token : tokens) {
		if (isdigit(token[0]) || token[0] == '.') {
			output.enfile(token);
			continue;
		}

		if (is_function(token)) {
			stack.empile(token);
			continue;
		}

		if (is_operator(token)) {
			while (   !stack.est_vide()
				   && is_operator(stack.haut())
				   && (has_lower_precedence(token, stack.haut())))
			{
				output.enfile(stack.depile());
			}

			stack.empile(token);
			continue;
		}

		if (token == "(") {
			stack.empile(token);
			continue;
		}

		if (token == ")") {
			if (stack.est_vide()) {
				std::cerr << "Parenthesis mismatch!\n";
				break;
			}

			while (stack.haut() != "(") {
				output.enfile(stack.depile());
			}

			// pop the left parenthesis from the stack
			if (stack.haut() == "(") {
				stack.depile();
			}

			continue;
		}
	}

	while (!stack.est_vide()) {
		if (stack.haut() == "(") {
			std::cerr << "Parenthesis mismatch!\n";
			break;
		}

		output.enfile(stack.depile());
	}

	return output;
}
