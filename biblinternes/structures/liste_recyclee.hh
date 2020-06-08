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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/memoire/logeuse_memoire.hh"

namespace dls {

template <typename T>
struct liste_recyclee {
	struct noeud {
		noeud *suivant = nullptr;

		T donnees{};
	};

	struct ramasse_miette {
		noeud *premier = nullptr;
	};

	ramasse_miette rm{};
	noeud *premier = nullptr;
	noeud *dernier = nullptr;
	long m_taille = 0;

	~liste_recyclee()
	{
		while (premier != nullptr) {
			auto p = premier;
			premier = p->suivant;
			memoire::deloge("liste_recyclee::noeud", p);
		}

		premier = rm.premier;

		while (premier != nullptr) {
			auto p = premier;
			premier = p->suivant;
			memoire::deloge("liste_recyclee::noeud", p);
		}
	}

	void pousse(T const &donnees)
	{
		auto n = static_cast<noeud *>(nullptr);

		if (rm.premier) {
			n = rm.premier;
			rm.premier = rm.premier->suivant;
		}
		else {
			n = memoire::loge<noeud>("liste_recyclee::noeud");
		}

		n->donnees = donnees;

		m_taille += 1;

		if (dernier) {
			dernier->suivant = n;
		}
		else {
			premier = n;
		}

		dernier = n;
	}

	T effronte()
	{
		m_taille -= 1;
		auto p = premier;
		premier = premier->suivant;

		if (premier == nullptr) {
			dernier = nullptr;
		}

		// met dans le ramasse miette
		p->suivant = rm.premier;
		rm.premier = p;

		return p->donnees;
	}

	bool est_vide() const
	{
		return premier == nullptr;
	}

	long taille() const
	{
		return m_taille;
	}
};

}
