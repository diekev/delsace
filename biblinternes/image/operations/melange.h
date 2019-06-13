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

#include "../../math/matrice/matrice.h"
#include "../../math/matrice/operations.h"
#include "../../math/interpolation.h"

#include "../pixel.h"

namespace dls {
namespace image {
namespace operation {

enum {
	MELANGE_NORMAL = 0,
	MELANGE_ADDITION,
	MELANGE_MULTIPLICATION,
	MELANGE_DIVISION,
	MELANGE_SOUSTRACTION,
	MELANGE_ECRAN,
	MELANGE_SUPERPOSITION,
	MELANGE_DIFFERENCE,
};

/**
 * Mélange deux images selon le mélange et le facteur spécifiés. Le résultat du
 * mélange est assigné à la première image.
 */
template <ConceptNombre nombre>
void melange_images(math::matrice<Pixel<nombre>> &image1, const math::matrice<Pixel<nombre>> &image2, int melange, float facteur)
{
	auto melange_normal = [&](const Pixel<nombre> &pixel1, const Pixel<nombre> &pixel2)
	{
		Pixel<nombre> resultat;
		resultat.r = math::interp_lineaire(pixel1.r, pixel2.r, facteur);
		resultat.g = math::interp_lineaire(pixel1.g, pixel2.g, facteur);
		resultat.b = math::interp_lineaire(pixel1.b, pixel2.b, facteur);
		resultat.a = pixel1.a;
		return resultat;
	};

	auto melange_addition = [&](const Pixel<nombre> &pixel1, const Pixel<nombre> &pixel2)
	{
		Pixel<nombre> resultat;
		resultat.r = math::interp_lineaire(pixel1.r, pixel1.r + pixel2.r, facteur);
		resultat.g = math::interp_lineaire(pixel1.g, pixel1.g + pixel2.g, facteur);
		resultat.b = math::interp_lineaire(pixel1.b, pixel1.b + pixel2.b, facteur);
		resultat.a = pixel1.a;
		return resultat;
	};

	auto melange_soustraction = [&](const Pixel<nombre> &pixel1, const Pixel<nombre> &pixel2)
	{
		Pixel<nombre> resultat;
		resultat.r = math::interp_lineaire(pixel1.r, pixel1.r - pixel2.r, facteur);
		resultat.g = math::interp_lineaire(pixel1.g, pixel1.g - pixel2.g, facteur);
		resultat.b = math::interp_lineaire(pixel1.b, pixel1.b - pixel2.b, facteur);
		resultat.a = pixel1.a;
		return resultat;
	};

	auto melange_multiplication = [&](const Pixel<nombre> &pixel1, const Pixel<nombre> &pixel2)
	{
		Pixel<nombre> resultat;
		resultat.r = math::interp_lineaire(pixel1.r, pixel1.r * pixel2.r, facteur);
		resultat.g = math::interp_lineaire(pixel1.g, pixel1.g * pixel2.g, facteur);
		resultat.b = math::interp_lineaire(pixel1.b, pixel1.b * pixel2.b, facteur);
		resultat.a = pixel1.a;
		return resultat;
	};

	auto melange_division = [&](const Pixel<nombre> &pixel1, const Pixel<nombre> &pixel2)
	{
		Pixel<nombre> resultat;
		resultat.r = (pixel2.r != 0.0f) ? math::interp_lineaire(pixel1.r, pixel1.r / pixel2.r, facteur) : 0.0f;
		resultat.g = (pixel2.g != 0.0f) ? math::interp_lineaire(pixel1.g, pixel1.g / pixel2.g, facteur) : 0.0f;
		resultat.b = (pixel2.b != 0.0f) ? math::interp_lineaire(pixel1.b, pixel1.b / pixel2.b, facteur) : 0.0f;
		resultat.a = pixel1.a;
		return resultat;
	};

	auto melange_ecran = [&](const Pixel<nombre> &pixel1, const Pixel<nombre> &pixel2)
	{
		Pixel<nombre> resultat;
		resultat.r = math::interp_lineaire(pixel1.r, 1.0f - (1.0f - pixel1.r) * (1.0f - pixel2.r), facteur);
		resultat.g = math::interp_lineaire(pixel1.g, 1.0f - (1.0f - pixel1.g) * (1.0f - pixel2.g), facteur);
		resultat.b = math::interp_lineaire(pixel1.b, 1.0f - (1.0f - pixel1.b) * (1.0f - pixel2.b), facteur);
		resultat.a = pixel1.a;
		return resultat;
	};

	auto melange_superposition = [&](const Pixel<nombre> &pixel1, const Pixel<nombre> &pixel2)
	{
		Pixel<nombre> resultat;

		if (pixel1.r < 0.5f) {
			resultat.r = math::interp_lineaire(pixel1.r, 2.0f * pixel1.r * pixel2.r, facteur);
		}
		else {
			resultat.r = math::interp_lineaire(pixel1.r, 1.0f - 2.0f * (1.0f - pixel1.r) * (1.0f - pixel2.r), facteur);
		}

		if (pixel1.g < 0.5f) {
			resultat.g = math::interp_lineaire(pixel1.g, 2.0f * pixel1.g * pixel2.g, facteur);
		}
		else {
			resultat.g = math::interp_lineaire(pixel1.g, 1.0f - 2.0f * (1.0f - pixel1.g) * (1.0f - pixel2.g), facteur);
		}

		if (pixel1.b < 0.5f) {
			resultat.b = math::interp_lineaire(pixel1.b, 2.0f * pixel1.b * pixel2.b, facteur);
		}
		else {
			resultat.b = math::interp_lineaire(pixel1.b, 1.0f - 2.0f * (1.0f - pixel1.b) * (1.0f - pixel2.b), facteur);
		}

		resultat.a = pixel1.a;

		return resultat;
	};

	auto melange_difference = [&](const Pixel<nombre> &pixel1, const Pixel<nombre> &pixel2)
	{
		Pixel<nombre> resultat;
		resultat.r = math::interp_lineaire(pixel1.r, std::abs(pixel1.r - pixel2.r), facteur);
		resultat.g = math::interp_lineaire(pixel1.g, std::abs(pixel1.g - pixel2.g), facteur);
		resultat.b = math::interp_lineaire(pixel1.b, std::abs(pixel1.b - pixel2.b), facteur);
		resultat.a = pixel1.a;
		return resultat;
	};

	switch (melange) {
		case MELANGE_NORMAL:
		{
			math::melange_matrices(image1, image2, melange_normal);
			break;
		}
		case MELANGE_ADDITION:
		{
			math::melange_matrices(image1, image2, melange_addition);
			break;
		}
		case MELANGE_SOUSTRACTION:
		{
			math::melange_matrices(image1, image2, melange_soustraction);
			break;
		}
		case MELANGE_MULTIPLICATION:
		{
			math::melange_matrices(image1, image2, melange_multiplication);
			break;
		}
		case MELANGE_DIVISION:
		{
			math::melange_matrices(image1, image2, melange_division);
			break;
		}
		case MELANGE_ECRAN:
		{
			math::melange_matrices(image1, image2, melange_ecran);
			break;
		}
		case MELANGE_SUPERPOSITION:
		{
			math::melange_matrices(image1, image2, melange_superposition);
			break;
		}
		case MELANGE_DIFFERENCE:
		{
			math::melange_matrices(image1, image2, melange_difference);
			break;
		}
	}
}

}  /* namespace operation */
}  /* namespace image */
}  /* namespace dls */
