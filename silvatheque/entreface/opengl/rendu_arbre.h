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

class Arbre;
class ContexteRendu;
class TamponRendu;

/**
 * La classe RenduArbre contient la logique de rendu d'un maillage dans la
 * scène 3D.
 */
class RenduArbre {
	TamponRendu *m_tampon_arrete = nullptr;

	Arbre *m_arbre = nullptr;

public:
	/**
	 * Construit une instance de RenduArbre pour le maillage spécifié.
	 */
	explicit RenduArbre(Arbre *arbre);

	RenduArbre(RenduArbre const &) = default;
	RenduArbre &operator=(RenduArbre const &) = default;

	/**
	 * Détruit les données de l'instance. Les tampons de rendu sont détruits et
	 * utiliser l'instance crashera le programme.
	 */
	~RenduArbre();

	void initialise();

	/**
	 * Dessine le maillage dans le contexte spécifié.
	 */
	void dessine(const ContexteRendu &contexte);
};
