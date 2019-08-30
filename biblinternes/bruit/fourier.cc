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

#include "fourier.hh"

#include <fftw3.h>
#include <mutex>

#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/gna.hh"

#include "outils.hh"

namespace bruit {

// shift spectrum to the format that FFTW expects
static void shift3D(float*& field, int xRes, int yRes, int zRes)
{
	int xHalf = xRes / 2;
	int yHalf = yRes / 2;
	int zHalf = zRes / 2;
	// int slabSize = xRes * yRes;
	for (int z = 0; z < zHalf; z++) {
		for (int y = 0; y < yHalf; y++) {
			for (int x = 0; x < xHalf; x++) {
				int index = x + y * xRes + z * xRes * yRes;
				float temp;
				int xSwap = xHalf;
				int ySwap = yHalf * xRes;
				int zSwap = zHalf * xRes * yRes;

				// [0,0,0] to [1,1,1]
				temp = field[index];
				field[index] = field[index + xSwap + ySwap + zSwap];
				field[index + xSwap + ySwap + zSwap] = temp;

				// [1,0,0] to [0,1,1]
				temp = field[index + xSwap];
				field[index + xSwap] = field[index + ySwap + zSwap];
				field[index + ySwap + zSwap] = temp;

				// [0,1,0] to [1,0,1]
				temp = field[index + ySwap];
				field[index + ySwap] = field[index + xSwap + zSwap];
				field[index + xSwap + zSwap] = temp;

				// [0,0,1] to [1,1,0]
				temp = field[index + zSwap];
				field[index + zSwap] = field[index + xSwap + ySwap];
				field[index + xSwap + ySwap] = temp;
			}
		}
	}
}

static void genere_tuile_bruit(float *&bruit, int res)
{
	int xRes = res;
	int yRes = res;
	int zRes = res;
	int totalCells = xRes * yRes * zRes;

	// create and shift the filter
	auto const sz = res * res * res * static_cast<long>(sizeof(float));
	auto filter = memoire::loge_tableau<float>("filter", sz);
	auto noise = memoire::loge_tableau<float>("noise", sz);
	bruit = new float[static_cast<size_t>(totalCells)];

	for (int z = 0; z < zRes; z++) {
		for (int y = 0; y < yRes; y++) {
			for (int x = 0; x < xRes; x++) {
				int index = x + y * xRes + z * xRes * yRes;
				float diff[] = {
					static_cast<float>(std::abs(x - xRes / 2)),
					static_cast<float>(std::abs(y - yRes / 2)),
					static_cast<float>(std::abs(z - zRes / 2))
				};

				auto radius = std::sqrt(diff[0] * diff[0] +
						diff[1] * diff[1] +
						diff[2] * diff[2]) / static_cast<float>(xRes / 2);
				radius *= constantes<float>::PI;
				auto H = std::cos((constantes<float>::PI / 2.0f) * std::log(4.0f * radius / constantes<float>::PI) / std::log(2.0f));
				H = H * H;
				float filtered = H;

				// clamp everything outside the wanted band
				if (radius >= constantes<float>::PI / 2.0f){
					filtered = 0.0f;
				}

				// make sure to capture all low frequencies
				if (radius <= constantes<float>::PI / 4.0f){
					filtered = 1.0f;
				}

				filter[index] = filtered;
			}
		}
	}

	shift3D(filter, xRes, yRes, zRes);

	// create the noise
	auto gna = GNA{};
	for (int i = 0; i < totalCells; i++) {
		noise[i] = gna.normale(0.0f, 1.0f);
	}

	// create padded field
	auto taille_complex = sizeof(fftw_complex) * static_cast<unsigned>(totalCells);
	auto forward = static_cast<fftw_complex *>(fftw_malloc(taille_complex));

	// init padded field

	for (int i = 0; i < totalCells; i++) {
		forward[i][0] = static_cast<double>(noise[i]);
		forward[i][1] = 0.0;
	}

	// forward FFT
	auto backward = static_cast<fftw_complex *>(fftw_malloc(taille_complex));
	auto forwardPlan = fftw_plan_dft_3d(xRes, yRes, zRes, forward, backward, FFTW_FORWARD, FFTW_ESTIMATE);
	fftw_execute(forwardPlan);
	fftw_destroy_plan(forwardPlan);

	// apply filter
	for (int i = 0; i < totalCells; i++) {
		backward[i][0] *= static_cast<double>(filter[i]);
		backward[i][1] *= static_cast<double>(filter[i]);
	}

	// backward FFT
	fftw_plan backwardPlan = fftw_plan_dft_3d(xRes, yRes, zRes, backward, forward, FFTW_BACKWARD, FFTW_ESTIMATE);
	fftw_execute(backwardPlan);
	fftw_destroy_plan(backwardPlan);

	// subtract out the low frequency components
	for (int i = 0; i < totalCells; i++) {
		noise[i] -= static_cast<float>(forward[i][0]) / static_cast<float>(totalCells);
	}

	// fill noiseTileData
	memcpy(bruit, noise, static_cast<size_t>(sz));

	fftw_free(forward);
	fftw_free(backward);

	memoire::deloge_tableau("filter", filter, sz);
	memoire::deloge_tableau("noise", noise, sz);
}

/* ************************************************************************** */

/* Le bruit d'ondelette est coûteux à produire donc assurons qu'une seule
 * version existe via cette variable globale.
 *
 * À FAIRE : stockage dans un fichier.
 */

struct constructrice_bruit_fourier {
private:
	static constructrice_bruit_fourier m_instance;
	std::atomic<float *> m_donnees = nullptr;
	std::mutex m_mutex{};

public:
	static constructrice_bruit_fourier &instance()
	{
		return m_instance;
	}

	~constructrice_bruit_fourier()
	{
		delete [] m_donnees;
	}

	float *genere_donnees()
	{
		/* Met une barrière sur l'opération de chargement pour que toute
		 * écriture survenant auparavant sur les données nous soit visible.
		 * Ainsi nous évitons toute concurrence critique, verrou mort, ou autre
		 * famine. */
		if (m_donnees.load(std::memory_order_acquire) == nullptr) {
			std::unique_lock<std::mutex> lock(m_mutex);

			/* performe un chargement non-atomic */
			if (m_donnees.load() == nullptr) {
				float *ptr;
				genere_tuile_bruit(ptr, 128);

				/* Relâche la barrière pour garantir que toutes les opérations
				 * de mémoire planifiée avant elle dans l'ordre du programme
				 * deviennent visible avant la prochaine barrière. */
				m_donnees.store(ptr, std::memory_order_release);
			}
		}

		return m_donnees;
	}
};

constructrice_bruit_fourier constructrice_bruit_fourier::m_instance = constructrice_bruit_fourier{};

/* ************************************************************************** */

void fourrier::construit(parametres &params, int graine)
{
	auto &inst_const = constructrice_bruit_fourier::instance();
	params.tuile = inst_const.genere_donnees();

	construit_defaut(params, graine);
}

float fourrier::evalue(const parametres &params, dls::math::vec3f pos)
{
	return evalue_tuile(params.tuile, 128, &pos[0]);
}

float fourrier::evalue_derivee(const parametres &params, dls::math::vec3f pos, dls::math::vec3f &derivee)
{
	INUTILISE(params);
	derivee.x = evalue_derivee_tuile_x(params.tuile, 128, &pos[0]);
	derivee.y = evalue_derivee_tuile_y(params.tuile, 128, &pos[0]);
	derivee.z = evalue_derivee_tuile_z(params.tuile, 128, &pos[0]);

	return evalue_tuile(params.tuile, 128, &pos[0]);
}

}  /* namespace bruit */
