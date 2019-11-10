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

#include "simplex.hh"

#include "biblinternes/math/outils.hh"

#include "outils.hh"

namespace bruit {

namespace detail {

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

}   /* namespace detail */

float simplex_3d(float xin, float yin, float zin, dls::math::vec3f *derivee = nullptr)
{
	using namespace detail;

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
	auto t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
	auto t20 = 0.0f;
	auto t40 = 0.0f;

	auto gx0 = 0.0f;
	auto gx1 = 0.0f;
	auto gx2 = 0.0f;
	auto gx3 = 0.0f;
	auto gy0 = 0.0f;
	auto gy1 = 0.0f;
	auto gy2 = 0.0f;
	auto gy3 = 0.0f;
	auto gz0 = 0.0f;
	auto gz1 = 0.0f;
	auto gz2 = 0.0f;
	auto gz3 = 0.0f;

	if (t0 < 0.0f) {
		n0 = 0.0f;
	}
	else {
		t20 = t0 * t0;
		t40 = t20 * t20;

		auto g = grad3[gi0];
		gx0 = g[0];
		gy0 = g[1];
		gz0 = g[2];

		n0 = t40 * dls::math::produit_scalaire(g, x0, y0, z0);
	}

	auto t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
	auto t21 = 0.0f;
	auto t41 = 0.0f;

	if (t1 < 0.0f) {
		n1 = 0.0f;
	}
	else {
		t21 = t1 * t1;
		t41 = t21 * t21;

		auto g = grad3[gi1];
		gx1 = g[0];
		gy1 = g[1];
		gz1 = g[2];

		n1 = t41 * dls::math::produit_scalaire(g, x1, y1, z1);
	}

	auto t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
	auto t22 = 0.0f;
	auto t42 = 0.0f;

	if (t2 < 0.0f) {
		n2 = 0.0f;
	}
	else {
		t22 = t2 * t2;
		t42 = t22 * t22;

		auto g = grad3[gi2];
		gx2 = g[0];
		gy2 = g[1];
		gz2 = g[2];

		n2 = t42 * dls::math::produit_scalaire(g, x2, y2, z2);
	}

	auto t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
	auto t23 = 0.0f;
	auto t43 = 0.0f;

	if (t3 < 0.0f) {
		n3 = 0.0f;
	}
	else {
		t23 = t3 * t3;
		t43 = t23 * t23;

		auto g = grad3[gi3];
		gx3 = g[0];
		gy3 = g[1];
		gz3 = g[2];

		n3 = t43 * dls::math::produit_scalaire(g, x3, y3, z3);
	}

	/* Source pour la dérivée :
	 * https://github.com/simongeilfus/SimplexNoise/blob/master/include/Simplex.h
	 *   A straight, unoptimised calculation would be like:
	 *    *dnoise_dx = -8.0f * t20 * t0 * x0 * dot(gx0, gy0, gz0, x0, y0, z0) + t40 * gx0;
	 *    *dnoise_dy = -8.0f * t20 * t0 * y0 * dot(gx0, gy0, gz0, x0, y0, z0) + t40 * gy0;
	 *    *dnoise_dz = -8.0f * t20 * t0 * z0 * dot(gx0, gy0, gz0, x0, y0, z0) + t40 * gz0;
	 *    *dnoise_dx += -8.0f * t21 * t1 * x1 * dot(gx1, gy1, gz1, x1, y1, z1) + t41 * gx1;
	 *    *dnoise_dy += -8.0f * t21 * t1 * y1 * dot(gx1, gy1, gz1, x1, y1, z1) + t41 * gy1;
	 *    *dnoise_dz += -8.0f * t21 * t1 * z1 * dot(gx1, gy1, gz1, x1, y1, z1) + t41 * gz1;
	 *    *dnoise_dx += -8.0f * t22 * t2 * x2 * dot(gx2, gy2, gz2, x2, y2, z2) + t42 * gx2;
	 *    *dnoise_dy += -8.0f * t22 * t2 * y2 * dot(gx2, gy2, gz2, x2, y2, z2) + t42 * gy2;
	 *    *dnoise_dz += -8.0f * t22 * t2 * z2 * dot(gx2, gy2, gz2, x2, y2, z2) + t42 * gz2;
	 *    *dnoise_dx += -8.0f * t23 * t3 * x3 * dot(gx3, gy3, gz3, x3, y3, z3) + t43 * gx3;
	 *    *dnoise_dy += -8.0f * t23 * t3 * y3 * dot(gx3, gy3, gz3, x3, y3, z3) + t43 * gy3;
	 *    *dnoise_dz += -8.0f * t23 * t3 * z3 * dot(gx3, gy3, gz3, x3, y3, z3) + t43 * gz3;
	 */
	if (derivee) {
		auto temp0 = t20 * t0 * ( gx0 * x0 + gy0 * y0 + gz0 * z0 );
		auto dnoise_dx = temp0 * x0;
		auto dnoise_dy = temp0 * y0;
		auto dnoise_dz = temp0 * z0;
		auto temp1 = t21 * t1 * ( gx1 * x1 + gy1 * y1 + gz1 * z1 );
		dnoise_dx += temp1 * x1;
		dnoise_dy += temp1 * y1;
		dnoise_dz += temp1 * z1;
		auto temp2 = t22 * t2 * ( gx2 * x2 + gy2 * y2 + gz2 * z2 );
		dnoise_dx += temp2 * x2;
		dnoise_dy += temp2 * y2;
		dnoise_dz += temp2 * z2;
		auto temp3 = t23 * t3 * ( gx3 * x3 + gy3 * y3 + gz3 * z3 );
		dnoise_dx += temp3 * x3;
		dnoise_dy += temp3 * y3;
		dnoise_dz += temp3 * z3;
		dnoise_dx *= -8.0f;
		dnoise_dy *= -8.0f;
		dnoise_dz *= -8.0f;
		dnoise_dx += t40 * gx0 + t41 * gx1 + t42 * gx2 + t43 * gx3;
		dnoise_dy += t40 * gy0 + t41 * gy1 + t42 * gy2 + t43 * gy3;
		dnoise_dz += t40 * gz0 + t41 * gz1 + t42 * gz2 + t43 * gz3;
		dnoise_dx *= 32.0f; /* Scale derivative to match the noise scaling */
		dnoise_dy *= 32.0f;
		dnoise_dz *= 32.0f;

		derivee->x = dnoise_dx;
		derivee->y = dnoise_dy;
		derivee->z = dnoise_dz;
	}

	/* Add contributions from each corner to get the final noise value.
	 * The result is scaled to stay just inside [-1,1] */
	return 32.0f * (n0 + n1 + n2 + n3);
}

void simplex::construit(parametres &params, int graine)
{
	construit_defaut(params, graine);
}

float simplex::evalue(parametres const &params, dls::math::vec3f pos)
{
	INUTILISE(params);
	return simplex_3d(pos.x, pos.y, pos.z);
}

float simplex::evalue_derivee(const parametres &params, dls::math::vec3f pos, dls::math::vec3f &derivee)
{
	INUTILISE(params);
	return simplex_3d(pos.x, pos.y, pos.z, &derivee);
}

}  /* namespace bruit */
