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

#include "gna.h"

GNA::GNA(unsigned int graine)
	: m_gna(graine)
	, m_dist_double(0.0, 1.0)
	, m_dist_unsigned(0, std::numeric_limits<unsigned int>::max())
{}

void GNA::graine(unsigned int graine)
{
	m_gna.seed(graine);
}

double GNA::nombre_aleatoire()
{
	return m_dist_double(m_gna);
}

unsigned int GNA::entier_aleatoire()
{
	return m_dist_unsigned(m_gna);
}
