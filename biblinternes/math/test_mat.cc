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
using mat3x2 = matrice<T, vecteur, paquet_index<0, 1>, paquet_index<0, 1, 2>>;
using mat3x2f = mat3x2<float>;
using mat3x2d = mat3x2<double>;

template <
		typename type_scalaire,
		template <int, typename, int...> class type_vecteur,
		size_t... Colonnes,
		size_t... Lignes
		>
auto imprime_mat(
		const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Lignes...>> &m1)
{
	auto &os = std::cerr;

	os << "matrice :\n";

	for (size_t i = 0; i < sizeof...(Lignes); ++i) {
		for (size_t j = 0; j < sizeof...(Colonnes); ++j) {
			os << m1[i][j] << ' ';
		}

		os << '\n';
	}

	os << '\n';
}
template <
		typename type_scalaire,
		template <int, typename, int...> class type_vecteur,
		size_t... Colonnes,
		size_t... Lignes
		>
[[nodiscard]] auto transpose2(
		const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Lignes...>> &m1)
{
	auto mat = matrice<type_scalaire, type_vecteur, paquet_index<Lignes...>, paquet_index<Colonnes...>>();
	//((mat[Lignes][Colonnes] = m1[Colonnes][Lignes]), ...);

	for (size_t i = 0; i < sizeof...(Lignes); ++i) {
		for (size_t j = 0; j < sizeof...(Colonnes); ++j) {
			mat[j][i] = m1[i][j];
		}
	}

	return mat;
}

template <
		typename type_scalaire,
		template <int, typename, int...> class type_vecteur,
		size_t... Colonnes1,
		size_t... Lignes1,
		size_t... Colonnes2,
		size_t... Lignes2
		>
[[nodiscard]] auto multiplie(
		const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes1...>, paquet_index<Lignes1...>> &m1,
		const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes2...>, paquet_index<Lignes2...>> &m2)
{
	static_assert (sizeof...(Lignes1) == sizeof...(Colonnes2), "");

	auto mat = matrice<type_scalaire, type_vecteur, paquet_index<Lignes1...>, paquet_index<Colonnes2...>>();

	for (auto l = 0ul; l < sizeof...(Lignes1); ++l) {
		for (auto c = 0ul; c < sizeof...(Colonnes2); ++c) {
			auto valeur = static_cast<type_scalaire>(0);

			for (auto m = 0ul; m < sizeof...(Colonnes1); ++m) {
				valeur += m1[l][m] * m2[m][c];
			}

			mat[l][c] = valeur;
		}
	}

	return mat;
}

template <typename T>
static void decomposition_QR(
		mat3x2<T> const &M,
		mat3x2<T> &Q,
		mat2x2<T> &R)
{
	/* extrait les vecteurs colonnes */
	auto m1 = dls::math::vec3<T>(M[0][0], M[1][0], M[2][0]);
	auto m2 = dls::math::vec3<T>(M[0][1], M[1][1], M[2][1]);

//	std::cerr << "m1 :\n" << m1 << '\n';
//	std::cerr << "m2 :\n" << m2 << '\n';

	/* trouve les bases orthonormales */
	auto q1 = normalise(m1);
	auto q2 = m2 - (produit_scalaire(m2, m1) / produit_scalaire(m1, m1)) * m1;
	q2 = normalise(q2);

//	std::cerr << "q1 :\n" << q1 << '\n';
//	std::cerr << "q2 :\n" << q2 << '\n';

	Q = dls::math::mat3x2<T>(
				q1[0], q2[0],
			q1[1], q2[1],
			q1[2], q2[2]
				);

//	std::cerr << "Q = ";
//	imprime_mat(Q);
//	std::cerr << "QT = ";
//	imprime_mat(transpose2(Q));

	/* NOTE : puisque nous avons Q et M, et que nous devons trouver M = QR,
	 * R = Q^-1M. Mais puisque Q n'est pas carrée, il nous faudrait la
	 * « fausse » inverse, à savoir (Q^T * Q)^-1 * Q^T. Or, puisque la matrice
	 * est orthonormale, (Q^T * Q)^-1 = (I)^-1 = I, donc nous pouvons simplement
	 * calculer R = Q^T * M.
	 */

	R = multiplie(transpose2(Q), M);

//	std::cerr << "R = ";
//	dls::math::imprime_mat(R);
}

template <typename T>
[[nodiscard]] auto produit_croix(vec2<T> const &u, vec2<T> const &v)
{
	return (u.x * v.y) - (v.x * u.y);
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
	}

	/* ********************************************************************** */
	/* Algorithme du papier :
	 * - a0 : la position de l'ancre
	 * - as : la position après la simulation de peau
	 * - a1 : la position corrigé
	 * - d  : le vecteur entre a0 et as
	 * - v0, v1, v2 : les vertex du triangle où se trouve a0
	 */

//	auto a0 = dls::math::vec3f();
//	auto as = dls::math::vec3f();
//	auto a1 = dls::math::vec3f();
	//auto d  = as - a0;
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
	auto ah = dls::math::vec2f(); //inverse(R) * transpose2(Q) * (as - v0);
	auto dh = dls::math::vec2f(); //inverse(R) * transpose2(Q) * d;

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
