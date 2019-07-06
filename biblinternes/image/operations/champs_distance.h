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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1201, USA.
 *
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "../../math/matrice/matrice.hh"
#include "../../math/vecteur.hh"

namespace dls {
namespace image {
namespace operation {

enum {
	CD_NAVIGATION_ESTIME = 0,
	CD_SSEDT,
};

namespace navigation_estime {

/**
 * Génère un champs en utilisant un algorithme de navigation à l'estime.
 * Implémentation basé sur :
 * http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.102.7988&rep=rep1&type=pdf
 */

void navigation_estime_ex(
		const math::matrice_dyn<float> &image,
		math::matrice_dyn<float> &distance,
		math::matrice_dyn<math::vec2i> &nearest_point)
{
	const auto sx = image.nombre_colonnes();
	const auto sy = image.nombre_lignes();
	const auto unit = std::min(1.0f / static_cast<float>(sx), 1.0f / static_cast<float>(sy));
	const auto diagonal = std::sqrt(2.0f) * unit;

	/* Forward pass from top left to bottom right. */
	for (auto l = 1; l < sy - 1; ++l) {
		for (auto c = 1; c < sx - 1; ++c) {
			auto dist = distance[l][c];

			if (dist == 0.0f) {
				continue;
			}

			auto p = nearest_point[l][c];

			if (distance[l][c - 1] + unit < dist) {
				p = nearest_point[l][c - 1];
				dist = static_cast<float>(hypot((l - p[0]), c - p[1])) * unit;
			}

			if (distance[l - 1][c - 1] + diagonal < dist) {
				p = nearest_point[l - 1][c - 1];
				dist = static_cast<float>(hypot(l - p[0], c - p[1])) * unit;
			}

			if (distance[l - 1][c] + unit < dist) {
				p = nearest_point[l - 1][c];
				dist = static_cast<float>(hypot(l - p[0], c - p[1])) * unit;
			}

			if (distance[l - 1][c + 1] + diagonal < dist) {
				p = nearest_point[l - 1][c + 1];
				dist = static_cast<float>(hypot(l - p[0], c - p[1])) * unit;
			}

			distance[l][c] = dist;
			nearest_point[l][c] = p;
		}
	}

	/* Backward pass from bottom right to top left. */
	for (auto l = sy - 2; l >= 1; --l) {
		for (auto c = sx - 2; c >= 1; --c) {
			auto dist = distance[l][c];

			if (dist == 0.0f) {
				continue;
			}

			auto p = nearest_point[l][c];

			if (distance[l][c + 1] + unit < dist) {
				p = nearest_point[l][c + 1];
				dist = static_cast<float>(hypot(l - p[0], c - p[1])) * unit;
			}

			if (distance[l + 1][c + 1] + diagonal < dist) {
				p = nearest_point[l + 1][c + 1];
				dist = static_cast<float>(hypot(l - p[0], c - p[1])) * unit;
			}

			if (distance[l + 1][c] + unit < dist) {
				p = nearest_point[l + 1][c];
				dist = static_cast<float>(hypot(l - p[0], c - p[1])) * unit;
			}

			if (distance[l + 1][c - 1] + diagonal < dist) {
				p = nearest_point[l + 1][c - 1];
				dist = static_cast<float>(hypot(l - p[0], c - p[1])) * unit;
			}

			distance[l][c] = dist;
			nearest_point[l][c] = p;
		}
	}

	/* indicate interior and exterior pixel */
	for (auto l = 0; l < sy; ++l) {
		for (auto c = 0; c < sx; ++c) {
			if (image[l][c] < 0.5f) {
				distance[l][c] = -distance[l][c];
			}
		}
	}
}

void normalise(math::matrice_dyn<float> &field)
{
	const auto nc = field.nombre_colonnes();
	const auto nl = field.nombre_lignes();

	for (auto l = 0; l < nl; ++l) {
		for (auto c = 0; c < nc; ++c) {
			float clamp = std::max(-1.0f, std::min(field[l][c], 1.0f));
			field[l][c] = ((clamp + 1.0f) * 0.5f);
		}
	}

#if 0
	const size_t sx = field.width(), sy = field.height();
	size_t x, y;

	float s;
	for (y = 0; y < sy; ++y) {
		for (x = 0; x < sx; ++x) {
			s = field(x, y) * -1.0f;
			s = 1.0f - s;
			s *= s;
			s *= s;
			s *= s;
			field(x, y) = s;
		}
	}
#endif
}

void initialise(const math::matrice_dyn<float> &image,
		   math::matrice_dyn<float> &distance,
		   math::matrice_dyn<math::vec2i> &nearest_point)
{
	const auto nc = image.nombre_colonnes();
	const auto nl = image.nombre_lignes();
	const auto max_dist = static_cast<float>(hypot(nc, nl));

	for (auto l = 0; l < nl; ++l) {
		for (auto c = 0; c < nc; ++c) {
			distance[l][c] = max_dist;
			nearest_point[l][c] = math::vec2i{ 0, 0 };
		}
	}

	/* initialize immediate interior & exterior elements */
	for (auto l = 1; l < nl - 2; ++l) {
		for (auto c = 1; c < nc - 2; ++c) {
			const bool inside = image[l][c] > 0.5f;

			if ((image[l][c - 1] > 0.5f) != inside ||
				(image[l][c + 1] > 0.5f) != inside ||
				(image[l - 1][c] > 0.5f) != inside ||
				(image[l + 1][c] > 0.5f) != inside)
			{
				distance[l][c] = 0.0f;
				nearest_point[l][c] = math::vec2i{ l, c };
			}
		}
	}
}

void genere_champs_distance(const math::matrice_dyn<float> &image,
				  math::matrice_dyn<float> &distance, float valeur_iso)
{
	math::matrice_dyn<math::vec2i> nearest_point(image.dimensions());

	initialise(image, distance, nearest_point);
	navigation_estime_ex(image, distance, nearest_point);

	normalise(distance);
}

}  /* namespace navigation_estime */

namespace ssedt {

/* 8-point Sequential Signed Euclidean Distance algorithm.
 * Similar to the Chamfer Distance Transform, 8SSED computes the distance field
 * by doing a double raster scan over the entire image. */

template <typename T>
T distance_carre(const math::vec2<T> &p)
{
	return p[0] * p[0] + p[1] * p[1];
}

inline void compare_dist(const math::matrice_dyn<math::vec2<int>> &g,
						 math::vec2<int> &p,
						 int c, int l,
						 int decalage_c, int decalage_l)
{
	math::vec2<int> other = g[l + decalage_l][c + decalage_c];
	other[0] += decalage_l;
	other[1] += decalage_c;

	if (distance_carre(other) < distance_carre(p)) {
		p = other;
	}
}

void scan(math::matrice_dyn<math::vec2<int>> &g)
{
	const auto sx = g.nombre_colonnes() - 1;
	const auto sy = g.nombre_lignes() - 1;

	/* Scan de haut en bas, de gauche à droite, en ne considérant que les
	 * voisins suivant :
	 * 1 1 1
	 * 1 x -
	 * - - -
	 */
	for (int l = 1; l < sy; ++l) {
		for (auto c = 1; c < sx; ++c) {
			math::vec2<int> p = g[l][c];
			compare_dist(g, p, c, l, -1,  0);
			compare_dist(g, p, c, l,  0, -1);
			compare_dist(g, p, c, l, -1, -1);
			compare_dist(g, p, c, l,  1, -1);
			g[l][c] = p;
		}

		for (auto c = sx; c > 0; --c) {
			math::vec2<int> p = g[l][c];
			compare_dist(g, p, c, l, 1, 0);
			g[l][c] = p;
		}
	}

	/* Scan de bas en haut, de droite à gauche, en ne considérant que les
	 * voisins suivant :
	 * - - -
	 * - x 1
	 * 1 1 1
	 */
	for (auto l = sy - 1; l > 0; --l) {
		for (auto c = sx - 1; c > 0; --c) {
			math::vec2<int> p = g[l][c];
			compare_dist(g, p, c, l,  1,  0);
			compare_dist(g, p, c, l,  0,  1);
			compare_dist(g, p, c, l, -1,  1);
			compare_dist(g, p, c, l,  1,  1);
			g[l][c] = p;
		}

		for (auto c = 1; c < sx; ++c) {
			math::vec2<int> p = g[l][c];
			compare_dist(g, p, c, l, -1, 0);
			g[l][c] = p;
		}
	}
}

void initialise(
		const math::matrice_dyn<float> &image,
		math::matrice_dyn<math::vec2<int>> &grille1,
		math::matrice_dyn<math::vec2<int>> &grille2)
{
	const auto sx = image.nombre_colonnes();
	const auto sy = image.nombre_lignes();
	const auto max_dist = static_cast<int>(std::hypot(sx, sy));

	for (auto l = 0; l < sy; ++l) {
		for (auto c = 0; c < sx; ++c) {
			if (image[l][c] < 0.5f) {
				grille1[l][c] = math::vec2i{ 0, 0 };
				grille2[l][c] = math::vec2i{ max_dist, max_dist };
			}
			else {
				grille1[l][c] = math::vec2i{ max_dist, max_dist };
				grille2[l][c] = math::vec2i{ 0, 0 };
			}
		}
	}
}

void genere_champs_distance(
		const math::matrice_dyn<float> &image,
		math::matrice_dyn<float> &distance,
        float valeur_iso)
{
	const auto sx = image.nombre_colonnes();
	const auto sy = image.nombre_lignes();
	const auto unite = std::min(1.0f / static_cast<float>(sx), 1.0f / static_cast<float>(sy));

	/* We use two point grids: one which keeps track of the interior distances,
	 * while the other, the exterior distances. */
	math::matrice_dyn<math::vec2<int>> grille1(image.dimensions());
	math::matrice_dyn<math::vec2<int>> grille2(image.dimensions());

	initialise(image, grille1, grille2);

	scan(grille1);
	scan(grille2);

	/* Compute signed distance field from grid1 & grid2, and make it unsigned. */
	for (auto l = 1, ye = sy - 1; l < ye; ++l) {
		for (auto c = 1, xe = sx - 1; c < xe; ++c) {
			const float dist1 = std::sqrt(static_cast<float>(distance_carre(grille1[l][c])));
			const float dist2 = std::sqrt(static_cast<float>(distance_carre(grille2[l][c])));
			const float dist  = dist1 - dist2;

			distance[l][c] = dist * unite;
		}
	}
}

}  /* namespace ssedt */

}  /* namespace operation */
}  /* namespace image */
}  /* namespace dls */
