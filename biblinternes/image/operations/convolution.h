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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/math/matrice/matrice.hh"

namespace dls {
namespace image {
namespace operation {

enum {
	NOYAU_NETTETE = 0,
	NOYAU_NETTETE_ALT,
	NOYAU_FLOU_GAUSSIEN,
	NOYAU_FLOU_ALT,
	NOYAU_FLOU_NON_PONDERE,
	NOYAU_RELIEF_NO,
	NOYAU_RELIEF_NE,
	NOYAU_RELIEF_SO,
	NOYAU_RELIEF_SE,
	NOYAU_SOBEL_X,
	NOYAU_SOBEL_Y,
};

static const float noyau_nettete[] = {
	-1, -1, -1,
	-1,  8, -1,
	-1, -1, -1
};

static const float noyau_nettete_alt[] = {
	-1, -1, -1,
	-1, 24, -1,
	-1, -1, -1
};

static const float noyau_flou_non_pondere[] = {
	1, 1, 1,
	1, 1, 1,
	1, 1, 1
};

static const float noyau_flou_gaussien[] = {
	0, 1, 0,
	1, 5, 1,
	0, 1, 0
};

static const float noyau_flou_alt[] = {
	1, 2, 1,
	2, 4, 2,
	1, 2, 1
};

static const float noyau_relief_nord_ouest[] = {
	-4, -4, 0,
	-4, 12, 0,
	0, 0, 0
};

static const float noyau_relief_nord_est[] = {
	0, -4, -4,
	0, 12, -4,
	0, 0, 0
};

static const float noyau_relief_sud_est[] = {
	0, 0, 0,
	0, 12, -4,
	0, -4, -4
};

static const float noyau_relief_sud_ouest[] = {
	0, 0, 0,
	-4, 12, 0,
	-4, -4, 0
};

static const float noyau_sobel_x[] = {
	-1, 0, 1,
	-2, 0, 2,
	-1, 0, 1
};

static const float noyau_sobel_y[] = {
	-1, -2, -1,
	0, 0, 0,
	1, 2, 1
};

/**
 * Retourne le résultat de l'application du noyau sur l'image à la position
 * <l, c> spécifiées.
 */
template <ConceptNombre nombre>
auto applique_noyau(const math::matrice_dyn<nombre> &image, const float *kernel, int l, int c)
{
	const auto &ul = image[l - 1][c - 1] * kernel[0];
	const auto &uc = image[l - 1][c] * kernel[1];
	const auto &ur = image[l - 1][c + 1] * kernel[2];
	const auto &cl = image[l][c - 1] * kernel[3];
	const auto &cc = image[l][c] * kernel[4];
	const auto &cr = image[l][c + 1] * kernel[5];
	const auto &dl = image[l + 1][c - 1] * kernel[6];
	const auto &dc = image[l + 1][c] * kernel[7];
	const auto &dr = image[l + 1][c + 1] * kernel[8];

	const auto &conv = (cl + cr + uc + ul + ur + dc + dl + dr + cc);

	return image[l][c] + conv;
}

/**
 * Retourne une matrice qui correspond à l'application d'une convolution sur la
 * matrice spécifiée.
 */
template <ConceptNombre nombre>
auto applique_convolution(const math::matrice_dyn<nombre> &matrice, const int convolution)
{
	const float *kernel = nullptr;

	switch (convolution) {
		default:
		case NOYAU_NETTETE: kernel = noyau_nettete; break;
		case NOYAU_NETTETE_ALT: kernel = noyau_nettete_alt; break;
		case NOYAU_FLOU_GAUSSIEN: kernel = noyau_flou_gaussien; break;
		case NOYAU_FLOU_ALT: kernel = noyau_flou_alt; break;
		case NOYAU_FLOU_NON_PONDERE: kernel = noyau_flou_non_pondere; break;
		case NOYAU_RELIEF_NO: kernel = noyau_relief_nord_ouest; break;
		case NOYAU_RELIEF_NE: kernel = noyau_relief_nord_est; break;
		case NOYAU_RELIEF_SO: kernel = noyau_relief_sud_ouest; break;
		case NOYAU_RELIEF_SE: kernel = noyau_relief_sud_est; break;
		case NOYAU_SOBEL_X: kernel = noyau_sobel_x; break;
		case NOYAU_SOBEL_Y: kernel = noyau_sobel_y; break;
	}

	float noyau[9] = {};

	for (int i = 0; i < 8; ++i) {
		noyau[i] = kernel[i] / 9.0f;
	}

	const auto hauteur = matrice.nombre_lignes();
	const auto largeur = matrice.nombre_colonnes();

	auto image_tampon = math::matrice_dyn<nombre>(matrice.dimensions());

	for (int l = 1; l < hauteur - 1; ++l) {
		for (int c = 1; c < largeur - 1; ++c) {
			image_tampon[l][c] = applique_noyau(matrice, noyau, l, c);
		}
	}

	return image_tampon;
}

enum {
	FILTRE_DIVERGENCE_X = 0,
	FILTRE_DIVERGENCE_Y = 1,
	FILTRE_LAPLACIAN    = 2,
	FILTRE_GRADIENT_X   = 3,
	FILTRE_GRADIENT_Y   = 4,
	FILTRE_GRADIENT     = 5,
};

/**
 * Retourne le laplacien de la matrice spécifiée, à la position spécifié où `l`
 * est l'index de la ligne, et `c` celui de la colonne.
 */
template <ConceptNombre nombre>
static nombre laplacien(const math::matrice_dyn<nombre> &image, size_t l, size_t c)
{
	const auto &x0 = image[l][c];
	const auto &x1 = image[l][c - 1];
	const auto &x2 = image[l][c + 1];
	const auto &x3 = image[l - 1][c];
	const auto &x4 = image[l + 1][c];

	return (x1 + x2 + x3 + x4) - x0 * 4.0f;
}

/**
 * Retourne la divergence sur l'axe des x de la matrice spécifiée, à la position
 * spécifié où `l` est l'index de la ligne, et `c` celui de la colonne.
 */
template <ConceptNombre nombre>
static nombre divergence_x(const math::matrice_dyn<nombre> &image, size_t l, size_t c)
{
	const auto &x0 = image[l][c];
	const auto &x1 = image[l][c - 1];
	const auto &y1 = image[l][c + 1];

	return (x1 - x0) + (y1 - x0);
}

/**
 * Retourne la divergence sur l'axe des y de la matrice spécifiée, à la position
 * spécifié où `l` est l'index de la ligne, et `c` celui de la colonne.
 */
template <ConceptNombre nombre>
static nombre divergence_y(const math::matrice_dyn<nombre> &image, size_t l, size_t c)
{
	const auto &x0 = image[l][c];
	const auto &x1 = image[l - 1][c];
	const auto &y1 = image[l + 1][c];

	return (x1 - x0) + (y1 - x0);
}

/**
 * Retourne le gradient sur l'axe des x de la matrice spécifiée, à la position
 * spécifié où `l` est l'index de la ligne, et `c` celui de la colonne.
 */
template <ConceptNombre nombre>
static nombre gradient_x(const math::matrice_dyn<nombre> &image, size_t l, size_t c)
{
	const auto &x1 = image[l][c - 1];
	const auto &x2 = image[l][c + 1];

	return (x1 - x2) / static_cast<nombre>(2);
}

/**
 * Retourne le gradient sur l'axe des y de la matrice spécifiée, à la position
 * spécifié où `l` est l'index de la ligne, et `c` celui de la colonne.
 */
template <ConceptNombre nombre>
static nombre gradient_y(const math::matrice_dyn<nombre> &image, size_t l, size_t c)
{
	const auto &y1 = image[l - 1][c];
	const auto &y2 = image[l + 1][c];

	return (y1 - y2) / static_cast<nombre>(2);
}

/**
 * Retourne le gradient de la matrice spécifiée, à la position spécifié où `l`
 * est l'index de la ligne, et `c` celui de la colonne.
 */
template <ConceptNombre nombre>
static nombre gradient(const math::matrice_dyn<nombre> &image, size_t l, size_t c)
{
	const auto &x0 = image[l][c];
	const auto &x2 = image[l][c - 1];
	const auto &y2 = image[l - 1][c];

	return ((x0 - x2) + (x0 - y2)) / static_cast<nombre>(2);
}

/**
 * Retourne une matrice qui correspond à l'application d'un filtre sur la
 * matrice spécifiée.
 */
template <ConceptNombre nombre>
auto applique_filtre(const math::matrice_dyn<nombre> &matrice, int type_filtre)
{
	typedef nombre(*type_fontion)(const math::matrice_dyn<nombre>&, size_t, size_t);

	type_fontion fonction;

	switch (type_filtre) {
		case FILTRE_DIVERGENCE_X:
			fonction = divergence_x;
			break;
		case FILTRE_DIVERGENCE_Y:
			fonction = divergence_y;
			break;
		case FILTRE_LAPLACIAN:
			fonction = laplacien;
			break;
		case FILTRE_GRADIENT_X:
			fonction = gradient_x;
			break;
		case FILTRE_GRADIENT_Y:
			fonction = gradient_y;
			break;
		case FILTRE_GRADIENT:
			fonction = gradient;
			break;
	}

	const auto largeur = matrice.nombre_colonnes();
	const auto hauteur = matrice.nombre_lignes();
	auto image_tampon = math::matrice_dyn<nombre>(matrice.dimensions());

	for (int l = 1; l < hauteur - 1; ++l) {
		for (int c = 1; c < largeur - 1; ++c) {
			image_tampon[l][c] = fonction(matrice, l, c);
		}
	}

	return image_tampon;
}

}  /* operation */
}  /* namespace image */
}  /* namespace dls */
