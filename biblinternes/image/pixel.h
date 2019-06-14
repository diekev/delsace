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

#include "../math/concepts.hh"

#include <cassert>

namespace dls {
namespace image {

enum {
	CANAL_R = 0,
	CANAL_G = 1,
	CANAL_B = 2,
	CANAL_A = 3,
};

/**
 * Cette structure représente un pixel de 4 éléments ou canaux : rouge, vert,
 * bleu, et alpha, dénotés respectivement r, g, b, et a.
 */
template <ConceptNombre nombre>
struct Pixel {
	nombre r;
	nombre g;
	nombre b;
	nombre a;

	/**
	 * Construit un pixel par défaut. Les valeurs du pixel ne sont pas
	 * initialisées !
	 */
	Pixel() = default;

	/**
	 * Construit un pixel à partir d'une valeur unique. Après cette opération,
	 * les quatres éléments ou canaux du pixel seront égaux à la valeur
	 * spécifiée.
	 */
	explicit Pixel(const nombre &valeur)
		: r(valeur)
		, g(valeur)
		, b(valeur)
		, a(valeur)
	{}

	/**
	 * Construit un pixel à partir d'un autre, qui reste inchangé. Après cette
	 * opération la condition `*this == autre` est avérée.
	 */
	Pixel(const Pixel &autre)
		: r(autre.r)
		, g(autre.g)
		, b(autre.b)
		, a(autre.a)
	{}

	const nombre operator[](int i) const
	{
		switch (i) {
			case 0:
				return r;
			case 1:
				return g;
			case 2:
				return b;
			case 3:
				return a;
		}

		assert("Le nombre passé en paramètre est invalide !");
		return 0;
	}

	nombre &operator[](int i)
	{
		switch (i) {
			case 0:
				return r;
			case 1:
				return g;
			case 2:
				return b;
			case 3:
				return a;
		}

		assert("Le nombre passé en paramètre est invalide !");
		return r;
	}

	/**
	 * Assigne les valeurs d'un autre pixel, qui reste inchangé, à celui-ci.
	 * Après cette opération la condition `*this == autre` est avérée.
	 */
	Pixel &operator=(const Pixel &autre)
	{
		r = autre.r;
		g = autre.g;
		b = autre.b;
		a = autre.a;

		return *this;
	}

	/**
	 * Additionne les valeurs d'un autre pixel à celles de celui-ci.
	 */
	Pixel &operator+=(const Pixel &autre)
	{
		/* on fait ça au lieu de x += autre.x car GCC converti silencieusement
		 * les unsigned chars en 'long' pour l'opération, et une erreur de
		 * compilation intervient lors de l'assignation du 'long' dans le uchar
		 * de départ. */
		r = static_cast<nombre>(r + autre.r);
		g = static_cast<nombre>(g + autre.g);
		b = static_cast<nombre>(b + autre.b);
		a = static_cast<nombre>(a + autre.a);

		return *this;
	}

	/**
	 * Additionne le nombre spécifié aux valeurs de ce pixel-ci.
	 */
	Pixel &operator+=(const nombre &autre)
	{
		r += autre;
		g += autre;
		b += autre;
		a += autre;

		return *this;
	}

	/**
	 * Soustrait les valeurs d'un autre pixel à celles de celui-ci.
	 */
	Pixel &operator-=(const Pixel &autre)
	{
		/* on fait ça au lieu de x -= autre.x car GCC converti silencieusement
		 * les unsigned chars en 'long' pour l'opération, et une erreur de
		 * compilation intervient lors de l'assignation du 'long' dans le uchar
		 * de départ. */
		r = static_cast<nombre>(r - autre.r);
		g = static_cast<nombre>(g - autre.g);
		b = static_cast<nombre>(b - autre.b);
		a = static_cast<nombre>(a - autre.a);

		return *this;
	}

	/**
	 * Soustrait le nombre spécifié aux valeurs de ce pixel-ci.
	 */
	Pixel &operator-=(const nombre &autre)
	{
		r -= autre;
		g -= autre;
		b -= autre;
		a -= autre;

		return *this;
	}

	/**
	 * Multiplie les valeurs de ce pixel par celles d'un autre.
	 */
	Pixel &operator*=(const Pixel &autre)
	{
		/* on fait ça au lieu de x *= autre.x car GCC converti silencieusement
		 * les unsigned chars en 'long' pour l'opération, et une erreur de
		 * compilation intervient lors de l'assignation du 'long' dans le uchar
		 * de départ. */
		r = static_cast<nombre>(r * autre.r);
		g = static_cast<nombre>(g * autre.g);
		b = static_cast<nombre>(b * autre.b);
		a = static_cast<nombre>(a * autre.a);

		return *this;
	}

	/**
	 * Multiplie les valeurs de ce pixel par le nombre spécifié.
	 */
	Pixel &operator*=(const nombre &autre)
	{
		r *= autre;
		g *= autre;
		b *= autre;
		a *= autre;

		return *this;
	}

	/**
	 * Divise les valeurs de ce pixel par les valeurs d'un autre. La division
	 * d'une valeur n'est effectuée que si la valeur dénominatrice est
	 * différente de zéro.
	 */
	Pixel &operator/=(const Pixel &autre)
	{
		if (autre.r != 0) {
			r /= autre.r;
		}

		if (autre.g != 0) {
			g /= autre.g;
		}

		if (autre.b != 0) {
			b /= autre.b;
		}

		if (autre.a != 0) {
			a /= autre.a;
		}

		return *this;
	}

