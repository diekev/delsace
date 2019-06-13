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

#include "outils.hh"
#include "vecteur.hh"

#include <cassert>

namespace dls::math {

template<size_t...>
struct paquet_index;

template <
		typename type_scalaire,
		template <int, typename, int...> class type_vecteur,
		typename...
		>
struct matrice;

template <
		typename type_scalaire,
		template <int, typename, int...> class type_vecteur,
		size_t... Colonnes,
		size_t... Lignes
		>
struct matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Lignes...>> {
	static constexpr auto N = sizeof...(Colonnes);
	static constexpr auto M = sizeof...(Lignes);

	using type_colonne = type_vecteur<TYPE_VECTEUR, type_scalaire, Colonnes...>;
	using type_ligne   = type_vecteur<TYPE_VECTEUR, type_scalaire, Lignes...>;

	type_colonne data[M];

	matrice() = default;

	/**
	 * Crée une matrice diagonale avec s comme étant la valeur en diagonale.
	 */
	explicit matrice(type_scalaire s)
	{
		((data[Lignes][Lignes] = s), ...);
	}

	template<typename... Args,
			 class = typename std::enable_if<
				 (sizeof... (Args) >= 2)
				 >::type>
	explicit matrice(Args&&... args)
	{
		static_assert((sizeof...(args) <= N*M), "too many arguments");

		size_t i = 0; //TODO: get rid of this
		(construct_at_index(i, detail::dechoie(std::forward<Args>(args))), ...);
	}

	type_colonne &operator[](size_t i) // row access
	{
		return data[i];
	}

	const type_colonne &operator[](size_t i) const // row access
	{
		return data[i];
	}

	matrice &operator+=(const matrice &other)
	{
		((data[Colonnes][Lignes] += other.data[Colonnes][Lignes]), ...);
		return *this;
	}

	matrice &operator-=(const matrice &other)
	{
		((data[Colonnes][Lignes] -= other.data[Colonnes][Lignes]), ...);
		return *this;
	}

	matrice &operator*=(const matrice &other)
	{
		*this = mul(*this, other);
		return *this;
	}

	friend type_colonne operator *(const matrice &m, const type_ligne &v)
	{
		return mul(m, v);
	}

	friend type_ligne operator *(const type_colonne &v, const matrice &m)
	{
		return mul(v, m);
	}

	const type_colonne &colonne(size_t i) const
	{
		return data[i];
	}

	type_ligne ligne(size_t i) const
	{
		type_ligne out;
		((out[Lignes] = data[Lignes][i]), ...);

		return out;
	}

private:
	template<size_t... OtherLignes>
	static auto mul(
			const matrice &m1,
			const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<OtherLignes...>> &m2)
	{
		matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<OtherLignes...>> out;
		((out[Colonnes] = m1 * m2.colonne(Colonnes)), ...);
		return out;
	}

	static type_colonne mul(const matrice &m, const type_ligne &v)
	{
		type_colonne out;
		((out[Lignes] = produit_scalaire(v, m.ligne(Lignes))), ...);
		return out;
	}

	static type_ligne mul(const type_colonne &v, const matrice &m)
	{
		type_ligne out;
		((out[Colonnes] = produit_scalaire(v, m.colonne(Colonnes))), ...);
		return out;
	}

	void construct_at_index(size_t &i, type_scalaire arg)
	{
		data[i / N][i % N] = arg;
		i++;
	}

