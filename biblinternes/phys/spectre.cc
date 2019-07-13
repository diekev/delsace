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

#include "spectre.hh"

#include <algorithm>
#include "biblinternes/math/entrepolation.hh"

#include "biblinternes/outils/definitions.h"

/* ************************************************************************** */

template <typename Predicate>
auto trouve_entrevalle(long taille, Predicate const &predicat)
{
	auto debut = 0l, longueur = taille;

	while (longueur > 0) {
		auto moitie = longueur >> 1, milieu = debut + moitie;

		/* Bissecte la plage selon la valeur du prédicat au milieu. */
		if (predicat(milieu)) {
			debut = milieu + 1;
			longueur -= moitie + 1;
		}
		else {
			longueur = moitie;
		}
	}

	return dls::math::restreint(debut - 1, 0l, taille - 2);
}

float moyenne_echantillons(
		const float *lambdas,
		const float *valeurs,
		long n,
		const float debut_lambda,
		const float fin_lambda)
{
	if (fin_lambda <= lambdas[0]) {
		return valeurs[0];
	}

	if (debut_lambda >= lambdas[n - 1]) {
		return valeurs[n - 1];
	}

	if (n == 1) {
		return valeurs[0];
	}

	auto somme = 0.0f;

	if (debut_lambda < lambdas[0]) {
		somme += valeurs[0] * (lambdas[0] - debut_lambda);
	}

	if (fin_lambda > lambdas[n - 1]) {
		somme += valeurs[n - 1] * (fin_lambda - lambdas[n - 1]);
	}

	auto i = 0;

	while (debut_lambda > lambdas[i + 1]) {
		++i;
	}

	auto entrepole = [lambdas, valeurs](float w, long iv)
	{
		return dls::math::entrepolation_lineaire(
					valeurs[iv],
					valeurs[iv + 1],
				(w - lambdas[iv]) / (lambdas[iv + 1] - lambdas[iv]));
	};

	for (; i + 1 < n && fin_lambda >= lambdas[i]; ++i) {
		auto debut_segment = std::max(debut_lambda, lambdas[i]);
		auto fin_segment = std::min(fin_lambda, lambdas[i + 1]);

		somme += 0.5f * entrepole(debut_segment, i) + entrepole(debut_segment, i) * (fin_segment - debut_segment);
	}

	return somme / (fin_lambda - debut_lambda);
}

bool echantillons_spectre_trie(const float *lambdas, const float *, long n)
{
	for (auto i = 0; i < n - 1; ++i) {
		if (lambdas[n] > lambdas[n + 1]) {
			return false;
		}
	}

	return true;
}

void trie_echantillons_spectre(
		float *lambdas,
		float *valeurs,
		long n)
{
	dls::tableau<std::pair<float, float>> vec;
	vec.reserve(n);

	for (auto i = 0; i < n; ++i) {
		vec.pousse(std::make_pair(lambdas[i], valeurs[i]));
	}

	std::sort(vec.debut(), vec.fin());

	for (auto i = 0; i < n; ++i) {
		lambdas[i] = vec[i].first;
		valeurs[i] = vec[i].second;
	}
}

float entrepole_echantillons_spectre(
		const float *lambdas,
		const float *valeurs,
		long n,
		float l)
{
	if (l <= lambdas[0]) {
		return valeurs[0];
	}

	if (l >= lambdas[n - 1]) {
		return valeurs[n - 1];
	}

	auto const offset = trouve_entrevalle(n, [&](long index) { return lambdas[index] <= l; });

	auto const t = (l - lambdas[offset]) / (lambdas[offset + 1] - lambdas[offset]);

	return dls::math::entrepolation_lineaire(valeurs[offset], valeurs[offset + 1], t);
}

// Given a piecewise-linear SPD with values in vIn[] at corresponding
// wavelengths lambdaIn[], where lambdaIn is assumed to be sorted but may
// be irregularly spaced, resample the spectrum over the range of
// wavelengths [lambdaMin, lambdaMax], with a total of nOut wavelength
// samples between lambdaMin and lamdbaMax (including those at
// endpoints). The resampled spectrum values are written to vOut.
//
// In general, this is a standard sampling and reconstruction problem, with
// the complication that for any given invocation, some of the
// reconstruction points may involve upsampling the input distribution and
// others may involve downsampling. For upsampling, we just point-sample,
// and for downsampling, we apply a box filter centered around the
// destination wavelength with total width equal to the sample spacing.
static void reechantillone_spectre_lineaire(
		const float *lambdaIn,
		const float *vIn,
		long nIn,
		float lambdaMin,
		float lambdaMax,
		size_t nOut,
		float *vOut)
{
	/* Espace entre les échantillons dans la distribution de sortie. */
	auto delta = (lambdaMax - lambdaMin) / static_cast<float>(nOut - 1);

	// We assume that the SPD is constant outside of the specified
	// wavelength range, taking on the respectively first/last SPD value
	// for out-of-range wavelengths.
	//
	// To make this convention fit cleanly into the code below, we create
	// virtual samples in the input distribution with index -1 for the
	// sample before the first valid sample and index nIn for the sample
	// after the last valid sample. In turn, can place those virtual
	// samples beyond the endpoints of the target range so that we can
	// always assume that the source range is broader than the target
	// range, which in turn lets us not worry about various boundary cases
	// below.

	// The wavelengths of the virtual samples at the endpoints are set so
	// that they are one destination sample spacing beyond the destination
	// endpoints.  (Note that this potentially means that if we swept along
	// indices from -1 to nIn, we wouldn't always see a monotonically
	// increasing set of wavelength values. However, this isn't a problem
	// since we only access these virtual samples if the destination range
	// is wider than the source range.)
	auto lambdaInClamped = [&](long index)
	{
		if (index == -1l) {
			return lambdaMin - delta;
		}

		if (index == nIn) {
			return lambdaMax + delta;
		}

		return lambdaIn[index];
	};

	// Due to the piecewise-constant assumption, the SPD values outside the
	// specified range are given by the valid endpoints.
	auto vInClamped = [&](long index)
	{
		return vIn[dls::math::restreint(index, 0l, nIn - 1)];
	};

	// Helper that upsamples ors downsample the given SPD at the given
	// wavelength lambda.
	auto resample = [&](float lambda) -> float
	{
		// Handle the edge cases first so that we don't need to worry about
		// them in the following.
		//
		// First, if the entire filtering range for the destination is
		// outside of the range of valid samples, we can just return the
		// endpoint value.
		if (lambda + delta / 2 <= lambdaIn[0]) {
			return vIn[0];
		}

		if (lambda - delta / 2 >= lambdaIn[nIn - 1]) {
			return vIn[nIn - 1];
		}

		// Second, if there's only one sample, then the SPD has the same
		// value everywhere, and we're done.
		if (nIn == 1) {
			return vIn[0];
		}

		// Otherwise, find indices into the input SPD that bracket the
		// wavelength range [lambda-delta, lambda+delta]. Note that this is
		// a 2x wider range than we will actually filter over in the end.
		long start, end;
		if (lambda - delta < lambdaIn[0]) {
			// Virtual sample at the start, as described above.
			start = -1l;
		}
		else {
			start = trouve_entrevalle(
				nIn, [&](long i) { return lambdaIn[i] <= lambda - delta; });
		}

		if (lambda + delta > lambdaIn[nIn - 1]){
			// Virtual sample at the end, as described above.
			end = nIn;
		}
		else {
			// Linear search from the starting point. (Presumably more
			// efficient than a binary search from scratch, or doesn't
			// matter either way.)
			end = start;
			while (end < nIn && lambda + delta > lambdaIn[end]) ++end;
		}

		if (end - start == 2 && lambdaInClamped(start) <= lambda - delta &&
			lambdaIn[start + 1] == lambda &&
			lambdaInClamped(end) >= lambda + delta) {
			// Downsampling: special case where the input and output
			// wavelengths line up perfectly, so just return the
			// corresponding point sample at lambda.
			return vIn[start + 1];
		}

		if (end - start == 1) {
			// Downsampling: evaluate the piecewise-linear function at
			// lambda.
			auto t = (lambda - lambdaInClamped(start)) /
					  (lambdaInClamped(end) - lambdaInClamped(start));
			return dls::math::entrepolation_lineaire(vInClamped(start), vInClamped(end), t);
		}

		// Upsampling: use a box filter and average all values in the
		// input spectrum from lambda +/- delta / 2.
		return moyenne_echantillons(
					lambdaIn, vIn, nIn, lambda - delta / 2, lambda + delta / 2);
	};

	// For each destination sample, compute the wavelength lambda for the
	// sample and then resample the source SPD distribution at that point.
	for (size_t outOffset = 0; outOffset < nOut; ++outOffset) {
		// TODO: Currently, resample() does a binary search each time,
		// even though we could do a single sweep across the input array,
		// since we're resampling it at a regular and increasing set of
		// lambdas. It would be nice to polish that up.
		auto lambda =
			dls::math::entrepolation_lineaire(
					lambdaMin,
					lambdaMax,
					static_cast<float>(outOffset) / static_cast<float>(nOut - 1));

		vOut[outOffset] = resample(lambda);
	}
}

