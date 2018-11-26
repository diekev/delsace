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

#include <delsace/math/matrice.hh>

#include <map>
#include <vector>

class ContexteRendu;
class Maillage;
class TamponRendu;

/**
 * La struct Page sert à stocker les données pour peindre un certains nombre de
 * polygones et leurs textures associées. Le nombre maximum de polygones dans
 * une page est déterminé par GL_MAX_ARRAY_TEXTURE_LAYERS_EXT, et chaque ne
 * contient que des textures ayant les mêmes résolutions.
 */
struct Page {
	TamponRendu *tampon;
	std::vector<uint> polys;
};

/**
 * La classe RenduMaillage contient la logique de rendu d'un maillage dans la
 * scène 3D.
 */
class RenduMaillage {
	TamponRendu *m_tampon_arrete = nullptr;
	TamponRendu *m_tampon_normal = nullptr;

	Maillage *m_maillage = nullptr;

	std::vector<Page> m_pages;

public:
	/**
	 * Construit une instance de RenduMaillage pour le maillage spécifié.
	 */
	explicit RenduMaillage(Maillage *maillage);

	/**
	 * Détruit les données de l'instance. Les tampons de rendu sont détruits et
	 * utiliser l'instance crashera le programme.
	 */
	~RenduMaillage();

	void initialise();

	/**
	 * Dessine le maillage dans le contexte spécifié.
	 */
	void dessine(const ContexteRendu &contexte);

	/**
	 * Retourne la matrice du maillage.
	 */
	dls::math::mat4x4d matrice() const;

	Maillage *maillage() const;

private:
	void ajourne_texture();

	void supprime_tampons();
};
