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

#include "biblinternes/ego/programme.h"
#include "biblinternes/ego/tampon_objet.h"
#include "biblinternes/ego/texture.h"

#include "biblinternes/math/matrice/matrice.hh"
#include "biblinternes/math/vecteur.hh"

class VueCanevas;

/**
 * La classe VisionneurImage contient la logique pour dessiner une image 2D avec
 * OpenGL dans une instance de VueCanevas.
 */
class VisionneurImage {
	VueCanevas *m_parent{};

	dls::ego::Programme m_program{};
	dls::ego::TamponObjet::Ptr m_buffer{};
	dls::ego::Texture2D::Ptr m_texture_R{};
	dls::ego::Texture2D::Ptr m_texture_G{};
	dls::ego::Texture2D::Ptr m_texture_B{};
	dls::ego::Texture2D::Ptr m_texture_A{};

	dls::tableau<float> m_donnees_r{};
	dls::tableau<float> m_donnees_g{};
	dls::tableau<float> m_donnees_b{};
	dls::tableau<float> m_donnees_a{};

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

	VisionneurImage(VisionneurImage const &) = default;
	VisionneurImage &operator=(VisionneurImage const &) = default;

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
	void charge_image(dls::math::matrice_dyn<dls::math::vec3d> const &image);
};
