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

#include "flux.hh"

#include "biblinternes/math/entrepolation.hh"
#include "biblinternes/math/matrice.hh"
#include "biblinternes/outils/constantes.h"

#include "outils.hh"

namespace bruit {

#ifdef BRUIT_LONG
static int table[76] = {
	0, 10, 2, 7, 3, 5, 6, 4, 8, 1, 9,
	5, 11, 6, 8, 1, 10, 12, 9, 3, 7, 0, 4, 2,
	13, 10, 11, 5, 6, 9, 4, 3, 8, 7, 14, 2, 0, 1, 15, 12,
	1, 13, 5, 14, 12, 3, 6, 16, 0, 8, 9, 2, 11, 4, 15, 7, 10,
	10, 6, 5, 8, 15, 0, 17, 7, 14, 18, 13, 16, 2, 9, 12, 1, 11, 4, 3,
};
#else
static int table[512] = {
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
#endif

static auto index_hash(int x, int y, int z)
{
#ifdef BRUIT_LONG
	int somme = 0;
	somme += table[ 0 + (table[ 0 + (table[ 0 + x % 11] + y) % 11] + z) % 11];
	somme += table[11 + (table[11 + (table[11 + x % 13] + y) % 13] + z) % 13];
	somme += table[24 + (table[24 + (table[24 + x % 16] + y) % 16] + z) % 16];
	somme += table[40 + (table[40 + (table[40 + x % 17] + y) % 17] + z) % 17];
	somme += table[57 + (table[57 + (table[57 + x % 19] + y) % 19] + z) % 19];
	return somme % 16;
#else
	return table[(table[(table[x & 255] + y) & 255] + z) & 255];
#endif
}

/*
 * For 3D, we define two orthogonal vectors in the desired rotation plane.
 * These vectors are based on the midpoints of the 12 edges of a cube,
 * they all rotate in their own plane and are never coincident or collinear.
 * A larger array of random vectors would also do the job, but these 12
 * (including 4 repeats to make the array length a power of two) work better.
 * They are not random, they are carefully chosen to represent a small
 * isotropic set of directions for any rotation angle.
 */

/* a = sqrt(2)/sqrt(3) = 0.816496580 */
#define a 0.81649658f

static float grad3u[16][3] = {
{ 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 1.0f }, // 12 cube edges
{ -1.0f, 0.0f, 1.0f }, { 0.0f, -1.0f, 1.0f },
{ 1.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, -1.0f },
{ -1.0f, 0.0f, -1.0f }, { 0.0f, -1.0f, -1.0f },
{ a, a, a }, { -a, a, -a },
{ -a, -a, a }, { a, -a, -a },
{ -a, a, a }, { a, -a, a },
{ a, -a, -a }, { -a, a, -a }
};

static float grad3v[16][3] = {
{ -a, a, a }, { -a, -a, a },
{ a, -a, a }, { a, a, a },
{ -a, -a, -a }, { a, -a, -a },
{ a, a, -a }, { -a, a, -a },
{ 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f },
{ -1.0f, 1.0f, 0.0f }, { -1.0f, -1.0f, 0.0f },
{ 1.0f, 0.0f, 1.0f }, { -1.0f, 0.0f, 1.0f }, // 4 repeats to make 16
{ 0.0f, 1.0f, -1.0f }, { 0.0f, -1.0f, -1.0f }
};

#undef a

inline auto gradrot3( int hash, float sin_t, float cos_t)
{
	int h = hash & 15;
	float gux = grad3u[h][0];
	float guy = grad3u[h][1];
	float guz = grad3u[h][2];
	float gvx = grad3v[h][0];
	float gvy = grad3v[h][1];
	float gvz = grad3v[h][2];

	auto gx = cos_t * gux + sin_t * gvx;
	auto gy = cos_t * guy + sin_t * gvy;
	auto gz = cos_t * guz + sin_t * gvz;
	return dls::math::vec3f(gx, gy, gz);
}

static auto evalue_flux(
		parametres const &params,
		float x,
		float y,
		float z,
		dls::math::vec3f *derivee = nullptr)
{
	auto const floorx = std::floor(x);
	auto const floory = std::floor(y);
	auto const floorz = std::floor(z);

	/* trouve le cube qui contient le point */
	auto const i = static_cast<int>(floorx);
	auto const j = static_cast<int>(floory);
	auto const k = static_cast<int>(floorz);

	/* trouve les coordonnées relative des points dans le cube */
	auto const fx = x - floorx;
	auto const fy = y - floory;
	auto const fz = z - floorz;

	auto const ux = dls::math::entrepolation_fluide<2>(fx);
	auto const uy = dls::math::entrepolation_fluide<2>(fy);
	auto const uz = dls::math::entrepolation_fluide<2>(fz);

	/* cherche les gradients */
	auto sin_t = std::sin(params.temps_anim);
	auto cos_t = std::cos(params.temps_anim);

	auto const &ga = gradrot3(index_hash(i,j,k) & 15, sin_t, cos_t);
	auto const &gb = gradrot3(index_hash(i+1,j,k) & 15, sin_t, cos_t);
	auto const &gc = gradrot3(index_hash(i,j+1,k) & 15, sin_t, cos_t);
	auto const &gd = gradrot3(index_hash(i+1,j+1,k) & 15, sin_t, cos_t);
	auto const &ge = gradrot3(index_hash(i,j,k+1) & 15, sin_t, cos_t);
	auto const &gf = gradrot3(index_hash(i+1,j,k+1) & 15, sin_t, cos_t);
	auto const &gg = gradrot3(index_hash(i,j+1,k+1) & 15, sin_t, cos_t);
	auto const &gh = gradrot3(index_hash(i+1,j+1,k+1) & 15, sin_t, cos_t);

	/* projette les gradients */
	auto const va = produit_scalaire(ga, dls::math::vec3f(fx       , fy       , fz));
	auto const vb = produit_scalaire(gb, dls::math::vec3f(fx - 1.0f, fy       , fz));
	auto const vc = produit_scalaire(gc, dls::math::vec3f(fx       , fy - 1.0f, fz));
	auto const vd = produit_scalaire(gd, dls::math::vec3f(fx - 1.0f, fy - 1.0f, fz));
	auto const ve = produit_scalaire(ge, dls::math::vec3f(fx       , fy       , fz - 1.0f));
	auto const vf = produit_scalaire(gf, dls::math::vec3f(fx - 1.0f, fy       , fz - 1.0f));
	auto const vg = produit_scalaire(gg, dls::math::vec3f(fx       , fy - 1.0f, fz - 1.0f));
	auto const vh = produit_scalaire(gh, dls::math::vec3f(fx - 1.0f, fy - 1.0f, fz - 1.0f));

	float v = va +
			ux*(vb-va) +
			uy*(vc-va) +
			uz*(ve-va) +
			ux*uy*(va-vb-vc+vd) +
			uy*uz*(va-vc-ve+vg) +
			uz*ux*(va-vb-ve+vf) +
			ux*uy*uz*(-va+vb+vc-vd+ve-vf-vg+vh);

	if (derivee != nullptr) {
		auto const dux = dls::math::derivee_fluide<2>(fx);
		auto const duy = dls::math::derivee_fluide<2>(fy);
		auto const duz = dls::math::derivee_fluide<2>(fz);
		auto u  = dls::math::vec3f(ux, uy, uz);
		auto du = dls::math::vec3f(dux, duy, duz);

		*derivee = ga +
				ux*(gb-ga) +
				uy*(gc-ga) +
				uz*(ge-ga) +
				ux*uy*(ga-gb-gc+gd) +
				uy*uz*(ga-gc-ge+gg) +
				uz*ux*(ga-gb-ge+gf) +
				ux*uy*uz*(-ga+gb+gc-gd+ge-gf-gg+gh) +
				du * (dls::math::vec3f(vb-va, vc-va, ve-va) +
					  dls::math::vec3f(u.yzx)*dls::math::vec3f(va-vb-vc+vd,va-vc-ve+vg,va-vb-ve+vf) +
					  dls::math::vec3f(u.zxy)*dls::math::vec3f(va-vb-ve+vf,va-vb-vc+vd,va-vc-ve+vg) +
					  dls::math::vec3f(u.yzx)*dls::math::vec3f(u.zxy)*(-va+vb+vc-vd+ve-vf-vg+vh) );
	}

	return v;
}

void flux::construit(parametres &params, int graine)
{
	construit_defaut(params, graine);
}

float flux::evalue(parametres const &params, dls::math::vec3f pos)
{
	return evalue_flux(params, pos.x, pos.y, pos.z, nullptr);
}

float flux::evalue_derivee(parametres const &params, dls::math::vec3f pos, dls::math::vec3f &derivee)
{
	return evalue_flux(params, pos.x, pos.y, pos.z, &derivee);
}

}  /* namespace bruit */
