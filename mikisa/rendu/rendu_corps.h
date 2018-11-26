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

class Corps;
class ContexteRendu;
class TamponRendu;

/**
 * La classe RenduCorps contient la logique de rendu d'un corps dans la
 * scène 3D.
 */
class RenduCorps {
	TamponRendu *m_tampon_points = nullptr;
	TamponRendu *m_tampon_polygones = nullptr;
	TamponRendu *m_tampon_segments = nullptr;

	Corps *m_corps = nullptr;

public:
	/**
	 * RenduCorps une instance de RenduMaillage pour le maillage spécifié.
	 */
	explicit RenduCorps(Corps *corps);

	/**
	 * Détruit les données de l'instance. Les tampons de rendu sont détruits et
	 * utiliser l'instance crashera le programme.
	 */
	~RenduCorps();

	void initialise();

	/**
	 * Dessine le maillage dans le contexte spécifié.
	 */
	void dessine(const ContexteRendu &contexte);
};
