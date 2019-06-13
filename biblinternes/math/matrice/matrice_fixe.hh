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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "../concepts.hh"

#include <cmath>
#include <iostream>

#include "../outils.hh"

namespace dls {
namespace math {

template <ConceptNombre nombre, int M, int N>
class matrice_fixe {
protected:
	nombre m_data[M][N];

	inline void copy_data(const nombre data[N][N])
	{
		for (int i = 0; i < M; ++i) {
			for (int j = 0; j < N; ++j) {
				m_data[i][j] = data[i][j];
			}
		}
	}

	inline void copy_data(const matrice_fixe &cote_droit)
	{
		for (int i = 0; i < M; ++i) {
			for (int j = 0; j < N; ++j) {
				m_data[i][j] = cote_droit[i][j];
			}
		}
	}

public:
	using value_type = nombre;

	constexpr matrice_fixe() = default;

	constexpr matrice_fixe(const matrice_fixe &cote_droit)
		: matrice_fixe()
	{
		copy_data(cote_droit);
	}

	constexpr matrice_fixe(const std::initializer_list<nombre> &list)
		: matrice_fixe()
	{
		int i = 0, j = 0;
		for (const auto &elem : list) {
			m_data[i][j++] = elem;

			if (j == N) {
				j = 0;
				++i;
			}
		}
	}

	virtual ~matrice_fixe() = default;

	void fill(const nombre value)
	{
		for (int i = 0; i < M; ++i) {
			for (int j = 0; j < N; ++j) {
				m_data[i][j] = value;
			}
		}
	}

	const nombre *operator[](int i) const
	{
		return &(m_data[i][0]);
	}

	nombre *operator[](int i)
	{
		return &(m_data[i][0]);
	}

	/* modifiers */

	matrice_fixe &operator=(const matrice_fixe &other)
	{
		copy_data(other);
		return *this;
	}

	matrice_fixe &operator+=(const matrice_fixe &cote_droit)
	{
		for (int i = 0; i < M; ++i) {
			for (int j = 0; j < N; ++j) {
				m_data[i][j] += cote_droit[i][j];
			}
		}

		return *this;
	}

	matrice_fixe &operator-=(const matrice_fixe &cote_droit)
	{
		for (int i = 0; i < M; ++i) {
			for (int j = 0; j < N; ++j) {
				m_data[i][j] -= cote_droit[i][j];
			}
		}

		return *this;
	}

	matrice_fixe &operator*=(const matrice_fixe &cote_droit)
	{
		nombre buffer[M][N];

		for (int i = 0; i < M; ++i) {
			for (int j = 0; j < N; ++j) {
				auto value = static_cast<nombre>(0);

				for (int k = 0; k < N; ++k) {
					value += m_data[i][k] * cote_droit[k][j];
				}

				buffer[i][j] = value;
			}
		}

		copy_data(buffer);

		return *this;
	}

	template <int M1, int N1>
	matrice_fixe &operator*=(const matrice_fixe<nombre, M1, N1> &cote_droit)
	{
		static_assert(M1 == N && N1 == M, "Matrix multiplication is undefined!");

		nombre tampon[M][N];

		for (int i = 0; i < M; ++i) {
			for (int j = 0; j < N1; ++j) {
				auto valeur = static_cast<nombre>(0);

				for (int k = 0; k < M1; ++k) {
					valeur += m_data[i][k] * cote_droit[k][j];
				}

				tampon[i][j] = valeur;
			}
		}

		copy_data(tampon);

		return *this;
	}

