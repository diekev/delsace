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

#include "bruit.hh"

#include <cmath>
#include "biblinternes/outils/gna.hh"

#include "entrepolation.hh"
#include "matrice.hh"

namespace dls::math {

/* ************************************************************************** */

static constexpr auto TAU = 2.0f * static_cast<float>(M_PI);

/* ************************************************************************** */

BruitPerlin2D::BruitPerlin2D(unsigned int seed)
{
	for (unsigned int i = 0; i < N; ++i) {
		const auto theta = static_cast<float>(i * 2 * M_PI) / N;
		m_basis[i][0] = std::cos(theta);
		m_basis[i][1] = std::sin(theta);
		m_perm[i] = i;
	}

	reinitialize(seed);
}

void BruitPerlin2D::reinitialize(unsigned int seed)
{
	for (unsigned int i = 1; i < N; ++i){
		const auto j = empreinte_n32(seed++) % (i + 1);
		std::swap(m_perm[i], m_perm[j]);
	}
}

float BruitPerlin2D::operator()(const vec2f &x) const
{
	return (*this)(x[0], x[1]);
}

float BruitPerlin2D::operator()(float x, float y) const
{
	const auto floorx = std::floor(x);
	const auto floory = std::floor(y);

	const auto i = static_cast<unsigned int>(floorx);
	const auto j = static_cast<unsigned int>(floory);

	const auto &n00 = m_basis[hash_index(i,j)];
	const auto &n10 = m_basis[hash_index(i+1,j)];
	const auto &n01 = m_basis[hash_index(i,j+1)];
	const auto &n11 = m_basis[hash_index(i+1,j+1)];

	const auto fx = x - floorx;
	const auto fy = y - floory;
	const auto sx = entrepolation_fluide<2>(fx);
	const auto sy = entrepolation_fluide<2>(fy);

	return entrepolation_bilineaire(
				fx * n00[0] + fy * n00[1],
				(fx - 1) * n10[0] + fy * n10[1],
				fx * n01[0] + (fy - 1) * n01[1],
				(fx - 1) * n11[0] + (fy - 1) * n11[1],
				sx, sy);
}

unsigned int BruitPerlin2D::hash_index(unsigned int i, unsigned int j) const
{
	return m_perm[(m_perm[i % N] + j) % N];
}

/* ************************************************************************** */

static constexpr auto NUM_TABLES = 5;

static float grad(const unsigned int hash, const float x, const float y, const float z)
{
	auto h = hash & 15;                      // CONVERT LO 4 BITS OF HASH CODE
	auto u = (h < 8) ? x : y;                 // INTO 12 GRADIENT DIRECTIONS.
	auto v = (h < 4) ? y : (h == 12 || h == 14) ? x : z;

	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

BruitPerlinLong2D::BruitPerlinLong2D()
	: m_table(76)
{
	const unsigned int tables_size[NUM_TABLES] = { 11, 13, 16, 17, 19 };
	const unsigned int size_offset[NUM_TABLES] = {  0, 11, 24, 40, 57 };

	for (unsigned int n = 0; n < NUM_TABLES; n++) {
		const auto size = tables_size[n];
		const auto offset = size_offset[n];

		for (unsigned int i = 0; i < size; ++i) {
			m_table[i + offset] = i;
		}

		for (unsigned int i = 0; i < (size - 1); ++i) {
			auto j = i + (static_cast<unsigned>(std::rand()) % (size - i));
			std::swap(m_table[i + offset], m_table[j + offset]);
		}
	}
}

float BruitPerlinLong2D::operator()(float x, float y, float z) const
{
	// find cube that contains point
	const auto x0 = static_cast<unsigned int>(std::floor(x)) & 76;
	const auto y0 = static_cast<unsigned int>(std::floor(y)) & 76;
	const auto z0 = static_cast<unsigned int>(std::floor(z)) & 76;

	// find relative x, y, z of point in cube
	x -= std::floor(x);
	y -= std::floor(y);
	z -= std::floor(z);

	// compute interp_fluide<2> curves for each of x, y, z
	const auto u = entrepolation_fluide<2>(x);
	const auto v = entrepolation_fluide<2>(y);
	const auto w = entrepolation_fluide<2>(z);

	// hash coordinates of the 8 cube corners
	const auto  A = hash(x0,     y0); // m_table[x0    ] + y0;
	const auto AA = hash( A,     z0); // m_table[A     ] + z0;
	const auto AB = hash( A + 1, z0); // m_table[A  + 1] + z0;
	const auto  B = hash(x0 + 1, y0); // m_table[x0 + 1] + y0;
	const auto BA = hash( B,     z0); // m_table[B     ] + z0;
	const auto BB = hash( B + 1, z0); // m_table[B  + 1] + z0;

	// and add blended results from the 8 corners of the cube
	return entrepolation_lineaire(w, entrepolation_lineaire(v, entrepolation_lineaire(u, grad(m_table[AA  ], x  , y  , z   ),
								   grad(m_table[BA  ], x-1, y  , z   )),
						   entrepolation_lineaire(u, grad(m_table[AB  ], x  , y-1, z   ),
								   grad(m_table[BB  ], x-1, y-1, z   ))),
				   entrepolation_lineaire(v, entrepolation_lineaire(u, grad(m_table[AA+1], x  , y  , z-1 ),
						grad(m_table[BA+1], x-1, y  , z-1 )),
			entrepolation_lineaire(u, grad(m_table[AB+1], x  , y-1, z-1 ),
			grad(m_table[BB+1], x-1, y-1, z-1 ))));
}

unsigned int BruitPerlinLong2D::hash(unsigned int x, unsigned int y) const
{
	//m_table[((m_table[((m_table[x % 11] + y) % 11)] + z) % 11)]
	return   (m_table[     ((m_table[      x % 11 ] + y) % 11)]
			+ m_table[11 + ((m_table[11 + (x % 13)] + y) % 13)]
			+ m_table[24 + ((m_table[24 + (x % 16)] + y) % 16)]
			+ m_table[40 + ((m_table[40 + (x % 17)] + y) % 17)]
			+ m_table[59 + ((m_table[59 + (x % 19)] + y) % 19)]) % 16;
}

/* ************************************************************************** */

BruitPerlin3D::BruitPerlin3D(unsigned int seed)
{
	auto gna = GNASimple(seed);
	for (unsigned int i = 0; i < N; ++i){
		m_basis[i] = echantillone_sphere<vec3f>(gna);
		m_perm[i] = i;
	}

	reinitialise(seed);
}

void BruitPerlin3D::reinitialise(unsigned int seed)
{
	for (unsigned int i = 1; i < N; ++i){
		auto j = empreinte_n32(seed++) % (i + 1);
		std::swap(m_perm[i], m_perm[j]);
	}
}

float BruitPerlin3D::operator()(float x, float y, float z) const
{
	const auto floorx = std::floor(x);
	const auto floory = std::floor(y);
	const auto floorz = std::floor(z);

	// find cube that contains point
	const auto i = static_cast<unsigned int>(floorx);
	const auto j = static_cast<unsigned int>(floory);
	const auto k = static_cast<unsigned int>(floorz);

	// find relative x, y, z of point in cube
	const auto fx = x - floorx;
	const auto fy = y - floory;
	const auto fz = z - floorz;

	// compute interp_fluide<2> curves for each of x, y, z
	const auto sx = entrepolation_fluide<2>(fx);
	const auto sy = entrepolation_fluide<2>(fy);
	const auto sz = entrepolation_fluide<2>(fz);

	const auto &n000 = m_basis[index_hash(i,j,k)];
	const auto &n100 = m_basis[index_hash(i+1,j,k)];
	const auto &n010 = m_basis[index_hash(i,j+1,k)];
	const auto &n110 = m_basis[index_hash(i+1,j+1,k)];
	const auto &n001 = m_basis[index_hash(i,j,k+1)];
	const auto &n101 = m_basis[index_hash(i+1,j,k+1)];
	const auto &n011 = m_basis[index_hash(i,j+1,k+1)];
	const auto &n111 = m_basis[index_hash(i+1,j+1,k+1)];

	return entrepolation_trilineaire(    fx*n000[0] +     fy*n000[1] +     fz*n000[2],
			(fx-1)*n100[0] +     fy*n100[1] +     fz*n100[2],
			fx*n010[0] + (fy-1)*n010[1] +     fz*n010[2],
			(fx-1)*n110[0] + (fy-1)*n110[1] +     fz*n110[2],
			fx*n001[0] +     fy*n001[1] + (fz-1)*n001[2],
			(fx-1)*n101[0] +     fy*n101[1] + (fz-1)*n101[2],
			fx*n011[0] + (fy-1)*n011[1] + (fz-1)*n011[2],
			(fx-1)*n111[0] + (fy-1)*n111[1] + (fz-1)*n111[2],
			sx, sy, sz);
}

float BruitPerlin3D::operator()(const vec3f &x) const
{
	return (*this)(x[0], x[1], x[2]);
}

unsigned int BruitPerlin3D::index_hash(unsigned int i, unsigned int j, unsigned int k) const
{
	return m_perm[(m_perm[(m_perm[i % N] + j) % N] + k) % N];
}

/* ************************************************************************** */

BruitFlux2D::BruitFlux2D(unsigned int graine, float variation_tournoiement)
	: BruitPerlin2D(graine)
{
	/* Évite probablement une superposition avec la séquence utilisée dans
	 * l'initialisation de la classe supérieure BruitPerlin2D. */
	graine += N;

	auto gna = GNASimple(graine);

	const auto a = 1.0f - 0.5f * variation_tournoiement;
	const auto b = 1.0f + 0.5f * variation_tournoiement;

	for(unsigned int i = 0; i < N; ++i) {
		m_base_originelle[i] = m_basis[i];
		m_taux_tournoiement[i]= TAU * gna.uniforme(a, b);
	}
}

void BruitFlux2D::change_temps(float temps)
{
	for (unsigned int i = 0; i < N; ++i) {
		const auto theta = m_taux_tournoiement[i] * temps;

		/* Crée la matrice de rotation. */
		auto matrice = matrice_rotation(theta);

		m_basis[i] = matrice * m_base_originelle[i];
	}
}

/* ************************************************************************** */

BruitFlux3D::BruitFlux3D(unsigned int graine, float variation_tournoiement)
	: BruitPerlin3D(graine)
{
	/* Évite probablement une superposition avec la séquence utilisée dans
	 * l'initialisation de la classe supérieure BruitPerlin3D. */
	graine += 8 * N;

	auto gna = GNASimple(graine);

	const auto a = 1.0f - 0.5f * variation_tournoiement;
	const auto b = 1.0f + 0.5f * variation_tournoiement;

	for (unsigned int i = 0; i < N; ++i) {
		original_basis[i] = m_basis[i];
		axe_tournoiement[i] = echantillone_sphere<vec3f>(gna);
		taux_tournoiement[i] = TAU * gna.uniforme(a, b);
	}
}

void BruitFlux3D::change_temps(float temps)
{
	for (unsigned int i = 0; i < N; ++i) {
		const auto theta = taux_tournoiement[i] * temps;

		/* Crée la matrice de rotation. */
		auto matrice = matrice_rotation(axe_tournoiement[i], theta);

		m_basis[i] = matrice * original_basis[i];
	}
}

/* ************************************************************************** */

namespace simplex {

static int perm[512] = {
	151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225,
	140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148,
	247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32,
	57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175,
	74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122,
	60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54,
	65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169,
	200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64,
	52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212,
	207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213,
	119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
	129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104,
	218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241,
	81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157,
	184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93,
	222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180,
	151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225,
	140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148,
	247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32,
	57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175,
	74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122,
	60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54,
	65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169,
	200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64,
	52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212,
	207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213,
	119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
	129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104,
	218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241,
	81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157,
	184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93,
	222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180,
};

static constexpr float F3 = 1.0f / 3.0f; // (std::sqrt(4.0f) - 1.0f) / 3.0f;
static constexpr float G3 = 1.0f / 6.0f;

static auto fastfloor(float x)
{
	auto const xi = static_cast<float>(static_cast<int>(x));
	return (x < xi) ? xi - 1.0f : xi;
}

static float grad3[12][3] = {
	{  1,  1,  0 }, { -1,  1,  0 }, {  1, -1,  0 }, { -1, -1,  0 },
	{  1,  0,  1 }, { -1,  0,  1 }, {  1,  0, -1 }, { -1,  0, -1 },
	{  0,  1,  1 }, {  0, -1,  1 }, {  0,  1, -1 }, {  0, -1, -1 }
};

}   /* namespace simplex */

float bruit_simplex_3d(float xin, float yin, float zin)
{
	using namespace simplex;

	/* Noise contributions from the four corners */
	float n0, n1, n2, n3;

	/* Skew the input space to determine which simplex cell we're in */

	/* Very nice and simple skew factor for 3D */
	auto const s = (xin + yin + zin) * F3;
	auto const i = fastfloor(xin + s);
	auto const j = fastfloor(yin + s);
	auto const k = fastfloor(zin + s);

	/* Unskew the cell origin back to (x,y,z) space */
	auto const t = (i + j + k) * G3;
	auto const X0 = i - t;
	auto const Y0 = j - t;
	auto const Z0 = k - t;

	/* The x,y,z distances from the cell origin */
	auto const x0 = xin - X0;
	auto const y0 = yin - Y0;
	auto const z0 = zin - Z0;

	/* For the 3D case, the simplex shape is a slightly irregular tetrahedron. */

	/* Determine which simplex we are in. */

	/* Offsets for second corner of simplex in (i,j,k) coords */
	int i1, j1, k1;
	/* Offsets for third corner of simplex in (i,j,k) coords */
	int i2, j2, k2;

	if (x0 >= y0) {
		/* XYZ order */
		if (y0 >= z0) {
			i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
		}
		/* XZY order */
		else if (x0 >= z0) {
			i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1;
		}
		/* ZXY order */
		else {
			i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1;
		}
	}
	else { /* x0 < y0 */
		/* ZYX order */
		if (y0 < z0) {
			i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1;
		}
		/* YZX order */
		else if (x0 < z0) {
			i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1;
		}
		/* YXZ order */
		else {
			i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
		}
	}

	/* A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
	 * a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
	 * a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
	 * c = 1/6. */

	/* Offsets for second corner in (x,y,z) coords */
	auto const x1 = x0 - static_cast<float>(i1) + G3;
	auto const y1 = y0 - static_cast<float>(j1) + G3;
	auto const z1 = z0 - static_cast<float>(k1) + G3;

	/* Offsets for third corner in (x,y,z) coords */
	auto const x2 = x0 - static_cast<float>(i2) + 2.0f * G3;
	auto const y2 = y0 - static_cast<float>(j2) + 2.0f * G3;
	auto const z2 = z0 - static_cast<float>(k2) + 2.0f * G3;

	/* Offsets for last corner in (x,y,z) coords */
	auto const x3 = x0 - 1.0f + 3.0f * G3;
	auto const y3 = y0 - 1.0f + 3.0f * G3;
	auto const z3 = z0 - 1.0f + 3.0f * G3;

	/* Work out the hashed gradient indices of the four simplex corners */
	auto const ii = static_cast<int>(i) & 255;
	auto const jj = static_cast<int>(j) & 255;
	auto const kk = static_cast<int>(k) & 255;

	auto const gi0 = perm[ii + perm[jj + perm[kk]]] % 12;
	auto const gi1 = perm[ii + i1 + perm[jj + j1 + perm[kk + k1]]] % 12;
	auto const gi2 = perm[ii + i2 + perm[jj + j2 + perm[kk + k2]]] % 12;
	auto const gi3 = perm[ii + 1 + perm[jj + 1 + perm[kk + 1]]] % 12;

	/* Calculate the contribution from the four corners */
	float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;

	if (t0 < 0.0f) {
		n0 = 0.0f;
	}
	else {
		t0 *= t0;
		n0 = t0 * t0 * produit_scalaire(grad3[gi0], x0, y0, z0);
	}

	float t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;

	if (t1 < 0.0f) {
		n1 = 0.0f;
	}
	else {
		t1 *= t1;
		n1 = t1 * t1 * produit_scalaire(grad3[gi1], x1, y1, z1);
	}

	float t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;

	if (t2 < 0.0f) {
		n2 = 0.0f;
	}
	else {
		t2 *= t2;
		n2 = t2 * t2 * produit_scalaire(grad3[gi2], x2, y2, z2);
	}

	float t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
	if (t3 < 0.0f) {
		n3 = 0.0f;
	}
	else {
		t3 *= t3;
		n3 = t3 * t3 * produit_scalaire(grad3[gi3], x3, y3, z3);
	}

	/* Add contributions from each corner to get the final noise value.
	 * The result is scaled to stay just inside [-1,1] */
	return 32.0f * (n0 + n1 + n2 + n3);
}

/* ************************************************************************** */

BruitCourbe2D::BruitCourbe2D()
{
	m_echelle_longueur_accroissement_bruit.pousse(std::make_pair(0.1f, 0.03f));
	m_echelle_longueur_accroissement_bruit.pousse(std::make_pair(0.06f, 0.03f * 0.35f));
	m_echelle_longueur_accroissement_bruit.pousse(std::make_pair(0.026f, 0.03f * 0.1f));
}

float BruitCourbe2D::distance_solide(float x, float y) const
{
	return longueur(vec2f(x, y) - m_centre_disque) - m_rayon_disque;
}

float BruitCourbe2D::potentiel(float x, float y) const
{
	/* Débute avec du flux laminaire arrière-plan (ajusté de sorte que le niveau
	 * zéro passe à travers le centre du disque). */
	auto p = -m_vitesse_flux_arriere_plan * (y - m_centre_disque[1]);

	/* Modifie pour le disque. */
	const auto d = distance_solide(x, y);

	if (d < m_influence_bruit) {
		p *= rampe<2>(d / m_influence_bruit);
	}

	/* Ajoute un sillage turbulent qui respecte le disque. */
	const auto sillage_x = entrepolation_fluide<2>(1 - (x - m_centre_disque[0]) / m_rayon_disque);

	if (sillage_x <= 0) {
		return p;
	}

	const auto sillage_y = entrepolation_fluide<2>(1 - std::fabs(y - m_centre_disque[1]) / (1.5f * m_rayon_disque + m_expansion_sillage * (m_centre_disque[0] - x)));

	if (sillage_y <= 0) {
		return p;
	}

	const auto sillage = sillage_x * sillage_y;
	float s = 0;

	for (const std::pair<float, float> &longueur_bruit : m_echelle_longueur_accroissement_bruit) {
		const auto pos_x = (x - m_vitesse_flux_arriere_plan * m_temps) / longueur_bruit.first;
		const auto pos_y = (y / longueur_bruit.first);
		const auto bruit = m_noise(pos_x, pos_y);

		s += rampe<2>(d / longueur_bruit.first) * longueur_bruit.second * bruit;
	}

	return p + sillage * s;
}

void BruitCourbe2D::avance_temps(float dt)
{
	m_temps += dt;
	m_noise.change_temps(0.5f * m_echelle_longueur_accroissement_bruit[0].second / m_echelle_longueur_accroissement_bruit[0].first * m_temps);
}

void BruitCourbe2D::velocite(const vec2f &x, vec2f &v) const
{
	v[0] = -(potentiel(x[0], x[1] + m_delta_x) - potentiel(x[0], x[1] - m_delta_x)) / (2 * m_delta_x);
	v[1] =  (potentiel(x[0] + m_delta_x, x[1]) - potentiel(x[0] - m_delta_x, x[1])) / (2 * m_delta_x);
}

float BruitCourbe2D::operator()(float x, float y) const
{
	auto pos = vec2f{ x, y };
	vec2f v;

	velocite(pos, v);

	return longueur(v);
}

}  /* namespace dls::math */
