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

void imprime_pile(CompileuseGraphe const &compileur, std::ostream &os = std::cout)
{
	auto debut = compileur.debut();
	auto fin = compileur.fin();

	os << "Pile :\n";

	while (debut != fin && static_cast<int>(*debut) != -1) {
		os << *debut << '\n';
		++debut;
	}
}

/* *************************************************************************** */

CompileuseGraphe::CompileuseGraphe()
	: m_pile(TAILLE_PILE, -1)
	, m_decalage(0)
{}

void CompileuseGraphe::ajoute_noeud(dls::math::vec3f const &v)
{
	this->ajoute_noeud(v.x, v.y, v.z);
}

long CompileuseGraphe::decalage_pile(PriseSortie *prise)
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

long CompileuseGraphe::decalage_pile(type_prise tprise)
{
	auto d = m_decalage;

	switch (tprise) {
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

	return d;
}

int CompileuseGraphe::decalage_pile(int taille)
{
	auto d = m_decalage;

	for (auto i = 0; i < taille; ++i) {
		m_pile[m_decalage++] = 0;
	}

	return static_cast<int>(d);
}

void CompileuseGraphe::stocke_decimal(long decalage, float const &v)
{
	m_pile[decalage++] = v;
}

void CompileuseGraphe::stocke_vec3f(long decalage, dls::math::vec3f const &v)
{
	m_pile[decalage++] = v.x;
	m_pile[decalage++] = v.y;
	m_pile[decalage++] = v.z;
}

CompileuseGraphe::iterateur CompileuseGraphe::debut()
{
	return m_pile.debut();
}

CompileuseGraphe::iterateur CompileuseGraphe::fin()
{
	return m_pile.fin();
}

CompileuseGraphe::iterateur_const CompileuseGraphe::debut() const
{
	return m_pile.debut();
}

CompileuseGraphe::iterateur_const CompileuseGraphe::fin() const
{
	return m_pile.fin();
}

dls::tableau<float> CompileuseGraphe::pile()
{
	return m_pile;
}
