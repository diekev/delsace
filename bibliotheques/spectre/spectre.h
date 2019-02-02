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
#include <vector>

#include "bibliotheques/outils/constantes.h"

/* ************************************************************************** */

inline void xyz_vers_rgb(const float xyz[3], float rgb[3])
{
	rgb[0] =  3.240479f * xyz[0] - 1.537150f * xyz[1] - 0.498535f * xyz[2];
	rgb[1] = -0.969256f * xyz[0] + 1.875991f * xyz[1] + 0.041556f * xyz[2];
	rgb[2] =  0.055648f * xyz[0] - 0.204043f * xyz[1] + 1.057311f * xyz[2];
}

inline void rgb_vers_xyz(const float rgb[3], float xyz[3])
{
	xyz[0] = 0.412453f * rgb[0] + 0.357580f * rgb[1] + 0.180423f * rgb[2];
	xyz[1] = 0.212671f * rgb[0] + 0.715160f * rgb[1] + 0.072169f * rgb[2];
	xyz[2] = 0.019334f * rgb[0] + 0.119193f * rgb[1] + 0.950227f * rgb[2];
}

float moyenne_echantillons(const float *lambdas, const float *valeurs, size_t n, const float debut_lambda, const float fin_lambda);

bool echantillons_spectre_trie(const float *lambdas, const float */*valeurs*/, size_t n);

void trie_echantillons_spectre(float *lambdas, float *valeurs, size_t n);

float entrepole_echantillons_spectre(const float *lambdas,
		const float *valeurs,
		size_t n,
		float l);

enum TypeSpectre {
	SPECTRE_REFLECTANCE,
	SPECTRE_ILLUMINANT,
};

/* ************************************************************************** */

// Spectral Data Declarations
static const int nCIESamples = 471;
extern const float CIE_X[nCIESamples];
extern const float CIE_Y[nCIESamples];
extern const float CIE_Z[nCIESamples];
extern const float CIE_lambda[nCIESamples];
static const float CIE_Y_integral = 106.856895f;

static const int nRGB2SpectSamples = 32;
extern const float RGB2SpectLambda[nRGB2SpectSamples];
extern const float RGBRefl2SpectWhite[nRGB2SpectSamples];
extern const float RGBRefl2SpectCyan[nRGB2SpectSamples];
extern const float RGBRefl2SpectMagenta[nRGB2SpectSamples];
extern const float RGBRefl2SpectYellow[nRGB2SpectSamples];
extern const float RGBRefl2SpectRed[nRGB2SpectSamples];
extern const float RGBRefl2SpectGreen[nRGB2SpectSamples];
extern const float RGBRefl2SpectBlue[nRGB2SpectSamples];
extern const float RGBIllum2SpectWhite[nRGB2SpectSamples];
extern const float RGBIllum2SpectCyan[nRGB2SpectSamples];
extern const float RGBIllum2SpectMagenta[nRGB2SpectSamples];
extern const float RGBIllum2SpectYellow[nRGB2SpectSamples];
extern const float RGBIllum2SpectRed[nRGB2SpectSamples];
extern const float RGBIllum2SpectGreen[nRGB2SpectSamples];
extern const float RGBIllum2SpectBlue[nRGB2SpectSamples];

/* ************************************************************************** */

/**
 * Représentation de l'élément de base pour définir le spectre lumineux.
 */
template <unsigned int NombreCoefficients>
class SpectreCoefficient {
protected:
	float m_coefficients[NombreCoefficients];

public:
	explicit SpectreCoefficient(float coefficient = 0.0f)
	{
		for (unsigned i = 0; i < NombreCoefficients; ++i) {
			m_coefficients[i] = coefficient;
		}
	}

	float operator[](const std::size_t i) const
	{
		return m_coefficients[i];
	}

	float &operator[](const std::size_t i)
	{
		return m_coefficients[i];
	}

	/**
	 * Ajoute les valeurs des coefficients du spectre spécifié à celui-ci.
	 */
	SpectreCoefficient &operator+=(SpectreCoefficient const &spectre)
	{
		for (unsigned i = 0; i < NombreCoefficients; ++i) {
			m_coefficients[i] += spectre.m_coefficients[i];
		}

		return *this;
	}

	/**
	 * Soustrait les valeurs des coefficients du spectre spécifié de celui-ci.
	 */
	SpectreCoefficient &operator-=(SpectreCoefficient const &spectre)
	{
		for (unsigned i = 0; i < NombreCoefficients; ++i) {
			m_coefficients[i] -= spectre.m_coefficients[i];
		}

		return *this;
	}

