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
#include <limits>
#include "biblinternes/structures/chaine.hh"

#include "concepts.hh"

#include "biblinternes/outils/definitions.h"

namespace dls::math {

/**
 * Converti un nombre de l'espace continu vers l'espace discret.
 */
template <typename Ent, typename Dec>
[[nodiscard]] inline auto continu_vers_discret(Dec cont)
{
	static_assert(std::is_floating_point<Dec>::value
				  && std::is_integral<Ent>::value,
				  "continu_vers_discret va de décimal à entier");

	return static_cast<Ent>(std::floor(cont));
}

/**
 * Converti un nombre de l'espace discret vers l'espace continu.
 */
template <typename Dec, typename Ent>
[[nodiscard]] inline auto discret_vers_continu(Ent disc)
{
	return static_cast<Dec>(disc) + static_cast<Dec>(0.5);
}

/**
 * Retourne l'hypoténuse des nombres réels a et b en évitant les
 * sous/dépassements via (a * racine(1 + (b/a) * (b/a))) au lieu de
 * racine(a*a + b*b).
 */
template <typename T>
[[nodiscard]] constexpr auto hypotenuse(T const a, T const b)
{
	if (a == 0) {
		return std::abs(b);
	}

	if (b == 0) {
		return std::abs(a);
	}

	auto const c = b / a;
	return std::abs(a) * std::sqrt(static_cast<T>(1) + c * c);
}

/**
 * Retourne une valeur équivalente à la restriction de la valeur 'a' entre 'min'
 * et 'max'.
 */
template <typename T>
[[nodiscard]] constexpr auto restreint(const T &a, const T &min, const T &max) noexcept
{
	if (a < min) {
		return min;
	}

	if (a > max) {
		return max;
	}

	return a;
}

/**
 * Retourne un nombre entre 'neuf_min' et 'neuf_max' qui est relatif à 'valeur'
 * dans la plage entre 'vieux_min' et 'vieux_max'. Si la valeur est hors de
 * 'vieux_min' et 'vieux_max' il sera restreint à la nouvelle plage.
 */
template <typename T>
[[nodiscard]] constexpr auto traduit(const T valeur, const T vieux_min, const T vieux_max, const T neuf_min, const T neuf_max) noexcept
{
	T tmp;

	if (vieux_min > vieux_max) {
		tmp = vieux_min - restreint(valeur, vieux_max, vieux_min);
	}
	else {
		tmp = restreint(valeur, vieux_min, vieux_max);
	}

	tmp = (tmp - vieux_min) / (vieux_max - vieux_min);
	return neuf_min + tmp * (neuf_max - neuf_min);
}

template <typename T>
[[nodiscard]] constexpr auto carre(T x) noexcept
{
	return x * x;
}

// transforms even the sequence 0,1,2,3,... into reasonably good random numbers
// challenge: improve on this in speed and "randomness"!
[[nodiscard]] inline auto hash_aleatoire(unsigned int seed) noexcept
{
   unsigned int i = (seed ^ 12345391u) * 2654435769u;

   i ^= (i << 6) ^ (i >> 26);
   i *= 2654435769u;
   i += (i << 5) ^ (i >> 12);

   return i;
}

template <typename T>
[[nodiscard]] inline auto hash_aleatoiref(unsigned int seed) noexcept
{
	return static_cast<T>(hash_aleatoire(seed)) / static_cast<T>(std::numeric_limits<unsigned int>::max());
}

template <typename T>
[[nodiscard]] inline auto hash_aleatoiref(unsigned int seed, T a, T b) noexcept
{
	return ((b - a) * hash_aleatoiref<T>(seed) + a);
}

template <typename type_vecteur>
[[nodiscard]] auto echantillone_sphere(unsigned int &seed) noexcept
{
	using type_scalaire = typename type_vecteur::type_scalaire;

	type_vecteur v;
	type_scalaire m2;

	do {
		m2 = static_cast<type_scalaire>(0);

		for (unsigned int i = 0; i < type_vecteur::nombre_composants; ++i) {
			v[i] = hash_aleatoiref(seed++, -static_cast<type_scalaire>(1), static_cast<type_scalaire>(1));
			m2 += carre(v[i]);
		}
	} while (m2 > static_cast<type_scalaire>(1) || m2 == static_cast<type_scalaire>(0));

	return v / std::sqrt(m2);
}

template <typename T>
[[nodiscard]] auto produit_scalaire(T g[3], T x, T y, T z) noexcept
{
	return g[0] * x + g[1] * y + g[2] * z;
}

template <typename T>
[[nodiscard]] auto radians_vers_degrees(T radian) noexcept
{
	return (radian / static_cast<T>(M_PI)) * static_cast<T>(180);
}

template <typename T>
[[nodiscard]] auto degrees_vers_radians(T degre) noexcept
{
	return (degre / static_cast<T>(180)) * static_cast<T>(M_PI);
}

/**
 * Cas de base, il semblerait que les templates variadiques ne foncionnent pas
 * sans lui.
 */
template <ConceptNombre nombre>
inline auto min(const nombre &a) -> const nombre&
{
	return a;
}

/**
 * Retourne le minimum de deux valeurs.
 */
template <ConceptNombre nombre>
inline auto min(const nombre &a, const nombre &b) -> const nombre&
{
	return (a > b) ? a : b;
}

/**
 * Retourne le minimum de plusieurs valeurs.
 */
template <ConceptNombre nombre, ConceptNombre... nombres>
inline auto min(const nombre &a, const nombre &b, const nombres &... c) -> const nombre&
{
	return min(a, min(b, c...));
}

/**
 * Cas de base, il semblerait que les templates variadiques ne foncionnent pas
 * sans lui.
 */
template <ConceptNombre nombre>
inline auto max(const nombre &a) -> const nombre&
{
	return a;
}

/**
 * Retourne le maximum de deux valeurs.
 */
template <ConceptNombre nombre>
inline auto max(const nombre &a, const nombre &b) -> const nombre&
{
	return (a > b) ? a : b;
}

/**
 * Retourne le maximum de plusieurs valeurs.
 */
template <ConceptNombre nombre, ConceptNombre... nombres>
inline auto max(const nombre &a, const nombre &b, const nombres &... c) -> const nombre&
{
	return max(a, max(b, c...));
}

template <ConceptNombre nombre>
inline auto radian_vers_degre(nombre radian)
{
	return (radian / static_cast<nombre>(M_PI)) * static_cast<nombre>(180);
}

template <ConceptNombre nombre>
inline auto degre_vers_radian(nombre degre)
{
	return (degre / static_cast<nombre>(180)) * static_cast<nombre>(M_PI);
}

template <ConceptNombre nombre>
inline auto etape_lisse(nombre r)
{
	static_assert(std::is_floating_point<nombre>::value,
				  "Cette opération ne fonctionne que sur des nombres décimaux !");

	if (r <= static_cast<nombre>(0)) {
		return static_cast<nombre>(0);
	}

	if (r > static_cast<nombre>(1)) {
		return static_cast<nombre>(1);
	}

	return r * r * r * (10 + r * (-15 + r * 6));
}

template <ConceptNombre nombre>
inline auto etape_lisse(nombre n, nombre n_bas, nombre n_haut, nombre valeur_basse, nombre valeur_haute)
{
	static_assert(std::is_floating_point<nombre>::value,
				  "Cette opération ne fonctionne que sur des nombres décimaux !");

	return valeur_basse + etape_lisse((n - n_bas) / (n_haut - n_bas)) * (valeur_haute - valeur_basse);
}

template <ConceptNombre nombre>
inline auto rampe(nombre n)
{
	static_assert(std::is_floating_point<nombre>::value,
				  "Cette opération ne fonctionne que sur des nombres décimaux !");

	return etape_lisse((n + 1) * 0.5) * 2 - 1;
}

/**
 * Retourne vrai si le nombre spécifié est d'un type intégral et pair.
 */
template <ConceptNombre nombre>
inline auto est_pair(nombre x)
{
	/* cppcheck-suppress syntaxError */
	if constexpr (std::is_integral<nombre>::value) {
		return (x & 0x1) == 0;
	}

	INUTILISE(x);
	return false;
}

template <typename T>
struct est_float : std::false_type {};

template <>
struct est_float<float> : std::true_type {};

template <typename T>
struct est_double : std::false_type {};

template <>
struct est_double<double> : std::true_type {};

template <typename T>
struct est_chaine : std::false_type {};

template <>
struct est_chaine<dls::chaine> : std::true_type {};

template <typename T>
struct est_bool : std::false_type {};

template <>
struct est_bool<bool> : std::true_type {};

/**
 * Retourne la valeur par défaut du type spécifié. Savoir, 0, false, ou "".
 */
template <typename T>
inline auto valeur_nulle()
{
	if constexpr (est_chaine<T>::value) {
		return dls::chaine{""};
	}
	else if constexpr (est_bool<T>::value) {
		return false;
	}

	return T{};
}

/**
 * Tolérance pour les types décimaux. Si le type du nombre n'est pas décimal,
 * retourne 0.
 *
 * NOTE : ne peut utiliser ConceptNombre car valeur_nulle peut prendre une
 * dls::chaine.
 */
template <typename nombre>
inline auto tolerance()
{
	if constexpr (est_float<nombre>::value) {
		return 1e-8f;
	}
	else if constexpr (est_double<nombre>::value) {
		return 1e-15;
	}

	return valeur_nulle<nombre>();
}

/**
 * Delta pour de petit écart de nombres décimaux. Si le type du nombre n'est pas
 * décimal, retourne 0.
 */
template <ConceptNombre nombre>
inline auto delta()
{
	if constexpr (est_float<nombre>::value) {
		return 1e-5f;
	}
	else if constexpr (est_double<nombre>::value) {
		return 1e-9;
	}

	return valeur_nulle<nombre>();
}

/**
 * Retourne vrai si le nombre spécifié est à peu près égal à zéro suivant la
 * comparaison de nombre décimaux par défaut.
 */
template <ConceptNombre nombre>
inline auto est_environ_zero(nombre n)
{
	const auto t = valeur_nulle<nombre>() + tolerance<nombre>();
	return !(n > t) && !(n < -t);
}

/**
 * Retourne vrai si le nombre spécifié est à peu près égal à zéro suivant la
 * tolérance spécifiée.
 */
template <ConceptNombre nombre>
inline auto est_environ_zero(nombre n, nombre t)
{
	return !(n > t) && !(n < -t);
}

/**
 * Retourne vrai si les nombres spécifiés sont à peu près égaux suivant la
 * comparaison de nombre décimaux par défaut.
 */
template <ConceptNombre nombre>
inline auto sont_environ_egaux(nombre a, nombre b)
{
	const auto t = valeur_nulle<nombre>() + tolerance<nombre>();
	return !(std::abs(a - b) > t);
}

/**
 * Retourne vrai si les nombres spécifiés sont à peu près égaux suivant la
 * tolérance spécifiée.
 */
template <ConceptNombre nombre>
inline auto sont_environ_egaux(nombre a, nombre b, nombre t)
{
	return !(std::abs(a - b) > t);
}

/**
 * Retourne vrai si les nombres spécifiés sont relativment égaux suivant la
 * tolérance absolue ou relative spécifiée.
 */
template <ConceptNombre nombre>
inline auto sont_relativement_egaux(nombre a, nombre b, nombre t_abs, nombre t_rel)
{
	/* D'abord, vérifions que nous sommes dans la tolérance absolue, ce qui est
	 * nécessaire pour les nombres proches de zéro. */
	if (!(std::abs(a - b) > t_abs)) {
		return true;
	}

	/* Ensuite, vérifions si nous sommes dans la tolérance relative pour prendre
	 * en compte les grands nombres qui ne sont pas dans la tolérance absolue
	 * mais qui pourrait être la représentation décimale la plus proche. */
	const auto erreur = std::abs(a - b) / ((std::abs(b) > std::abs(a)) ? b : a);
	return (erreur <= t_rel);
}

template <ConceptNombre N>
inline auto reciproque(N n)
{
	return static_cast<N>(1) / n;
}

template <typename T>
struct nombre_bit {
	static constexpr std::size_t valeur = sizeof(T) * 8;
};

/* Puissance de nombres entiers positifs. */

template <std::size_t B, std::size_t E>
struct puissance {
	enum {
		valeur = B * puissance<B, E - 1>::valeur
	};
};

template <std::size_t E>
struct puissance<1, E> {
	enum {
		valeur = 1
	};
};

template <std::size_t E>
struct puissance<0, E> {
	enum {
		valeur = 0
	};
};

template <std::size_t B>
struct puissance<B, 0> {
	enum {
		valeur = 1
	};
};

}  /* namespace dls::math */
