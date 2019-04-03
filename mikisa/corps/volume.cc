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

#include "volume.hh"

#include "bibloc/logeuse_memoire.hh"

/* ************************************************************************** */

type_primitive Volume::type_prim() const
{
	return type_primitive::VOLUME;
}

long BaseGrille::calcul_index(size_t x, size_t y, size_t z) const
{
	return static_cast<long>(x + (y + z * m_res[1]) * m_res[0]);
}

bool BaseGrille::hors_des_limites(size_t x, size_t y, size_t z) const
{
	if (x >= m_res[0]) {
		return true;
	}

	if (y >= m_res[1]) {
		return true;
	}

	if (z >= m_res[2]) {
		return true;
	}

	return false;
}

dls::math::vec3<size_t> BaseGrille::resolution() const
{
	return m_res;
}

/* ************************************************************************** */

Volume::~Volume()
{
	switch (grille->type()) {
		case type_volume::SCALAIRE:
		{
			auto ptr = dynamic_cast<Grille<float> *>(grille);
			memoire::deloge("grille", ptr);
			break;
		}
		case type_volume::VECTOR:
		{
			auto ptr = dynamic_cast<Grille<dls::math::vec3f> *>(grille);
			memoire::deloge("grille", ptr);
			break;
		}
	}
}
