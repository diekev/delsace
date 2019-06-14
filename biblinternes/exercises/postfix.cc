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

#include "postfix.h"

#include <cmath>
#include <iostream>
#include <stack>
#include <vector>

#include "../outils/conditions.h"

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

auto has_lower_precedence(const std::string &o1, const std::string &o2)
{
	const auto &op1 = Operator(o1.c_str());
	const auto &op2 = Operator(o2.c_str());

	return op1.has_lower_precedence(op2);
}

auto is_operator(const std::string &token)
{
	return dls::outils::est_element(token, "+", "-", "/", "*", "^");
}

auto is_function(const std::string &token)
{
	return dls::outils::est_element(token, "sqrt", "sin", "cos", "tan", "!");
}

auto is_operator(const char token)
{
	return dls::outils::est_element(token, '+', '-', '/', '*', '^');
}

auto split(const std::string &source)
{
	std::vector<std::string> result;

	for (size_t i(0); i < source.size(); ++i) {
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

			result.push_back(source.substr(i, next - i));
			i += (next - i - 1);
			continue;
		}
	}

	return result;
}

auto postfix(const std::string &expression) -> std::queue<std::string>
{
	std::queue<std::string> output;
	std::stack<std::string> stack;

	const auto &tokens = split(expression);

	for (const auto &token : tokens) {
		if (isdigit(token[0]) || token[0] == '.') {
			output.push(token);
			continue;
		}

		if (is_function(token)) {
			stack.push(token);
			continue;
		}

		if (is_operator(token)) {
			while (   !stack.empty()
			       && is_operator(stack.top())
			       && (has_lower_precedence(token, stack.top())))
			{
				output.push(stack.top());
				stack.pop();
			}

			stack.push(token);
			continue;
		}

		if (token == "(") {
			stack.push(token);
			continue;
		}

		if (token == ")") {
			if (stack.empty()) {
				std::cerr << "Parenthesis mismatch!\n";
				break;
			}

			while (stack.top() != "(") {
				output.push(stack.top());
				stack.pop();
			}

			// pop the left parenthesis from the stack
			if (stack.top() == "(") {
				stack.pop();
			}

			continue;
		}
	}

	while (!stack.empty()) {
		if (stack.top() == "(") {
			std::cerr << "Parenthesis mismatch!\n";
			break;
		}

		output.push(stack.top());
		stack.pop();
	}

	return output;
}

auto evaluate(const double op1, const double op2, const char *op)
{
	switch (*op) {
		case '+':
			return op1 + op2;
		case '-':
			return op1 - op2;
		case '*':
			return op1 * op2;
		case '/':
			return op1 / op2;
		case '^':
			return std::pow(op1, op2);
	}

	return 0.0;
}

auto factorial(double value)
{
	if (value == 1.0) {
		return 1.0;
	}

	return value * factorial(value - 1.0);
}

auto evaluate(const double value, const std::string &fn)
{
	if (fn == "sqrt") {
		return sqrt(value);
	}
	else if (fn == "sin") {
		return sin(value);
	}
	else if (fn == "cos") {
		return cos(value);
	}
	else if (fn == "tan") {
		return tan(value);
	}
	else if (fn == "!") {
		return factorial(value);
	}

	return 0.0;
}

auto evaluate_postfix(std::queue<std::string> &expression) -> double
{
	std::stack<double> stack;

	/* Push a zero on the stack in case the expression starts with a negative
	 * number, or is empty. */
	stack.push(0);

	while (!expression.empty()) {
		auto token = expression.front();
		expression.pop();

		if (is_operator(token)) {
			auto op1 = stack.top();
			stack.pop();

			auto op2 = stack.top();
			stack.pop();

			auto result = evaluate(op2, op1, token.c_str());
			stack.push(result);

			continue;
		}
		else if (is_function(token)) {
			auto op1 = stack.top();
			stack.pop();

			auto result = evaluate(op1, token);
			stack.push(result);

			continue;
		}

		stack.push(std::stod(token));
	}

	return stack.top();
}
