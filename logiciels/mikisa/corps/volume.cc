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

/* ************************************************************************** */

type_primitive Volume::type_prim() const
{
	return type_primitive::VOLUME;
}

long BaseGrille::calcul_index(size_t x, size_t y, size_t z) const
{
	return static_cast<long>(x + (y + z * static_cast<size_t>(m_desc.resolution[1])) * static_cast<size_t>(m_desc.resolution[0]));
}

long BaseGrille::nombre_voxels() const
{
	return static_cast<long>(m_nombre_voxels);
}

bool BaseGrille::hors_des_limites(size_t x, size_t y, size_t z) const
{
	if (x >= static_cast<size_t>(m_desc.resolution[0])) {
		return true;
	}

	if (y >= static_cast<size_t>(m_desc.resolution[1])) {
		return true;
	}

	if (z >= static_cast<size_t>(m_desc.resolution[2])) {
		return true;
	}

	return false;
}

BaseGrille::BaseGrille(description_volume const &descr)
	: m_desc(descr)
{
	auto taille = m_desc.etendue.taille();
	m_desc.resolution[0] = static_cast<int>(taille.x / m_desc.taille_voxel);
	m_desc.resolution[1] = static_cast<int>(taille.y / m_desc.taille_voxel);
	m_desc.resolution[2] = static_cast<int>(taille.z / m_desc.taille_voxel);

	m_nombre_voxels = static_cast<size_t>(m_desc.resolution[0]) * static_cast<size_t>(m_desc.resolution[1]) * static_cast<size_t>(m_desc.resolution[2]);
}

dls::math::vec3f BaseGrille::index_vers_unit(const dls::math::vec3i &vsp) const
{
	auto p = dls::math::discret_vers_continu<float>(vsp);
	return index_vers_unit(p);
}

dls::math::vec3f BaseGrille::index_vers_unit(const dls::math::vec3f &vsp) const
{
	return dls::math::vec3f(
				vsp.x / static_cast<float>(m_desc.resolution.x),
				vsp.y / static_cast<float>(m_desc.resolution.y),
				vsp.z / static_cast<float>(m_desc.resolution.z));
}

dls::math::vec3f BaseGrille::index_vers_monde(const dls::math::vec3i &isp) const
{
	auto const dim = etendue().taille();
	auto const min = etendue().min;
	auto const cont = dls::math::discret_vers_continu<float>(isp);
	return dls::math::vec3f(
				cont.x / static_cast<float>(m_desc.resolution.x) * dim.x + min.x,
				cont.y / static_cast<float>(m_desc.resolution.y) * dim.y + min.y,
				cont.z / static_cast<float>(m_desc.resolution.z) * dim.z + min.z);
}

dls::math::vec3f BaseGrille::unit_vers_monde(const dls::math::vec3f &vsp) const
{
	return vsp * etendue().taille() + etendue().min;
}

dls::math::vec3f BaseGrille::monde_vers_unit(const dls::math::vec3f &wsp) const
{
	return (wsp - etendue().min) / etendue().taille();
}

dls::math::vec3f BaseGrille::monde_vers_continu(const dls::math::vec3f &wsp) const
{
	return monde_vers_unit(wsp) * dls::math::vec3f(
				static_cast<float>(m_desc.resolution.x),
				static_cast<float>(m_desc.resolution.y),
				static_cast<float>(m_desc.resolution.z));
}

dls::math::vec3i BaseGrille::monde_vers_index(const dls::math::vec3f &wsp) const
{
	auto mnd = monde_vers_continu(wsp);
	return dls::math::vec3i(
				static_cast<int>(mnd.x),
				static_cast<int>(mnd.y),
				static_cast<int>(mnd.z));
}

dls::math::vec3f BaseGrille::continu_vers_monde(const dls::math::vec3f &csp) const
{
	return (csp / etendue().taille()) + etendue().min;
}

const description_volume &BaseGrille::desc() const
{
	return m_desc;
}

dls::math::vec3i BaseGrille::resolution() const
{
	return m_desc.resolution;
}

const limites3f &BaseGrille::etendue() const
{
	return m_desc.etendue;
}

const limites3f &BaseGrille::fenetre_donnees() const
{
	return m_desc.fenetre_donnees;
}

float BaseGrille::taille_voxel() const
{
	return m_desc.taille_voxel;
}

/* ************************************************************************** */

Volume::~Volume()
{
	if (grille == nullptr) {
		return;
	}

	switch (grille->type()) {
		case type_volume::SCALAIRE:
		{
			if (!grille->est_eparse()) {
				auto ptr = dynamic_cast<Grille<float> *>(grille);
				memoire::deloge("grille", ptr);
			}
			else {
				auto ptr = dynamic_cast<grille_eparse<float> *>(grille);
				memoire::deloge("grille_eparse", ptr);
			}

			break;
		}
		case type_volume::VECTOR:
		{
			if (!grille->est_eparse()) {
				auto ptr = dynamic_cast<Grille<dls::math::vec3f> *>(grille);
				memoire::deloge("grille", ptr);
			}
			else {
				auto ptr = dynamic_cast<grille_eparse<dls::math::vec3f> *>(grille);
				memoire::deloge("grille_eparse", ptr);
			}

			break;
		}
	}
}