	/**
	 * Multiplie les valeurs des coefficients de ce spectre par celles du
	 * spectre spécifié.
	 */
	SpectreCoefficient &operator*=(SpectreCoefficient const &spectre)
	{
		for (unsigned i = 0; i < NombreCoefficients; ++i) {
			m_coefficients[i] *= spectre.m_coefficients[i];
		}

		return *this;
	}

	/**
	 * Multiplie les valeurs des coefficients de ce spectre par la valeur
	 * spécifiée.
	 */
	SpectreCoefficient &operator*=(const float valeur)
	{
		for (unsigned i = 0; i < NombreCoefficients; ++i) {
			m_coefficients[i] *= valeur;
		}

		return *this;
	}

	/**
	 * Divise les valeurs des coefficients de ce spectre par celles du
	 * spectre spécifié.
	 */
	SpectreCoefficient &operator/=(SpectreCoefficient const &spectre)
	{
		for (unsigned i = 0; i < NombreCoefficients; ++i) {
			if (m_coefficients[i] == 0.0f || spectre.m_coefficients[i] == 0.0f) {
				continue;
			}

			m_coefficients[i] /= spectre.m_coefficients[i];
		}

		return *this;
	}

	/**
	 * Divise les valeurs des coefficients de ce spectre par celles du
	 * spectre spécifié.
	 */
	SpectreCoefficient &operator/=(float const &valeur)
	{
		for (unsigned i = 0; i < NombreCoefficients; ++i) {
			if (m_coefficients[i] == 0.0f || valeur == 0.0f) {
				continue;
			}

			m_coefficients[i] /= valeur;
		}

		return *this;
	}

	/**
	 * Retourne un spectre dont les coefficients résultent de la négation des
	 * coefficient de celui-ci.
	 */
	SpectreCoefficient operator-() const
	{
		SpectreCoefficient resultat = *this;

		for (int i = 0; i < NombreCoefficients; ++i) {
			resultat.m_coefficients[i] = -resultat.m_coefficients[i];
		}

		return resultat;
	}

	/**
	 * Retourne vrai si tous les coefficients de ce spectre sont égaux à zéro.
	 */
	bool est_noir() const
	{
		for (int i = 0; i < NombreCoefficients; ++i) {
			if (m_coefficients[i] != 0.0) {
				return false;
			}
		}

		return true;
	}

	/**
	 * Retourne vrai si l'un des coefficients n'est pas un nombre.
	 */
	bool a_nans() const
	{
		for (int i = 0; i < NombreCoefficients; ++i) {
			if (std::isnan(m_coefficients[i])) {
				return true;
			}
		}

		return false;
	}
};

/**
 * Retourne un spectre dont les coefficients résultent de l'addition de ceux
 * des deux spectres spécifiés.
 */
template <unsigned int NombreCoefficients>
inline auto operator+(
		SpectreCoefficient<NombreCoefficients> const &a,
		SpectreCoefficient<NombreCoefficients> const &b)
{
	SpectreCoefficient tmp(a);
	tmp += b;
	return tmp;
}

/**
 * Retourne un spectre dont les coefficients résultent de la soustraction de
 * ceux du deuxième spectre à ceux du premier spécifié.
 */
template <unsigned int NombreCoefficients>
inline auto operator-(
		SpectreCoefficient<NombreCoefficients> const &a,
		SpectreCoefficient<NombreCoefficients> const &b)
{
	SpectreCoefficient tmp(a);
	tmp -= b;
	return tmp;
}

/**
 * Retourne un spectre dont les coefficients résultent de la multiplication de
 * ceux de premier spectre par ceux du deuxième spectre spécifié.
 */
template <unsigned int NombreCoefficients>
inline auto operator*(
		SpectreCoefficient<NombreCoefficients> const &a,
		SpectreCoefficient<NombreCoefficients> const &b)
{
	SpectreCoefficient tmp(a);
	tmp *= b;
	return tmp;
}

/**
 * Retourne un spectre dont les coefficients résultent de la multiplication de
 * ceux de premier spectre par la valeur spécifiée.
 */
template <unsigned int NombreCoefficients>
inline auto operator*(
		const float a,
		SpectreCoefficient<NombreCoefficients> const &b)
{
	SpectreCoefficient<NombreCoefficients> tmp(a);
	tmp *= b;
	return tmp;
}

/**
 * Retourne un spectre dont les coefficients résultent de la multiplication de
 * ceux du spectre par la valeur spécifiée.
 */