void corps_noir(const float *lambda, int n, float T, float *Le)
{
	if (T <= 0) {
		for (int i = 0; i < n; ++i) {
			Le[i] = 0.f;
		}

		return;
	}

	auto const c = 299792458.0f;
	auto const h = 6.62606957e-34f;
	auto const kb = 1.3806488e-23f;

	for (int i = 0; i < n; ++i) {
		/* Calcule la radiance émise pour le corps noir à la longeur d'onde
		 * lambda[i] */
		auto l = lambda[i] * 1e-9f;
		auto lambda5 = (l * l) * (l * l) * l;

		Le[i] = (2.0f * h * c * c) /
				(lambda5 * std::expm1((h * c) / (l * kb * T)));
	}
}

void corps_noir_normalise(const float *lambda, int n, float T, float *Le)
{
	corps_noir(lambda, n, T, Le);

	// Normalize _Le_ values based on maximum blackbody radiance
	float lambda_max = 2.8977721e-3f / T * 1e9f;

	float maxL;
	corps_noir(&lambda_max, 1, T, &maxL);

	for (int i = 0; i < n; ++i) {
		Le[i] /= maxL;
	}
}

/* ************************************************************************** */

// Spectral Data Definitions
SpectreEchantillon SpectreEchantillon::X;
SpectreEchantillon SpectreEchantillon::Y;
SpectreEchantillon SpectreEchantillon::Z;
SpectreEchantillon SpectreEchantillon::rgbRefl2SpectWhite;
SpectreEchantillon SpectreEchantillon::rgbRefl2SpectCyan;
SpectreEchantillon SpectreEchantillon::rgbRefl2SpectMagenta;
SpectreEchantillon SpectreEchantillon::rgbRefl2SpectYellow;
SpectreEchantillon SpectreEchantillon::rgbRefl2SpectRed;
SpectreEchantillon SpectreEchantillon::rgbRefl2SpectGreen;
SpectreEchantillon SpectreEchantillon::rgbRefl2SpectBlue;
SpectreEchantillon SpectreEchantillon::rgbIllum2SpectWhite;
SpectreEchantillon SpectreEchantillon::rgbIllum2SpectCyan;
SpectreEchantillon SpectreEchantillon::rgbIllum2SpectMagenta;
SpectreEchantillon SpectreEchantillon::rgbIllum2SpectYellow;
SpectreEchantillon SpectreEchantillon::rgbIllum2SpectRed;
SpectreEchantillon SpectreEchantillon::rgbIllum2SpectGreen;
SpectreEchantillon SpectreEchantillon::rgbIllum2SpectBlue;

SpectreEchantillon::SpectreEchantillon(const float coefficient)
	: SpectreCoefficient<NOMBRE_ECHANTILLONS>(coefficient)
{}

SpectreEchantillon::SpectreEchantillon(SpectreCoefficient<NOMBRE_ECHANTILLONS> const &spectre)
	: SpectreCoefficient<NOMBRE_ECHANTILLONS>(spectre)
{}

SpectreEchantillon::SpectreEchantillon::SpectreEchantillon(SpectreRGB const &r, TypeSpectre t)
{
	float rgb[3];
	r.vers_rvb(rgb);
	*this = SpectreEchantillon::depuis_rgb(rgb, t);
}

SpectreEchantillon SpectreEchantillon::depuis_echantillons(const float *lambda, const float *v, long n)
{
	if (!echantillons_spectre_trie(lambda, v, n)) {
		dls::tableau<float> slambda(&lambda[0], &lambda[n]);
		dls::tableau<float> sv(&v[0], &v[n]);

		trie_echantillons_spectre(slambda.donnees(), sv.donnees(), n);

		return depuis_echantillons(slambda.donnees(), sv.donnees(), n);
	}

	SpectreEchantillon resultat;

	for (size_t i = 0; i < NOMBRE_ECHANTILLONS; ++i) {
		auto const lambda0 = dls::math::entrepolation_lineaire(DEBUT_LAMBDA_SPECTRE, FIN_LAMBDA_SPECTRE, static_cast<float>(i) / static_cast<float>(NOMBRE_ECHANTILLONS));
		auto const lambda1 = dls::math::entrepolation_lineaire(DEBUT_LAMBDA_SPECTRE, FIN_LAMBDA_SPECTRE, static_cast<float>(i + 1) / static_cast<float>(NOMBRE_ECHANTILLONS));

		resultat[i] = moyenne_echantillons(lambda, v, n, lambda0, lambda1);
	}

	return resultat;
}

SpectreEchantillon SpectreEchantillon::depuis_rgb(const float rgb[], TypeSpectre type)
{
	SpectreEchantillon resultat;

	if (type == TypeSpectre::SPECTRE_REFLECTANCE) {
		// Convert reflectance spectrum to RGB
		if (rgb[0] <= rgb[1] && rgb[0] <= rgb[2]) {
			// Compute reflectance _SpectreEchantillon_ with _rgb[0]_ as minimum
			resultat += rgb[0] * rgbRefl2SpectWhite;

			if (rgb[1] <= rgb[2]) {
				resultat += (rgb[1] - rgb[0]) * rgbRefl2SpectCyan;
				resultat += (rgb[2] - rgb[1]) * rgbRefl2SpectBlue;
			}
			else {
				resultat += (rgb[2] - rgb[0]) * rgbRefl2SpectCyan;
				resultat += (rgb[1] - rgb[2]) * rgbRefl2SpectGreen;
			}
		}
		else if (rgb[1] <= rgb[0] && rgb[1] <= rgb[2]) {
			// Compute reflectance _SpectreEchantillon_ with _rgb[1]_ as minimum
			resultat += rgb[1] * rgbRefl2SpectWhite;

			if (rgb[0] <= rgb[2]) {
				resultat += (rgb[0] - rgb[1]) * rgbRefl2SpectMagenta;
				resultat += (rgb[2] - rgb[0]) * rgbRefl2SpectBlue;
			}
			else {
				resultat += (rgb[2] - rgb[1]) * rgbRefl2SpectMagenta;
				resultat += (rgb[0] - rgb[2]) * rgbRefl2SpectRed;
			}
		}
		else {
			// Compute reflectance _SpectreEchantillon_ with _rgb[2]_ as minimum
			resultat += rgb[2] * rgbRefl2SpectWhite;
			if (rgb[0] <= rgb[1]) {
				resultat += (rgb[0] - rgb[2]) * rgbRefl2SpectYellow;
				resultat += (rgb[1] - rgb[0]) * rgbRefl2SpectGreen;
			} else {
				resultat += (rgb[1] - rgb[2]) * rgbRefl2SpectYellow;
				resultat += (rgb[0] - rgb[1]) * rgbRefl2SpectRed;
			}
		}

		resultat *= 0.94f;
	}
	else {
		// Convert illuminant spectrum to RGB
		if (rgb[0] <= rgb[1] && rgb[0] <= rgb[2]) {
			// Compute illuminant _SpectreEchantillon_ with _rgb[0]_ as minimum
			resultat += rgb[0] * rgbIllum2SpectWhite;

			if (rgb[1] <= rgb[2]) {
				resultat += (rgb[1] - rgb[0]) * rgbIllum2SpectCyan;
				resultat += (rgb[2] - rgb[1]) * rgbIllum2SpectBlue;
			}
			else {
				resultat += (rgb[2] - rgb[0]) * rgbIllum2SpectCyan;
				resultat += (rgb[1] - rgb[2]) * rgbIllum2SpectGreen;
			}
		}
		else if (rgb[1] <= rgb[0] && rgb[1] <= rgb[2]) {
			// Compute illuminant _SpectreEchantillon_ with _rgb[1]_ as minimum
			resultat += rgb[1] * rgbIllum2SpectWhite;

			if (rgb[0] <= rgb[2]) {
				resultat += (rgb[0] - rgb[1]) * rgbIllum2SpectMagenta;
				resultat += (rgb[2] - rgb[0]) * rgbIllum2SpectBlue;
			}
			else {
				resultat += (rgb[2] - rgb[1]) * rgbIllum2SpectMagenta;
				resultat += (rgb[0] - rgb[2]) * rgbIllum2SpectRed;
			}
		}
		else {
			// Compute illuminant _SpectreEchantillon_ with _rgb[2]_ as minimum
			resultat += rgb[2] * rgbIllum2SpectWhite;

			if (rgb[0] <= rgb[1]) {
				resultat += (rgb[0] - rgb[2]) * rgbIllum2SpectYellow;
				resultat += (rgb[1] - rgb[0]) * rgbIllum2SpectGreen;
			}
			else {
				resultat += (rgb[1] - rgb[2]) * rgbIllum2SpectYellow;
				resultat += (rgb[0] - rgb[1]) * rgbIllum2SpectRed;
			}
		}

		resultat *= .86445f;
	}

	return restreint(resultat);
}

SpectreEchantillon SpectreEchantillon::depuis_rgb(float r, float g, float b, TypeSpectre type)
{
	float rgb[3] = { r, g, b };
	return SpectreEchantillon::depuis_rgb(rgb, type);
}

SpectreRGB SpectreEchantillon::vers_spectre_rvb()
{
	float rgb[3];
	vers_rvb(rgb);
	return SpectreRGB::depuis_rgb(rgb);
}

void SpectreEchantillon::vers_xyz(float xyz[]) const
{
	xyz[0] = xyz[1] = xyz[2] = 0.f;

	for (int i = 0; i < NOMBRE_ECHANTILLONS; ++i) {
		xyz[0] += X.m_coefficients[i] * m_coefficients[i];
		xyz[1] += Y.m_coefficients[i] * m_coefficients[i];
		xyz[2] += Z.m_coefficients[i] * m_coefficients[i];
	}

	auto scale = (FIN_LAMBDA_SPECTRE - DEBUT_LAMBDA_SPECTRE) /
				   (CIE_Y_integral * NOMBRE_ECHANTILLONS);
	xyz[0] *= scale;
	xyz[1] *= scale;
	xyz[2] *= scale;
}

