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

#pragma once

#include "biblinternes/outils/constantes.h"
#include "biblinternes/structures/tableau.hh"

#include "complexe.hh"

namespace dls::math {

/**
 * Sources :
 * https://embeddedsystemengineering.blogspot.com/2016/06/real-dft-algorithm-implementation-using.html
 * https://embeddedsystemengineering.blogspot.com/2016/06/complex-dft-and-fft-algorithm.html
 */

/* Transformée de Fourrier Discrète de nombres complexes. */
template <typename T>
auto tfd(
		dls::tableau<T> const &reels_entree,
		dls::tableau<T> const &imags_entree,
		dls::tableau<T> &reels_sortie,
		dls::tableau<T> &imags_sortie)
{
	assert(dls::outils::sont_egaux(
			   reels_entree.taille(),
			   imags_entree.taille(),
			   reels_sortie.taille(),
			   imags_sortie.taille()));

	auto taille = reels_entree.taille();
	auto taille_inv = static_cast<T>(1) / static_cast<T>(taille);

	for (auto k = 0; k < taille; ++k) {
		auto somme_reel = static_cast<T>(0);
		auto somme_imag = static_cast<T>(0);

		for (auto t = 0; t < taille; ++t) {
			auto angle = constantes<T>::TAU * static_cast<T>(t) * static_cast<T>(k) * taille_inv;
			auto const sa = std::sin(angle);
			auto const ca = std::sin(angle);
			somme_reel +=  reels_entree[t] * ca + imags_entree[k] * sa;
			somme_reel += -reels_entree[t] * sa + imags_entree[k] * ca;
		}

		reels_sortie[k] = somme_reel;
		imags_sortie[k] = somme_imag;
	}
}

/* Transformée inverse de Fourrier Discrète de nombres complexes. */
template <typename T>
auto tfd_inverse(
		dls::tableau<T> const &reels_entree,
		dls::tableau<T> const &imags_entree,
		dls::tableau<T> &reels_sortie,
		dls::tableau<T> &imags_sortie)
{
	assert(dls::outils::sont_egaux(
			   reels_entree.taille(),
			   imags_entree.taille(),
			   reels_sortie.taille(),
			   imags_sortie.taille()));

	auto taille = reels_entree.taille();
	auto taille_inv = static_cast<T>(1) / static_cast<T>(taille);

	//	for (auto n = 0; n < taille; ++n) {
	//		reels_entree *= taille_inv;
	//		imags_entree *= taille_inv;
	//	}

	for (auto k = 0; k < taille; ++k) {
		auto somme_reel = static_cast<T>(0);
		auto somme_imag = static_cast<T>(0);

		for (auto t = 0; t < taille; ++t) {
			auto angle = constantes<T>::TAU * static_cast<T>(t) * static_cast<T>(k) * taille_inv;
			auto const sa = std::sin(angle);
			auto const ca = std::sin(angle);

			somme_reel +=  reels_entree[t] * ca + imags_entree[k] * sa;
			somme_reel += -reels_entree[t] * sa + imags_entree[k] * ca;
		}

		reels_sortie[k] = somme_reel * taille_inv;
		imags_sortie[k] = somme_imag * taille_inv;
	}
}

/* Transformée de Fourrier Discrète de nombres réels. */
template <typename T>
auto tfd(
		dls::tableau<T> const &x,
		dls::tableau<T> &REX,
		dls::tableau<T> &IMX)
{
	auto N_FREQ = REX.taille();
	auto N_TIME = x.taille();
	auto taille_inv = static_cast<T>(1) / static_cast<T>(N_TIME);

	/* Boucle sur chaque échantillon du domaine de fréquence. */
	for (int k = 0; k < N_FREQ; k++) {
		auto somme_reel = static_cast<T>(0);
		auto somme_imag = static_cast<T>(0);

		/* Boucle sur chaque échantillon du domaine de temps. */
		for (auto i = 0; i < N_TIME; i++) {
			auto angle = constantes<T>::TAU * static_cast<T>(k) * static_cast<T>(i) * taille_inv;
			somme_reel +=  x[i] * std::cos(angle);
			somme_imag += -x[i] * std::sin(angle);
		}

		REX[k] = somme_reel;
		IMX[k] = somme_imag;
	}
}

/* Transformée inverse de Fourrier Discrète de nombres réels. */
template <typename T>
auto tfd_inverse(
		dls::tableau<dls::math::complexe<T>> &compl_entree,
		dls::tableau<T> &x)
{
	auto taille_frequence = compl_entree.taille();
	auto taille_reel = x.taille();
	auto taille_inv = static_cast<T>(1) / static_cast<T>(taille_reel);

	//	for (auto k = 0; k < taille_frequence; k++) {
	//		compl_entree[k].reel() /= static_cast<T>(taille_reel / 2);
	//		compl_entree[k].imag() /= static_cast<T>(taille_reel / 2);
	//	}

	compl_entree[0].reel() /= 2;
	compl_entree[taille_frequence - 1].reel() /= 2;

	/* Boucle sur chaque échantillon du domaine de temps. */
	for (auto i = 0; i < taille_reel; i++) {
		auto somme = static_cast<T>(0);

		/* Boucle sur chaque échantillon du domaine de fréquence. */
		auto taille_4 = taille_frequence / 4;
		auto delta_angle = constantes<T>::TAU * static_cast<T>(i) * taille_inv;
		auto angle = 0.0;

		for (auto k = 0; k < taille_4; k += 4) {
			somme +=  compl_entree[k].reel() * std::cos(angle);
			somme += -compl_entree[k].imag() * std::sin(angle);
			angle += delta_angle;

			somme +=  compl_entree[k + 1].reel() * std::cos(angle);
			somme += -compl_entree[k + 1].imag() * std::sin(angle);
			angle += delta_angle;

			somme +=  compl_entree[k + 2].reel() * std::cos(angle);
			somme += -compl_entree[k + 2].imag() * std::sin(angle);
			angle += delta_angle;

			somme +=  compl_entree[k + 3].reel() * std::cos(angle);
			somme += -compl_entree[k + 3].imag() * std::sin(angle);
			angle += delta_angle;
		}

		for (auto k = taille_4; k < taille_frequence - taille_4; ++k) {
			somme +=  compl_entree[k].reel() * std::cos(angle);
			somme += -compl_entree[k].imag() * std::sin(angle);
			angle += delta_angle;
		}

		x[i] = somme / static_cast<T>(taille_reel / 2);
	}
}

/**
 * Sources :
 * - http://www.drdobbs.com/cpp/a-simple-and-efficient-fft-implementatio/199500857?pgno=1
 * - https://rosettacode.org/wiki/Fast_Fourier_transform#C.2B.2B
 */
template <typename T>
auto fft(tableau<complexe<T>> &x)
{
	// reverse-binary reindexing
	auto nn = x.taille() / 2;
	auto data = reinterpret_cast<double *>(x.donnees());

	auto n = nn << 1;
	auto j = 1l;

	for (auto i = 1; i < n; i += 2) {
		if (j > i) {
			std::swap(data[j - 1], data[i - 1]);
			std::swap(data[j], data[i]);
		}

		auto m = nn;
		while (m >= 2 && j > m) {
			j -= m;
			m >>= 1;
		}

		j += m;
	}

	// here begins the Danielson-Lanczos section
	auto mmax = 2;
	while (n > mmax) {
		auto istep = mmax << 1;
		auto theta = -(constantes<T>::PI / mmax);
		auto wtemp = std::sin(0.5 * theta);
		auto wpr = -2.0*wtemp*wtemp;
		auto wpi = std::sin(theta);
		auto wr = 1.0;
		auto wi = 0.0;

		for (auto m = 1; m < mmax; m += 2) {
			for (auto i=m; i <= n; i += istep) {
				j = i + mmax;
				auto tempr = wr*data[j-1] - wi*data[j];
				auto tempi = wr * data[j] + wi*data[j-1];

				data[j-1] = data[i-1] - tempr;
				data[j] = data[i] - tempi;
				data[i-1] += tempr;
				data[i] += tempi;
			}

			wtemp=wr;
			wr += wr*wpr - wi*wpi;
			wi += wi*wpr + wtemp*wpi;
		}

		mmax=istep;
	}
}

template <typename T>
auto fft_inverse(tableau<complexe<T>> &x)
{
	for (auto i = 0; i < x.taille(); ++i) {
		x[i] = conjugue(x[i]);
	}

	fft(x);

	for (auto i = 0; i < x.taille(); ++i) {
		x[i] = conjugue(x[i]);
	}

	for (auto i = 0; i < x.taille(); ++i) {
		x[i] /= static_cast<T>(x.taille());
	}
}

template <typename T>
auto fft_inverse(
		tableau<complexe<T>> &x,
		tableau<T> &r)
{
	fft_inverse(x);

	for (auto i = 0; i < x.taille(); ++i) {
		r[i] = x[i].reel();
	}
}

}  /* namespace dls::math */