template <unsigned int NombreCoefficients>
inline auto operator*(
		SpectreCoefficient<NombreCoefficients> const &a,
		const float b)
{
	SpectreCoefficient<NombreCoefficients> tmp(b);
	tmp *= a;
	return tmp;
}

/**
 * Retourne un spectre dont les coefficients résultent de la division de ceux
 * de premier spectre par ceux du deuxième spectre spécifié.
 */
template <unsigned int NombreCoefficients>
inline auto operator/(
		SpectreCoefficient<NombreCoefficients> const &a,
		SpectreCoefficient<NombreCoefficients> const &b)
{
	SpectreCoefficient tmp(a);
	tmp /= b;
	return tmp;
}

/**
 * Retourne un spectre dont les coefficients résultent de la division de ceux
 * de premier spectre par la valeur spécifié.
 */
template <unsigned int NombreCoefficients>
inline auto operator/(
		SpectreCoefficient<NombreCoefficients> const &a,
		float const &b)
{
	SpectreCoefficient<NombreCoefficients> tmp(a);
	tmp /= b;
	return tmp;
}

/**
 * Retourne un spectre dont les coefficients résultent de la réciproque de ceux
 * de premier spectre par la valeur spécifié.
 */
template <unsigned int NombreCoefficients>
inline auto operator/(
		float const &a,
		SpectreCoefficient<NombreCoefficients> const &b)
{
	SpectreCoefficient<NombreCoefficients> tmp(a);
	tmp /= b;
	return tmp;
}

/**
 * Retourne un spectre dont les coefficients sont égaux à la racine carré des
 * coefficients du spectre spécifié.
 */
template <unsigned int NombreCoefficients>
inline auto racine_carre(SpectreCoefficient<NombreCoefficients> const &spectre)
{
	SpectreCoefficient<NombreCoefficients> resultat;

	for (int i = 0; i < NombreCoefficients; ++i) {
		resultat[i] = std::sqrt(spectre[i]);
	}

	return resultat;
}

/**
 * Retourne un spectre dont les coefficients sont égaux aux coefficients du
 * spectre spécifié élévés à la puissance exposant.
 */
template <unsigned int NombreCoefficients>
inline auto puissance(SpectreCoefficient<NombreCoefficients> const &spectre, float exposant)
{
	SpectreCoefficient<NombreCoefficients> resultat;

	for (unsigned int i = 0; i < NombreCoefficients; ++i) {
		resultat[i] = std::pow(spectre[i], exposant);
	}

	return resultat;
}

/**
 * Retourne un spectre dont les coefficients sont égaux à l'exponentielle des
 * coefficients du spectre spécifié.
 */
template <unsigned int NombreCoefficients>
inline auto exponentielle(SpectreCoefficient<NombreCoefficients> const &spectre)
{
	SpectreCoefficient<NombreCoefficients> resultat;

	for (int i = 0; i < NombreCoefficients; ++i) {
		resultat[i] = std::exp(spectre[i]);
	}

	return resultat;
}

/**
 * Retourne un spectre dont les coefficients résultent de l'entrepolation
 * linéaire des deux spectres spécifiés par le coefficient donné.
 */
template <unsigned int NombreCoefficients>
inline auto entrepolation_lineaire(
		SpectreCoefficient<NombreCoefficients> const &a,
		SpectreCoefficient<NombreCoefficients> const &b,
		float t)
{
	return (1.0f - t) * a + t * b;
}

/**
 * Retourne un spectre dont les coefficients résultent de la restriction de
 * ceux du spectre spécifié entre les valeurs min et max données en paramètre.
 */
template <unsigned int NombreCoefficients>
inline auto restreint(
		SpectreCoefficient<NombreCoefficients> const &spectre,
		const float min = 0.0,
		const float max = INFINITEF)
{
	SpectreCoefficient resultat(spectre);

	for (unsigned i = 0; i < NombreCoefficients; ++i) {
		if (resultat[i] < min) {
			resultat[i] = min;
		}
		else if (resultat[i] > max) {
			resultat[i] = max;
		}
	}

	return resultat;
}

/* ************************************************************************** */

class SpectreRGB;

/**
 * Début du spectre à échantillonner en nanomètre.
 */
static constexpr auto DEBUT_LAMBDA_SPECTRE = 400.0f;

/**
 * Fin du spectre à échantillonner en nanomètre.
 */
static constexpr auto FIN_LAMBDA_SPECTRE = 700.0f;

/**
 * Nombre d'échantillons à prendre dans le spectre.
 */
static constexpr auto NOMBRE_ECHANTILLONS = 30;