float SpectreEchantillon::y() const
{
	auto yy = 0.0f;

	for (int i = 0; i < NOMBRE_ECHANTILLONS; ++i) {
		yy += Y.m_coefficients[i] * m_coefficients[i];
	}

	return yy * (FIN_LAMBDA_SPECTRE - DEBUT_LAMBDA_SPECTRE) /
			(CIE_Y_integral * NOMBRE_ECHANTILLONS);
}

void SpectreEchantillon::vers_rvb(float rgb[]) const
{
	float xyz[3];
	vers_xyz(xyz);
	xyz_vers_rgb(xyz, rgb);
}

void SpectreEchantillon::initialisation()
{
	// Compute XYZ matching functions for _SampledSpectrum_
	for (int i = 0; i < NOMBRE_ECHANTILLONS; ++i) {
		auto const wl0 = dls::math::entrepolation_lineaire(
							 DEBUT_LAMBDA_SPECTRE,
							 FIN_LAMBDA_SPECTRE,
							 float(i) / float(NOMBRE_ECHANTILLONS));

		auto const wl1 = dls::math::entrepolation_lineaire(
							 DEBUT_LAMBDA_SPECTRE,
							 FIN_LAMBDA_SPECTRE,
							 float(i + 1) / float(NOMBRE_ECHANTILLONS));

		X.m_coefficients[i] = moyenne_echantillons(
					 CIE_lambda, CIE_X, nCIESamples, wl0, wl1);

		Y.m_coefficients[i] = moyenne_echantillons(
					 CIE_lambda, CIE_Y, nCIESamples, wl0, wl1);

		Z.m_coefficients[i] = moyenne_echantillons(
					 CIE_lambda, CIE_Z, nCIESamples, wl0, wl1);
	}

	// Compute RGB to spectrum functions for _SampledSpectrum_
	for (int i = 0; i < NOMBRE_ECHANTILLONS; ++i) {
		auto const wl0 = dls::math::entrepolation_lineaire(
							 DEBUT_LAMBDA_SPECTRE,
							 FIN_LAMBDA_SPECTRE,
							 float(i) / float(NOMBRE_ECHANTILLONS));

		auto const wl1 = dls::math::entrepolation_lineaire(
							 DEBUT_LAMBDA_SPECTRE,
							 FIN_LAMBDA_SPECTRE,
							 float(i + 1) / float(NOMBRE_ECHANTILLONS));

		rgbRefl2SpectWhite.m_coefficients[i] =
				moyenne_echantillons(RGB2SpectLambda, RGBRefl2SpectWhite,
									 nRGB2SpectSamples, wl0, wl1);
		rgbRefl2SpectCyan.m_coefficients[i] =
				moyenne_echantillons(RGB2SpectLambda, RGBRefl2SpectCyan,
									 nRGB2SpectSamples, wl0, wl1);
		rgbRefl2SpectMagenta.m_coefficients[i] =
				moyenne_echantillons(RGB2SpectLambda, RGBRefl2SpectMagenta,
									 nRGB2SpectSamples, wl0, wl1);
		rgbRefl2SpectYellow.m_coefficients[i] =
				moyenne_echantillons(RGB2SpectLambda, RGBRefl2SpectYellow,
									 nRGB2SpectSamples, wl0, wl1);
		rgbRefl2SpectRed.m_coefficients[i] = moyenne_echantillons(
									RGB2SpectLambda, RGBRefl2SpectRed, nRGB2SpectSamples, wl0, wl1);
		rgbRefl2SpectGreen.m_coefficients[i] =
				moyenne_echantillons(RGB2SpectLambda, RGBRefl2SpectGreen,
									 nRGB2SpectSamples, wl0, wl1);
		rgbRefl2SpectBlue.m_coefficients[i] =
				moyenne_echantillons(RGB2SpectLambda, RGBRefl2SpectBlue,
									 nRGB2SpectSamples, wl0, wl1);

		rgbIllum2SpectWhite.m_coefficients[i] =
				moyenne_echantillons(RGB2SpectLambda, RGBIllum2SpectWhite,
									 nRGB2SpectSamples, wl0, wl1);
		rgbIllum2SpectCyan.m_coefficients[i] =
				moyenne_echantillons(RGB2SpectLambda, RGBIllum2SpectCyan,
									 nRGB2SpectSamples, wl0, wl1);
		rgbIllum2SpectMagenta.m_coefficients[i] =
				moyenne_echantillons(RGB2SpectLambda, RGBIllum2SpectMagenta,
									 nRGB2SpectSamples, wl0, wl1);
		rgbIllum2SpectYellow.m_coefficients[i] =
				moyenne_echantillons(RGB2SpectLambda, RGBIllum2SpectYellow,
									 nRGB2SpectSamples, wl0, wl1);
		rgbIllum2SpectRed.m_coefficients[i] =
				moyenne_echantillons(RGB2SpectLambda, RGBIllum2SpectRed,
									 nRGB2SpectSamples, wl0, wl1);
		rgbIllum2SpectGreen.m_coefficients[i] =
				moyenne_echantillons(RGB2SpectLambda, RGBIllum2SpectGreen,
									 nRGB2SpectSamples, wl0, wl1);
		rgbIllum2SpectBlue.m_coefficients[i] =
				moyenne_echantillons(RGB2SpectLambda, RGBIllum2SpectBlue,
									 nRGB2SpectSamples, wl0, wl1);
	}
}

/* ************************************************************************** */

SpectreRGB::SpectreRGB(float valeur)
	: SpectreCoefficient<3>(valeur)
{}

SpectreRGB::SpectreRGB(double valeur)
	: SpectreCoefficient<3>(static_cast<float>(valeur))
{}

SpectreRGB::SpectreRGB(SpectreCoefficient<3> const &spectre)
	: SpectreCoefficient<3>(spectre)
{}

SpectreRGB::SpectreRGB(SpectreRGB const &spectre, TypeSpectre type)
	: SpectreCoefficient<3>()
{
	INUTILISE(type);
	*this = spectre;
}

SpectreRGB SpectreRGB::depuis_rgb(float r, float g, float b, TypeSpectre type)
{
	INUTILISE(type);
	SpectreRGB resultat;
	resultat.m_coefficients[0] = r;
	resultat.m_coefficients[1] = g;
	resultat.m_coefficients[2] = b;
	return resultat;
}

SpectreRGB SpectreRGB::depuis_rgb(float rgb[], TypeSpectre type)
{
	return SpectreRGB::depuis_rgb(rgb[0], rgb[1], rgb[2], type);
}

void SpectreRGB::vers_rvb(float *rgb) const
{
	rgb[0] = m_coefficients[0];
	rgb[1] = m_coefficients[1];
	rgb[2] = m_coefficients[2];
}

SpectreRGB SpectreRGB::vers_spectre_rvb() const
{
	return *this;
}

SpectreRGB SpectreRGB::depuis_echantillons(const float *lambda, const float *v, long n)
{
	if (!echantillons_spectre_trie(lambda, v, n)) {
		dls::tableau<float> slambda(&lambda[0], &lambda[n]);
		dls::tableau<float> sv(&v[0], &v[n]);

		trie_echantillons_spectre(slambda.donnees(), sv.donnees(), n);

		return depuis_echantillons(slambda.donnees(), sv.donnees(), n);
	}

	float xyz[3] = { 0.0, 0.0, 0.0 };

	for (int i = 0; i < nCIESamples; ++i) {
		float val = entrepole_echantillons_spectre(lambda, v, n, CIE_lambda[i]);
		xyz[0] += val * CIE_X[i];
		xyz[1] += val * CIE_Y[i];
		xyz[2] += val * CIE_Z[i];
	}

	auto scale = (CIE_lambda[nCIESamples - 1] - CIE_lambda[0]) / (CIE_Y_integral * nCIESamples);

	xyz[0] *= scale;
	xyz[1] *= scale;
	xyz[2] *= scale;

	return depuis_xyz(xyz);
}

SpectreRGB SpectreRGB::depuis_xyz(float *xyz)
{
	SpectreRGB spectre;
	xyz_vers_rgb(xyz, spectre.m_coefficients);
	return spectre;
}

void SpectreRGB::vers_xyz(float *xyz)
{
	rgb_vers_xyz(m_coefficients, xyz);
}

float SpectreRGB::y() const
{
	const float poids[3] = { 0.212671f, 0.715160f, 0.072169f };
	return poids[0] * m_coefficients[0] + poids[1] * m_coefficients[1] + poids[2] * m_coefficients[2];
}

/* ************************************************************************** */

