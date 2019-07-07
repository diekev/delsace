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

#include "assembleuse_logique.h"

#include <algorithm>

namespace danjo {

Variable *AssembleuseLogique::ajoute_variable(const dls::chaine &nom)
{
	auto var = new Variable;
	var->degree = 0;
	var->nom = nom;

	m_graphe.ajoute_variable(var);
	m_noms_variables.insere(nom);

	return var;
}

void AssembleuseLogique::ajoute_contrainte(contrainte *c)
{
	m_graphe.ajoute_contrainte(c);
}

bool AssembleuseLogique::variable_connue(const dls::chaine &nom)
{
	return m_noms_variables.trouve(nom) != m_noms_variables.fin();
}

Variable *AssembleuseLogique::variable(const dls::chaine &nom)
{
	auto debut = m_graphe.debut_variable();
	auto fin = m_graphe.fin_variable();

	auto iter = std::find_if(debut, fin, [&](const Variable *variable)
	{
		return variable->nom == nom;
	});

	if (iter == fin) {
		return nullptr;
	}

	return *iter;
}

}  /* namespace danjo */
