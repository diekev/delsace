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

#include <iostream>
#include <stack>
#include <vector>

class Noeud;
enum class type_noeud : unsigned int;
struct DonneesMorceaux;

class assembleuse_arbre {
	std::stack<Noeud *> m_pile;
	std::vector<Noeud *> m_noeuds;

public:
	~assembleuse_arbre();

	Noeud *cree_noeud(type_noeud type, const DonneesMorceaux &donnees);

	void ajoute_noeud(type_noeud type, const DonneesMorceaux &donnees);

	void empile_noeud(type_noeud type, const DonneesMorceaux &donnees);

	void depile_noeud(type_noeud type);

	void escompte_type(type_noeud type);

	void imprime_arbre(std::ostream &os);
};