const float CIE_X[nCIESamples] = {
	// CIE X function values
	0.0001299000f,   0.0001458470f,   0.0001638021f,   0.0001840037f,
	0.0002066902f,   0.0002321000f,   0.0002607280f,   0.0002930750f,
	0.0003293880f,   0.0003699140f,   0.0004149000f,   0.0004641587f,
	0.0005189860f,   0.0005818540f,   0.0006552347f,   0.0007416000f,
	0.0008450296f,   0.0009645268f,   0.001094949f,    0.001231154f,
	0.001368000f,    0.001502050f,    0.001642328f,    0.001802382f,
	0.001995757f,    0.002236000f,    0.002535385f,    0.002892603f,
	0.003300829f,    0.003753236f,    0.004243000f,    0.004762389f,
	0.005330048f,    0.005978712f,    0.006741117f,    0.007650000f,
	0.008751373f,    0.01002888f,     0.01142170f,     0.01286901f,
	0.01431000f,     0.01570443f,     0.01714744f,     0.01878122f,
	0.02074801f,     0.02319000f,     0.02620736f,     0.02978248f,
	0.03388092f,     0.03846824f,     0.04351000f,     0.04899560f,
	0.05502260f,     0.06171880f,     0.06921200f,     0.07763000f,
	0.08695811f,     0.09717672f,     0.1084063f,      0.1207672f,
	0.1343800f,      0.1493582f,      0.1653957f,      0.1819831f,
	0.1986110f,      0.2147700f,      0.2301868f,      0.2448797f,
	0.2587773f,      0.2718079f,      0.2839000f,      0.2949438f,
	0.3048965f,      0.3137873f,      0.3216454f,      0.3285000f,
	0.3343513f,      0.3392101f,      0.3431213f,      0.3461296f,
	0.3482800f,      0.3495999f,      0.3501474f,      0.3500130f,
	0.3492870f,      0.3480600f,      0.3463733f,      0.3442624f,
	0.3418088f,      0.3390941f,      0.3362000f,      0.3331977f,
	0.3300411f,      0.3266357f,      0.3228868f,      0.3187000f,
	0.3140251f,      0.3088840f,      0.3032904f,      0.2972579f,
	0.2908000f,      0.2839701f,      0.2767214f,      0.2689178f,
	0.2604227f,      0.2511000f,      0.2408475f,      0.2298512f,
	0.2184072f,      0.2068115f,      0.1953600f,      0.1842136f,
	0.1733273f,      0.1626881f,      0.1522833f,      0.1421000f,
	0.1321786f,      0.1225696f,      0.1132752f,      0.1042979f,
	0.09564000f,     0.08729955f,     0.07930804f,     0.07171776f,
	0.06458099f,     0.05795001f,     0.05186211f,     0.04628152f,
	0.04115088f,     0.03641283f,     0.03201000f,     0.02791720f,
	0.02414440f,     0.02068700f,     0.01754040f,     0.01470000f,
	0.01216179f,     0.009919960f,    0.007967240f,    0.006296346f,
	0.004900000f,    0.003777173f,    0.002945320f,    0.002424880f,
	0.002236293f,    0.002400000f,    0.002925520f,    0.003836560f,
	0.005174840f,    0.006982080f,    0.009300000f,    0.01214949f,
	0.01553588f,     0.01947752f,     0.02399277f,     0.02910000f,
	0.03481485f,     0.04112016f,     0.04798504f,     0.05537861f,
	0.06327000f,     0.07163501f,     0.08046224f,     0.08973996f,
	0.09945645f,     0.1096000f,      0.1201674f,      0.1311145f,
	0.1423679f,      0.1538542f,      0.1655000f,      0.1772571f,
	0.1891400f,      0.2011694f,      0.2133658f,      0.2257499f,
	0.2383209f,      0.2510668f,      0.2639922f,      0.2771017f,
	0.2904000f,      0.3038912f,      0.3175726f,      0.3314384f,
	0.3454828f,      0.3597000f,      0.3740839f,      0.3886396f,
	0.4033784f,      0.4183115f,      0.4334499f,      0.4487953f,
	0.4643360f,      0.4800640f,      0.4959713f,      0.5120501f,
	0.5282959f,      0.5446916f,      0.5612094f,      0.5778215f,
	0.5945000f,      0.6112209f,      0.6279758f,      0.6447602f,
	0.6615697f,      0.6784000f,      0.6952392f,      0.7120586f,
	0.7288284f,      0.7455188f,      0.7621000f,      0.7785432f,
	0.7948256f,      0.8109264f,      0.8268248f,      0.8425000f,
	0.8579325f,      0.8730816f,      0.8878944f,      0.9023181f,
	0.9163000f,      0.9297995f,      0.9427984f,      0.9552776f,
	0.9672179f,      0.9786000f,      0.9893856f,      0.9995488f,
	1.0090892f,      1.0180064f,      1.0263000f,      1.0339827f,
	1.0409860f,      1.0471880f,      1.0524667f,      1.0567000f,
	1.0597944f,      1.0617992f,      1.0628068f,      1.0629096f,
	1.0622000f,      1.0607352f,      1.0584436f,      1.0552244f,
	1.0509768f,      1.0456000f,      1.0390369f,      1.0313608f,
	1.0226662f,      1.0130477f,      1.0026000f,      0.9913675f,
	0.9793314f,      0.9664916f,      0.9528479f,      0.9384000f,
	0.9231940f,      0.9072440f,      0.8905020f,      0.8729200f,
	0.8544499f,      0.8350840f,      0.8149460f,      0.7941860f,
	0.7729540f,      0.7514000f,      0.7295836f,      0.7075888f,
	0.6856022f,      0.6638104f,      0.6424000f,      0.6215149f,
	0.6011138f,      0.5811052f,      0.5613977f,      0.5419000f,
	0.5225995f,      0.5035464f,      0.4847436f,      0.4661939f,
	0.4479000f,      0.4298613f,      0.4120980f,      0.3946440f,
	0.3775333f,      0.3608000f,      0.3444563f,      0.3285168f,
	0.3130192f,      0.2980011f,      0.2835000f,      0.2695448f,
	0.2561184f,      0.2431896f,      0.2307272f,      0.2187000f,
	0.2070971f,      0.1959232f,      0.1851708f,      0.1748323f,
	0.1649000f,      0.1553667f,      0.1462300f,      0.1374900f,
	0.1291467f,      0.1212000f,      0.1136397f,      0.1064650f,
	0.09969044f,     0.09333061f,     0.08740000f,     0.08190096f,
	0.07680428f,     0.07207712f,     0.06768664f,     0.06360000f,
	0.05980685f,     0.05628216f,     0.05297104f,     0.04981861f,
	0.04677000f,     0.04378405f,     0.04087536f,     0.03807264f,
	0.03540461f,     0.03290000f,     0.03056419f,     0.02838056f,
	0.02634484f,     0.02445275f,     0.02270000f,     0.02108429f,
	0.01959988f,     0.01823732f,     0.01698717f,     0.01584000f,
	0.01479064f,     0.01383132f,     0.01294868f,     0.01212920f,
	0.01135916f,     0.01062935f,     0.009938846f,    0.009288422f,
	0.008678854f,    0.008110916f,    0.007582388f,    0.007088746f,
	0.006627313f,    0.006195408f,    0.005790346f,    0.005409826f,
	0.005052583f,    0.004717512f,    0.004403507f,    0.004109457f,
	0.003833913f,    0.003575748f,    0.003334342f,    0.003109075f,
	0.002899327f,    0.002704348f,    0.002523020f,    0.002354168f,
	0.002196616f,    0.002049190f,    0.001910960f,    0.001781438f,
	0.001660110f,    0.001546459f,    0.001439971f,    0.001340042f,
	0.001246275f,    0.001158471f,    0.001076430f,    0.0009999493f,
	0.0009287358f,   0.0008624332f,   0.0008007503f,   0.0007433960f,
	0.0006900786f,   0.0006405156f,   0.0005945021f,   0.0005518646f,
	0.0005124290f,   0.0004760213f,   0.0004424536f,   0.0004115117f,
	0.0003829814f,   0.0003566491f,   0.0003323011f,   0.0003097586f,
	0.0002888871f,   0.0002695394f,   0.0002515682f,   0.0002348261f,
	0.0002191710f,   0.0002045258f,   0.0001908405f,   0.0001780654f,
	0.0001661505f,   0.0001550236f,   0.0001446219f,   0.0001349098f,
	0.0001258520f,   0.0001174130f,   0.0001095515f,   0.0001022245f,
	0.00009539445f,  0.00008902390f,  0.00008307527f,  0.00007751269f,
	0.00007231304f,  0.00006745778f,  0.00006292844f,  0.00005870652f,
	0.00005477028f,  0.00005109918f,  0.00004767654f,  0.00004448567f,
	0.00004150994f,  0.00003873324f,  0.00003614203f,  0.00003372352f,
	0.00003146487f,  0.00002935326f,  0.00002737573f,  0.00002552433f,
	0.00002379376f,  0.00002217870f,  0.00002067383f,  0.00001927226f,
	0.00001796640f,  0.00001674991f,  0.00001561648f,  0.00001455977f,
	0.00001357387f,  0.00001265436f,  0.00001179723f,  0.00001099844f,
	0.00001025398f,  0.000009559646f, 0.000008912044f, 0.000008308358f,
	0.000007745769f, 0.000007221456f, 0.000006732475f, 0.000006276423f,
	0.000005851304f, 0.000005455118f, 0.000005085868f, 0.000004741466f,
	0.000004420236f, 0.000004120783f, 0.000003841716f, 0.000003581652f,
	0.000003339127f, 0.000003112949f, 0.000002902121f, 0.000002705645f,
	0.000002522525f, 0.000002351726f, 0.000002192415f, 0.000002043902f,
	0.000001905497f, 0.000001776509f, 0.000001656215f, 0.000001544022f,
	0.000001439440f, 0.000001341977f, 0.000001251141f};

