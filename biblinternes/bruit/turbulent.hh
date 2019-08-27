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

#pragma once

#include "parametres.hh"

namespace bruit {

struct param_turbulence {
	float echelle = 1.0f;
	float inv_echelle = 1.0f / echelle;
	float octaves = 8.0f * 1.618f;
	float gain = 1.0f;
	float lacunarite = 1.0f;
	float amplitude = 1.0f;
	bool dur = false;
};

template <typename bruit_base>
struct turbulent {
	static void construit(int graine, parametres &params, param_turbulence const &/*params_turb*/)
	{
		bruit_base::construit(params, graine);
	}

	static float evalue(parametres const &params, param_turbulence const &params_turb, dls::math::vec3f pos)
	{
		/* mise à l'échelle du point d'échantillonage */
		auto p = pos;
		p += params.decalage_graine;
		p *= params.echelle_pos;
		p += params.decalage_pos;

		/* initialisation des variables */
		auto resultat = 0.0f;
		auto contribution_octave = params_turb.amplitude;
		auto octave = params_turb.octaves;

		for (; octave > 1.0f; octave -= 1.0f) {
			auto b = (bruit_base::evalue(params, p) * 0.5f + 0.5f);

			if (params_turb.dur) {
				b = std::abs(2.0f * b - 1.0f);
			}

			resultat += b * contribution_octave;
			contribution_octave *= params_turb.gain;
			p *= params_turb.lacunarite;
		}

		if (octave > 0.0f) {
			auto b = (bruit_base::evalue(params, p) * 0.5f + 0.5f);

			if (params_turb.dur) {
				b = std::abs(2.0f * b - 1.0f);
			}

			resultat += b * contribution_octave * octave;
		}

		auto nom = 1 << static_cast<int>(params_turb.octaves);
		auto denom = (1 << (static_cast<int>(params_turb.octaves) + 1)) - 1;

		resultat *= static_cast<float>(nom) / static_cast<float>(denom);

		return resultat;
	}

	static inline dls::math::vec2f limites()
	{
		return bruit_base::limites();
	}
};

}  /* namespace bruit */
