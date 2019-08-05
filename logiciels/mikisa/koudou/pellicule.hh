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

#include "biblinternes/math/matrice/matrice.hh"
#include "biblinternes/math/vecteur.hh"

#include "biblinternes/structures/tableau.hh"

namespace kdo {

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
	dls::math::matrice_dyn<dls::math::vec3d> m_matrice;

	dls::tableau<PixelPellicule> m_pixels_pellicule{};

public:
	Pellicule();

	int hauteur() const;

	int largeur() const;

	void ajoute_echantillon(long i, long j, dls::math::vec3d const &couleur, const double poids = 1.0);

	dls::math::vec3d const &couleur(int i, int j);

	dls::math::matrice_dyn<dls::math::vec3d> const &donnees();

	void reinitialise();

	void redimensionne(dls::math::Hauteur const &hauteur, dls::math::Largeur const &largeur);

	void creer_image();
};

}  /* namespace kdo */
