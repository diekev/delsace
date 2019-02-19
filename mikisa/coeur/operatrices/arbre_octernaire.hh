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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "bibliotheques/geometrie/boite_englobante.hh"
#include "../corps/triangulation.hh"

class ArbreOcternaire {
public:
	struct Noeud {
		BoiteEnglobante boite{};

		int profondeur = 0;
		bool est_feuille = false;
		bool pad[3];

		Noeud *enfants[8] = {
			nullptr, nullptr, nullptr, nullptr,
			nullptr, nullptr, nullptr, nullptr
		};

		std::vector<Triangle> triangles{};

		~Noeud()
		{
			for (int i = 0; i < 8; ++i) {
				delete enfants[i];
			}
		}
	};

private:
	int m_profondeur_max = 4;
	Noeud m_racine;

public:
	ArbreOcternaire(BoiteEnglobante const &boite);

	void ajoute_triangle(Triangle const &triangle);

	void insert_triangle(Noeud *noeud, Triangle const &triangle, BoiteEnglobante const &boite_enfant);

	void construit_enfants(Noeud *noeud);

	Noeud *racine();
};

void rassemble_topologie(ArbreOcternaire::Noeud *noeud, Corps &corps);