	/**
	 * Divise les valeurs de ce pixel par le nombre spécifié. La division n'est
	 * effectuée que si la nombre est différent de zéro.
	 */
	Pixel &operator/=(const nombre &autre)
	{
		if (autre != 0) {
			r /= autre;
			g /= autre;
			b /= autre;
			a /= autre;
		}

		return *this;
	}
};

/* ************************ Opérateurs mathématiques ************************ */

/**
 * Retourne un pixel dont les valeurs correspondent à l'addition des valeurs des
 * deux pixels spécifiés.
 */
template <ConceptNombre nombre>
auto operator+(const Pixel<nombre> &cote_droit, const Pixel<nombre> &cote_gauche)
{
	Pixel<nombre> temp(cote_droit);
	temp += cote_gauche;
	return temp;
}

/**
 * Retourne un pixel dont les valeurs correspondent à la soustraction des
 * valeurs du deuxième pixel spécifié à celles du premier.
 */
template <ConceptNombre nombre>
auto operator-(const Pixel<nombre> &cote_droit, const Pixel<nombre> &cote_gauche)
{
	Pixel<nombre> temp(cote_droit);
	temp -= cote_gauche;
	return temp;
}

/**
 * Retourne un pixel dont les valeurs correspondent à la multiplication des
 * valeurs des deux pixels spécifiés.
 */
template <ConceptNombre nombre>
auto operator*(const Pixel<nombre> &cote_droit, const Pixel<nombre> &cote_gauche)
{
	Pixel<nombre> temp(cote_droit);
	temp *= cote_gauche;
	return temp;
}

/**
 * Retourne un pixel dont les valeurs correspondent à la multiplication des
 * valeurs du pixel par le nombre spécifié.
 */
template <ConceptNombre nombre>
auto operator*(const nombre &cote_droit, const Pixel<nombre> &cote_gauche)
{
	Pixel<nombre> temp(cote_gauche);
	temp *= cote_droit;
	return temp;
}

/**
 * Retourne un pixel dont les valeurs correspondent à la multiplication des
 * valeurs du pixel par le nombre spécifié.
 */
template <ConceptNombre nombre>
auto operator*(const Pixel<nombre> &cote_droit, const nombre &cote_gauche)
{
	Pixel<nombre> temp(cote_droit);
	temp *= cote_gauche;
	return temp;
}

/**
 * Retourne un pixel dont les valeurs correspondent à la division des valeurs du
 * deuxième pixel spécifié par celles du premier.
 */
template <ConceptNombre nombre>
auto operator/(const Pixel<nombre> &cote_droit, const Pixel<nombre> &cote_gauche)
{
	Pixel<nombre> temp(cote_droit);
	temp /= cote_gauche;
	return temp;
}

/* *********************** Opérateurs de comparaison ************************ */

/**
 * Retourne vrai si les valeurs des deux pixels spécifiés en paramètre sont
 * toutes égales entre elles.
 */
template <ConceptNombre nombre>
auto operator==(const Pixel<nombre> &cote_droit, const Pixel<nombre> &cote_gauche)
{
	return (cote_droit.r == cote_gauche.r)
			&& (cote_droit.g == cote_gauche.g)
			&& (cote_droit.b == cote_gauche.b)
			&& (cote_droit.a == cote_gauche.a);
}

/**
 * Retourne vrai si toutes ou certaines des valeurs des deux pixels spécifiés
 * en paramètre sont différentes.
 */
template <ConceptNombre nombre>
auto operator!=(const Pixel<nombre> &cote_droit, const Pixel<nombre> &cote_gauche)
{
	return !(cote_droit == cote_gauche);
}

/**
 * Retourne vrai si les valeurs du premier pixel spécifié sont toutes
 * inférieures à celles du deuxième pixel spécifié.
 */
template <ConceptNombre nombre>
auto operator<(const Pixel<nombre> &cote_droit, const Pixel<nombre> &cote_gauche)
{
	return (cote_droit.r < cote_gauche.r)
			&& (cote_droit.g < cote_gauche.g)
			&& (cote_droit.b < cote_gauche.b)
			&& (cote_droit.a < cote_gauche.a);
}

/**
 * Retourne vrai si les valeurs du premier pixel spécifié sont toutes
 * inférieures ou égales à celles du deuxième pixel spécifié.
 */
template <ConceptNombre nombre>
auto operator<=(const Pixel<nombre> &cote_droit, const Pixel<nombre> &cote_gauche)
{
	return (cote_droit < cote_gauche) || (cote_droit == cote_gauche);
}

/**
 * Retourne vrai si les valeurs du premier pixel spécifié sont toutes
 * supérieures à celles du deuxième pixel spécifié.
 */
template <ConceptNombre nombre>
auto operator>(const Pixel<nombre> &cote_droit, const Pixel<nombre> &cote_gauche)
{
	return (cote_droit.r > cote_gauche.r)
			&& (cote_droit.g > cote_gauche.g)
			&& (cote_droit.b > cote_gauche.b)
			&& (cote_droit.a > cote_gauche.a);
}

/**
 * Retourne vrai si les valeurs du premier pixel spécifié sont toutes
 * supérieures ou égales à celles du deuxième pixel spécifié.
 */
template <ConceptNombre nombre>
auto operator>=(const Pixel<nombre> &cote_droit, const Pixel<nombre> &cote_gauche)
{
	return (cote_droit > cote_gauche) || (cote_droit == cote_gauche);
}

/* ********************* Déclaration de types communs *********************** */

using PixelChar = Pixel<unsigned char>;
using PixelFloat = Pixel<float>;

}  /* namespace image */
}  /* namespace dls */
