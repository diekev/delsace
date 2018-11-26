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

#include "corps.h"
#include "listes.h"

class TextureImage;

/**
 * La classe Maillage contient les polygones, les sommets, et les arrêtes
 * formant un objet dans l'espace tridimensionel.
 */
class Maillage : public Corps {
	ListePolygones m_polys;
	ListePoints m_sommets;
	ListeSegments m_arretes;

	std::map<std::pair<int, int>, Arrete *> m_tableau_arretes;

	TextureImage *m_texture = nullptr;

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
	Sommet *sommet(size_t i);

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
	Polygone *ajoute_quad(const int s0, const int s1, const int s2, const int s3);

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

	void calcule_boite_englobante();

	void texture(TextureImage *t);

	TextureImage *texture() const;

	void reinitialise();

	void reserve_sommets(size_t nombre);

	void reserve_polygones(size_t nombre);

	ListePoints *liste_points();

	ListePolygones *liste_polys();

	ListeSegments *liste_segments();
};
