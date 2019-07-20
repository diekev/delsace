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

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico.hh"
#include "biblinternes/structures/tableau.hh"

#include "biblinternes/math/transformation.hh"

/**
 * Représentation d'un sommet dans l'espace tridimensionel.
 */
struct Sommet {
	dls::math::vec3f pos{};
	long index{};
};

struct Polygone;

/**
 * Représentation d'une arrête d'un polygone.
 */
struct Arrete {
	/* Les sommets connectés à cette arrête. */
	Sommet *s[2] = { nullptr, nullptr };

	/* Le polygone propriétaire de cette arrête. */
	Polygone *p = nullptr;

	/* L'arrête du polygone adjacent allant dans la direction opposée de
	 * celle-ci. */
	Arrete *opposee = nullptr;

	/* L'index de l'arrête dans la boucle d'arrêtes du polygone. */
	long index = 0;

	Arrete() = default;
};

/**
 * Représentation d'un quadrilatère et de son vecteur normal dans l'espace
 * tridimensionel.
 */
struct Polygone {
	/* Les sommets formant ce polygone. */
	Sommet *s[4] = { nullptr, nullptr, nullptr, nullptr };

	/* Les arrêtes formant ce polygone. */
	Arrete *a[4] = { nullptr, nullptr, nullptr, nullptr };

	/* Le vecteur normal de ce polygone. */
	dls::math::vec3f nor{};

	/* L'index de ce polygone. */
	long index = 0;

	/* La résolution UV de ce polygone. */
	unsigned int res_u = 0;
	unsigned int res_v = 0;

	int x = 0;
	int y = 0;

	Polygone() = default;
};

/**
 * La classe Maillage contient les polygones, les sommets, et les arrêtes
 * formant un objet dans l'espace tridimensionel.
 */
class Maillage {
	dls::tableau<Polygone *> m_polys{};
	dls::tableau<Sommet *> m_sommets{};
	dls::tableau<Arrete *> m_arretes{};

	dls::dico<std::pair<int, int>, Arrete *> m_tableau_arretes{};

	math::transformation m_transformation{};

	dls::chaine m_nom{};

	dls::math::vec3f m_min{};
	dls::math::vec3f m_max{};
	dls::math::vec3f m_taille{};

public:
	Maillage();

	~Maillage();

	/**
	 * Ajoute un sommet à ce maillage.
	 */
	void ajoute_sommet(dls::math::vec3f const &coord);

	/**
	 * Ajoute une suite de sommets à ce maillage.
	 */
	void ajoute_sommets(const dls::math::vec3f *sommets, long nombre);

	/**
	 * Retourne un pointeur vers le sommet dont l'index correspond à l'index
	 * passé en paramètre.
	 */
	const Sommet *sommet(long i) const;

	/**
	 * Retourne le nombre de sommets de ce maillage.
	 */
	long nombre_sommets() const;

	/**
	 * Ajoute un quadrilatère à ce maillage. Les paramètres sont les index des
	 * sommets déjà ajoutés à ce maillage.
	 */
	void ajoute_quad(const int s0, const int s1, const int s2, const int s3);

	/**
	 * Retourne le nombre de polygones de ce maillage.
	 */
	long nombre_polygones() const;

	/**
	 * Retourne un pointeur vers le polygone dont l'index correspond à l'index
	 * passé en paramètre.
	 */
	Polygone *polygone(long i);

	/**
	 * Retourne un pointeur vers le polygone dont l'index correspond à l'index
	 * passé en paramètre.
	 */
	const Polygone *polygone(long i) const;

	/**
	 * Retourne le nombre d'arrêtes de ce maillage.
	 */
	long nombre_arretes() const;

	/**
	 * Retourne un pointeur vers l'arrête dont l'index correspond à l'index
	 * passé en paramètre.
	 */
	Arrete *arrete(long i);

	/**
	 * Retourne un pointeur vers le polygone dont l'index correspond à l'index
	 * passé en paramètre.
	 */
	const Arrete *arrete(long i) const;

	/**
	 * Renseigne la transformation de ce maillage.
	 */
	void transformation(math::transformation const &transforme);

	/**
	 * Retourne la transformation de ce maillage.
	 */
	math::transformation const &transformation() const;

	/**
	 * Retourne le nom de ce maillage.
	 */
	dls::chaine const &nom() const;

	/**
	 * Renomme ce maillage.
	 */
	void nom(dls::chaine const &nom);

	void calcule_boite_englobante();

	dls::math::vec3f const &min() const;

	dls::math::vec3f const &max() const;

	dls::math::vec3f const &taille() const;
};
