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

#include "gna.hh"

GNA::GNA(int graine)
	: m_gna(graine)
{}

int GNA::uniforme(int min, int max)
{
	auto dist = std::uniform_int_distribution<int>(min, max);
	return dist(m_gna);
}

float GNA::uniforme(float min, float max)
{
	auto dist = std::uniform_real_distribution<float>(min, max);
	return dist(m_gna);
}

double GNA::uniforme(double min, double max)
{
	auto dist = std::uniform_real_distribution<double>(min, max);
	return dist(m_gna);
}