/**
 * Représente un spectre échantillonné de manière uniforme entre
 * DEBUT_LAMBDA_SPECTRE et FIN_LAMBDA_SPECTRE NOMBRE_ECHANTILLONS de fois.
 */
class SpectreEchantillon : public SpectreCoefficient<NOMBRE_ECHANTILLONS> {
private:
	static SpectreEchantillon X, Y, Z;
	static SpectreEchantillon rgbRefl2SpectWhite, rgbRefl2SpectCyan;
	static SpectreEchantillon rgbRefl2SpectMagenta, rgbRefl2SpectYellow;
	static SpectreEchantillon rgbRefl2SpectRed, rgbRefl2SpectGreen;
	static SpectreEchantillon rgbRefl2SpectBlue;
	static SpectreEchantillon rgbIllum2SpectWhite, rgbIllum2SpectCyan;
	static SpectreEchantillon rgbIllum2SpectMagenta, rgbIllum2SpectYellow;
	static SpectreEchantillon rgbIllum2SpectRed, rgbIllum2SpectGreen;
	static SpectreEchantillon rgbIllum2SpectBlue;

public:
	explicit SpectreEchantillon(const float coefficient = 0.0);

	/* cppcheck-suppress noExplicitConstructor */
	SpectreEchantillon(SpectreCoefficient<NOMBRE_ECHANTILLONS> const &spectre);

	SpectreEchantillon(SpectreRGB const &r, TypeSpectre t);

	static SpectreEchantillon depuis_echantillons(const float *lambda, const float *v, size_t n);

	static SpectreEchantillon depuis_rgb(const float rgb[3], TypeSpectre type = TypeSpectre::SPECTRE_REFLECTANCE);

	static SpectreEchantillon depuis_rgb(float r, float g, float b, TypeSpectre type = SPECTRE_REFLECTANCE);

	SpectreRGB vers_spectre_rvb();

	void vers_xyz(float xyz[3]) const;

	float y() const;

	void vers_rvb(float rgb[3]) const;

	static void initialisation();
};

inline auto entrepolation_lineaire(SpectreEchantillon const &a, SpectreEchantillon const &b, const float t)
{
	return (1.0f - t) * a + t * b;
}

/* ************************************************************************** */

class SpectreRGB : public SpectreCoefficient<3> {
public:
	explicit SpectreRGB(float valeur = 0.0f);

	explicit SpectreRGB(double valeur);

	/* cppcheck-suppress noExplicitConstructor */
	SpectreRGB(SpectreCoefficient<3> const &spectre);

	SpectreRGB(SpectreRGB const &spectre, TypeSpectre type = SPECTRE_REFLECTANCE);

	static SpectreRGB depuis_rgb(float rgb[3], TypeSpectre type = SPECTRE_REFLECTANCE);

	static SpectreRGB depuis_rgb(float r, float g, float b, TypeSpectre type = SPECTRE_REFLECTANCE);

	void vers_rvb(float *rgb) const;

	SpectreRGB vers_spectre_rvb() const;

	static SpectreRGB depuis_echantillons(const float *lambda, const float *v, size_t n);

	static SpectreRGB depuis_xyz(float *xyz);

	void vers_xyz(float *xyz);

	float y() const;
};

inline auto entrepolation_lineaire(SpectreRGB const &a, SpectreRGB const &b, const float t)
{
	return (1.0f - t) * a + t * b;
}

inline bool operator==(SpectreRGB const &a, SpectreRGB const &b)
{
	for (size_t i = 0; i < 3; ++i) {
		if (std::abs(a[i] - b[i]) > 1e-6f) {
			return false;
		}
	}

	return true;
}

inline bool operator!=(SpectreRGB const &a, SpectreRGB const &b)
{
	return !(a == b);
}

inline bool operator<(SpectreRGB const &a, SpectreRGB const &b)
{
	for (size_t i = 0; i < 3; ++i) {
		if (a[i] > b[i]) {
			return false;
		}
	}

	return true;
}

inline bool operator<=(SpectreRGB const &a, SpectreRGB const &b)
{
	return (a == b) || (a < b);
}

inline bool operator>(SpectreRGB const &a, SpectreRGB const &b)
{
	for (size_t i = 0; i < 3; ++i) {
		if (a[i] > b[i]) {
			return false;
		}
	}

	return true;
}

inline bool operator>=(SpectreRGB const &a, SpectreRGB const &b)
{
	return (a == b) || (a > b);
}

/* ************************************************************************** */

#define SPECTRE_RGB

#ifdef SPECTRE_RGB
using Spectre = SpectreRGB;
#else
using Spectre = SpectreEchantillon;
#endif
