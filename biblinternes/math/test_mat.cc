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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "matrice.hh"

/* Tests en vue d'une implémentation future de l'algorithme de traçage de rayon
 * 2D pour une simulation de peau trouvé dans « Robust Skin Simulation in
 * Incredibles 2 »
 * http://graphics.pixar.com/library/SkinForIncredibles2/paper.pdf
 */
namespace dls::math {

template <typename T>
static void decomposition_QR(
		mat3x2<T> const &M,
		mat3x2<T> &Q,
		mat2x2<T> &R)
{
	/* extrait les vecteurs colonnes */
	auto m1 = vec3<T>(M[0][0], M[1][0], M[2][0]);
	auto m2 = vec3<T>(M[0][1], M[1][1], M[2][1]);

	/* trouve les bases orthonormales */
	auto q1 = normalise(m1);
	auto q2 = m2 - (produit_scalaire(m2, m1) / produit_scalaire(m1, m1)) * m1;
	q2 = normalise(q2);

	Q = mat3x2<T>(q1.x, q2.x,
				  q1.y, q2.y,
				  q1.z, q2.z);

	/* NOTE : puisque nous avons Q et M, et que nous devons trouver M = QR,
	 * R = Q^-1M. Mais puisque Q n'est pas carrée, il nous faudrait la
	 * « fausse » inverse, à savoir (Q^T * Q)^-1 * Q^T. Or, puisque la matrice
	 * est orthonormale, (Q^T * Q)^-1 = (I)^-1 = I, donc nous pouvons simplement
	 * calculer R = Q^T * M.
	 */
	R = transpose(Q) * M;
}

}  /* namespace dls::math */

/* ça c'est une intersection entre un rayon et un segment, il nous en faut
 * une entre un rayon et un triangle !!!
 */
auto intersecte_2d(
		dls::math::vec2f const &orig,
		dls::math::vec2f const &dir,
		dls::math::vec2f const &p1,
		dls::math::vec2f const &p2,
		float &t)
{
	auto const v1 = orig - p1;
	auto const v2 = p2 - p1;
	auto const v3 = dls::math::vec2f(-dir.y, dir.x);

	auto const dot = produit_scalaire(v2, v3);

	if (std::abs(dot) < 0.000001f) {
		return false;
	}

	auto const t1 = produit_croix(v2, v1) / dot;
	auto const t2 = produit_scalaire(v1, v3) / dot;

	if (t1 >= 0.0f && (t2 >= 0.0f && t2 <= 1.0f)) {
		t = t1;
		return true;
	}

	return false;
}

int main()
{
	{
		auto M = dls::math::mat3x2d(
					12.0, 27.0,
					4.0,  2.0,
					6.0, 10.0
					);

		/* Résultat attendu :
		 * Q =
		 * 6/7  3/7
		 * 2/7 -6/7
		 * 3/7 -3/7
		 *
		 * R =
		 * 14 28
		 *  0  7
		 */

		auto Q = dls::math::mat3x2d();
		auto R = dls::math::mat2x2d();

		/* décomposition QR */
		decomposition_QR(M, Q, R);

		std::cerr << "M =\n" << M << '\n';
		std::cerr << "Q =\n" << Q << '\n';
		std::cerr << "R =\n" << R << '\n';
	}

	/* ********************************************************************** */
	/* Algorithme du papier :
	 * - a0 : la position de l'ancre
	 * - as : la position après la simulation de peau
	 * - a1 : la position corrigée
	 * - d  : le vecteur entre a0 et as
	 * - v0, v1, v2 : les vertex du triangle où se trouve a0
	 */

	auto a0 = dls::math::vec3f();
	auto as = dls::math::vec3f();
//	auto a1 = dls::math::vec3f();
	auto d  = as - a0;
	auto v0 = dls::math::vec3f();
	auto v1 = dls::math::vec3f();
	auto v2 = dls::math::vec3f();

	/* construit la matrice de l'espace canonique */
	auto e0 = v1 - v0;
	auto e1 = v2 - v0;
	auto Dm = dls::math::mat3x2f(
				e0.x, e1.x,
				e0.y, e1.y,
				e0.z, e1.z
				);

	auto Q = dls::math::mat3x2f();
	auto R = dls::math::mat2x2f();

	/* décomposition QR */
	decomposition_QR(Dm, Q, R);

	/* transforme l'ancre et la direction dans l'espace canonique */
	auto tf_canon = inverse(R) * transpose(Q);
	auto ah = tf_canon * (as - v0);
	auto dh = tf_canon * d;

	/* Care must be taken to ensure that the transformed point aˆ is always
	 * inside the canonical triangle, but we found that a simple clamp
	 * suffices. */

	/* Traçage de rayon 2D : déterminer si le rayon (ah, dh) traverse l'un des
	 * côtés {(0, 0), (0, 1)}, {(0, 0), (1, 0)}, ou {(1, 0), (0, 1)}.
	 * La position d'intersection est hh.
	 */
	auto vv0 = dls::math::vec2f(0.0f, 0.0f);
	auto vv1 = dls::math::vec2f(0.0f, 1.0f);
	auto vv2 = dls::math::vec2f(1.0f, 0.0f);

	dls::math::vec2f const cotes_tc[3][2] = {
		{ vv0, vv1 },
		{ vv0, vv2 },
		{ vv2, vv1 }
	};

	auto t = 0.0f;

	for (auto &cote : cotes_tc) {
		if (intersecte_2d(ah, dh, cote[0], cote[1], t)) {
			break;
		}
	}

	/* If no edge was hit, it is because the ray terminated at point aˆ1 on
	 * the interior of the triangle, and a1 = Dm * ah1 + v0 is the new anchor
	 * position.
	 * Otherwise, we walk across the edge to the opposing face, and perform the ray trace again.
	 * If there is no opposing face, we are at the edge of the mesh, so we set a1 = h.
	 */

	/* Transforme la position dans l'espace mondiale */
	//auto const hh = ah + dh * t;
	//auto h = dls::math::vec3f();// Dm * hh + v0;

	return 0;
}