const float CIE_Y[nCIESamples] = {
	// CIE Y function values
	0.000003917000f,  0.000004393581f,  0.000004929604f,  0.000005532136f,
	0.000006208245f,  0.000006965000f,  0.000007813219f,  0.000008767336f,
	0.000009839844f,  0.00001104323f,   0.00001239000f,   0.00001388641f,
	0.00001555728f,   0.00001744296f,   0.00001958375f,   0.00002202000f,
	0.00002483965f,   0.00002804126f,   0.00003153104f,   0.00003521521f,
	0.00003900000f,   0.00004282640f,   0.00004691460f,   0.00005158960f,
	0.00005717640f,   0.00006400000f,   0.00007234421f,   0.00008221224f,
	0.00009350816f,   0.0001061361f,    0.0001200000f,    0.0001349840f,
	0.0001514920f,    0.0001702080f,    0.0001918160f,    0.0002170000f,
	0.0002469067f,    0.0002812400f,    0.0003185200f,    0.0003572667f,
	0.0003960000f,    0.0004337147f,    0.0004730240f,    0.0005178760f,
	0.0005722187f,    0.0006400000f,    0.0007245600f,    0.0008255000f,
	0.0009411600f,    0.001069880f,     0.001210000f,     0.001362091f,
	0.001530752f,     0.001720368f,     0.001935323f,     0.002180000f,
	0.002454800f,     0.002764000f,     0.003117800f,     0.003526400f,
	0.004000000f,     0.004546240f,     0.005159320f,     0.005829280f,
	0.006546160f,     0.007300000f,     0.008086507f,     0.008908720f,
	0.009767680f,     0.01066443f,      0.01160000f,      0.01257317f,
	0.01358272f,      0.01462968f,      0.01571509f,      0.01684000f,
	0.01800736f,      0.01921448f,      0.02045392f,      0.02171824f,
	0.02300000f,      0.02429461f,      0.02561024f,      0.02695857f,
	0.02835125f,      0.02980000f,      0.03131083f,      0.03288368f,
	0.03452112f,      0.03622571f,      0.03800000f,      0.03984667f,
	0.04176800f,      0.04376600f,      0.04584267f,      0.04800000f,
	0.05024368f,      0.05257304f,      0.05498056f,      0.05745872f,
	0.06000000f,      0.06260197f,      0.06527752f,      0.06804208f,
	0.07091109f,      0.07390000f,      0.07701600f,      0.08026640f,
	0.08366680f,      0.08723280f,      0.09098000f,      0.09491755f,
	0.09904584f,      0.1033674f,       0.1078846f,       0.1126000f,
	0.1175320f,       0.1226744f,       0.1279928f,       0.1334528f,
	0.1390200f,       0.1446764f,       0.1504693f,       0.1564619f,
	0.1627177f,       0.1693000f,       0.1762431f,       0.1835581f,
	0.1912735f,       0.1994180f,       0.2080200f,       0.2171199f,
	0.2267345f,       0.2368571f,       0.2474812f,       0.2586000f,
	0.2701849f,       0.2822939f,       0.2950505f,       0.3085780f,
	0.3230000f,       0.3384021f,       0.3546858f,       0.3716986f,
	0.3892875f,       0.4073000f,       0.4256299f,       0.4443096f,
	0.4633944f,       0.4829395f,       0.5030000f,       0.5235693f,
	0.5445120f,       0.5656900f,       0.5869653f,       0.6082000f,
	0.6293456f,       0.6503068f,       0.6708752f,       0.6908424f,
	0.7100000f,       0.7281852f,       0.7454636f,       0.7619694f,
	0.7778368f,       0.7932000f,       0.8081104f,       0.8224962f,
	0.8363068f,       0.8494916f,       0.8620000f,       0.8738108f,
	0.8849624f,       0.8954936f,       0.9054432f,       0.9148501f,
	0.9237348f,       0.9320924f,       0.9399226f,       0.9472252f,
	0.9540000f,       0.9602561f,       0.9660074f,       0.9712606f,
	0.9760225f,       0.9803000f,       0.9840924f,       0.9874812f,
	0.9903128f,       0.9928116f,       0.9949501f,       0.9967108f,
	0.9980983f,       0.9991120f,       0.9997482f,       1.0000000f,
	0.9998567f,       0.9993046f,       0.9983255f,       0.9968987f,
	0.9950000f,       0.9926005f,       0.9897426f,       0.9864444f,
	0.9827241f,       0.9786000f,       0.9740837f,       0.9691712f,
	0.9638568f,       0.9581349f,       0.9520000f,       0.9454504f,
	0.9384992f,       0.9311628f,       0.9234576f,       0.9154000f,
	0.9070064f,       0.8982772f,       0.8892048f,       0.8797816f,
	0.8700000f,       0.8598613f,       0.8493920f,       0.8386220f,
	0.8275813f,       0.8163000f,       0.8047947f,       0.7930820f,
	0.7811920f,       0.7691547f,       0.7570000f,       0.7447541f,
	0.7324224f,       0.7200036f,       0.7074965f,       0.6949000f,
	0.6822192f,       0.6694716f,       0.6566744f,       0.6438448f,
	0.6310000f,       0.6181555f,       0.6053144f,       0.5924756f,
	0.5796379f,       0.5668000f,       0.5539611f,       0.5411372f,
	0.5283528f,       0.5156323f,       0.5030000f,       0.4904688f,
	0.4780304f,       0.4656776f,       0.4534032f,       0.4412000f,
	0.4290800f,       0.4170360f,       0.4050320f,       0.3930320f,
	0.3810000f,       0.3689184f,       0.3568272f,       0.3447768f,
	0.3328176f,       0.3210000f,       0.3093381f,       0.2978504f,
	0.2865936f,       0.2756245f,       0.2650000f,       0.2547632f,
	0.2448896f,       0.2353344f,       0.2260528f,       0.2170000f,
	0.2081616f,       0.1995488f,       0.1911552f,       0.1829744f,
	0.1750000f,       0.1672235f,       0.1596464f,       0.1522776f,
	0.1451259f,       0.1382000f,       0.1315003f,       0.1250248f,
	0.1187792f,       0.1127691f,       0.1070000f,       0.1014762f,
	0.09618864f,      0.09112296f,      0.08626485f,      0.08160000f,
	0.07712064f,      0.07282552f,      0.06871008f,      0.06476976f,
	0.06100000f,      0.05739621f,      0.05395504f,      0.05067376f,
	0.04754965f,      0.04458000f,      0.04175872f,      0.03908496f,
	0.03656384f,      0.03420048f,      0.03200000f,      0.02996261f,
	0.02807664f,      0.02632936f,      0.02470805f,      0.02320000f,
	0.02180077f,      0.02050112f,      0.01928108f,      0.01812069f,
	0.01700000f,      0.01590379f,      0.01483718f,      0.01381068f,
	0.01283478f,      0.01192000f,      0.01106831f,      0.01027339f,
	0.009533311f,     0.008846157f,     0.008210000f,     0.007623781f,
	0.007085424f,     0.006591476f,     0.006138485f,     0.005723000f,
	0.005343059f,     0.004995796f,     0.004676404f,     0.004380075f,
	0.004102000f,     0.003838453f,     0.003589099f,     0.003354219f,
	0.003134093f,     0.002929000f,     0.002738139f,     0.002559876f,
	0.002393244f,     0.002237275f,     0.002091000f,     0.001953587f,
	0.001824580f,     0.001703580f,     0.001590187f,     0.001484000f,
	0.001384496f,     0.001291268f,     0.001204092f,     0.001122744f,
	0.001047000f,     0.0009765896f,    0.0009111088f,    0.0008501332f,
	0.0007932384f,    0.0007400000f,    0.0006900827f,    0.0006433100f,
	0.0005994960f,    0.0005584547f,    0.0005200000f,    0.0004839136f,
	0.0004500528f,    0.0004183452f,    0.0003887184f,    0.0003611000f,
	0.0003353835f,    0.0003114404f,    0.0002891656f,    0.0002684539f,
	0.0002492000f,    0.0002313019f,    0.0002146856f,    0.0001992884f,
	0.0001850475f,    0.0001719000f,    0.0001597781f,    0.0001486044f,
	0.0001383016f,    0.0001287925f,    0.0001200000f,    0.0001118595f,
	0.0001043224f,    0.00009733560f,   0.00009084587f,   0.00008480000f,
	0.00007914667f,   0.00007385800f,   0.00006891600f,   0.00006430267f,
	0.00006000000f,   0.00005598187f,   0.00005222560f,   0.00004871840f,
	0.00004544747f,   0.00004240000f,   0.00003956104f,   0.00003691512f,
	0.00003444868f,   0.00003214816f,   0.00003000000f,   0.00002799125f,
	0.00002611356f,   0.00002436024f,   0.00002272461f,   0.00002120000f,
	0.00001977855f,   0.00001845285f,   0.00001721687f,   0.00001606459f,
	0.00001499000f,   0.00001398728f,   0.00001305155f,   0.00001217818f,
	0.00001136254f,   0.00001060000f,   0.000009885877f,  0.000009217304f,
	0.000008592362f,  0.000008009133f,  0.000007465700f,  0.000006959567f,
	0.000006487995f,  0.000006048699f,  0.000005639396f,  0.000005257800f,
	0.000004901771f,  0.000004569720f,  0.000004260194f,  0.000003971739f,
	0.000003702900f,  0.000003452163f,  0.000003218302f,  0.000003000300f,
	0.000002797139f,  0.000002607800f,  0.000002431220f,  0.000002266531f,
	0.000002113013f,  0.000001969943f,  0.000001836600f,  0.000001712230f,
	0.000001596228f,  0.000001488090f,  0.000001387314f,  0.000001293400f,
	0.000001205820f,  0.000001124143f,  0.000001048009f,  0.0000009770578f,
	0.0000009109300f, 0.0000008492513f, 0.0000007917212f, 0.0000007380904f,
	0.0000006881098f, 0.0000006415300f, 0.0000005980895f, 0.0000005575746f,
	0.0000005198080f, 0.0000004846123f, 0.0000004518100f};