	template<int O, typename Other, size_t... Other_Ns>
	void construct_at_index(size_t &i, type_vecteur<O, Other, Other_Ns...> &&arg)
	{
		//TODO: do not go over N*M
		detail::boucle_statique<0, sizeof...(Other_Ns)>()([&](size_t j) {
			data[i / N][i % N] = arg[j];
			i++;
		});
	}
};

/*
 * Arithmétique.
 */

template <
		typename type_scalaire,
		template <int, typename, int...> class type_vecteur,
		size_t... Colonnes,
		size_t... Lignes
		>
[[nodiscard]] auto operator+(
		const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Lignes...>> &m1,
		const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Lignes...>> &m2)
{
	auto tmp(m1);
	tmp += m2;
	return tmp;
}

template <
		typename type_scalaire,
		template <int, typename, int...> class type_vecteur,
		size_t... Colonnes,
		size_t... Lignes
		>
[[nodiscard]] auto operator-(
		const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Lignes...>> &m1,
		const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Lignes...>> &m2)
{
	auto tmp(m1);
	tmp -= m2;
	return tmp;
}

template <
		typename type_scalaire,
		template <int, typename, int...> class type_vecteur,
		size_t... Colonnes,
		size_t... Lignes
		>
[[nodiscard]] auto operator*(
		const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Lignes...>> &m1,
		const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Lignes...>> &m2)
{
	auto tmp(m1);
	tmp *= m2;
	return tmp;
}

/*
 * Comparaisons.
 */

template <
		typename type_scalaire,
		template <int, typename, int...> class type_vecteur,
		size_t... Colonnes,
		size_t... Lignes
		>
[[nodiscard]] auto operator==(
		const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Lignes...>> &m1,
		const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Lignes...>> &m2)
{
	return ((m1[Colonnes] == m2[Colonnes]) && ...);
}

template <
		typename type_scalaire,
		template <int, typename, int...> class type_vecteur,
		size_t... Colonnes,
		size_t... Lignes
		>
[[nodiscard]] auto operator!=(
		const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Lignes...>> &m1,
		const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Lignes...>> &m2)
{
	return !(m1 == m2);
}

/*
 * Fonctions.
 */

template <
		typename type_scalaire,
		template <int, typename, int...> class type_vecteur,
		size_t... Colonnes,
		size_t... Lignes
		>
[[nodiscard]] auto transpose(
		const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Lignes...>> &m1)
{
	auto mat = matrice<type_scalaire, type_vecteur, paquet_index<Lignes...>, paquet_index<Colonnes...>>();
	//((mat[Lignes][Colonnes] = m1[Colonnes][Lignes]), ...);

	for (size_t i = 0; i < sizeof...(Colonnes); ++i) {
		for (size_t j = 0; j < sizeof...(Lignes); ++j) {
			mat[j][i] = m1[i][j];
		}
	}

	return mat;
}

namespace detail {

/**
 * Implémention de la fonction inverse.
 */
template <
		typename type_scalaire,
		template <int, typename, int...> class type_vecteur,
		size_t... Colonnes
		>
[[nodiscard]] auto inverse(
		matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Colonnes...>> &matrice_inverse)
{
	static constexpr auto N = sizeof...(Colonnes);
	using Nombre = type_scalaire;

	constexpr auto ZERO = static_cast<Nombre>(0);
	constexpr auto UN = static_cast<Nombre>(1);
	unsigned long index_colonnes[N];
	unsigned long index_lignes[N];
	unsigned long index_pivots[N] = { 0 };

	for (auto i = 0ul; i < N; i++) {
		auto il = 0ul;
		auto ic = 0ul;
		auto big = ZERO;

		/* Choisie le pivot */
		for (auto j = 0ul; j < N; j++) {
			if (index_pivots[j] == 1) {
				continue;
			}

			for (auto k = 0ul; k < N; k++) {
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
			for (auto k = 0ul; k < N; ++k) {
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

		for (auto j = 0ul; j < N; j++) {
			matrice_inverse[ic][j] *= pivot_inverse;
		}

		/* Soustrait cette ligne des autres pour mettre à zéro leurs colonnes. */
		for (auto j = 0ul; j < N; j++) {
			if (j == ic) {
				continue;
			}

			const auto sauvegarde = matrice_inverse[j][ic];
			matrice_inverse[j][ic] = ZERO;

			for (auto k = 0ul; k < N; k++) {
				matrice_inverse[j][k] -= matrice_inverse[ic][k] * sauvegarde;
			}
		}
	}

	/* Échange les colonnes pour réfléchir la permutation. */
	for (int j = N - 1; j >= 0; j--) {
		if (index_lignes[j] == index_colonnes[j]) {
			continue;
		}

		for (auto k = 0ul; k < N; k++) {
			std::swap(matrice_inverse[k][index_lignes[j]],
					matrice_inverse[k][index_colonnes[j]]);
		}
	}

	return true;
}

}  /* namespace detail */

template <
		typename type_scalaire,
		template <int, typename, int...> class type_vecteur,
		size_t... Colonnes
		>
[[nodiscard]] auto inverse(
		const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Colonnes...>> &m)
{
	auto matrice_inverse = m;

	if (!detail::inverse(matrice_inverse)) {
		/* À FAIRE : Considère modifier la diagonale de la matrice un peu pour
		 * éviter les cas de matrices dégénérées. */
		return matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Colonnes...>>(1.0);
	}

	return matrice_inverse;
}
template <
		typename type_scalaire,
		template <int, typename, int...> class type_vecteur,
		size_t... Colonnes
		>
[[nodiscard]] auto inverse_transpose(
		const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Colonnes...>> &m)
{
	return transpose(inverse(m));
}

template <
		typename type_scalaire,
		template <int, typename, int...> class type_vecteur,
		size_t... Colonnes,
		size_t... Lignes
		>
[[nodiscard]] auto sont_environ_egales(
		const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Lignes...>> &m1,
		const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Lignes...>> &m2)
{
	/* À FAIRE : epsilon * 2... */
	return ((std::abs(m1[Colonnes][Lignes] - m2[Colonnes][Lignes]) <= std::numeric_limits<type_scalaire>::epsilon() * 2) && ...);
}

/**
 * Retourne une matrice dont les valeurs correspondent à la normalisation des
 * valeurs des lignes de la matrice spécifiée. La normalisation est effectuée en
 * traitant les lignes comme des vecteurs et en normalisant ceux-ci.
 */
template <
        typename type_scalaire,
        template <int, typename, int...> class type_vecteur,
        size_t... Colonnes,
        size_t... Lignes
        >
[[nodiscard]] auto normalise(
        matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Colonnes...>> const &mat)
{
	auto ret = matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Colonnes...>>{};

	for (size_t i = 0; i < sizeof...(Colonnes); i++) {
		ret[i] = normalise(mat[i]);
	}

	return ret;
}

/**
 * Retourne vrai si la matrice spécifiée est négative.
 */
template <
        typename type_scalaire,
        template <int, typename, int...> class type_vecteur,
        size_t... Colonnes,
        size_t... Lignes
        >
[[nodiscard]] auto est_negative(
        matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Colonnes...>> const &mat)
{
	auto vec = produit_croix(mat[0], mat[1]);
	return (produit_scalaire(vec, mat[2]) < static_cast<type_scalaire>(0.0));
}

/**
 * Nie les valeurs de la matrice spécifiée, à savoir x = -x.
 */
template <
        typename type_scalaire,
        template <int, typename, int...> class type_vecteur,
        size_t... Colonnes,
        size_t... Lignes
        >
void nie(matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Colonnes...>> &m)
{
	for (size_t i = 0; i < sizeof...(Colonnes); i++) {
		for (size_t j = 0; j < sizeof...(Lignes); j++) {
			m[i][j] = -m[i][j];
		}
	}
}

template <
		typename CharT,
		typename Traits,
		typename type_scalaire,
		template <int, typename, int...> class type_vecteur,
		size_t... Colonnes,
		size_t... Lignes
		>
auto &operator<<(
		std::basic_ostream<CharT, Traits> &os,
		const matrice<type_scalaire, type_vecteur, paquet_index<Colonnes...>, paquet_index<Lignes...>> &m1)
{
	for (size_t i = 0; i < sizeof...(Colonnes); ++i) {
		for (size_t j = 0; j < sizeof...(Lignes); ++j) {
			os << m1[i][j] << ' ';
		}

		os << '\n';
	}

	return os;
}

/*
 * Types communs
 */

template <ConceptNombre T>
using mat2x2 = matrice<T, vecteur, paquet_index<0, 1>, paquet_index<0, 1>>;
using mat2x2i = mat2x2<int>;
using mat2x2f = mat2x2<float>;
using mat2x2d = mat2x2<double>;

template <ConceptNombre T>
using mat3x3 = matrice<T, vecteur, paquet_index<0, 1, 2>, paquet_index<0, 1, 2>>;
using mat3x3i = mat3x3<int>;
using mat3x3f = mat3x3<float>;
using mat3x3d = mat3x3<double>;

template <ConceptNombre T>
using mat4x4 = matrice<T, vecteur, paquet_index<0, 1, 2, 3>, paquet_index<0, 1, 2, 3>>;
using mat4x4i = mat4x4<int>;
using mat4x4f = mat4x4<float>;
using mat4x4d = mat4x4<double>;

template <typename T>
[[nodiscard]] auto mat3_depuis_mat4(mat4x4<T> const &m4)
{
	auto m3 = mat3x3<T>{};

	for (size_t i = 0; i < 3; ++i) {
		for (size_t j = 0; j < 3; ++j) {
			m3[i][j] = m4[i][j];
		}
	}

	return m3;
}

template <typename T>
[[nodiscard]] auto operator*(mat4x4<T> const &m4, vec3<T> const &vec)
{
	auto res = vec3<T>{};

	for (size_t i = 0; i < 3; ++i) {
		res[i] = m4[i][0] * vec[0] + m4[i][1] * vec[1] + m4[i][2] * vec[2];
	}

	return res;
}

template <typename T>
[[nodiscard]] auto matrice_rotation(T theta)
{
	auto mat = mat2x2<T>();

	auto c = std::cos(theta);
	auto s = std::sin(theta);

	mat[0][0] = c;
	mat[0][1] = s;

	mat[1][0] = -s;
	mat[1][1] = c;

	return mat;
}

template <typename T>
[[nodiscard]] auto matrice_rotation(const vec3<T> &axis, T angle)
{
	auto tmp = mat3x3<T>();

	auto c = std::cos(angle);
	auto s = std::sin(angle);

	tmp[0][0] = c + (1.0f - c) * carre(axis[0]);
	tmp[0][1] = (1.0f - c) * axis[0] * axis[1] - s * axis[2];
	tmp[0][2] = (1.0f - c) * axis[0] * axis[2] + s * axis[1];

	tmp[1][0] = (1.0f - c) * axis[0] * axis[1] + s * axis[2];
	tmp[1][1] = c + (1.0f - c) * carre(axis[1]);
	tmp[1][2] = (1.0f - c) * axis[1] * axis[2] - s * axis[0];

	tmp[2][0] = (1.0f - c) * axis[0] * axis[2] - s * axis[1];
	tmp[2][1] = (1.0f - c) * axis[1] * axis[2] + s * axis[0];
	tmp[2][2] = c + (1.0f - c) * carre(axis[2]);

	return tmp;
}

/**
 * Construit une matrice encodant la rotation nécessaire pour aligner
 * 'd' à 'z'. 'd' et 'z' doivent être normalisés !
 *
 * Source : http://iquilezles.org/www/articles/noacos/noacos.htm
 */
template <typename T>
[[nodiscard]] auto aligne_rotation(vec3<T> const &z, vec3<T> const &d)
{
	auto const v = produit_croix(z, d);
	auto const c = produit_scalaire(z, d);
	auto const k = static_cast<T>(1) / (static_cast<T>(1) + c);

	return mat3x3<T>(
		v.x * v.x + c,       v.y * v.x * k - v.z, v.z * v.x * k + v.y,
		v.x * v.y * k + v.z, v.y * v.y * k + c,   v.z * v.y * k - v.x,
		v.x * v.z * k - v.y, v.y * v.z * k + v.x, v.z * v.z * k + c  );
}

template <typename T>
[[nodiscard]] auto translation(mat4x4<T> const &m, vec3<T> const &v)
{
	mat4x4<T> Result(m);
	Result[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
	return Result;
}

template <typename T>
[[nodiscard]] auto pre_translation(mat4x4<T> const &m, vec3<T> const &v)
{
	return translation(mat4x4<T>(T(1)), v) * m;
}

template <typename T>
[[nodiscard]] auto post_translation(mat4x4<T> const &m, vec3<T> const &v)
{
	return m * translation(mat4x4<T>(T(1)), v);
}

template <typename T>
[[nodiscard]] auto rotation(mat4x4<T> const & m, T const & angle, vec3<T> const & v)
{
	auto const a = angle;
	auto const c = std::cos(a);
	auto const s = std::sin(a);

	vec3<T> axis(normalise(v));
	vec3<T> temp((T(1) - c) * axis);

	mat4x4<T> Rotate{};
	Rotate[0][0] = c + temp[0] * axis[0];
	Rotate[0][1] = 0 + temp[0] * axis[1] + s * axis[2];
	Rotate[0][2] = 0 + temp[0] * axis[2] - s * axis[1];

	Rotate[1][0] = 0 + temp[1] * axis[0] - s * axis[2];
	Rotate[1][1] = c + temp[1] * axis[1];
	Rotate[1][2] = 0 + temp[1] * axis[2] + s * axis[0];

	Rotate[2][0] = 0 + temp[2] * axis[0] + s * axis[1];
	Rotate[2][1] = 0 + temp[2] * axis[1] - s * axis[0];
	Rotate[2][2] = c + temp[2] * axis[2];

	mat4x4<T> Result{};
	Result[0] = m[0] * Rotate[0][0] + m[1] * Rotate[0][1] + m[2] * Rotate[0][2];
	Result[1] = m[0] * Rotate[1][0] + m[1] * Rotate[1][1] + m[2] * Rotate[1][2];
	Result[2] = m[0] * Rotate[2][0] + m[1] * Rotate[2][1] + m[2] * Rotate[2][2];
	Result[3] = m[3];
	return Result;
}

template <typename T>
[[nodiscard]] auto pre_rotation(mat4x4<T> const & m, T const & angle, vec3<T> const & v)
{
	return rotation(mat4x4<T>(T(1)), angle, v) * m;
}

template <typename T>
[[nodiscard]] auto post_rotation(mat4x4<T> const & m, T const & angle, vec3<T> const & v)
{
	return m * rotation(mat4x4<T>(T(1)), angle, v);
}

template <typename T>
[[nodiscard]] auto dimension(mat4x4<T> const & m, vec3<T> const & v)
{
	mat4x4<T> Result(T(0));
	Result[0] = m[0] * v[0];
	Result[1] = m[1] * v[1];
	Result[2] = m[2] * v[2];
	Result[3] = m[3];
	return Result;
}

template <typename T>
[[nodiscard]] auto pre_dimension(mat4x4<T> const & m, vec3<T> const & v)
{
	return dimension(mat4x4<T>(T(1)), v) * m;
}

template <typename T>
[[nodiscard]] auto post_dimension(mat4x4<T> const & m, vec3<T> const & v)
{
	return m * dimension(mat4x4<T>(T(1)), v);
}

template <typename T>
[[nodiscard]] auto ortho
(
		T const & left,
		T const & right,
		T const & bottom,
		T const & top,
		T const & zNear,
		T const & zFar
		)
{
	mat4x4<T> Result(1);
	Result[0][0] = static_cast<T>(2) / (right - left);
	Result[1][1] = static_cast<T>(2) / (top - bottom);
	Result[2][2] = - T(2) / (zFar - zNear);
	Result[3][0] = - (right + left) / (right - left);
	Result[3][1] = - (top + bottom) / (top - bottom);
	Result[3][2] = - (zFar + zNear) / (zFar - zNear);
	return Result;
}

template <typename T>
[[nodiscard]] auto ortho(
		T const & left,
		T const & right,
		T const & bottom,
		T const & top
		)
{
	mat4x4<T> Result(1);
	Result[0][0] = static_cast<T>(2) / (right - left);
	Result[1][1] = static_cast<T>(2) / (top - bottom);
	Result[2][2] = - T(1);
	Result[3][0] = - (right + left) / (right - left);
	Result[3][1] = - (top + bottom) / (top - bottom);
	return Result;
}

template <typename T>
[[nodiscard]] auto perspective(
		T const & fovy,
		T const & aspect,
		T const & zNear,
		T const & zFar
		)
{
	assert(aspect != T(0));
	assert(zFar != zNear);

	T const rad = fovy;
	T tanHalfFovy = std::tan(rad / T(2));

	mat4x4<T> Result(T(0));
	Result[0][0] = T(1) / (aspect * tanHalfFovy);
	Result[1][1] = T(1) / (tanHalfFovy);
	Result[2][2] = - (zFar + zNear) / (zFar - zNear);
	Result[2][3] = - T(1);
	Result[3][2] = - (T(2) * zFar * zNear) / (zFar - zNear);
	return Result;
}

template <typename T>
[[nodiscard]] auto projette
(
		vec3<T> const & obj,
		mat4x4<T> const & model,
		mat4x4<T> const & proj,
		vec4<T> const & viewport
		)
{
	auto tmp = vec4<T>(obj, T(1));
	tmp = model * tmp;
	tmp = proj * tmp;

	tmp /= tmp.w;
	tmp = tmp * T(0.5) + vec4<T>(0.5);
	tmp[0] = tmp[0] * T(viewport[2]) + T(viewport[0]);
	tmp[1] = tmp[1] * T(viewport[3]) + T(viewport[1]);

	return vec3<T>(tmp);
}

template <typename T>
[[nodiscard]] auto deprojette
(
		vec3<T> const & win,
		mat4x4<T> const & model,
		mat4x4<T> const & proj,
		vec4<T> const & viewport
		)
{
	auto Inverse = inverse(proj * model);

	auto tmp = vec4<T>(win, T(1));
	tmp.x = (tmp.x - T(viewport[0])) / T(viewport[2]);
	tmp.y = (tmp.y - T(viewport[1])) / T(viewport[3]);
	tmp = tmp * T(2) - vec4<T>(1);

	auto obj = Inverse * tmp;
	obj /= obj.w;

	return point3<T>(obj);
}

template <typename T>
[[nodiscard]] auto mire(vec3<T> const & eye, vec3<T> const & center, vec3<T> const & up)
{
	auto f(normalise(center - eye));
	auto s(normalise(produit_croix(f, up)));
	auto u(produit_croix(s, f));

	mat4x4<T> Result(1);
	Result[0][0] = s.x;
	Result[1][0] = s.y;
	Result[2][0] = s.z;
	Result[0][1] = u.x;
	Result[1][1] = u.y;
	Result[2][1] = u.z;
	Result[0][2] =-f.x;
	Result[1][2] =-f.y;
	Result[2][2] =-f.z;
	Result[3][0] =-produit_scalaire(s, eye);
	Result[3][1] =-produit_scalaire(u, eye);
	Result[3][2] = produit_scalaire(f, eye);
	return Result;
}

}  /* namespace dls::math */
