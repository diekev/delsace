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

#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>

#include "concepts.hh"
#include "base_vecteur.hh"

namespace dls::math {

namespace detail {

template <size_t Debut, size_t Fin>
struct boucle_statique {
	template <class Func>
	constexpr void operator()(Func &&f)
	{
		if constexpr (Debut < Fin) {
			f(Debut);
			boucle_statique<Debut + 1, Fin>()(std::forward<Func>(f));
		}
	}
};

/**
 * Retourne la valeur déchue du paramètre passé en paramètre selon son type.
 *
 * Si le type est scalaire, retourne la valeur passée en paramètre.
 * Sinon appel t.dechoie(), donc le type doit être déchoiable.
 */
template <typename T>
constexpr auto dechoie(T &&t)
{
	if constexpr (std::is_arithmetic<typename std::remove_reference<T>::type >::value) {
		return t;
	}
	else {
		return t.dechoie();
	}
}

}  /* namespace detail */

/**
 * Valeurs utilisées pour un avoir un typage strict entre vecteurs, points, et
 * vecteurs normaux.
 */
static constexpr auto TYPE_POINT = 0;
static constexpr auto TYPE_VECTEUR = 1;
static constexpr auto TYPE_NORMAL = 2;

/**
 * Classe représentant un vecteur mathématique générique de N dimensions.
 *
 * Le nombre de dimension est défini par le nombe de Ns passé en paramètres de
 * gabarit, les Ns définissant les index du vecteur. Par exemple un vecteur 3D
 * doit avoir Ns = 0, 1, 2.
 *
 * Le premier paramètre du gabarit définit le « type » du vecteur : point,
 * vecteur, ou vecteur normal.
 */
template <int O, typename T, int... Ns>
struct vecteur final : public detail::selecteur_base_vecteur<O, T, Ns...>::type_base {
	using type_scalaire = T;
	using type_vecteur = vecteur<O, T, Ns...>;
	static constexpr auto nombre_composants = sizeof...(Ns);
	using type_dechu = typename std::conditional<nombre_composants == 1, type_scalaire, type_vecteur>::type;

	vecteur()
	{
		((this->data[Ns] = 0), ...);
	}

	explicit vecteur(type_scalaire scalaire)
	{
		((this->data[Ns] = scalaire), ...);
	}

	template <typename Arg0, typename... Args, class = typename std::enable_if<
				  ((sizeof... (Args) >= 1) ||
				  ((sizeof...(Args) == 0) && !std::is_scalar_v<Arg0>))>::type>
	explicit vecteur(Arg0 &&a0, Args... args)
	{
		static_assert((sizeof...(args) < nombre_composants), "trop d'arguments");

		size_t i = 0;
		construit_index(i, detail::dechoie(std::forward<Arg0>(a0)));
		(construit_index(i, detail::dechoie(std::forward<Args>(args))), ...);
	}

	type_dechu dechoie() const
	{
		return static_cast<const type_dechu &>(*this);
	}

	type_scalaire &operator[](size_t i)
	{
		return this->data[i];
	}

	const type_scalaire &operator[](size_t i) const
	{
		return this->data[i];
	}

	type_vecteur &operator+=(type_scalaire scalaire)
	{
		((this->data[Ns] += scalaire), ...);
		return *this;
	}

	type_vecteur &operator+=(const type_vecteur &v)
	{
		((this->data[Ns] += v.data[Ns]), ...);
		return *this;
	}

	type_vecteur &operator-=(type_scalaire scalaire)
	{
		((this->data[Ns] -= scalaire), ...);
		return *this;
	}

	type_vecteur &operator-=(const type_vecteur &v)
	{
		((this->data[Ns] -= v.data[Ns]), ...);
		return *this;
	}

	type_vecteur &operator*=(type_scalaire scalaire)
	{
		((this->data[Ns] *= scalaire), ...);
		return *this;
	}

	type_vecteur &operator*=(const type_vecteur &v)
	{
		((this->data[Ns] *= v.data[Ns]), ...);
		return *this;
	}