const float CIE_Z[nCIESamples] = {
	// CIE Z function values
	0.0006061000f,
	0.0006808792f,
	0.0007651456f,
	0.0008600124f,
	0.0009665928f,
	0.001086000f,
	0.001220586f,
	0.001372729f,
	0.001543579f,
	0.001734286f,
	0.001946000f,
	0.002177777f,
	0.002435809f,
	0.002731953f,
	0.003078064f,
	0.003486000f,
	0.003975227f,
	0.004540880f,
	0.005158320f,
	0.005802907f,
	0.006450001f,
	0.007083216f,
	0.007745488f,
	0.008501152f,
	0.009414544f,
	0.01054999f,
	0.01196580f,
	0.01365587f,
	0.01558805f,
	0.01773015f,
	0.02005001f,
	0.02251136f,
	0.02520288f,
	0.02827972f,
	0.03189704f,
	0.03621000f,
	0.04143771f,
	0.04750372f,
	0.05411988f,
	0.06099803f,
	0.06785001f,
	0.07448632f,
	0.08136156f,
	0.08915364f,
	0.09854048f,
	0.1102000f,
	0.1246133f,
	0.1417017f,
	0.1613035f,
	0.1832568f,
	0.2074000f,
	0.2336921f,
	0.2626114f,
	0.2947746f,
	0.3307985f,
	0.3713000f,
	0.4162091f,
	0.4654642f,
	0.5196948f,
	0.5795303f,
	0.6456000f,
	0.7184838f,
	0.7967133f,
	0.8778459f,
	0.9594390f,
	1.0390501f,
	1.1153673f,
	1.1884971f,
	1.2581233f,
	1.3239296f,
	1.3856000f,
	1.4426352f,
	1.4948035f,
	1.5421903f,
	1.5848807f,
	1.6229600f,
	1.6564048f,
	1.6852959f,
	1.7098745f,
	1.7303821f,
	1.7470600f,
	1.7600446f,
	1.7696233f,
	1.7762637f,
	1.7804334f,
	1.7826000f,
	1.7829682f,
	1.7816998f,
	1.7791982f,
	1.7758671f,
	1.7721100f,
	1.7682589f,
	1.7640390f,
	1.7589438f,
	1.7524663f,
	1.7441000f,
	1.7335595f,
	1.7208581f,
	1.7059369f,
	1.6887372f,
	1.6692000f,
	1.6475287f,
	1.6234127f,
	1.5960223f,
	1.5645280f,
	1.5281000f,
	1.4861114f,
	1.4395215f,
	1.3898799f,
	1.3387362f,
	1.2876400f,
	1.2374223f,
	1.1878243f,
	1.1387611f,
	1.0901480f,
	1.0419000f,
	0.9941976f,
	0.9473473f,
	0.9014531f,
	0.8566193f,
	0.8129501f,
	0.7705173f,
	0.7294448f,
	0.6899136f,
	0.6521049f,
	0.6162000f,
	0.5823286f,
	0.5504162f,
	0.5203376f,
	0.4919673f,
	0.4651800f,
	0.4399246f,
	0.4161836f,
	0.3938822f,
	0.3729459f,
	0.3533000f,
	0.3348578f,
	0.3175521f,
	0.3013375f,
	0.2861686f,
	0.2720000f,
	0.2588171f,
	0.2464838f,
	0.2347718f,
	0.2234533f,
	0.2123000f,
	0.2011692f,
	0.1901196f,
	0.1792254f,
	0.1685608f,
	0.1582000f,
	0.1481383f,
	0.1383758f,
	0.1289942f,
	0.1200751f,
	0.1117000f,
	0.1039048f,
	0.09666748f,
	0.08998272f,
	0.08384531f,
	0.07824999f,
	0.07320899f,
	0.06867816f,
	0.06456784f,
	0.06078835f,
	0.05725001f,
	0.05390435f,
	0.05074664f,
	0.04775276f,
	0.04489859f,
	0.04216000f,
	0.03950728f,
	0.03693564f,
	0.03445836f,
	0.03208872f,
	0.02984000f,
	0.02771181f,
	0.02569444f,
	0.02378716f,
	0.02198925f,
	0.02030000f,
	0.01871805f,
	0.01724036f,
	0.01586364f,
	0.01458461f,
	0.01340000f,
	0.01230723f,
	0.01130188f,
	0.01037792f,
	0.009529306f,
	0.008749999f,
	0.008035200f,
	0.007381600f,
	0.006785400f,
	0.006242800f,
	0.005749999f,
	0.005303600f,
	0.004899800f,
	0.004534200f,
	0.004202400f,
	0.003900000f,
	0.003623200f,
	0.003370600f,
	0.003141400f,
	0.002934800f,
	0.002749999f,
	0.002585200f,
	0.002438600f,
	0.002309400f,
	0.002196800f,
	0.002100000f,
	0.002017733f,
	0.001948200f,
	0.001889800f,
	0.001840933f,
	0.001800000f,
	0.001766267f,
	0.001737800f,
	0.001711200f,
	0.001683067f,
	0.001650001f,
	0.001610133f,
	0.001564400f,
	0.001513600f,
	0.001458533f,
	0.001400000f,
	0.001336667f,
	0.001270000f,
	0.001205000f,
	0.001146667f,
	0.001100000f,
	0.001068800f,
	0.001049400f,
	0.001035600f,
	0.001021200f,
	0.001000000f,
	0.0009686400f,
	0.0009299200f,
	0.0008868800f,
	0.0008425600f,
	0.0008000000f,
	0.0007609600f,
	0.0007236800f,
	0.0006859200f,
	0.0006454400f,
	0.0006000000f,
	0.0005478667f,
	0.0004916000f,
	0.0004354000f,
	0.0003834667f,
	0.0003400000f,
	0.0003072533f,
	0.0002831600f,
	0.0002654400f,
	0.0002518133f,
	0.0002400000f,
	0.0002295467f,
	0.0002206400f,
	0.0002119600f,
	0.0002021867f,
	0.0001900000f,
	0.0001742133f,
	0.0001556400f,
	0.0001359600f,
	0.0001168533f,
	0.0001000000f,
	0.00008613333f,
	0.00007460000f,
	0.00006500000f,
	0.00005693333f,
	0.00004999999f,
	0.00004416000f,
	0.00003948000f,
	0.00003572000f,
	0.00003264000f,
	0.00003000000f,
	0.00002765333f,
	0.00002556000f,
	0.00002364000f,
	0.00002181333f,
	0.00002000000f,
	0.00001813333f,
	0.00001620000f,
	0.00001420000f,
	0.00001213333f,
	0.00001000000f,
	0.000007733333f,
	0.000005400000f,
	0.000003200000f,
	0.000001333333f,
	0.000000000000f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	0.0f
};

const float CIE_lambda[nCIESamples] = {
	360, 361, 362, 363, 364, 365, 366, 367, 368, 369, 370, 371, 372, 373, 374,
	375, 376, 377, 378, 379, 380, 381, 382, 383, 384, 385, 386, 387, 388, 389,
	390, 391, 392, 393, 394, 395, 396, 397, 398, 399, 400, 401, 402, 403, 404,
	405, 406, 407, 408, 409, 410, 411, 412, 413, 414, 415, 416, 417, 418, 419,
	420, 421, 422, 423, 424, 425, 426, 427, 428, 429, 430, 431, 432, 433, 434,
	435, 436, 437, 438, 439, 440, 441, 442, 443, 444, 445, 446, 447, 448, 449,
	450, 451, 452, 453, 454, 455, 456, 457, 458, 459, 460, 461, 462, 463, 464,
	465, 466, 467, 468, 469, 470, 471, 472, 473, 474, 475, 476, 477, 478, 479,
	480, 481, 482, 483, 484, 485, 486, 487, 488, 489, 490, 491, 492, 493, 494,
	495, 496, 497, 498, 499, 500, 501, 502, 503, 504, 505, 506, 507, 508, 509,
	510, 511, 512, 513, 514, 515, 516, 517, 518, 519, 520, 521, 522, 523, 524,
	525, 526, 527, 528, 529, 530, 531, 532, 533, 534, 535, 536, 537, 538, 539,
	540, 541, 542, 543, 544, 545, 546, 547, 548, 549, 550, 551, 552, 553, 554,
	555, 556, 557, 558, 559, 560, 561, 562, 563, 564, 565, 566, 567, 568, 569,
	570, 571, 572, 573, 574, 575, 576, 577, 578, 579, 580, 581, 582, 583, 584,
	585, 586, 587, 588, 589, 590, 591, 592, 593, 594, 595, 596, 597, 598, 599,
	600, 601, 602, 603, 604, 605, 606, 607, 608, 609, 610, 611, 612, 613, 614,
	615, 616, 617, 618, 619, 620, 621, 622, 623, 624, 625, 626, 627, 628, 629,
	630, 631, 632, 633, 634, 635, 636, 637, 638, 639, 640, 641, 642, 643, 644,
	645, 646, 647, 648, 649, 650, 651, 652, 653, 654, 655, 656, 657, 658, 659,
	660, 661, 662, 663, 664, 665, 666, 667, 668, 669, 670, 671, 672, 673, 674,
	675, 676, 677, 678, 679, 680, 681, 682, 683, 684, 685, 686, 687, 688, 689,
	690, 691, 692, 693, 694, 695, 696, 697, 698, 699, 700, 701, 702, 703, 704,
	705, 706, 707, 708, 709, 710, 711, 712, 713, 714, 715, 716, 717, 718, 719,
	720, 721, 722, 723, 724, 725, 726, 727, 728, 729, 730, 731, 732, 733, 734,
	735, 736, 737, 738, 739, 740, 741, 742, 743, 744, 745, 746, 747, 748, 749,
	750, 751, 752, 753, 754, 755, 756, 757, 758, 759, 760, 761, 762, 763, 764,
	765, 766, 767, 768, 769, 770, 771, 772, 773, 774, 775, 776, 777, 778, 779,
	780, 781, 782, 783, 784, 785, 786, 787, 788, 789, 790, 791, 792, 793, 794,
	795, 796, 797, 798, 799, 800, 801, 802, 803, 804, 805, 806, 807, 808, 809,
	810, 811, 812, 813, 814, 815, 816, 817, 818, 819, 820, 821, 822, 823, 824,
	825, 826, 827, 828, 829, 830
};

