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

class ContexteRendu;
class TamponRendu;

/**
 * La classe RenduGrille contient la logique de rendu d'une grille dans la scène
 * 3D. La grille jouant le rôle de repère pour savoir où l'on se trouve dans la
 * scène.
 */
class RenduGrille {
	TamponRendu *m_tampon_grille = nullptr;
	TamponRendu *m_tampon_axe_x = nullptr;
	TamponRendu *m_tampon_axe_z = nullptr;

public:
	/**
	 * Construit une grille selon les dimensions spécifiées. La construction
	 * implique la création de tampons OpenGL, donc elle doit se faire dans un
	 * contexte OpenGL valide.
	 */
	RenduGrille(int largeur, int hauteur);

	RenduGrille(RenduGrille const &) = default;
	RenduGrille &operator=(RenduGrille const &) = default;

	/**
	 * Détruit les données de l'instance. Les tampons de rendu sont détruits et
	 * utiliser l'instance crashera le programme.
	 */
	~RenduGrille();

	/**
	 * Dessine la grille dans le contexte spécifié.
	 */
	void dessine(ContexteRendu const &contexte);
};
