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

#include "arbre.h"

#include "creation_arbre.h"

/* ************************************************************************** */

Arbre::Arbre()
	: m_transformation(numero7::math::mat4d::identity())
	, m_parametres(new Parametres)
{
	parametres_tremble(m_parametres);
	cree_arbre(this);
}

Arbre::~Arbre()
{
	delete m_parametres;
	reinitialise();
}

Arbre::Arbre(const Arbre &autre)
	: m_transformation(numero7::math::mat4d::identity())
	, m_parametres(new Parametres(*autre.m_parametres))
{
	cree_arbre(this);
}

void Arbre::ajoute_sommet(const numero7::math::vec3f &pos)
{
	Sommet *s = new Sommet;
	s->pos = pos;
	s->index = m_sommets.size();

	m_sommets.push_back(s);
}

void Arbre::ajoute_arrete(int s0, int s1)
{
	Arrete *a = new Arrete;
	a->s[0] = m_sommets[s0];
	a->s[1] = m_sommets[s1];

	m_arretes.push_back(a);
}

void Arbre::reinitialise()
{
	for (auto &s : m_sommets) {
		delete s;
	}

	m_sommets.clear();

	for (auto &a : m_arretes) {
		delete a;
	}

	m_arretes.clear();
}

const math::transformation &Arbre::transformation() const
{
	return m_transformation;
}

Arbre::plage_sommets Arbre::sommets() const
{
	return plage_sommets(m_sommets.begin(), m_sommets.end());
}

Arbre::plage_arretes Arbre::arretes() const
{
	return plage_arretes(m_arretes.begin(), m_arretes.end());
}

Parametres *Arbre::parametres()
{
	return m_parametres;
}
