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

#include "bruit_vaguelette.hh"

#include "biblinternes/outils/gna.hh"
#include "biblinternes/memoire/logeuse_memoire.hh"

/* http://graphics.pixar.com/library/WaveletNoise/paper.pdf */
/* Note: this code is designed for brevity, not efficiency; many operations can be hoisted,
* precomputed, or vectorized. Some of the straightforward details, such as tile meshing,
* decorrelating bands and fading out the last band, are omitted in the interest of space.*/
static inline auto mod_lent(int x, int n)
{
	int m = x % n;
	return (m < 0) ? m + n : m;
}

static inline auto mod_rapide_128(int n)
{
	return n & 127;
}

#define ARAD 16
void Downsample (float *from, float *to, int n, int stride )
{
	float *a, aCoeffs[2*ARAD] = {
		0.000334f,-0.001528f, 0.000410f, 0.003545f,-0.000938f,-0.008233f, 0.002172f, 0.019120f,
		-0.005040f,-0.044412f, 0.011655f, 0.103311f,-0.025936f,-0.243780f, 0.033979f, 0.655340f,
		0.655340f, 0.033979f,-0.243780f,-0.025936f, 0.103311f, 0.011655f,-0.044412f,-0.005040f,
		0.019120f, 0.002172f,-0.008233f,-0.000938f, 0.003546f, 0.000410f,-0.001528f, 0.000334f
	};

	a = &aCoeffs[ARAD];

	for (int i = 0; i < n / 2; i++) {
		to[i * stride] = 0;

		for (int k = 2 * i - ARAD; k < 2 * i + ARAD; k++) {
			auto f = from[mod_rapide_128(k) * stride];
			auto a_ = a[k - 2 * i];
			to[i * stride] += a_ * f;
		}
	}
}

void Upsample( float *from, float *to, int n, int stride)
{
	float *p, pCoeffs[4] = { 0.25, 0.75, 0.75, 0.25 };
	p = &pCoeffs[2];

	for (int i = 0; i < n; i++) {
		to[i * stride] = 0;

		for (int k= i / 2; k <= i / 2 + 1; k++) {
			to[i * stride] += p[i - 2 * k] * from[mod_lent(k, n / 2) * stride];
		}
	}
}

void GenerateNoiseTile(float *&noise, int n, int olap)
{
	if (n % 2 != 0) {
		n++; /* tile size must be even */
	}

	auto const sz = n * n * n * static_cast<long>(sizeof(float));

	auto temp1 = memoire::loge_tableau<float>("temp1", sz);
	auto temp2 = memoire::loge_tableau<float>("temp2", sz);
	noise = memoire::loge_tableau<float>("noise", sz);

	/* Step 1. Fill the tile with random numbers in the range -1 to 1. */
	auto gna = GNA{};
	for (auto i = 0; i < sz; i++) {
		noise[i] = gna.normale(0.0f, 1.0f);
	}

	/* Steps 2 and 3. Downsample and upsample the tile */
	for (auto iy = 0; iy < n; iy++) {
		for (auto iz = 0; iz < n; iz++) { /* each x row */
			auto i = iy * n + iz * n * n;
			Downsample(&noise[i], &temp1[i], n, 1);
			Upsample(&temp1[i], &temp2[i], n, 1);
		}
	}

	for (auto ix = 0; ix < n; ix++) {
		for (auto iz = 0; iz < n; iz++) { /* each y row */
			auto i = ix + iz * n * n;
			Downsample( &temp2[i], &temp1[i], n, n );
			Upsample( &temp1[i], &temp2[i], n, n );
		}
	}

	for (auto ix = 0; ix < n; ix++) {
		for (auto iy = 0; iy < n; iy++) { /* each z row */
			auto i = ix + iy * n;
			Downsample( &temp2[i], &temp1[i], n, n*n );
			Upsample( &temp1[i], &temp2[i], n, n*n );
		}
	}

	/* Step 4. Subtract out the coarse-scale contribution */
	for (auto i = 0; i < sz; i++) {
		noise[i] -= temp2[i];
	}

	/* Avoid even/odd variance difference by adding odd-offset version of noise to itself.*/
	auto offset = n / 2;

	if (offset % 2 == 0){
		offset++;
	}

	for (auto i = 0, ix = 0; ix < n; ix++) {
		for (auto iy = 0; iy < n; iy++) {
			for (auto iz = 0; iz < n; iz++) {
				temp1[i++] = noise[ mod_lent(ix+offset,n) + mod_lent(iy+offset,n)*n + mod_lent(iz+offset,n)*n*n ];
			}
		}
	}

	for (auto i = 0; i < sz; i++) {
		noise[i] += temp1[i];
	}

	memoire::deloge_tableau("temp1", temp1, sz);
	memoire::deloge_tableau("temp2", temp2, sz);
}

static float WNoise(float *noiseTileData, int noiseTileSize, float p[3])
{
	auto n = noiseTileSize;
	/* Non-projected 3D noise */
	int f[3], c[3], mid[3]; /* f, c = filter, noise coeff indices */
	float w[3][3];

	/* Evaluate quadratic B-spline basis functions */
	for (int i = 0; i < 3; i++) {
		mid[i] = static_cast<int>(std::ceil(p[i] - 0.5f));
		auto const t = static_cast<float>(mid[i]) - (p[i] - 0.5f);

		w[i][0] = t * t / 2.0f;
		w[i][2] = (1 - t) * (1 - t) / 2.0f;
		w[i][1]= 1.0f - w[i][0] - w[i][2];
	}

	/* Evaluate noise by weighting noise coefficients by basis function values */
	auto result = 0.0f;

	for (f[2] = -1; f[2] <= 1; f[2]++) {
		for (f[1] = -1; f[1] <= 1; f[1]++) {
			for (f[0] = -1; f[0] <= 1; f[0]++) {
				float weight = 1.0f;

				for (int i=0; i<3; i++) {
					c[i] = mod_lent(mid[i] + f[i], n);
					weight *= w[i][f[i] + 1];
				}

				result += weight * noiseTileData[c[2]*n*n+c[1]*n+c[0]];
			}
		}
	}

	return result;
}

/* ************************************************************************** */

bruit_vaguelette::~bruit_vaguelette()
{
	memoire::deloge_tableau("noise", m_donnees, 128 * 128 * 128 * static_cast<long>(sizeof(float)));
}

void bruit_vaguelette::genere_donnees()
{
	if (m_donnees != nullptr) {
		return;
	}

	GenerateNoiseTile(m_donnees, 128, 1);
}

float bruit_vaguelette::evalue(float pos[3]) const
{
	return WNoise(m_donnees, 128, pos);
}
