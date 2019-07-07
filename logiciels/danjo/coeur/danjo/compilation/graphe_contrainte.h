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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <vector>

#include "biblinternes/structures/chaine.hh"

#include "expression.h"

namespace danjo {

class Variable;

class contrainte {
public:
	std::vector<Variable *> m_variables{};
	std::vector<Symbole> m_expression{};
	Variable *m_sortie{};
	std::vector<Symbole> m_condition{};
};

class Variable {
public:
	std::vector<contrainte *> m_contraintes{};

	dls::chaine nom{}; // nom de la propriété du manipulable
	int degree{};
};

class graphe_contrainte {
	std::vector<contrainte *> m_contraintes{};
	std::vector<Variable *> m_variables{};

public:
	using iterateur_contrainte = std::vector<contrainte *>::iterator;
	using iterateur_contrainte_const = std::vector<contrainte *>::const_iterator;
	using iterateur_variable = std::vector<Variable *>::iterator;
	using iterateur_variable_const = std::vector<Variable *>::const_iterator;

	~graphe_contrainte();

	void ajoute_contrainte(contrainte *c);

	void ajoute_variable(Variable *v);

	/* Itérateurs contrainte. */

	iterateur_contrainte debut_contrainte();

	iterateur_contrainte fin_contrainte();

	iterateur_contrainte_const debut_contrainte() const;

	iterateur_contrainte_const fin_contrainte() const;

	/* Itérateurs variables. */

	iterateur_variable debut_variable();

	iterateur_variable fin_variable();

	iterateur_variable_const debut_variable() const;

	iterateur_variable_const fin_variable() const;
};

void connecte(contrainte *c, Variable *v);

void imprime_graphe(std::ostream &os, const graphe_contrainte &graphe);

}  /* namespace danjo */