const float RGB2SpectLambda[nRGB2SpectSamples] = {
	380.000000f, 390.967743f, 401.935486f, 412.903229f, 423.870972f, 434.838715f,
	445.806458f, 456.774200f, 467.741943f, 478.709686f, 489.677429f, 500.645172f,
	511.612915f, 522.580627f, 533.548340f, 544.516052f, 555.483765f, 566.451477f,
	577.419189f, 588.386902f, 599.354614f, 610.322327f, 621.290039f, 632.257751f,
	643.225464f, 654.193176f, 665.160889f, 676.128601f, 687.096313f, 698.064026f,
	709.031738f, 720.000000f
};

const float RGBRefl2SpectWhite[nRGB2SpectSamples] = {
	1.0618958571272863e+00f, 1.0615019980348779e+00f, 1.0614335379927147e+00f,
	1.0622711654692485e+00f, 1.0622036218416742e+00f, 1.0625059965187085e+00f,
	1.0623938486985884e+00f, 1.0624706448043137e+00f, 1.0625048144827762e+00f,
	1.0624366131308856e+00f, 1.0620694238892607e+00f, 1.0613167586932164e+00f,
	1.0610334029377020e+00f, 1.0613868564828413e+00f, 1.0614215366116762e+00f,
	1.0620336151299086e+00f, 1.0625497454805051e+00f, 1.0624317487992085e+00f,
	1.0625249140554480e+00f, 1.0624277664486914e+00f, 1.0624749854090769e+00f,
	1.0625538581025402e+00f, 1.0625326910104864e+00f, 1.0623922312225325e+00f,
	1.0623650980354129e+00f, 1.0625256476715284e+00f, 1.0612277619533155e+00f,
	1.0594262608698046e+00f, 1.0599810758292072e+00f, 1.0602547314449409e+00f,
	1.0601263046243634e+00f, 1.0606565756823634e+00f
};

const float RGBRefl2SpectCyan[nRGB2SpectSamples] = {
	1.0414628021426751e+00f,  1.0328661533771188e+00f,  1.0126146228964314e+00f,
	1.0350460524836209e+00f,  1.0078661447098567e+00f,  1.0422280385081280e+00f,
	1.0442596738499825e+00f,  1.0535238290294409e+00f,  1.0180776226938120e+00f,
	1.0442729908727713e+00f,  1.0529362541920750e+00f,  1.0537034271160244e+00f,
	1.0533901869215969e+00f,  1.0537782700979574e+00f,  1.0527093770467102e+00f,
	1.0530449040446797e+00f,  1.0550554640191208e+00f,  1.0553673610724821e+00f,
	1.0454306634683976e+00f,  6.2348950639230805e-01f,  1.8038071613188977e-01f,
	-7.6303759201984539e-03f, -1.5217847035781367e-04f, -7.5102257347258311e-03f,
	-2.1708639328491472e-03f, 6.5919466602369636e-04f,  1.2278815318539780e-02f,
	-4.4669775637208031e-03f, 1.7119799082865147e-02f,  4.9211089759759801e-03f,
	5.8762925143334985e-03f,  2.5259399415550079e-02f
};

const float RGBRefl2SpectMagenta[nRGB2SpectSamples] = {
	9.9422138151236850e-01f,  9.8986937122975682e-01f, 9.8293658286116958e-01f,
	9.9627868399859310e-01f,  1.0198955019000133e+00f, 1.0166395501210359e+00f,
	1.0220913178757398e+00f,  9.9651666040682441e-01f, 1.0097766178917882e+00f,
	1.0215422470827016e+00f,  6.4031953387790963e-01f, 2.5012379477078184e-03f,
	6.5339939555769944e-03f,  2.8334080462675826e-03f, -5.1209675389074505e-11f,
	-9.0592291646646381e-03f, 3.3936718323331200e-03f, -3.0638741121828406e-03f,
	2.2203936168286292e-01f,  6.3141140024811970e-01f, 9.7480985576500956e-01f,
	9.7209562333590571e-01f,  1.0173770302868150e+00f, 9.9875194322734129e-01f,
	9.4701725739602238e-01f,  8.5258623154354796e-01f, 9.4897798581660842e-01f,
	9.4751876096521492e-01f,  9.9598944191059791e-01f, 8.6301351503809076e-01f,
	8.9150987853523145e-01f,  8.4866492652845082e-01f
};

const float RGBRefl2SpectYellow[nRGB2SpectSamples] = {
	5.5740622924920873e-03f,  -4.7982831631446787e-03f, -5.2536564298613798e-03f,
	-6.4571480044499710e-03f, -5.9693514658007013e-03f, -2.1836716037686721e-03f,
	1.6781120601055327e-02f,  9.6096355429062641e-02f,  2.1217357081986446e-01f,
	3.6169133290685068e-01f,  5.3961011543232529e-01f,  7.4408810492171507e-01f,
	9.2209571148394054e-01f,  1.0460304298411225e+00f,  1.0513824989063714e+00f,
	1.0511991822135085e+00f,  1.0510530911991052e+00f,  1.0517397230360510e+00f,
	1.0516043086790485e+00f,  1.0511944032061460e+00f,  1.0511590325868068e+00f,
	1.0516612465483031e+00f,  1.0514038526836869e+00f,  1.0515941029228475e+00f,
	1.0511460436960840e+00f,  1.0515123758830476e+00f,  1.0508871369510702e+00f,
	1.0508923708102380e+00f,  1.0477492815668303e+00f,  1.0493272144017338e+00f,
	1.0435963333422726e+00f,  1.0392280772051465e+00f
};

const float RGBRefl2SpectRed[nRGB2SpectSamples] = {
	1.6575604867086180e-01f,  1.1846442802747797e-01f,  1.2408293329637447e-01f,
	1.1371272058349924e-01f,  7.8992434518899132e-02f,  3.2205603593106549e-02f,
	-1.0798365407877875e-02f, 1.8051975516730392e-02f,  5.3407196598730527e-03f,
	1.3654918729501336e-02f,  -5.9564213545642841e-03f, -1.8444365067353252e-03f,
	-1.0571884361529504e-02f, -2.9375521078000011e-03f, -1.0790476271835936e-02f,
	-8.0224306697503633e-03f, -2.2669167702495940e-03f, 7.0200240494706634e-03f,
	-8.1528469000299308e-03f, 6.0772866969252792e-01f,  9.8831560865432400e-01f,
	9.9391691044078823e-01f,  1.0039338994753197e+00f,  9.9234499861167125e-01f,
	9.9926530858855522e-01f,  1.0084621557617270e+00f,  9.8358296827441216e-01f,
	1.0085023660099048e+00f,  9.7451138326568698e-01f,  9.8543269570059944e-01f,
	9.3495763980962043e-01f,  9.8713907792319400e-01f
};

const float RGBRefl2SpectGreen[nRGB2SpectSamples] = {
	2.6494153587602255e-03f,  -5.0175013429732242e-03f, -1.2547236272489583e-02f,
	-9.4554964308388671e-03f, -1.2526086181600525e-02f, -7.9170697760437767e-03f,
	-7.9955735204175690e-03f, -9.3559433444469070e-03f, 6.5468611982999303e-02f,
	3.9572875517634137e-01f,  7.5244022299886659e-01f,  9.6376478690218559e-01f,
	9.9854433855162328e-01f,  9.9992977025287921e-01f,  9.9939086751140449e-01f,
	9.9994372267071396e-01f,  9.9939121813418674e-01f,  9.9911237310424483e-01f,
	9.6019584878271580e-01f,  6.3186279338432438e-01f,  2.5797401028763473e-01f,
	9.4014888527335638e-03f,  -3.0798345608649747e-03f, -4.5230367033685034e-03f,
	-6.8933410388274038e-03f, -9.0352195539015398e-03f, -8.5913667165340209e-03f,
	-8.3690869120289398e-03f, -7.8685832338754313e-03f, -8.3657578711085132e-06f,
	5.4301225442817177e-03f,  -2.7745589759259194e-03f
};

