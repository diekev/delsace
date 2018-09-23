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

#include <vector>
#include <map>

#include <numero7/math/matrice/matrice.h>
#include <numero7/math/point3.h>
#include <numero7/math/vec3.h>

#include "bibliotheques/outils/iterateurs.h"
#include "bibliotheques/transformation/transformation.h"

struct Parametres;

/**
 * Représentation d'un sommet dans l'espace tridimensionel.
 */
struct Sommet {
	numero7::math::vec3f pos;
	int index;
};

/**
 * Représentation d'une arrête d'un polygone.
 */
struct Arrete {
	/* Les sommets connectés à cette arrête. */
	Sommet *s[2] = { nullptr, nullptr };

	Arrete() = default;
};

class Arbre {
	std::vector<Sommet *> m_sommets;
	std::vector<Arrete *> m_arretes;

	math::transformation m_transformation;

	Parametres *m_parametres;

public:
	using plage_sommets = plage_iterable<std::vector<Sommet *>::const_iterator>;
	using plage_arretes = plage_iterable<std::vector<Arrete *>::const_iterator>;

	Arbre();

	~Arbre();

	Arbre(const Arbre &autre);

	Arbre &operator=(const Arbre &autre) = default;

	void ajoute_sommet(const numero7::math::vec3f &pos);

	void ajoute_arrete(int s0, int s1);

	void reinitialise();

	const math::transformation &transformation() const;

	plage_sommets sommets() const;

	plage_arretes arretes() const;

	Parametres *parametres();
};
