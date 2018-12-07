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

#include <queue>
#include <vector>

struct Polygone;

/**
 * La classe PaqueuseTexture utilise un arbre binaire pour paqueter les textures
 * des polygones dans un seul atlas texture et assigne à ceux-ci les coordonnées
 * de leurs textures respectives dans l'atlas.
 *
 * L'algorithme est basé sur https://codeincomplete.com/posts/bin-packing/
 */
class PaqueuseTexture {
	struct Noeud {
		Noeud *droite = nullptr;
		Noeud *gauche = nullptr;

		bool utilise = false;
		unsigned x = 0;
		unsigned y = 0;
		unsigned largeur = 0;
		unsigned hauteur = 0;

		Noeud() = default;

		Noeud(Noeud const &) = default;
		Noeud &operator=(Noeud const &) = default;

		~Noeud();
	};

	struct CompareNoeud {
		bool operator()(const Noeud *a, const Noeud *b)
		{
			return a->largeur < b->largeur || a->hauteur < b->hauteur;
		}
	};

	Noeud *m_racine = nullptr;

	std::priority_queue<Noeud *, std::vector<Noeud *>, CompareNoeud> m_queue_priorite{};

	unsigned int max_x = 0;
	unsigned int max_y = 0;

public:
	PaqueuseTexture();

	~PaqueuseTexture();

	PaqueuseTexture(const PaqueuseTexture &) = delete;
	PaqueuseTexture &operator=(PaqueuseTexture const &) = delete;

	/**
	 * Démarre l'empaquetage des polygones spécifiés.
	 */
	void empaquete(const std::vector<Polygone *> &polygones);

	/**
	 * Retourne la largeur de la texture empaquettée.
	 */
	unsigned int largeur() const;

	/**
	 * Retourne la hauteur de la texture empaquettée.
	 */
	unsigned int hauteur() const;

private:
	/**
	 * Trouve un noeud ayant suffisament d'espace pour contenir une texture
	 * dont la largeur et la hauteur sont spécifiées en paramètre.
	 */
	Noeud *trouve_noeud(Noeud *racine, unsigned largeur, unsigned hauteur);

	/**
	 * Brise un noeud pour contenir la texture dont la largeur et la hauteur
	 * sont spécifiées en paramètre.
	 */
	Noeud *brise_noeud(Noeud *noeud, unsigned largeur, unsigned hauteur);

	/**
	 * Élargi la racine pour faire de la place pour la texture dont la largeur
	 * et la hauteur sont spécifiées en paramètre.
	 */
	Noeud *elargi_noeud(unsigned largeur, unsigned hauteur);

	/**
	 * Élargi la largeur racine pour faire de la place pour la texture dont la
	 * largeur et la hauteur sont spécifiées en paramètre.
	 */
	Noeud *elargi_largeur(unsigned largeur, unsigned hauteur);

	/**
	 * Élargi la hauteur racine pour faire de la place pour la texture dont la
	 * largeur et la hauteur sont spécifiées en paramètre.
	 */
	Noeud *elargi_hauteur(unsigned largeur, unsigned hauteur);
};
