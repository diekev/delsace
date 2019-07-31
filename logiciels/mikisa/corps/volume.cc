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
	return static_cast<long>(x + (y + z * static_cast<size_t>(m_res[1])) * static_cast<size_t>(m_res[0]));
}

long BaseGrille::nombre_voxels() const
{
	return static_cast<long>(m_nombre_voxels);
}

bool BaseGrille::hors_des_limites(size_t x, size_t y, size_t z) const
{
	if (x >= static_cast<size_t>(m_res[0])) {
		return true;
	}

	if (y >= static_cast<size_t>(m_res[1])) {
		return true;
	}

	if (z >= static_cast<size_t>(m_res[2])) {
		return true;
	}

	return false;
}

BaseGrille::BaseGrille(const limites3f &etendu, const limites3f &fenetre_donnees, float taille_voxel)
	: m_taille_voxel(taille_voxel)
	, m_etendu(etendu)
	, m_fenetre_donnees(fenetre_donnees)
{
	auto taille = etendu.taille();
	m_res[0] = static_cast<int>(taille.x / taille_voxel);
	m_res[1] = static_cast<int>(taille.y / taille_voxel);
	m_res[2] = static_cast<int>(taille.z / taille_voxel);

	m_nombre_voxels = static_cast<size_t>(m_res[0]) * static_cast<size_t>(m_res[1]) * static_cast<size_t>(m_res[2]);
}

dls::math::vec3f BaseGrille::index_vers_unit(const dls::math::vec3i &vsp) const
{
	auto p = dls::math::discret_vers_continue<float>(vsp);
	return index_vers_unit(p);
}

dls::math::vec3f BaseGrille::index_vers_unit(const dls::math::vec3f &vsp) const
{
	return dls::math::vec3f(
				vsp.x / static_cast<float>(m_res.x),
				vsp.y / static_cast<float>(m_res.y),
				vsp.z / static_cast<float>(m_res.z));
}

dls::math::vec3f BaseGrille::index_vers_monde(const dls::math::vec3i &isp) const
{
	auto const dim = etendu().taille();
	auto const min = etendu().min;
	auto const cont = dls::math::discret_vers_continue<float>(isp);
	return dls::math::vec3f(
				cont.x / static_cast<float>(m_res.x) * dim.x + min.x,
				cont.y / static_cast<float>(m_res.y) * dim.y + min.y,
				cont.z / static_cast<float>(m_res.z) * dim.z + min.z);
}

dls::math::vec3f BaseGrille::unit_vers_monde(const dls::math::vec3f &vsp) const
{
	return vsp * etendu().taille() + etendu().min;
}

dls::math::vec3f BaseGrille::monde_vers_unit(const dls::math::vec3f &wsp) const
{
	return (wsp - etendu().min) / etendu().taille();
}

dls::math::vec3f BaseGrille::monde_vers_continue(const dls::math::vec3f &wsp) const
{
	return monde_vers_unit(wsp) * dls::math::vec3f(
				static_cast<float>(m_res.x),
				static_cast<float>(m_res.y),
				static_cast<float>(m_res.z));
}

dls::math::vec3i BaseGrille::resolution() const
{
	return m_res;
}

const limites3f &BaseGrille::etendu() const
{
	return m_etendu;
}

const limites3f &BaseGrille::fenetre_donnees() const
{
	return m_fenetre_donnees;
}

float BaseGrille::taille_voxel() const
{
	return m_taille_voxel;
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
