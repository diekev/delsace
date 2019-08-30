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

#include "blanc.hh"

#include "biblinternes/math/entrepolation.hh"
#include "biblinternes/outils/empreintes.hh"

#include "outils.hh"

namespace bruit {

blanc blanc::construit(int graine)
{
	auto b = blanc();
	b.m_graine = graine;
	return b;
}

float blanc::evalue(dls::math::vec3f pos) const
{
	pos += static_cast<float>(m_graine);
	return empreinte_r32_vers_r32(pos.x, pos.y, pos.z);
}

/* ************************************************************************** */

static auto bruit_valeur(dls::math::vec3f const &pos, dls::math::vec3f *derivee = nullptr)
{
	/* trouve le cube qui contient le point */
	auto const floorx = std::floor(pos.x);
	auto const floory = std::floor(pos.y);
	auto const floorz = std::floor(pos.z);

	/* trouve les coordonnées relative des points dans le cube */
	auto const fx = pos.x - floorx;
	auto const fy = pos.y - floory;
	auto const fz = pos.z - floorz;

	auto const ux = dls::math::entrepolation_fluide<2>(fx);
	auto const uy = dls::math::entrepolation_fluide<2>(fy);
	auto const uz = dls::math::entrepolation_fluide<2>(fz);

	auto const a = empreinte_r32_vers_r32(floorx       , floory       , floorz);
	auto const b = empreinte_r32_vers_r32(floorx + 1.0f, floory       , floorz);
	auto const c = empreinte_r32_vers_r32(floorx       , floory + 1.0f, floorz);
	auto const d = empreinte_r32_vers_r32(floorx + 1.0f, floory + 1.0f, floorz);
	auto const e = empreinte_r32_vers_r32(floorx       , floory       , floorz + 1.0f);
	auto const f = empreinte_r32_vers_r32(floorx + 1.0f, floory       , floorz + 1.0f);
	auto const g = empreinte_r32_vers_r32(floorx       , floory + 1.0f, floorz + 1.0f);
	auto const h = empreinte_r32_vers_r32(floorx + 1.0f, floory + 1.0f, floorz + 1.0f);

	auto const k0 =   a;
	auto const k1 =   b - a;
	auto const k2 =   c - a;
	auto const k3 =   e - a;
	auto const k4 =   a - b - c + d;
	auto const k5 =   a - c - e + g;
	auto const k6 =   a - b - e + f;
	auto const k7 = - a + b + c - d + e - f - g + h;

	if (derivee != nullptr) {
		auto const dux = dls::math::derivee_fluide<2>(fx);
		auto const duy = dls::math::derivee_fluide<2>(fy);
		auto const duz = dls::math::derivee_fluide<2>(fz);

		derivee->x = 2.0f * dux * (k1 + k4*uy + k6*uz + k7*uy*uz);
		derivee->y = 2.0f * duy * (k2 + k5*uz + k4*ux + k7*uz*ux);
		derivee->z = 2.0f * duz * (k3 + k6*ux + k5*uy + k7*ux*uy);
	}

	return -1.0f + 2.0f * (k0 + k1*ux + k2*uy + k3*uz + k4*ux*uy + k5*uy*uz + k6*uz*ux + k7*ux*uy*uz);
}

void valeur::construit(parametres &params, int graine)
{
	construit_defaut(params, graine);
}

float valeur::evalue(const parametres &params, dls::math::vec3f pos)
{
	INUTILISE(params);
	return bruit_valeur(pos);
}

float valeur::evalue_derivee(const parametres &params, dls::math::vec3f pos, dls::math::vec3f &derivee)
{
	INUTILISE(params);
	return bruit_valeur(pos, &derivee);
}

}  /* namespace bruit */
