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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <math/matrice/matrice.h>
#include <delsace/math/vecteur.hh>

#include <vector>

struct CarreauPellicule {
	unsigned int x;
	unsigned int y;
	unsigned int largeur;
	unsigned int hauteur;
};

struct PixelPellicule {
	dls::math::vec3d couleur{};
	double poids{};
};

class Pellicule {
	numero7::math::matrice<dls::math::vec3d> m_matrice;

	std::vector<PixelPellicule> m_pixels_pellicule{};

public:
	Pellicule();

	int hauteur() const;

	int largeur() const;

	void ajoute_echantillon(size_t i, size_t j, dls::math::vec3d const &couleur, const double poids = 1.0);

	dls::math::vec3d const &couleur(int i, int j);

	numero7::math::matrice<dls::math::vec3d> const &donnees();

	void reinitialise();

	void redimensionne(numero7::math::Hauteur const &hauteur, numero7::math::Largeur const &largeur);

	void creer_image();
};
