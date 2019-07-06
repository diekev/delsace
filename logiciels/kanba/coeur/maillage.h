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

#include "biblinternes/transformation/transformation.h"

#include "calques.h"

/**
 * Représentation d'un sommet dans l'espace tridimensionel.
 */
struct Sommet {
	dls::math::vec3f pos{};
	size_t index{};
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
	size_t index = 0;

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
	size_t index = 0;

	/* La résolution UV de ce polygone. */
	unsigned int res_u = 0;
	unsigned int res_v = 0;

	unsigned int x = 0;
	unsigned int y = 0;

	Polygone() = default;
};

/**
 * La classe Maillage contient les polygones, les sommets, et les arrêtes
 * formant un objet dans l'espace tridimensionel.
 */
class Maillage {
	std::vector<Polygone *> m_polys{};
	std::vector<Sommet *> m_sommets{};
	std::vector<Arrete *> m_arretes{};

	std::map<std::pair<int, int>, Arrete *> m_tableau_arretes{};

	math::transformation m_transformation{};

	bool m_texture_surrannee{};

	CanauxTexture m_canaux{};

	Calque *m_calque_actif = nullptr;

	unsigned int m_largeur_texture{};

	std::string m_nom{};

	Maillage(Maillage const &autre) = default;
	Maillage &operator=(Maillage const &autre) = default;

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
	void ajoute_quad(const size_t s0, const size_t s1, const size_t s2, const size_t s3);

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
	void transformation(math::transformation const &transforme);

	/**
	 * Retourne la transformation de ce maillage.
	 */
	math::transformation const &transformation() const;

	/**
	 * Crée un tampon PTex par défaut. À FAIRE : supprimer.
	 */
	void cree_tampon();

	/**
	 * Retourne vrai si la texture de ce maillage est surrannée et doit être
	 * redessinnée dans la vue 3D.
	 */
	bool texture_surrannee() const;

	/**
	 * Renseigne si la texture de ce maillage est surrannée et doit être
	 * redessinnée dans la vue 3D.
	 */
	void marque_texture_surrannee(bool ouinon);

	/**
	 * Retourne la largeur de la texture paquettée contenant les tampons de
	 * chaque polygone.
	 */
	unsigned int largeur_texture() const;

	/**
	 * Renseigne le calque actif de ce maillage.
	 */
	void calque_actif(Calque *calque);

	/**
	 * Retourne un pointeur vers le calque actif de ce maillage.
	 */
	Calque *calque_actif();

	/**
	 * Retourne une référence constante vers les données de canaux de ce maillage.
	 */
	CanauxTexture const &canaux_texture() const;

	/**
	 * Retourne une référence vers les données de canaux de ce maillage.
	 */
	CanauxTexture &canaux_texture();

	/**
	 * Retourne le nom de ce maillage.
	 */
	std::string const &nom() const;

	/**
	 * Renomme ce maillage.
	 */
	void nom(std::string const &nom);
};
