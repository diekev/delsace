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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "alea.h"

#include <random>

extern "C" {

struct GNA {
	std::mt19937 mt;
};

GNA *ALEA_cree_gna(unsigned graine)
{
	auto gna = static_cast<GNA *>(malloc(sizeof(GNA)));
	gna->mt = std::mt19937(graine);
	return gna;
}

void ALEA_reseme_gna(GNA *gna, unsigned graine)
{
	gna->mt.seed(graine);
}

void ALEA_detruit_gna(GNA *gna)
{
	free(gna);
}

float ALEA_uniforme_r32(GNA *gna, float min, float max)
{
	auto dist = std::uniform_real_distribution<float>(min, max);
	return dist(gna->mt);
}

double ALEA_uniforme_r64(GNA *gna, double min, double max)
{
	auto dist = std::uniform_real_distribution<double>(min, max);
	return dist(gna->mt);
}

float ALEA_normale_r32(GNA *gna, float moyenne, float ecart)
{
	auto dist = std::normal_distribution<float>(moyenne, ecart);
	return dist(gna->mt);
}

double ALEA_normale_r64(GNA *gna, double moyenne, double ecart)
{
	auto dist = std::normal_distribution<double>(moyenne, ecart);
	return dist(gna->mt);
}

}
