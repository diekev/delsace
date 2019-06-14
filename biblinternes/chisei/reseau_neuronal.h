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

#pragma once

#include "../math/matrice/matrice.hh"

#include <vector>

using Matrice = dls::math::matrice<double>;

enum TypeActivation {
	AUCUNE = 0,
	ULRE = 1,
	TANGENTE_HYPERBOLIQUE = 2,
	SIGMOIDE = 3,
};

enum TypeInitialisation {
	ZERO = 0,
	UNITE = 1,
	ALEATOIRE = 2,
};

struct CoucheReseau {
	CoucheReseau *precendente{}, *suivante{};

	Matrice biais{};  /* b */
	Matrice poids{};  /* W */
	Matrice neurones{};  /* x */

	TypeActivation activation{};

	int taille{};

	Matrice traduit(const Matrice &matrice);
};

class ReseauNeuronal {
	std::vector<CoucheReseau *> m_couches;

public:
	ReseauNeuronal() = default;
	~ReseauNeuronal();

	CoucheReseau *ajoute_entree(int taille_entree);

	CoucheReseau *ajoute_couche(CoucheReseau *couche_precedente, int taille_couche, TypeActivation activation);

	CoucheReseau *ajoute_sortie(CoucheReseau *couche_precedente, int taille_couche);

	void compile();

	void initialise_couches(TypeInitialisation initialisation);
};
