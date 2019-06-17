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

#include "graphe_contrainte.h"

#include <iostream>

namespace danjo {

graphe_contrainte::~graphe_contrainte()
{
	imprime_graphe(std::cerr, *this);
	for (auto &v : m_variables) {
		delete v;
	}

	for (auto &c : m_contraintes) {
		delete c;
	}
}

void graphe_contrainte::ajoute_contrainte(contrainte *c)
{
	m_contraintes.push_back(c);
}

void graphe_contrainte::ajoute_variable(Variable *v)
{
	m_variables.push_back(v);
}

graphe_contrainte::iterateur_contrainte graphe_contrainte::debut_contrainte()
{
	return m_contraintes.begin();
}

graphe_contrainte::iterateur_contrainte graphe_contrainte::fin_contrainte()
{
	return m_contraintes.end();
}

graphe_contrainte::iterateur_contrainte_const graphe_contrainte::debut_contrainte() const
{
	return m_contraintes.cbegin();
}

graphe_contrainte::iterateur_contrainte_const graphe_contrainte::fin_contrainte() const
{
	return m_contraintes.cend();
}

graphe_contrainte::iterateur_variable graphe_contrainte::debut_variable()
{
	return m_variables.begin();
}

graphe_contrainte::iterateur_variable graphe_contrainte::fin_variable()
{
	return m_variables.end();
}

graphe_contrainte::iterateur_variable_const graphe_contrainte::debut_variable() const
{
	return m_variables.cbegin();
}

graphe_contrainte::iterateur_variable_const graphe_contrainte::fin_variable() const
{
	return m_variables.cend();
}

void connecte(contrainte *c, Variable *v)
{
	c->m_variables.push_back(v);
	v->m_contraintes.push_back(c);
}

void imprime_graphe(std::ostream &os, const graphe_contrainte &graphe)
{
	auto debut_contrainte = graphe.debut_contrainte();
	auto fin_contrainte = graphe.fin_contrainte();

	auto index = 0;

	os << "graph graphe_contrainte {\n";

	while (debut_contrainte != fin_contrainte) {
		contrainte *c = *debut_contrainte;

		for (Variable *v : c->m_variables) {
			//os << "C" << index << " (" << c->m_sortie->nom << ") -- " << v->nom << '\n';
			os << "\t" << c->m_sortie->nom << " -> " << v->nom << ";\n";
		}

		++index;
		++debut_contrainte;
	}

	os << "}\n";
}

}  /* namespace danjo */
