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

#include "compileuse_graphe.h"

#include "noeud.h"

/* *************************************************************************** */

CompileuseGraphe::CompileuseGraphe()
	: m_pile(TAILLE_PILE, -1)
	, m_decalage(0)
{}

void CompileuseGraphe::ajoute_noeud(float x)
{
	if (m_decalage + 1 >= TAILLE_PILE) {
		throw "Il n'y a plus de place dans la pile !";
	}

	m_pile[m_decalage++] = x;
}

void CompileuseGraphe::ajoute_noeud(float x, float y)
{
	if (m_decalage + 2 >= TAILLE_PILE) {
		throw "Il n'y a plus de place dans la pile !";
	}

	m_pile[m_decalage++] = x;
	m_pile[m_decalage++] = y;
}

void CompileuseGraphe::ajoute_noeud(float x, float y, float z)
{
	if (m_decalage + 3 >= TAILLE_PILE) {
		throw "Il n'y a plus de place dans la pile !";
	}

	m_pile[m_decalage++] = x;
	m_pile[m_decalage++] = y;
	m_pile[m_decalage++] = z;
}

void CompileuseGraphe::ajoute_noeud(const numero7::math::vec3f &v)
{
	if (m_decalage + 3 >= TAILLE_PILE) {
		throw "Il n'y a plus de place dans la pile !";
	}

	m_pile[m_decalage++] = v.x;
	m_pile[m_decalage++] = v.y;
	m_pile[m_decalage++] = v.z;
}

void CompileuseGraphe::ajoute_noeud(float x, float y, float z, float w)
{
	if (m_decalage + 4 >= TAILLE_PILE) {
		throw "Il n'y a plus de place dans la pile !";
	}

	m_pile[m_decalage++] = x;
	m_pile[m_decalage++] = y;
	m_pile[m_decalage++] = z;
	m_pile[m_decalage++] = w;
}

size_t CompileuseGraphe::decalage_pile(PriseSortie *prise)
{
	if (prise == nullptr) {
		return 0;
	}

	if (prise->decalage_pile == 0) {
		prise->decalage_pile = m_decalage;

		switch (prise->type) {
			default:
			case type_prise::DECIMAL:
			case type_prise::ENTIER:
				m_pile[m_decalage++] = 0;
				break;
			case type_prise::VECTEUR:
				m_pile[m_decalage++] = 0;
				m_pile[m_decalage++] = 0;
				m_pile[m_decalage++] = 0;
				break;
			case type_prise::COULEUR:
				m_pile[m_decalage++] = 0;
				m_pile[m_decalage++] = 0;
				m_pile[m_decalage++] = 0;
				m_pile[m_decalage++] = 0;
				break;
		}
	}

	return prise->decalage_pile;
}

void CompileuseGraphe::stocke_decimal(size_t decalage, const float &v)
{
	m_pile[decalage++] = v;
}

void CompileuseGraphe::stocke_vec3f(size_t decalage, const numero7::math::vec3f &v)
{
	m_pile[decalage++] = v.x;
	m_pile[decalage++] = v.y;
	m_pile[decalage++] = v.z;
}

CompileuseGraphe::iterateur CompileuseGraphe::debut()
{
	return m_pile.begin();
}

CompileuseGraphe::iterateur CompileuseGraphe::fin()
{
	return m_pile.end();
}

CompileuseGraphe::iterateur_const CompileuseGraphe::debut() const
{
	return m_pile.cbegin();
}

CompileuseGraphe::iterateur_const CompileuseGraphe::fin() const
{
	return m_pile.cend();
}

std::vector<float> CompileuseGraphe::pile()
{
	return m_pile;
}
