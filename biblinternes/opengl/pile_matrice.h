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

#include "biblinternes/math/matrice.hh"
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
	std::stack<dls::math::mat4x4d, std::vector<dls::math::mat4x4d>> m_pile{};

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
	inline void pousse(dls::math::mat4x4d const &mat);

	/**
	 * Fais sauter le sommet de la pile de celle-ci.
	 */
	inline void enleve_sommet();

	/**
	 * Retourne la matrice se trouvant au sommet de la pile.
	 */
	inline dls::math::mat4x4d const &sommet() const;
};

/* Implémentation des méthodes inlignées. */

inline void PileMatrice::pousse(dls::math::mat4x4d const &mat)
{
	m_pile.push(m_pile.top() * mat);
}

inline void PileMatrice::enleve_sommet()
{
	m_pile.pop();
}

inline dls::math::mat4x4d const &PileMatrice::sommet() const
{
	return m_pile.top();
}

