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

#include <experimental/filesystem>
#include "biblinternes/math/vecteur.hh"
#include <vector>

class ContexteRendu;
class Koudou;
class Monde;
class TamponRendu;

/**
 * La classe RenduMonde contient la logique de rendu du monde de la scène 3D.
 */
class RenduMonde {
	TamponRendu *m_tampon = nullptr;
	Monde *m_monde{};

	std::vector<dls::math::vec3f> m_sommets{};
	std::vector<unsigned int> m_index{};

	/* Mémorisation des anciennes données. */
	int m_ancien_type{};
	std::experimental::filesystem::path m_ancien_chemin{};

public:
	/**
	 * Construit une instance de RenduMonde. La construction implique la
	 * création d'un tampon OpenGL, donc elle doit se faire dans un contexte
	 * OpenGL valide.
	 */
	explicit RenduMonde(Koudou *koudou);

	RenduMonde(RenduMonde const &) = default;
	RenduMonde &operator=(RenduMonde const &) = default;

	/**
	 * Détruit les données de l'instance. Les tampons de rendu sont détruits et
	 * utiliser l'instance crashera le programme.
	 */
	~RenduMonde();

	/**
	 * Dessine le monde dans le contexte spécifié.
	 */
	void dessine(ContexteRendu const &contexte);

	/**
	 * Ajourne les données du tampon.
	 */
	void ajourne();
};
