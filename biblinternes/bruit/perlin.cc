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

#include "perlin.hh"

#include "biblinternes/math/entrepolation.hh"
#include "biblinternes/outils/definitions.h"

namespace bruit {

/**
 * Long-Period Hash Functions for Procedural Texturing
 * http://graphics.cs.kuleuven.be/publications/LD06LPHFPT/LD06LPHFPT_paper.pdf
 */

static unsigned int m_table[76] = {
	0, 10, 2, 7, 3, 5, 6, 4, 8, 1, 9,
	5, 11, 6, 8, 1, 10, 12, 9, 3, 7, 0, 4, 2,
	13, 10, 11, 5, 6, 9, 4, 3, 8, 7, 14, 2, 0, 1, 15, 12,
	1, 13, 5, 14, 12, 3, 6, 16, 0, 8, 9, 2, 11, 4, 15, 7, 10,
	10, 6, 5, 8, 15, 0, 17, 7, 14, 18, 13, 16, 2, 9, 12, 1, 11, 4, 3,
};

static auto grad(const unsigned int hash, const float x, const float y, const float z)
{
	auto h = hash & 15;                      // CONVERT LO 4 BITS OF HASH CODE
	auto u = (h < 8) ? x : y;                 // INTO 12 GRADIENT DIRECTIONS.
	auto v = (h < 4) ? y : (h == 12 || h == 14) ? x : z;

	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

static auto index_hash(unsigned int x, unsigned int y, unsigned int z)
{
	//return p[(p[(p[i & 255] + j) & 255] + k) & 255];
	return   (m_table[     ((m_table[      x % 11 ] + y) % 11)]
			+ m_table[11 + ((m_table[11 + (x % 13)] + y) % 13)]
			+ m_table[24 + ((m_table[24 + (x % 16)] + y) % 16)]
			+ m_table[40 + ((m_table[40 + (x % 17)] + y) % 17)]
			+ m_table[59 + ((m_table[59 + (x % 19)] + y) % 19)]) % 16;
}

static auto perlin_3d(float x, float y, float z)
{
	auto const floorx = std::floor(x);
	auto const floory = std::floor(y);
	auto const floorz = std::floor(z);

	// find cube that contains point
	auto const i = static_cast<unsigned int>(floorx);
	auto const j = static_cast<unsigned int>(floory);
	auto const k = static_cast<unsigned int>(floorz);

	// find relative x, y, z of point in cube
	auto const fx = x - floorx;
	auto const fy = y - floory;
	auto const fz = z - floorz;

	// compute interp_fluide<2> curves for each of x, y, z
	auto const sx = dls::math::entrepolation_fluide<2>(fx);
	auto const sy = dls::math::entrepolation_fluide<2>(fy);
	auto const sz = dls::math::entrepolation_fluide<2>(fz);

	auto const g000 = index_hash(i,j,k);
	auto const g100 = index_hash(i+1,j,k);
	auto const g010 = index_hash(i,j+1,k);
	auto const g110 = index_hash(i+1,j+1,k);
	auto const g001 = index_hash(i,j,k+1);
	auto const g101 = index_hash(i+1,j,k+1);
	auto const g011 = index_hash(i,j+1,k+1);
	auto const g111 = index_hash(i+1,j+1,k+1);

	auto const &n000 = grad(g000, x       , y       , z);
	auto const &n100 = grad(g100, x + 1.0f, y       , z);
	auto const &n010 = grad(g010, x       , y + 1.0f, z);
	auto const &n110 = grad(g110, x + 1.0f, y + 1.0f, z);
	auto const &n001 = grad(g001, x       , y       , z + 1.0f);
	auto const &n101 = grad(g101, x + 1.0f, y       , z + 1.0f);
	auto const &n011 = grad(g011, x       , y + 1.0f, z + 1.0f);
	auto const &n111 = grad(g111, x + 1.0f, y + 1.0f, z + 1.0f);

	return dls::math::entrepolation_trilineaire(
				n000,
				n100,
				n010,
				n110,
				n001,
				n101,
				n011,
				n111,
				sx, sy, sz);
}

void perlin::construit(parametres &params, int graine)
{
	/* RAF */
	INUTILISE(params);
	INUTILISE(graine);
}

float perlin::evalue(const parametres &params, dls::math::vec3f pos)
{
	INUTILISE(params);
	return perlin_3d(pos.x, pos.y, pos.z);
}

}  /* namespace bruit */
