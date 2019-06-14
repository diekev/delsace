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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "xml.h"

#include <new>		// yes, this one new style header, is in the Android SDK.
#include <cstddef>

#include "element.h"
#include "noeud.h"
#include "paire_string.h"
#include "outils.h"

namespace dls {
namespace xml {

class iterateur_adelphe {
	Noeud *m_pointeur = nullptr;
	const char *m_nom = nullptr;

public:
	/* Construit l'itérateur de fin. */
	iterateur_adelphe() = default;

	/* Construit l'itérateur de début. */
	iterateur_adelphe(Noeud *noeud)
		: iterateur_adelphe(noeud, nullptr)
	{}

	/* Construit l'itérateur de début. */
	iterateur_adelphe(Noeud *noeud, const char *nom)
		: m_pointeur(noeud)
		, m_nom(nom)
	{}

	iterateur_adelphe &operator++()
	{
		m_pointeur = m_pointeur->NextSiblingElement(m_nom);
		return *this;
	}

	iterateur_adelphe &operator--()
	{
		m_pointeur = m_pointeur->PreviousSiblingElement(m_nom);
		return *this;
	}

	Noeud *pointeur() const
	{
		return m_pointeur;
	}

	Noeud *operator*()
	{
		return m_pointeur;
	}
};

bool operator==(const iterateur_adelphe &a, const iterateur_adelphe &b)
{
	return a.pointeur() == b.pointeur();
}

bool operator!=(const iterateur_adelphe &a, const iterateur_adelphe &b)
{
	return !(a == b);
}

iterateur_adelphe begin(const iterateur_adelphe &it)
{
	return it;
}

iterateur_adelphe end(const iterateur_adelphe &)
{
	return iterateur_adelphe();
}

void test()
{
//	for (Noeud *noeud : iterateur_adelphe(nullptr)) {

//	}
}

}  /* namespace xml */
}  /* namespace dls */