	matrice_fixe &operator*=(const nombre value)
	{
		for (int i = 0; i < M; ++i) {
			for (int j = 0; j < N; ++j) {
				m_data[i][j] *= value;
			}
		}

		return *this;
	}
};

template <ConceptNombre nombre, int M, int N>
auto transpose(const matrice_fixe<nombre, M, N> &mat)
{
	matrice_fixe<nombre, N, M> res;

	for (int i = 0; i < M; ++i) {
		for (int j = 0; j < N; ++j) {
			res[j][i] = mat[i][j];
		}
	}

	return res;
}

template <ConceptNombre nombre, int M, int N>
auto operator+(const matrice_fixe<nombre, M, N> &cote_gauche, const matrice_fixe<nombre, M, N> &cote_droit)
{
	matrice_fixe<nombre, M, N> tmp(cote_droit);
	tmp += cote_gauche;
	return tmp;
}

template <ConceptNombre nombre, int M, int N>
auto operator-(const matrice_fixe<nombre, M, N> &cote_gauche, const matrice_fixe<nombre, M, N> &cote_droit)
{
	matrice_fixe<nombre, M, N> tmp(cote_droit);
	tmp -= cote_gauche;
	return tmp;
}

template <ConceptNombre nombre, int M, int N>
auto operator*(const matrice_fixe<nombre, M, N> &cote_gauche, const matrice_fixe<nombre, M, N> &cote_droit)
{
	matrice_fixe<nombre, M, N> tmp(cote_gauche);
	tmp *= cote_droit;
	return tmp;
}

template <ConceptNombre nombre, int M, int N>
auto operator*(const matrice_fixe<nombre, M, N> &cote_gauche, const nombre cote_droit)
{
	matrice_fixe<nombre, M, N> tmp(cote_gauche);
	tmp *= cote_droit;
	return tmp;
}

template <ConceptNombre nombre, int M, int N>
auto operator*(const nombre cote_gauche, const matrice_fixe<nombre, M, N> &cote_droit)
{
	matrice_fixe<nombre, M, N> tmp(cote_droit);
	tmp *= cote_gauche;
	return tmp;
}

template <ConceptNombre nombre, int M, int N>
bool operator==(const matrice_fixe<nombre, M, N> &cote_gauche, const matrice_fixe<nombre, M, N> &cote_droit)
{
	for (int i = 0; i < N; ++i) {
		for (int j = 0; j < N; ++j) {
			if (cote_gauche[i][j] != cote_droit[i][j]) {
				return false;
			}
		}
	}

	return true;
}

template <ConceptNombre nombre, int M, int N>
bool operator!=(const matrice_fixe<nombre, M, N> &cote_gauche, const matrice_fixe<nombre, M, N> &cote_droit)
{
	return !(cote_gauche == cote_droit);
}

template <ConceptNombre nombre, int M, int N>
auto &operator<<(std::ostream &os, const matrice_fixe<nombre, M, N> &mat)
{
	for (int i = 0; i < M; ++i) {
		os << '[';

		for (int j = 0; j < N - 1; ++j) {
			os << mat[i][j] << ' ';
		}

		os << mat[i][N - 1] << "]\n";
	}

	return os;
}

template <ConceptNombre nombre, int M, int N>
auto &operator>>(std::istream &is, matrice_fixe<nombre, M, N> &mat)
{
	char c;

	for (int i = 0; i < M; ++i) {
		is >> c;

		for (int j = 0; j < N - 1; ++j) {
			is >> mat[i][j];
		}

		is >> mat[i][N - 1] >> c;
	}

	return is;
}

/* ****************************** square matrix ***************************** */

template <ConceptNombre nombre, int N>
class matrice_carree : public matrice_fixe<nombre, N, N> {
	using super = matrice_fixe<nombre, N, N>;

public:
	constexpr matrice_carree() = default;

	constexpr matrice_carree(const matrice_carree &cote_droit)
		: super(cote_droit)
	{}

	constexpr matrice_carree(const std::initializer_list<nombre> &list)
		: super(list)
	{}

	~matrice_carree() = default;

	static matrice_carree identity()
	{
		matrice_carree m;
		m.fill(0);

		for (int i = 0; i < N; ++i) {
			m[i][i] = 1;
		}

		return m;
	}