	type_vecteur &operator/=(type_scalaire scalaire)
	{
		if (scalaire != static_cast<type_scalaire>(0)) {
			((this->data[Ns] /= scalaire), ...);
		}
		else {
			((this->data[Ns] = 0), ...);
		}

		return *this;
	}

	type_vecteur &operator/=(const type_vecteur &v)
	{
		((this->data[Ns] = (v.data[Ns] == 0) ? 0 : this->data[Ns] / v.data[Ns]), ...);
		return *this;
	}

private:
	auto construit_index(size_t &i, type_scalaire scalaire)
	{
		this->data[i++] = scalaire;
	}

	template <int OO, typename Other, int... Other_Ns>
	auto construit_index(size_t &i, vecteur<OO, Other, Other_Ns...> &&argument)
	{
		constexpr auto other_num = vecteur<OO, Other, Other_Ns...>::nombre_composants;
		constexpr auto compte = nombre_composants <= other_num ? nombre_composants : other_num;

		detail::boucle_statique<0, compte>()([&, this](size_t j)
		{
			this->data[i++] = argument.data[j];
		});
	}
};

/*
 * Arithmétique.
 */

/**
 * Retourne un vecteur dont les valeurs correspondent à l'addition des valeurs
 * des deux vecteurs spécifiés.
 *
 * Si l'un des vecteurs est de type point et l'autre de type vecteur, retourne
 * un de type point.
 */
template <int O1, int O2, typename T, int... Ns>
[[nodiscard]] auto operator+(const vecteur<O1, T, Ns...> &v1, const vecteur<O2, T, Ns...> &v2)
{
	if constexpr (O1 == O2) {
		auto tmp(v1);
		tmp += v2;
		return tmp;
	}
	else if constexpr (O1 == TYPE_VECTEUR && O2 == TYPE_POINT) {
		auto tmp = vecteur<TYPE_POINT, T, Ns...>(v1);
		tmp += v2;
		return tmp;
	}
	else if constexpr (O1 == TYPE_POINT && O2 == TYPE_VECTEUR) {
		auto tmp = vecteur<TYPE_POINT, T, Ns...>(v2);
		tmp += v1;
		return tmp;
	}
}

/**
 * Retourne un vecteur dont les valeurs correspondent à la soustraction des
 * valeurs du vecteur cote_droit à celles du vecteur cote_gauche.
 *
 * Si les vecteurs sont de types 'TYPE_POINT', le résultat sera de type
 * 'TYPE_VECTEUR'.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] auto operator-(vecteur<O, T, Ns...> const &v1, vecteur<O, T, Ns...> const &v2)
{
	auto tmp(v1);
	tmp -= v2;

	if constexpr (O == TYPE_POINT) {
		return vecteur<TYPE_VECTEUR, T, Ns...>(tmp);
	}
	else {
		return tmp;
	}
}

/**
 * Retourne un vecteur dont les valeurs correspondent à la soustraction de la
 * valeur spécifiée aux valeurs de vecteur donné.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] auto operator-(vecteur<O, T, Ns...> const &v)
{
	auto tmp(v);
	((tmp[Ns] = -tmp[Ns]), ...);
	return tmp;
}

/**
 * Retourne un vecteur dont les valeurs correspondent à la multiplication des
 * valeurs des deux vecteurs spécifiés.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] auto operator*(vecteur<O, T, Ns...> const &v1, vecteur<O, T, Ns...> const &v2)
{
	auto tmp(v1);
	tmp *= v2;
	return tmp;
}

/**
 * Retourne un vecteur dont les valeurs correspondent à la multiplication des
 * valeurs du vecteur spécifié avec la valeur donnée.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] auto operator*(vecteur<O, T, Ns...> const &v, const T valeur)
{
	auto tmp(v);
	tmp *= valeur;
	return tmp;
}

/**
 * Retourne un vecteur dont les valeurs correspondent à la multiplication des
 * valeurs du vecteur spécifié avec la valeur donnée.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] auto operator*(const T valeur, vecteur<O, T, Ns...> const &v)
{
	auto tmp(v);
	tmp *= valeur;
	return tmp;
}

/**
 * Retourne un vecteur dont les valeurs correspondent à la division des valeurs du
 * premier vecteur par celles du deuxième.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] auto operator/(vecteur<O, T, Ns...> const &v1, vecteur<O, T, Ns...> const &v2)
{
	auto tmp(v1);
	tmp /= v2;
	return tmp;
}

/**
 * Retourne un vecteur dont les valeurs correspondent à la division des valeurs
 * du vecteur spécifié avec la valeur donnée.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] auto operator/(vecteur<O, T, Ns...> const &v, const T valeur)
{
	if (valeur == static_cast<T>(0)) {
		return vecteur<O, T, Ns...>();
	}

	auto tmp(v);
	tmp /= valeur;
	return tmp;
}

/**
 * Retourne un vecteur dont les valeurs correspondent à la division réciproque
 * des valeurs du vecteur spécifié avec la valeur donnée.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] auto operator/(const T valeur, vecteur<O, T, Ns...> const &v)
{
	auto tmp(v);
	tmp /= valeur;
	return tmp;
}

/*
 * Comparaison.
 */

/**
 * Retourne vrai si les valeurs des deux vecteurs sont égales entre elles.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto operator==(vecteur<O, T, Ns...> const &v1, vecteur<O, T, Ns...> const &v2)
{
	return ((v1[Ns] == v2[Ns]) && ...);
}

/**
 * Retourne vrai si les valeurs des deux vecteurs diffèrent.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto operator!=(vecteur<O, T, Ns...> const &v1, vecteur<O, T, Ns...> const &v2)
{
	return !(v1 == v2);
}

/**
 * Retourne vrai si les valeurs du premier vecteur sont inférieures à celles du
 * deuxième.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto operator<(vecteur<O, T, Ns...> const &v1, vecteur<O, T, Ns...> const &v2)
{
	return ((v1[Ns] < v2[Ns]) || ...);
}

/**
 * Retourne vrai si les valeurs du premier vecteur sont inférieures ou égales
 * à celles du deuxième.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto operator<=(vecteur<O, T, Ns...> const &v1, vecteur<O, T, Ns...> const &v2)
{
	return ((v1[Ns] <= v2[Ns]) && ...);
}

/**
 * Retourne vrai si les valeurs du premier vecteur sont inférieures à celles du
 * deuxième.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto operator>(vecteur<O, T, Ns...> const &v1, vecteur<O, T, Ns...> const &v2)
{
	return ((v1[Ns] > v2[Ns]) || ...);
}

/**
 * Retourne vrai si les valeurs du premier vecteur sont supérieures ou égales
 * à celles du deuxième.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto operator>=(vecteur<O, T, Ns...> const &v1, vecteur<O, T, Ns...> const &v2)
{
	return ((v1[Ns] >= v2[Ns]) && ...);
}

/*
 * Outils
 */

/**
 * Retourne le produit scalaire des deux vecteurs, à savoir le cosinus de
 * l'angle entre eux.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto produit_scalaire(vecteur<O, T, Ns...> const &u, vecteur<O, T, Ns...> const &v)
{
	return ((u[Ns] * v[Ns]) + ...);
}

/**
 * Retourne la valeur absolue du produit scalaire des deux vecteurs, à savoir le
 * cosinus de l'angle entre eux.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto produit_scalaire_abs(vecteur<O, T, Ns...> const &u, vecteur<O, T, Ns...> const &v)
{
	return std::abs(((u[Ns] * v[Ns]) + ...));
}

/**
 * Retourne la longueur du vecteur.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto longueur(vecteur<O, T, Ns...> const &v)
{
	auto produit_interne = ((v[Ns] * v[Ns]) + ...);
	return std::sqrt(produit_interne);
}

/**
 * Retourne la longueur carrée du vecteur.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto longueur_carree(vecteur<O, T, Ns...> const &v)
{
	return ((v[Ns] * v[Ns]) + ...);
}

/**
 * Retourne un vecteur dont les valeurs résultent du produit vectorielle des
 * deux vecteurs spécifiés.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto produit_vectorielle(vecteur<O, T, Ns...> const &u, vecteur<O, T, Ns...> const &v)
{
	constexpr auto N = sizeof...(Ns);

	auto tmp = vecteur<O, T, Ns...>{};
	((tmp[Ns] = u[(Ns + 1) % N] * v[(Ns + 2) % N] - u[(Ns + 2) % N] * v[(Ns + 1) % N]), ...);

	return tmp;
}

/**
 * Retourne un vecteur dont les valeurs résultent du produit vectorielle des
 * deux vecteurs spécifiés.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto produit_croix(vecteur<O, T, Ns...> const &u, vecteur<O, T, Ns...> const &v)
{
	return produit_vectorielle(u, v);
}

/**
 * Retourne un vecteur dont les valeurs correspondent à celles du vecteur
 * spécifié mais ayant une longueur égale à un.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto normalise(vecteur<O, T, Ns...> const &v)
{
	auto l = longueur(v);

	if (l == static_cast<T>(0)) {
		return vecteur<O, T, Ns...>(static_cast<T>(0));
	}

	return v / l;
}

/**
 * Retourne un vecteur dont les valeurs correspondent à celles du vecteur
 * spécifié mais ayant une longueur égale à un.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto normalise(vecteur<O, T, Ns...> const &v, T &l)
{
	l = longueur(v);

	if (l == static_cast<T>(0)) {
		return vecteur<O, T, Ns...>(static_cast<T>(0));
	}

	return v / l;
}

/**
 * Retourne un vecteur dont les valeurs correspondent à celles du vecteur
 * spécifié mais ayant une longueur égale à un.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto vecteur_unitaire(vecteur<O, T, Ns...> const &v)
{
	return normalise(v);
}

/**
 * Retourne vrai si le vecteur contient des NANs.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto est_nan(vecteur<O, T, Ns...> const &v)
{
	return ((std::isnan(v[Ns])) || ...);
}

/**
 * Retourne vrai si le vecteur contient des valeurs infinies.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto est_infini(vecteur<O, T, Ns...> const &v)
{
	return ((std::isinf(v[Ns])) || ...);
}

/**
 * Retourne vrai si le vecteur ne contient que des valeurs finies.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto est_fini(vecteur<O, T, Ns...> const &v)
{
	return ((std::isfinite(v[Ns])) && ...);
}

/**
 * Retourne un vecteur contenant les valeurs absolues du vecteur spécifié.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto absolu(vecteur<O, T, Ns...> const &v)
{
	auto tmp = vecteur<O, T, Ns...>();
	((tmp[Ns] = std::abs(v[Ns])), ...);
	return tmp;
}

/**
 * Retourne un vecteur contenant les valeurs exponentielles du vecteur spécifié.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto exponentiel(vecteur<O, T, Ns...> const &v)
{
	auto tmp = vecteur<O, T, Ns...>();
	((tmp[Ns] = std::exp(v[Ns])), ...);
	return tmp;
}

/**
 * Compare les composants de v avec ceux de min et max, et stocke dans min les
 * composants de v plus petits que ceux de min, et dans max, ceux plus grand.
 */
template <int O, typename T, int... Ns>
constexpr auto extrait_min_max(
		vecteur<O, T, Ns...> const &v,
		vecteur<O, T, Ns...> &min,
		vecteur<O, T, Ns...> &max)
{
	for (auto i = 0ul; i < sizeof...(Ns); ++i) {
		if (v[i] < min[i]) {
			min[i] = v[i];
		}
		else if (v[i] > max[i]) {
			max[i] = v[i];
		}
	}
}

/**
 * Retourne la valeur minimale du vecteur.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto min(vecteur<O, T, Ns...> const &v)
{
	auto ret = v[0];

	for (auto i = 1ul; i < sizeof...(Ns); ++i) {
		ret = std::min(ret, v[i]);
	}

	return ret;
}

/**
 * Retourne la valeur maximale du vecteur.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] constexpr auto max(vecteur<O, T, Ns...> const &v)
{
	auto ret = v[0];

	for (auto i = 1ul; i < sizeof...(Ns); ++i) {
		ret = std::max(ret, v[i]);
	}

	return ret;
}

/**
 * Retourne l'index de l'axe dont la valeur absolue est la valeur la plus grande
 * du vecteur.
 */
template <int O, typename T, int... Ns>
[[nodiscard]] auto axe_dominant_abs(vecteur<O, T, Ns...> const &v)
{
	auto index = 0ul;
	auto vmax = std::abs(v[0]);

	for (auto i = 1ul; i < sizeof...(Ns); ++i) {
		if (std::abs(v[i]) > vmax) {
			index = i;
			vmax = std::abs(v[i]);
		}
	}

	return index;
}

/**
 * Retourne l'aire du triangle représenté par les trois vecteurs spécifiés.
 */
template <int O, typename T, int... Ns>
auto calcule_aire(
        vecteur<O, T, Ns...> const &v0,
        vecteur<O, T, Ns...> const &v1,
        vecteur<O, T, Ns...> const &v2)
{
	auto const c1 = v1 - v0;
	auto const c2 = v2 - v0;

	return longueur(produit_croix(c1, c2)) * static_cast<T>(0.5);
}

/**
 * Retourne un vecteur correspondant à la réfraction du vecteur 'I' dans la
 * surface avec 'N' comme vecteur normal selon l'index de réfraction fourni.
 */
template <int O, typename T, int... Ns>
auto refracte(
        vecteur<O, T, Ns...> const &I,
        vecteur<O, T, Ns...> const &N,
        T const idr)
{
	auto Nrefr = N;
	auto cos_theta = produit_scalaire(Nrefr, I);
	auto eta_dehors = static_cast<T>(1);
	auto eta_dedans = idr;

	if (cos_theta < static_cast<T>(0)) {
		/* Nous sommes hors de la surface, nous voulons cos(theta) positif. */
		cos_theta = -cos_theta;
	}
	else {
		/* Nous sommes dans la surface, cos(theta) est déjà positif, mais nie le
		 * normal. */
		Nrefr = -N;

		std::swap(eta_dehors, eta_dedans);
	}

	auto eta = eta_dehors / eta_dedans;
	auto k = static_cast<T>(1) - eta * eta * (static_cast<T>(1) - cos_theta * cos_theta);

	if (k < static_cast<T>(0)) {
		return vecteur<O, T, Ns...>(static_cast<T>(0));
	}

	return eta * I + (eta * cos_theta - std::sqrt(k)) * Nrefr;
}

/**
 * Calcul le coefficient de fresnel avec l'index de réfraction fourni.
 */
template <int O, typename T, int... Ns>
auto fresnel(
        vecteur<O, T, Ns...> const &I,
        vecteur<O, T, Ns...> const &N,
        T const &idr)
{
	auto cosi = produit_scalaire(I, N);
	auto eta_dehors = static_cast<T>(1);
	auto eta_dedans = idr;

	if (cosi > 0) {
		std::swap(eta_dehors, eta_dedans);
	}

	/* Calcul sin_i selon la loi de Snell. */
	auto sint = eta_dehors / eta_dedans * std::sqrt(std::max(static_cast<T>(0), static_cast<T>(1) - cosi * cosi));

	/* Réflection interne totale. */
	if (sint >= static_cast<T>(1)) {
		return static_cast<T>(1);
	}

	auto cost = std::sqrt(std::max(static_cast<T>(0), static_cast<T>(1) - sint * sint));
	cosi = std::abs(cosi);
	auto Rs = ((eta_dedans * cosi) - (eta_dehors * cost)) / ((eta_dedans * cosi) + (eta_dehors * cost));
	auto Rp = ((eta_dehors * cosi) - (eta_dedans * cost)) / ((eta_dehors * cosi) + (eta_dedans * cost));
	auto kr = (Rs * Rs + Rp * Rp) / static_cast<T>(2);

	/* En conséquence de la conservation d'énergie, la transmittance est donnée
	 * par kt = 1 - kr. */
	return kr;
}

/**
 * Retourne un vecteur correspondant à la réfléction du vecteur 'I' sur la
 * surface dont le vecteur normal est 'N'.
 */
template <int O, typename T, int... Ns>
auto reflechi(
        vecteur<O, T, Ns...> const &I,
        vecteur<O, T, Ns...> const &N)
{
	return I - static_cast<T>(2) * produit_scalaire(I, N) * N;
}

/* ***************************** opérateurs flux **************************** */

/**
 * Passe le vecteur au basic_ostream spécifié au format (x,...,).
 */
template <int O, typename T, int... Ns, typename CharT, typename Traits>
auto &operator<<(std::basic_ostream<CharT, Traits> &os, vecteur<O, T, Ns...> const &v)
{
	std::basic_ostringstream<CharT, Traits> s;
	s.flags(os.flags());
	s.imbue(os.getloc());
	s.precision(os.precision());

	auto virgule = '(';

	for (auto i = 0ul; i < sizeof...(Ns); ++i) {
		s << virgule << v[i];
		virgule = ',';
	}

	s << ')';

	return os << s.str();
}

/**
 * Prend le vecteur du basic_istream spécifié du format (x,...,).
 */
template <int O, typename T, int... Ns, typename CharT, typename Traits>
auto &operator>>(std::basic_istream<CharT, Traits> &is, vecteur<O, T, Ns...> &v)
{
	char c;

	for (auto i = 0ul; i < sizeof...(Ns); ++i) {
		is >> c >> v[i];
	}

	is >> c;

	return is;
}

/*
 * Types communs.
 */

template <typename T>
using vec2 = vecteur<TYPE_VECTEUR, T, 0, 1>;
using vec2i = vec2<int>;
using vec2f = vec2<float>;
using vec2d = vec2<double>;

template <typename T>
using point2 = vecteur<TYPE_POINT, T, 0, 1>;
using point2i = point2<int>;
using point2f = point2<float>;
using point2d = point2<double>;

template <typename T>
using norm2 = vecteur<TYPE_NORMAL, T, 0, 1>;
using norm2i = norm2<int>;
using norm2f = norm2<float>;
using norm2d = norm2<double>;

static_assert(sizeof(vec2f) == (2 * sizeof(float)), "");

template <typename T>
using vec3 = vecteur<TYPE_VECTEUR, T, 0, 1, 2>;
using vec3i = vec3<int>;
using vec3f = vec3<float>;
using vec3d = vec3<double>;

template <typename T>
using point3 = vecteur<TYPE_POINT, T, 0, 1, 2>;
using point3i = point3<int>;
using point3f = point3<float>;
using point3d = point3<double>;

template <typename T>
using norm3 = vecteur<TYPE_NORMAL, T, 0, 1, 2>;
using norm3i = norm3<int>;
using norm3f = norm3<float>;
using norm3d = norm3<double>;

static_assert(sizeof(vec3f) == (3 * sizeof(float)), "");

template <typename T>
using vec4 = vecteur<TYPE_VECTEUR, T, 0, 1, 2, 3>;
using vec4i = vec4<int>;
using vec4f = vec4<float>;
using vec4d = vec4<double>;

template <typename T>
using point4 = vecteur<TYPE_POINT, T, 0, 1, 2, 3>;
using point4i = point4<int>;
using point4f = point4<float>;
using point4d = point4<double>;

template <typename T>
using norm4 = vecteur<TYPE_NORMAL, T, 0, 1, 2, 3>;
using norm4i = norm4<int>;
using norm4f = norm4<float>;
using norm4d = norm4<double>;

static_assert(sizeof(vec4f) == (4 * sizeof(float)), "");

/**
 * "Building an orthonormal basis, revisited"
 * http://graphics.pixar.com/library/OrthonormalB/paper.pdf
 */
template <typename T>
void cree_base_orthonormale(
        vec3<T> const &n,
        vec3<T> &b0,
        vec3<T> &b1)
{
	auto const sign = std::copysign(static_cast<T>(1.0), n.z);
	auto const a = static_cast<T>(-1.0) / (sign + n.z);
	auto const b = n.x * n.y * a;
	b0 = vec3<T>(static_cast<T>(1.0) + sign * n.x * n.x * a, sign * b, -sign * n.x);
	b1 = vec3<T>(b, sign + n.y * n.y * a, -n.y);
}

}  /* namespace dls::math */
