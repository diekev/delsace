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

#include "biblinternes/memoire/logeuse_memoire.hh"

#include "wolika/grille_dense.hh"
#include "wolika/grille_eparse.hh"

/* ************************************************************************** */

template <typename T>
static auto deloge_grille_impl(wlk::base_grille_3d *&grille)
{
	if (!grille->est_eparse()) {
		auto ptr = dynamic_cast<wlk::grille_dense_3d<T> *>(grille);
		memoire::deloge("grille", ptr);
	}
	else {
		auto ptr = dynamic_cast<wlk::grille_eparse<T> *>(grille);
		memoire::deloge("grille_eparse", ptr);
	}

	grille = nullptr;
}

/* ************************************************************************** */

Volume::Volume(wlk::base_grille_3d *grl)
	: grille(grl)
{}

Volume::~Volume()
{
	if (grille == nullptr) {
		return;
	}

	auto const &desc = grille->desc();

	switch (desc.type_donnees) {
		case wlk::type_grille::N32:
		{
			deloge_grille_impl<char>(grille);
			break;
		}
		case wlk::type_grille::Z8:
		{
			deloge_grille_impl<char>(grille);
			break;
		}
		case wlk::type_grille::Z32:
		{
			deloge_grille_impl<int>(grille);
			break;
		}
		case wlk::type_grille::R32:
		{
			deloge_grille_impl<float>(grille);
			break;
		}
		case wlk::type_grille::R32_PTR:
		{
			deloge_grille_impl<float *>(grille);
			break;
		}
		case wlk::type_grille::R64:
		{
			deloge_grille_impl<double>(grille);
			break;
		}
		case wlk::type_grille::VEC2:
		{
			deloge_grille_impl<dls::math::vec2f>(grille);
			break;
		}
		case wlk::type_grille::VEC3:
		{
			deloge_grille_impl<dls::math::vec3f>(grille);
			break;
		}
		case wlk::type_grille::VEC3_R64:
		{
			deloge_grille_impl<dls::math::vec3d>(grille);
			break;
		}
	}
}

type_primitive Volume::type_prim() const
{
	return type_primitive::VOLUME;
}
