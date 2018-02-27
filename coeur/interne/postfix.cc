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

#include "morceaux.h"

namespace kangao {

struct DonneesVariables {
	int identifiant;
	double valeur;
	std::string nom_propriete;
};

struct DonneesExpression {
	std::string propriete_sortie;
	std::vector<DonneesVariables> donnees;
};

auto est_operateur(int identifiant)
{
	switch (identifiant) {
		case IDENTIFIANT_ADDITION:
		case IDENTIFIANT_DIVISION:
		case IDENTIFIANT_MULTIPLICATION:
		case IDENTIFIANT_SOUSTRACTION:
		case IDENTIFIANT_EGALITE:
		case IDENTIFIANT_INEGALITE:
		case IDENTIFIANT_INFERIEUR:
		case IDENTIFIANT_SUPERIEUR:
		case IDENTIFIANT_INFERIEUR_EGAL:
		case IDENTIFIANT_SUPERIEUR_EGAL:
		case IDENTIFIANT_ET:
		case IDENTIFIANT_OU:
		case IDENTIFIANT_OUX:
			return true;
		default:
			return false;
	}
}

auto est_operateur_logique(int identifiant)
{
	switch (identifiant) {
		case IDENTIFIANT_NON:
			return true;
		default:
			return false;
	}
}

auto evalue_operation(const double op1, const double op2, int identifiant)
{
	switch (identifiant) {
		case IDENTIFIANT_ADDITION:
			return op1 + op2;
		case IDENTIFIANT_SOUSTRACTION:
			return op1 - op2;
		case IDENTIFIANT_MULTIPLICATION:
			return op1 * op2;
		case IDENTIFIANT_DIVISION:
			return op1 / op2;
		case IDENTIFIANT_EGALITE:
			return static_cast<double>(op1 == op2);
		case IDENTIFIANT_INEGALITE:
			return static_cast<double>(op1 != op2);
		case IDENTIFIANT_INFERIEUR:
			return static_cast<double>(op1 < op2);
		case IDENTIFIANT_SUPERIEUR:
			return static_cast<double>(op1 > op2);
		case IDENTIFIANT_INFERIEUR_EGAL:
			return static_cast<double>(op1 <= op2);
		case IDENTIFIANT_SUPERIEUR_EGAL:
			return static_cast<double>(op1 >= op2);
		case IDENTIFIANT_ET:
			return static_cast<double>(static_cast<long int>(op1) & static_cast<long int>(op2));
		case IDENTIFIANT_OU:
			return static_cast<double>(static_cast<long int>(op1) | static_cast<long int>(op2));
		case IDENTIFIANT_OUX:
			return static_cast<double>(static_cast<long int>(op1) ^ static_cast<long int>(op2));
	}

	return 0.0;
}

auto evalue_operation_logique(const double op1, int identifiant)
{
	switch (identifiant) {
		case IDENTIFIANT_NON:
			return static_cast<double>(~static_cast<long int>(op1));
	}

	return 0.0;
}

auto evalue_expression(const std::vector<DonneesVariables> &expression) -> double
{
	std::stack<double> stack;

	/* Push a zero on the stack in case the expression starts with a negative
	 * number, or is empty. */
	stack.push(0);

	for (const DonneesVariables &donnees : expression) {
		if (est_operateur(donnees.identifiant)) {
			auto op1 = stack.top();
			stack.pop();

			auto op2 = stack.top();
			stack.pop();

			auto result = evalue_operation(op2, op1, donnees.identifiant);
			stack.push(result);

			continue;
		}
		else if (est_operateur_logique(donnees.identifiant)) {
			auto op1 = stack.top();
			stack.pop();

			auto result = evalue_operation_logique(op1, donnees.identifiant);
			stack.push(result);

			continue;
		}

	//	stack.push(std::stod(donnees));
	}

	return stack.top();
}

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

auto postfix(const std::string &expression) -> std::queue<std::string>
{
#if 0
	std::queue<std::string> output;
	std::stack<std::string> stack;

	const auto &tokens = split(expression);

	for (const auto &token : tokens) {
		if (isdigit(token[0]) || token[0] == '.') {
			output.push(token);
			continue;
		}

		if (est_fonction(token)) {
			stack.push(token);
			continue;
		}

		if (est_operateur(token)) {
			while (   !stack.empty()
				   && est_operateur(stack.top())
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
#else
	return std::queue<std::string>();
#endif
}

}  /* namespace kangao */