const float RGBRefl2SpectBlue[nRGB2SpectSamples] = {
	9.9209771469720676e-01f,  9.8876426059369127e-01f,  9.9539040744505636e-01f,
	9.9529317353008218e-01f,  9.9181447411633950e-01f,  1.0002584039673432e+00f,
	9.9968478437342512e-01f,  9.9988120766657174e-01f,  9.8504012146370434e-01f,
	7.9029849053031276e-01f,  5.6082198617463974e-01f,  3.3133458513996528e-01f,
	1.3692410840839175e-01f,  1.8914906559664151e-02f,  -5.1129770932550889e-06f,
	-4.2395493167891873e-04f, -4.1934593101534273e-04f, 1.7473028136486615e-03f,
	3.7999160177631316e-03f,  -5.5101474906588642e-04f, -4.3716662898480967e-05f,
	7.5874501748732798e-03f,  2.5795650780554021e-02f,  3.8168376532500548e-02f,
	4.9489586408030833e-02f,  4.9595992290102905e-02f,  4.9814819505812249e-02f,
	3.9840911064978023e-02f,  3.0501024937233868e-02f,  2.1243054765241080e-02f,
	6.9596532104356399e-03f,  4.1733649330980525e-03f
};

const float RGBIllum2SpectWhite[nRGB2SpectSamples] = {
	1.1565232050369776e+00f, 1.1567225000119139e+00f, 1.1566203150243823e+00f,
	1.1555782088080084e+00f, 1.1562175509215700e+00f, 1.1567674012207332e+00f,
	1.1568023194808630e+00f, 1.1567677445485520e+00f, 1.1563563182952830e+00f,
	1.1567054702510189e+00f, 1.1565134139372772e+00f, 1.1564336176499312e+00f,
	1.1568023181530034e+00f, 1.1473147688514642e+00f, 1.1339317140561065e+00f,
	1.1293876490671435e+00f, 1.1290515328639648e+00f, 1.0504864823782283e+00f,
	1.0459696042230884e+00f, 9.9366687168595691e-01f, 9.5601669265393940e-01f,
	9.2467482033511805e-01f, 9.1499944702051761e-01f, 8.9939467658453465e-01f,
	8.9542520751331112e-01f, 8.8870566693814745e-01f, 8.8222843814228114e-01f,
	8.7998311373826676e-01f, 8.7635244612244578e-01f, 8.8000368331709111e-01f,
	8.8065665428441120e-01f, 8.8304706460276905e-01f
};

const float RGBIllum2SpectCyan[nRGB2SpectSamples] = {
	1.1334479663682135e+00f,  1.1266762330194116e+00f,  1.1346827504710164e+00f,
	1.1357395805744794e+00f,  1.1356371830149636e+00f,  1.1361152989346193e+00f,
	1.1362179057706772e+00f,  1.1364819652587022e+00f,  1.1355107110714324e+00f,
	1.1364060941199556e+00f,  1.1360363621722465e+00f,  1.1360122641141395e+00f,
	1.1354266882467030e+00f,  1.1363099407179136e+00f,  1.1355450412632506e+00f,
	1.1353732327376378e+00f,  1.1349496420726002e+00f,  1.1111113947168556e+00f,
	9.0598740429727143e-01f,  6.1160780787465330e-01f,  2.9539752170999634e-01f,
	9.5954200671150097e-02f,  -1.1650792030826267e-02f, -1.2144633073395025e-02f,
	-1.1148167569748318e-02f, -1.1997606668458151e-02f, -5.0506855475394852e-03f,
	-7.9982745819542154e-03f, -9.4722817708236418e-03f, -5.5329541006658815e-03f,
	-4.5428914028274488e-03f, -1.2541015360921132e-02f
};

const float RGBIllum2SpectMagenta[nRGB2SpectSamples] = {
	1.0371892935878366e+00f,  1.0587542891035364e+00f,  1.0767271213688903e+00f,
	1.0762706844110288e+00f,  1.0795289105258212e+00f,  1.0743644742950074e+00f,
	1.0727028691194342e+00f,  1.0732447452056488e+00f,  1.0823760816041414e+00f,
	1.0840545681409282e+00f,  9.5607567526306658e-01f,  5.5197896855064665e-01f,
	8.4191094887247575e-02f,  8.7940070557041006e-05f,  -2.3086408335071251e-03f,
	-1.1248136628651192e-03f, -7.7297612754989586e-11f, -2.7270769006770834e-04f,
	1.4466473094035592e-02f,  2.5883116027169478e-01f,  5.2907999827566732e-01f,
	9.0966624097105164e-01f,  1.0690571327307956e+00f,  1.0887326064796272e+00f,
	1.0637622289511852e+00f,  1.0201812918094260e+00f,  1.0262196688979945e+00f,
	1.0783085560613190e+00f,  9.8333849623218872e-01f,  1.0707246342802621e+00f,
	1.0634247770423768e+00f,  1.0150875475729566e+00f
};

const float RGBIllum2SpectYellow[nRGB2SpectSamples] = {
	2.7756958965811972e-03f,  3.9673820990646612e-03f,  -1.4606936788606750e-04f,
	3.6198394557748065e-04f,  -2.5819258699309733e-04f, -5.0133191628082274e-05f,
	-2.4437242866157116e-04f, -7.8061419948038946e-05f, 4.9690301207540921e-02f,
	4.8515973574763166e-01f,  1.0295725854360589e+00f,  1.0333210878457741e+00f,
	1.0368102644026933e+00f,  1.0364884018886333e+00f,  1.0365427939411784e+00f,
	1.0368595402854539e+00f,  1.0365645405660555e+00f,  1.0363938240707142e+00f,
	1.0367205578770746e+00f,  1.0365239329446050e+00f,  1.0361531226427443e+00f,
	1.0348785007827348e+00f,  1.0042729660717318e+00f,  8.4218486432354278e-01f,
	7.3759394894801567e-01f,  6.5853154500294642e-01f,  6.0531682444066282e-01f,
	5.9549794132420741e-01f,  5.9419261278443136e-01f,  5.6517682326634266e-01f,
	5.6061186014968556e-01f,  5.8228610381018719e-01f
};

const float RGBIllum2SpectRed[nRGB2SpectSamples] = {
	5.4711187157291841e-02f,  5.5609066498303397e-02f,  6.0755873790918236e-02f,
	5.6232948615962369e-02f,  4.6169940535708678e-02f,  3.8012808167818095e-02f,
	2.4424225756670338e-02f,  3.8983580581592181e-03f,  -5.6082252172734437e-04f,
	9.6493871255194652e-04f,  3.7341198051510371e-04f,  -4.3367389093135200e-04f,
	-9.3533962256892034e-05f, -1.2354967412842033e-04f, -1.4524548081687461e-04f,
	-2.0047691915543731e-04f, -4.9938587694693670e-04f, 2.7255083540032476e-02f,
	1.6067405906297061e-01f,  3.5069788873150953e-01f,  5.7357465538418961e-01f,
	7.6392091890718949e-01f,  8.9144466740381523e-01f,  9.6394609909574891e-01f,
	9.8879464276016282e-01f,  9.9897449966227203e-01f,  9.8605140403564162e-01f,
	9.9532502805345202e-01f,  9.7433478377305371e-01f,  9.9134364616871407e-01f,
	9.8866287772174755e-01f,  9.9713856089735531e-01f
};

const float RGBIllum2SpectGreen[nRGB2SpectSamples] = {
	2.5168388755514630e-02f,  3.9427438169423720e-02f,  6.2059571596425793e-03f,
	7.1120859807429554e-03f,  2.1760044649139429e-04f,  7.3271839984290210e-12f,
	-2.1623066217181700e-02f, 1.5670209409407512e-02f,  2.8019603188636222e-03f,
	3.2494773799897647e-01f,  1.0164917292316602e+00f,  1.0329476657890369e+00f,
	1.0321586962991549e+00f,  1.0358667411948619e+00f,  1.0151235476834941e+00f,
	1.0338076690093119e+00f,  1.0371372378155013e+00f,  1.0361377027692558e+00f,
	1.0229822432557210e+00f,  9.6910327335652324e-01f,  -5.1785923899878572e-03f,
	1.1131261971061429e-03f,  6.6675503033011771e-03f,  7.4024315686001957e-04f,
	2.1591567633473925e-02f,  5.1481620056217231e-03f,  1.4561928645728216e-03f,
	1.6414511045291513e-04f,  -6.4630764968453287e-03f, 1.0250854718507939e-02f,
	4.2387394733956134e-02f,  2.1252716926861620e-02f
};

const float RGBIllum2SpectBlue[nRGB2SpectSamples] = {
	1.0570490759328752e+00f,  1.0538466912851301e+00f,  1.0550494258140670e+00f,
	1.0530407754701832e+00f,  1.0579930596460185e+00f,  1.0578439494812371e+00f,
	1.0583132387180239e+00f,  1.0579712943137616e+00f,  1.0561884233578465e+00f,
	1.0571399285426490e+00f,  1.0425795187752152e+00f,  3.2603084374056102e-01f,
	-1.9255628442412243e-03f, -1.2959221137046478e-03f, -1.4357356276938696e-03f,
	-1.2963697250337886e-03f, -1.9227081162373899e-03f, 1.2621152526221778e-03f,
	-1.6095249003578276e-03f, -1.3029983817879568e-03f, -1.7666600873954916e-03f,
	-1.2325281140280050e-03f, 1.0316809673254932e-02f,  3.1284512648354357e-02f,
	8.8773879881746481e-02f,  1.3873621740236541e-01f,  1.5535067531939065e-01f,
	1.4878477178237029e-01f,  1.6624255403475907e-01f,  1.6997613960634927e-01f,
	1.5769743995852967e-01f,  1.9069090525482305e-01f
};
