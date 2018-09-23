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

#include <ego/program.h>
#include <ego/bufferobject.h>
#include <ego/texture.h>

#include <math/matrice/matrice.h>
#include <math/vec3.h>

class VueCanevas;

/**
 * La classe VisionneurImage contient la logique pour dessiner une image 2D avec
 * OpenGL dans une instance de VueCanevas.
 */
class VisionneurImage {
	VueCanevas *m_parent;

	numero7::ego::Program m_program;
	numero7::ego::BufferObject::Ptr m_buffer;
	numero7::ego::Texture2D::Ptr m_texture_R;
	numero7::ego::Texture2D::Ptr m_texture_G;
	numero7::ego::Texture2D::Ptr m_texture_B;
	numero7::ego::Texture2D::Ptr m_texture_A;

	std::vector<float> m_donnees_r;
	std::vector<float> m_donnees_g;
	std::vector<float> m_donnees_b;
	std::vector<float> m_donnees_a;

	const float m_vertices[8] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};

	const GLushort m_indices[6] = { 0, 1, 2, 0, 2, 3 };

	int m_hauteur = 0;
	int m_largeur = 0;

public:
	/**
	 * Empêche la construction d'un visionneur sans VueCanevas.
	 */
	VisionneurImage() = delete;

	/**
	 * Construit un visionneur avec un pointeur vers le VueCanevas parent.
	 */
	explicit VisionneurImage(VueCanevas *parent);

	/**
	 * Détruit le visionneur image. Les tampons de rendus sont détruits, et
	 * utiliser cette instance après la destruction crashera le programme.
	 */
	~VisionneurImage() = default;

	/**
	 * Crée les différents tampons de rendus OpenGL. Cette méthode est à appeler
	 * dans un contexte OpenGL valide.
	 */
	void initialise();

	/**
	 * Dessine l'image avec OpenGL.
	 */
	void peint_opengl();

	/**
	 * Redimensionne le visionneur selon la largeur et la hauteur spécifiées.
	 */
	void redimensionne(int largeur, int hauteur);

	/**
	 * Charge l'image spécifiée dans le visionneur. Les données de l'image sont
	 * copiées dans des tampons OpenGL pour le rendu.
	 */
	void charge_image(const numero7::math::matrice<numero7::math::vec3d> &image);
};
