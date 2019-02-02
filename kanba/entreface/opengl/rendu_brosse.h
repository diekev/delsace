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
 * La classe RenduBrosse contient la logique de rendu de la rendu d'une brosse
 * sur l'écran.
 */
class RenduBrosse {
	TamponRendu *m_tampon_contour = nullptr;

public:
	/**
	 * Construit une instance de RenduBrosse avec des valeurs par défaut.
	 */
	RenduBrosse() = default;

	RenduBrosse(RenduBrosse const &) = default;
	RenduBrosse &operator=(RenduBrosse const &) = default;

	/**
	 * Détruit les données de l'instance. Les tampons de rendu sont détruits et
	 * utiliser l'instance crashera le programme.
	 */
	~RenduBrosse();

	/**
	 * Initialise le contenu du tampon.
	 */
	void initialise();

	/**
	 * Dessine la grille dans le contexte spécifié.
	 */
	void dessine(
			ContexteRendu const &contexte,
			const float taille_x,
			const float taille_y,
			const float pos_x,
			const float pos_y);
};
