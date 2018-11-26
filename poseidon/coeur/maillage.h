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

#include "bibliotheques/transformation/transformation.h"

/**
 * Représentation d'un sommet dans l'espace tridimensionel.
 */
struct Sommet {
	dls::math::vec3f pos;
	int index;
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
	char index = 0;

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
	dls::math::vec3f nor;

	/* L'index de ce polygone. */
	int index = 0;

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
	std::vector<Polygone *> m_polys;
	std::vector<Sommet *> m_sommets;
	std::vector<Arrete *> m_arretes;

	std::map<std::pair<int, int>, Arrete *> m_tableau_arretes;

	math::transformation m_transformation;

	std::string m_nom;

	dls::math::vec3f m_min, m_max, m_taille;

public:
	Maillage();

	~Maillage();

	/**
	 * Ajoute un sommet à ce maillage.
	 */
	void ajoute_sommet(const dls::math::vec3f &coord);

	/**
	 * Ajoute une suite de sommets à ce maillage.
	 */
	void ajoute_sommets(const dls::math::vec3f *sommets, size_t nombre);

	/**
	 * Retourne un pointeur vers le sommet dont l'index correspond à l'index
	 * passé en paramètre.
	 */
	const Sommet *sommet(size_t i) const;

	/**
	 * Retourne le nombre de sommets de ce maillage.
	 */
	size_t nombre_sommets() const;

	/**
	 * Ajoute un quadrilatère à ce maillage. Les paramètres sont les index des
	 * sommets déjà ajoutés à ce maillage.
	 */
	void ajoute_quad(const int s0, const int s1, const int s2, const int s3);

	/**
	 * Retourne le nombre de polygones de ce maillage.
	 */
	size_t nombre_polygones() const;

	/**
	 * Retourne un pointeur vers le polygone dont l'index correspond à l'index
	 * passé en paramètre.
	 */
	Polygone *polygone(size_t i);

	/**
	 * Retourne un pointeur vers le polygone dont l'index correspond à l'index
	 * passé en paramètre.
	 */
	const Polygone *polygone(size_t i) const;

	/**
	 * Retourne le nombre d'arrêtes de ce maillage.
	 */
	size_t nombre_arretes() const;

	/**
	 * Retourne un pointeur vers l'arrête dont l'index correspond à l'index
	 * passé en paramètre.
	 */
	Arrete *arrete(size_t i);

	/**
	 * Retourne un pointeur vers le polygone dont l'index correspond à l'index
	 * passé en paramètre.
	 */
	const Arrete *arrete(size_t i) const;

	/**
	 * Renseigne la transformation de ce maillage.
	 */
	void transformation(const math::transformation &transforme);

	/**
	 * Retourne la transformation de ce maillage.
	 */
	const math::transformation &transformation() const;

	/**
	 * Retourne le nom de ce maillage.
	 */
	const std::string &nom() const;

	/**
	 * Renomme ce maillage.
	 */
	void nom(const std::string &nom);

	void calcule_boite_englobante();

	const dls::math::vec3f &min() const;

	const dls::math::vec3f &max() const;

	const dls::math::vec3f &taille() const;
};
