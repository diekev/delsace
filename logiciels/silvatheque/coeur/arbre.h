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

#include "biblinternes/structures/tableau.hh"

#include "biblinternes/outils/iterateurs.h"
#include "biblinternes/math/transformation.hh"

struct Parametres;

/**
 * Représentation d'un sommet dans l'espace tridimensionel.
 */
struct Sommet {
	dls::math::vec3f pos{};
	long index{};
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
	dls::tableau<Sommet *> m_sommets{};
	dls::tableau<Arrete *> m_arretes{};

	math::transformation m_transformation{};

	Parametres *m_parametres{};

public:
	using plage_sommets = dls::outils::plage_iterable<dls::tableau<Sommet *>::const_iteratrice>;
	using plage_arretes = dls::outils::plage_iterable<dls::tableau<Arrete *>::const_iteratrice>;

	Arbre();

	~Arbre();

	Arbre(Arbre const &autre);

	Arbre &operator=(Arbre const &autre) = default;

	void ajoute_sommet(dls::math::vec3f const &pos);

	void ajoute_arrete(int s0, int s1);

	void reinitialise();

	math::transformation const &transformation() const;

	plage_sommets sommets() const;

	plage_arretes arretes() const;

	Parametres *parametres();
};
