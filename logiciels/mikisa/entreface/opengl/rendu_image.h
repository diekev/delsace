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

#include <math/matrice/matrice.h>
#include <image/pixel.h>

class ContexteRendu;
class TamponRendu;

class RenduImage {
	TamponRendu *m_tampon_image{};
	TamponRendu *m_tampon_bordure{};

public:
	/**
	 * Construit une instance de RenduImage. La construction implique la
	 * création de tampons OpenGL, donc elle doit se faire dans un contexte
	 * OpenGL valide.
	 */
	RenduImage();

	RenduImage(RenduImage const &) = default;
	RenduImage &operator=(RenduImage const &) = default;

	/**
	 * Détruit les données de l'instance. Les tampons de rendu sont détruits et
	 * utiliser l'instance crashera le programme.
	 */
	~RenduImage();

	void charge_image(const numero7::math::matrice<numero7::image::Pixel<float> > &image);

	/**
	 * Dessine l'image dans le contexte spécifié.
	 */
	void dessine(ContexteRendu const &contexte);
};

TamponRendu *cree_tampon_image();

void genere_texture_image(TamponRendu *tampon, const float *data, int size[2]);
