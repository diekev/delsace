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

#include "biblinternes/math/vecteur.hh"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/tableau.hh"

struct Corps;

/**
 * Structure en mi-arête inspirée de
 * http://www.flipcode.com/archives/The_Half-Edge_Data_Structure.shtml
 *
 * L'idée est d'avoir une structure comprenant des donnant d'adjacence pour les
 * points, arêtes, et polygones afin de pouvoir implémenter des algorithmes de
 * travail sur les maillages plus avancés et plus simplement que ce que nous
 * permet la structure Corps.
 */

/* ************************************************************************** */

enum class mi_drapeau : int {
	AUCUN      = 0,
	SUPPRIME   = (1 << 0),
	NOUVEAU    = (1 << 1),
	SOUSFACE   = (1 << 2),
	EXTERIEURE = (1 << 3),
	VALIDE     = (1 << 4),
};

inline auto operator&(mi_drapeau lhs, mi_drapeau rhs)
{
	return static_cast<mi_drapeau>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

inline auto operator&(mi_drapeau lhs, int rhs)
{
	return static_cast<mi_drapeau>(static_cast<int>(lhs) & rhs);
}

inline auto operator|(mi_drapeau lhs, mi_drapeau rhs)
{
	return static_cast<mi_drapeau>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline auto operator^(mi_drapeau lhs, mi_drapeau rhs)
{
	return static_cast<mi_drapeau>(static_cast<int>(lhs) ^ static_cast<int>(rhs));
}

inline auto operator~(mi_drapeau lhs)
{
	return static_cast<mi_drapeau>(~static_cast<int>(lhs));
}

inline auto &operator&=(mi_drapeau &lhs, mi_drapeau rhs)
{
	return (lhs = lhs & rhs);
}

inline auto &operator|=(mi_drapeau &lhs, mi_drapeau rhs)
{
	return (lhs = lhs | rhs);
}

inline auto &operator^=(mi_drapeau &lhs, mi_drapeau rhs)
{
	return (lhs = lhs ^ rhs);
}

/* ************************************************************************** */

struct mi_arete;
struct mi_sommet;
struct mi_face;

struct mi_sommet {
	/* la position dans l'espace du sommet */
	dls::math::vec3f p{};

	/* la mi-arête de ce sommet */
	mi_arete *arete = nullptr;

	/* l'index du sommet dans la liste des sommets du polyèdre */
	long index = 0;

	/* label pour les algorithmes, par exemple pour stocker un index d'origine */
	unsigned int label = 0;

	/* drapeaux divers */
	mi_drapeau drapeaux = mi_drapeau::AUCUN;

	mi_sommet() = default;

	COPIE_CONSTRUCT(mi_sommet);
};

struct mi_arete {
	/* sommet à la fin de la mi-arête */
	mi_sommet *sommet = nullptr;

	/* mi-arête adjacente opposément orientée */
	mi_arete *paire = nullptr;

	/* mi-face que la mi-arête borde */
	mi_face *face = nullptr;

	/* mi-arête suivante */
	mi_arete *suivante = nullptr;

	/* l'index de l'arête dans la liste des arêtes du polyèdre */
	long index = 0;

	/* label pour les algorithmes, par exemple pour stocker un index d'origine */
	unsigned int label = 0;

	/* drapeaux divers */
	mi_drapeau drapeaux = mi_drapeau::AUCUN;

	mi_arete() = default;

	COPIE_CONSTRUCT(mi_arete);
};

struct mi_face {
	/* l'une des arête de la face */
	mi_arete *arete = nullptr;

	/* l'index de la face dans la liste des faces du polyèdre */
	long index = 0;

	/* label pour les algorithmes, par exemple pour stocker un index d'origine */
	unsigned int label = 0;

	/* drapeaux divers */
	mi_drapeau drapeaux = mi_drapeau::AUCUN;

	mi_face() = default;

	COPIE_CONSTRUCT(mi_face);
};

struct Polyedre {
	dls::tableau<mi_sommet *> sommets{};
	dls::tableau<mi_arete *> aretes{};
	dls::tableau<mi_face *> faces{};

	Polyedre() = default;

	~Polyedre();

	mi_sommet *cree_sommet(dls::math::vec3f const &p);

	mi_arete *cree_arete(mi_sommet *s, mi_face *f);

	mi_face *cree_face();
};

mi_arete *suivante_autour_point(mi_arete *arete);

bool valide_polyedre(Polyedre const &polyedre);

Polyedre construit_corps_polyedre_triangle(Corps const &corps);

Polyedre converti_corps_polyedre(Corps const &corps);

void initialise_donnees(Polyedre &polyedre);

void converti_polyedre_corps(Polyedre const &polyedre, Corps &corps);

inline auto calcul_direction_normal(mi_arete *a)
{
	return produit_croix(
				a->suivante->sommet->p - a->sommet->p,
				a->suivante->suivante->sommet->p - a->sommet->p);
}
