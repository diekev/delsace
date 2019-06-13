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

#include "../../math/matrice/matrice.hh"

#include <cstring>  /* for std::memcpy */
#include <tuple>
#include <vector>

namespace dls {
namespace image {
namespace operation {

/**
 * Type pour le retour de la fonction `calcul_index_et_poids`.
 */
using tuple_vecteurs = std::tuple<std::vector<int>, std::vector<int>, std::vector<float>>;

/**
 * Retourne des vecteurs contenant les index à gauche et à droite, ainsi que le
 * poids des index à gauche pour pouvoir calculer un sous-échantillonage des
 * pixels d'une image de taille_source vers ceux d'une image de taille_dest.
 * C'est-à-dire que cet algorithme définit quel pixel d'origine ira vers quel
 * pixel de destinantion avec quel poids.
 *
 * L'algorithme retourne un std::tuple<std::vector<int>, std::vector<int>,
 * std::vector<float>>, où les deux premiers vecteurs sont les index à gauche et
 * les index à doite, et le troisième vecteur, les poids à gauche. Les tailles
 * des vecteurs sont égales à la taille source.
 *
 * Les index à gauche sont les index de tous les pixels sources devant se
 * trouver à gauche du pixel de destination et qui sont, dû au sous-
 * échantillonage, compressés dans celui-ci. Idem pour les index à droite.
 *
 * Les poids correspondent aux poids des pixels de gauche, et sont calculer par
 * interpolation linéaire en fonction de l'échelle de sous-échantillonage
 * (taille_dest / taille_source) et de la distance entre le pixel à gauche et le
 * pixel courant. Pour trouver le poids des pixels de droite il suffit de
 * calculer : ((taille_dest / taille_source) - poids_gauche) pour les pixels
 * correspondants (par exemple le poids du troisième pixel à droite (x + 3) est
 * égale à la formule sus-sité avec comme poids_gauche celui du troisième pixel
 * à gauche (x - 3)).
 *
 * Selon le contexte, la taille peut être soit la hauteur, soit la largeur des
 * images. Si les tailles correspondent à la hauteur, les index à gauche et à
 * droite devront être compris comme étant respectivement index en haut et en
 * bas du pixel courant.
 *
 * Puisque toutes les lignes ou colonnes de l'image seront sous-échantillonées
 * de la même manière, il nous suffit de calculer les index et les poids que
 * pour une ligne ou colonne.
 */
tuple_vecteurs calcul_index_et_poids(
		const size_t taille_soure,
		const size_t taille_dest);

/**
 * Sous-échantillone sur l'axe horizontal l'image src dans l'image dst. Les deux
 * images doivent avoir la même hauteur, mais la largeur de l'image dst doit
 * impérativement être inférieure à celle de l'image src.
 */
template <ConceptNombre nombre>
static void sousechantillonage_horizontal(const math::matrice<nombre> &src, math::matrice<nombre> &dst)
{
	std::vector<int>   index_gauche;
	std::vector<int>   index_droite;
	std::vector<float> poids;

	std::tie(index_gauche, index_droite, poids) = calcul_index_et_poids(
					static_cast<size_t>(src.nombre_colonnes()),
					static_cast<size_t>(dst.nombre_colonnes()));

	const auto echelle = static_cast<float>(dst.nombre_colonnes()) / static_cast<float>(src.nombre_colonnes());

	dst.remplie(static_cast<nombre>(0));

	for (auto c = 0; c < src.nombre_colonnes(); c++) {
		const auto poids_gauche = poids[static_cast<size_t>(c)];
		const auto poids_droite = echelle - poids_gauche;
		const auto ig = index_gauche[static_cast<size_t>(c)];
		const auto id = index_droite[static_cast<size_t>(c)];

		for (auto l = 0; l < src.nombre_lignes(); l++) {
			dst[l][ig] += poids_gauche * src[l][c];
			dst[l][id] += poids_droite * src[l][c];
		}
	}
}

/**
 * Sous-échantillone sur l'axe vertical l'image src dans l'image dst. Les deux
 * images doivent avoir la même largeur, mais la hauteur de l'image dst doit
 * impérativement être inférieure à celle de l'image src.
 */
template <ConceptNombre nombre>
static void sousechantillonage_vertical(const math::matrice<nombre> &src, math::matrice<nombre> &dst)
{
	std::vector<int>   index_haut;
	std::vector<int>   index_bas;
	std::vector<float> poids;

	std::tie(index_haut, index_bas, poids) = calcul_index_et_poids(
				static_cast<size_t>(src.nombre_lignes()),
				static_cast<size_t>(dst.nombre_lignes()));

	const auto echelle = static_cast<float>(dst.nombre_lignes()) / static_cast<float>(src.nombre_lignes());

	dst.remplie(static_cast<nombre>(0));

	for (int l = 0; l < src.nombre_lignes(); l++) {
		const auto poids_haut = poids[static_cast<size_t>(l)];
		const auto poids_bas = echelle - poids_haut;
		const auto ih = index_haut[static_cast<size_t>(l)];
		const auto ib = index_bas[static_cast<size_t>(l)];

		for (int c = 0; c < src.nombre_colonnes(); c++) {
			dst[ih][c] += poids_haut * src[l][c];
			dst[ib][c] += poids_bas * src[l][c];
		}
	}
}

/**
 * Sur-échantillone l'image src dans l'image dst. L'image dst doit avoir une
 * taille supérieure à l'image src sur au moins l'une des deux dimensions.
 */
template <ConceptNombre nombre>
static void surechantillonage(const math::matrice<nombre> &src, math::matrice<nombre> &dst)
{
	auto dst_width = dst.nombre_colonnes();
	auto dst_height = dst.nombre_lignes();
	auto src_width = src.nombre_colonnes();
	auto src_height = src.nombre_lignes();
	auto xmax = dst_width;
	int dx, dy, sx, sy, k;
	auto prev_sy0 = -1, prev_sy1 = -1;
	float fx, fy;
	auto scale_x = static_cast<float>(src_width) / static_cast<float>(dst_width);
	auto scale_y = static_cast<float>(src_height) / static_cast<float>(dst_height);

	std::vector<int> xofs_i_arr(static_cast<size_t>(dst_width));
	std::vector<int> yofs_i_arr(static_cast<size_t>(dst_height));
	std::vector<float> xofs_f_arr(static_cast<size_t>(dst_width));
	std::vector<float> yofs_f_arr(static_cast<size_t>(dst_height));
	std::vector<nombre> buf0_arr(static_cast<size_t>(dst_width));
	std::vector<nombre> buf1_arr(static_cast<size_t>(dst_width));
	auto xofs_i = &xofs_i_arr[0];
	auto yofs_i = &yofs_i_arr[0];
	auto xofs_f = &xofs_f_arr[0];
	auto yofs_f = &yofs_f_arr[0];
	auto buf0 = &buf0_arr[0];
	auto buf1 = &buf1_arr[0];
	auto ptr_d = dst[0];

	for (dx = 0; dx < dst_width; dx++) {
		fx = (static_cast<float>(dx) + 0.5f) * scale_x - 0.5f;
		sx = static_cast<int>(fx);
		fx -= static_cast<float>(sx);
		if (sx < 0)
			fx = 0, sx = 0;

		if (sx >= src_width-1) {
			fx = 0, sx = src_width-1;
			if( xmax >= dst_width )
				xmax = dx;
		}
		xofs_i[dx] = sx;
		xofs_f[dx] = fx;
	}

	for (dy = 0; dy < dst_height; dy++) {
		fy = (static_cast<float>(dy) + 0.5f) * scale_y - 0.5f;
		sy = static_cast<int>(fy);
		fy -= static_cast<float>(sy);
		if (sy < 0)
			sy = 0, fy = 0;

		yofs_i[dy] = sy;
		yofs_f[dy] = fy;
	}

	for (dy = 0; dy < dst_height; dy++, ptr_d = dst[dy]) {
		fy = yofs_f[dy];
		int sy0 = yofs_i[dy], sy1 = sy0 + (fy > 0 && sy0 < src_height-1);

		if (sy0 == prev_sy0 && sy1 == prev_sy1){
			k = 2;
		}
		else if (sy0 == prev_sy1) {
			auto swap_t = buf0;
			buf0 = buf1;
			buf1 = swap_t;
			k = 1;
		}
		else{
			k = 0;
		}

		for (; k < 2; k++) {
			if (k == 1 && sy1 == sy0) {
				std::memcpy(buf1, buf0, static_cast<size_t>(dst_width) * sizeof(nombre));
				continue;
			}

			auto *_buf = k == 0 ? buf0 : buf1;
			const auto _src = src[sy];
			sy = k == 0 ? sy0 : sy1;

			for (dx = 0; dx < xmax; dx++) {
				sx = xofs_i[dx];
				fx = xofs_f[dx];
				auto t = _src[sx];
				_buf[dx] = t + fx*(_src[sx+1] - t);
			}

			for ( ; dx < dst_width; dx++) {
				_buf[dx] = _src[xofs_i[dx]];
			}
		}
		prev_sy0 = sy0;
		prev_sy1 = sy1;

		if (sy0 == sy1){
			for (dx = 0; dx < dst_width; dx++){
				ptr_d[dx] = buf0[dx];
			}
		}
		else{
			for (dx = 0; dx < dst_width; dx++){
				ptr_d[dx] = buf0[dx] + fy*(buf1[dx] - buf0[dx]);
			}
		}
	}
}

/**
 * Redimensionne l'image src aux dimensions de l'image dst. Si les deux images
 * ont les mêmes dimensions, l'image src est simplement copiée vers l'image dst.
 */
template <ConceptNombre nombre>
void redimensionne(const math::matrice<nombre> &src, math::matrice<nombre> &dst)
{
	if (src.nombre_colonnes() > dst.nombre_colonnes() && src.nombre_lignes() > dst.nombre_lignes() ) {
		math::matrice<nombre> tmp(math::Hauteur(src.nombre_lignes()), math::Largeur(dst.nombre_colonnes()));
		sousechantillonage_horizontal(src, tmp);
		sousechantillonage_vertical(tmp, dst);
	}
	else if (src.nombre_colonnes() > dst.nombre_colonnes() && src.nombre_lignes() == dst.nombre_lignes() ) {
		sousechantillonage_horizontal(src, dst);
	}
	else if (src.nombre_colonnes() > dst.nombre_colonnes() && src.nombre_lignes() < dst.nombre_lignes() ) {
		math::matrice<nombre> tmp(math::Hauteur(src.nombre_lignes()), math::Largeur(dst.nombre_colonnes()));
		sousechantillonage_horizontal(src, tmp);
		surechantillonage(tmp, dst);
	}
	else if (src.nombre_colonnes() == dst.nombre_colonnes() && src.nombre_lignes() > dst.nombre_lignes() ) {
		sousechantillonage_vertical(src, dst);
	}
	else if (src.nombre_colonnes() == dst.nombre_colonnes() && src.nombre_lignes() == dst.nombre_lignes() ) {
		math::copie(src, dst);
	}
	else if (src.nombre_colonnes() == dst.nombre_colonnes() && src.nombre_lignes() < dst.nombre_lignes() ) {
		surechantillonage(src, dst);
	}
	else if (src.nombre_colonnes() < dst.nombre_colonnes() && src.nombre_lignes() > dst.nombre_lignes() ) {
		math::matrice<nombre> tmp(math::Hauteur(dst.nombre_lignes()), math::Largeur(src.nombre_colonnes()));
		sousechantillonage_vertical(src, tmp);
		surechantillonage(tmp, dst);
	}
	else if (src.nombre_colonnes() < dst.nombre_colonnes() && src.nombre_lignes() == dst.nombre_lignes() ) {
		surechantillonage(src, dst);
	}
	else if (src.nombre_colonnes() < dst.nombre_colonnes() && src.nombre_lignes() < dst.nombre_lignes() ) {
		surechantillonage(src, dst);
	}
}

}  /* namespace operation */
}  /* namespace image */
}  /* namespace dls */
