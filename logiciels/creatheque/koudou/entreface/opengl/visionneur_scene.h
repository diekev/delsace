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

#include "bibliotheques/opengl/contexte_rendu.h"
#include "bibliotheques/opengl/pile_matrice.h"

class Koudou;
class RenduCamera;
class RenduGrille;
class RenduLumiere;
class RenduMaillage;
class RenduMonde;
class RenduTexte;
class VueCanevas3D;

/**
 * La classe VisionneurScene contient la logique pour dessiner une scène 3D avec
 * OpenGL dans une instance de VueCanevas.
 */
class VisionneurScene {
	VueCanevas3D *m_parent;
	Koudou *m_koudou;

	RenduCamera *m_rendu_camera;
	RenduGrille *m_rendu_grille;
	RenduMonde *m_rendu_monde;
	RenduTexte *m_rendu_texte;

	std::vector<RenduMaillage *> m_maillages{};
	std::vector<RenduLumiere *> m_lumieres{};

	ContexteRendu m_contexte{};

	PileMatrice m_stack = {};

	double m_debut;

public:
	/**
	 * Empêche la construction d'un visionneur sans VueCanevas.
	 */
	VisionneurScene() = delete;

	/**
	 * Construit un visionneur avec un pointeur vers le VueCanevas parent, et un
	 * pointeur vers l'instance de Koudou du programme en cours.
	 */
	VisionneurScene(VueCanevas3D *parent, Koudou *koudou);

	/**
	 * Empêche la copie d'un visionneur.
	 */
	VisionneurScene(VisionneurScene const &visionneur) = delete;
	VisionneurScene &operator=(VisionneurScene const &) = default;

	/**
	 * Détruit le visionneur scène. Les tampons de rendus sont détruits, et
	 * utiliser cette instance après la destruction crashera le programme.
	 */
	~VisionneurScene();

	/**
	 * Crée les différents tampons de rendus OpenGL. Cette méthode est à appeler
	 * dans un contexte OpenGL valide.
	 */
	void initialise();

	/**
	 * Dessine la scène avec OpenGL.
	 */
	void peint_opengl();

	/**
	 * Redimensionne le visionneur selon la largeur et la hauteur spécifiées.
	 */
	void redimensionne(int largeur, int hauteur);

	void reconstruit_scene();
};