	matrice_carree<nombre, N - 1> sub_matrix(int row, int column) const
	{
		matrice_carree<nombre, N - 1> sub;

		for (int m = 0, mr = 0; m < N; ++m) {
			if (m == row) {
				continue;
			}

			for (int n = 0, nr = 0; n < N; ++n) {
				if (n == column) {
					continue;
				}

				sub[mr][nr] = super::m_data[m][n];

				++nr;
			}

			++mr;
		}

		return sub;
	}
};

template <ConceptNombre nombre, int N>
auto operator*(const matrice_carree<nombre, N> &cote_gauche, const matrice_carree<nombre, N> &cote_droit)
{
	matrice_carree<nombre, N> tmp(cote_gauche);
	tmp *= cote_droit;
	return tmp;
}

template <ConceptNombre nombre, int N>
auto operator*(const matrice_carree<nombre, N> &cote_gauche, const nombre cote_droit)
{
	matrice_carree<nombre, N> tmp(cote_gauche);
	tmp *= cote_droit;
	return tmp;
}

template <ConceptNombre nombre, int N>
auto operator*(const nombre cote_gauche, const matrice_carree<nombre, N> &cote_droit)
{
	matrice_carree<nombre, N> tmp(cote_droit);
	tmp *= cote_gauche;
	return tmp;
}

template <ConceptNombre nombre, int N>
auto minor_det(const matrice_carree<nombre, N> &mat, int m, int n)
{
	return determinant(mat.sub_matrix(m, n));
}

template <ConceptNombre nombre, int N>
auto cofactor(const matrice_carree<nombre, N> &mat, int m, int n)
{
	return std::pow(-1, m + n) * minor_det(mat, m, n);
}

template <ConceptNombre nombre, int N>
struct determinant_helper {
	static nombre calculate(const matrice_carree<nombre, N> &mat)
	{
		nombre det = 0;

		for (int i = 0; i < N; ++i) {
			det = mat[0][i] * cofactor(mat, 0, i);
		}

		return det;
	}
};

template <ConceptNombre nombre>
struct determinant_helper<nombre, 2> {
	static nombre calculate(const matrice_carree<nombre, 2> &mat)
	{
		return (mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0]);
	}
};

template <ConceptNombre nombre, int N>
auto determinant(const matrice_carree<nombre, N> &mat)
{
	return determinant_helper<nombre, N>::calculate(mat);
}

namespace detail {

/**
 * Implémention de la fonction inverse.
 */
template <ConceptNombre Nombre, int N>
bool inverse(matrice_carree<Nombre, N> &matrice_inverse)
{
	constexpr auto ZERO = static_cast<Nombre>(0);
	constexpr auto UN = static_cast<Nombre>(1);
	int index_colonnes[N];
	int index_lignes[N];
	int index_pivots[N] = { 0 };

	for (int i = 0; i < N; i++) {
		auto il = 0;
		auto ic = 0;
		auto big = ZERO;

		/* Choisie le pivot */
		for (int j = 0; j < N; j++) {
			if (index_pivots[j] == 1) {
				continue;
			}

			for (int k = 0; k < N; k++) {
				if (index_pivots[k] == 0) {
					if (std::abs(matrice_inverse[j][k]) >= big) {
						big = static_cast<Nombre>(std::abs(matrice_inverse[j][k]));
						il = j;
						ic = k;
					}
				}
				else if (index_pivots[k] > 1) {
					/* À FAIRE : Considère lancer une erreur :
					 * "Matrice singulière donnée à la fonction inverse". */
					return false;
				}
			}
		}

		++index_pivots[ic];

		/* Permute les lignes _irow_ et _icol_ pour le pivot. */
		if (il != ic) {
			for (int k = 0; k < N; ++k) {
				std::swap(matrice_inverse[il][k], matrice_inverse[ic][k]);
			}
		}

		index_lignes[i] = il;
		index_colonnes[i] = ic;

		if (matrice_inverse[ic][ic] == ZERO) {
			/* À FAIRE : Considère lancer une erreur :
			 * "Matrice singulière donnée à la fonction inverse". */
			return false;
		}

		/* Mise à l'échelle la ligne pour faire en sorte que sa valeur sur la
		 * diagonale soit égal à 1 (un). */
		const auto pivot_inverse = UN / matrice_inverse[ic][ic];
		matrice_inverse[ic][ic] = UN;

		for (int j = 0; j < N; j++) {
			matrice_inverse[ic][j] *= pivot_inverse;
		}

		/* Soustrait cette ligne des autres pour mettre à zéro leurs colonnes. */
		for (int j = 0; j < N; j++) {
			if (j == ic) {
				continue;
			}

			const auto sauvegarde = matrice_inverse[j][ic];
			matrice_inverse[j][ic] = ZERO;

			for (int k = 0; k < N; k++) {
				matrice_inverse[j][k] -= matrice_inverse[ic][k] * sauvegarde;
			}
		}
	}

	/* Échange les colonnes pour réfléchir la permutation. */
	for (int j = N - 1; j >= 0; j--) {
		if (index_lignes[j] == index_colonnes[j]) {
			continue;
		}

		for (int k = 0; k < N; k++) {
			std::swap(matrice_inverse[k][index_lignes[j]],
					  matrice_inverse[k][index_colonnes[j]]);
		}
	}

	return true;
}

}  /* namespace detail */

/**
 * Inverse la matrice spécifiée.
 *
 * Si une inversion n'est pas possible, retourne une matrice identité.
 */
template <ConceptNombre Nombre, int N>
auto inverse(const matrice_carree<Nombre, N> &m)
{
	auto matrice_inverse = m;

	if (!detail::inverse(matrice_inverse)) {
		/* À FAIRE : Considère modifier la diagonale de la matrice un peu pour
		 * éviter les cas de matrices dégénérées. */
		return matrice_carree<Nombre, N>::identity();
	}

	return matrice_inverse;
}

template <ConceptNombre nombre, int N>
auto pow(const matrice_carree<nombre, N> &mat, int e)
{
	if (e == 1) {
		return mat;
	}

	auto res = matrice_carree<nombre, N>::identity();

	for (int i = 0; i < e; ++i) {
		res *= mat;
	}

	return res;
}

template <ConceptNombre nombre, int N>
auto transpose(const matrice_carree<nombre, N> &mat)
{
	matrice_carree<nombre, N> res;

	for (int i = 0; i < N; ++i) {
		for (int j = 0; j < N; ++j) {
			res[j][i] = mat[i][j];
		}
	}

	return res;
}

/**
 * Imprime la matrice dans le flux spécifié avec un certain préfixe.
 */
template <ConceptNombre Nombre, int N>
static void imprime_matrice(
		const matrice_carree<Nombre, N> &mat,
		const char *prefixe,
		std::ostream &os = std::cerr)
{
	os << '\n' << prefixe << '\n' << mat << '\n';
}

/**
 * Retourne vrai si les deux matrices spécifiées sont environ égales selon
 * les épsilons de leur type décimal.
 */
template <ConceptNombre Nombre, int N>
bool sont_environ_egales(
		const matrice_carree<Nombre, N> &a,
		const matrice_carree<Nombre, N> &b)
{
	for (int i = 0; i < N; ++i) {
		for (int j = 0; j < N; ++j) {
			if (!sont_environ_egaux(a[i][j], b[i][j])) {
				return false;
			}
		}
	}

	return true;
}

}  /* namespace math */
}  /* namespace dls */
