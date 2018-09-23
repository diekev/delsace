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

#include <glm/glm.hpp>
#include <stack>
#include <vector>

/**
 * La classe PileMatrice gère l'accumulation et la composition de matrices dans
 * une pile pour le rendu des objets dans une scène OpenGL 3D.
 *
 * À chaque qu'une matrice est poussée sur la pile, elle est multipliée avec la
 * matrice se trouvant alors au sommet de la pile afin de créer la matrice
 * pour dessiner l'objet courant.
 */
class PileMatrice {
	std::stack<glm::mat4, std::vector<glm::mat4>> m_pile;

public:
	/**
	 * Construit une instance de PileMatrice. Par défaut une matrice identité
	 * est poussée sur la pile.
	 */
	PileMatrice();

	/**
	 * Pousse la matrice spécifiée sur la pile. Le somment de la pile sera égal
	 * à la matrice alors au sommet multipliée par la matrice passée en
	 * paramètre.
	 */
	inline void pousse(const glm::mat4 &mat);

	/**
	 * Fais sauter le sommet de la pile de celle-ci.
	 */
	inline void enleve_sommet();

	/**
	 * Retourne la matrice se trouvant au sommet de la pile.
	 */
	inline const glm::mat4 &sommet() const;
};

/* Implémentation des méthodes inlignées. */

inline void PileMatrice::pousse(const glm::mat4 &mat)
{
	m_pile.push(m_pile.top() * mat);
}

inline void PileMatrice::enleve_sommet()
{
	m_pile.pop();
}

inline const glm::mat4 &PileMatrice::sommet() const
{
	return m_pile.top();
}

