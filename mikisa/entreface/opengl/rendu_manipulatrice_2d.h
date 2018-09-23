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

class RenduManipulatrice2D {
	TamponRendu *m_tampon;

public:
	/**
	 * Construit une instance de RenduManipulatrice2D. La construction implique la
	 * création de tampons OpenGL, donc elle doit se faire dans un contexte
	 * OpenGL valide.
	 */
	RenduManipulatrice2D();

	/* pour faire taire cppcheck */
	RenduManipulatrice2D(const RenduManipulatrice2D &) = delete;

	/**
	 * Détruit les données de l'instance. Les tampons de rendu sont détruits et
	 * utiliser l'instance crashera le programme.
	 */
	~RenduManipulatrice2D();

	/**
	 * Dessine la manipulatrice dans le contexte spécifié.
	 */
	void dessine(const ContexteRendu &contexte);
};
