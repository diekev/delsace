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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/phys/spectre.hh"

#include "types.h"

struct ContexteNuancage;

class GNA;
class ParametresRendu;

struct Volume {
	ContexteNuancage &contexte;

	explicit Volume(ContexteNuancage &ctx);

	virtual ~Volume() = default;

	virtual bool integre(GNA &gna, ParametresRendu const &parametres, Spectre &L, Spectre &tr, Spectre &poids, Rayon &wo) = 0;

	virtual Spectre transmittance(GNA &gna, ParametresRendu const &parametres, dls::math::point3d const &P0, dls::math::point3d const &P1) = 0;
};

struct VolumeLoiBeers : public Volume {
	Spectre absorption = Spectre(1.0);

	explicit VolumeLoiBeers(ContexteNuancage &ctx);

	bool integre(GNA &gna, ParametresRendu const &parametres, Spectre &L, Spectre &tr, Spectre &poids, Rayon &wo) override;

	Spectre transmittance(GNA &gna, ParametresRendu const &parametres, dls::math::point3d const &P0, dls::math::point3d const &P1) override;
};

struct VolumeHeterogeneLoiBeers : public Volume {
	Spectre absorption = Spectre(1.0);
	double absorption_max = 1.0;

	explicit VolumeHeterogeneLoiBeers(ContexteNuancage &ctx);

	bool integre(GNA &gna, ParametresRendu const &parametres, Spectre &L, Spectre &tr, Spectre &poids, Rayon &wo) override;

	Spectre transmittance(GNA &gna, ParametresRendu const &parametres, dls::math::point3d const &P0, dls::math::point3d const &P1) override;
};

struct VolumeDiffusionSimple : public Volume {
	Spectre extinction = Spectre(0.5);
	Spectre albedo_diffusion = Spectre(0.5);

	explicit VolumeDiffusionSimple(ContexteNuancage &ctx);

	bool integre(GNA &gna, ParametresRendu const &parametres, Spectre &L, Spectre &tr, Spectre &poids, Rayon &wo) override;

	Spectre transmittance(GNA &gna, ParametresRendu const &parametres, dls::math::point3d const &P0, dls::math::point3d const &P1) override;
};

struct VolumeHeterogeneDiffusionSimple : public Volume {
	double extinction_max = 1.0;

	explicit VolumeHeterogeneDiffusionSimple(ContexteNuancage &ctx);

	bool integre(GNA &gna, ParametresRendu const &parametres, Spectre &L, Spectre &tr, Spectre &poids, Rayon &wo) override;

	Spectre transmittance(GNA &gna, ParametresRendu const &parametres, dls::math::point3d const &P0, dls::math::point3d const &P1) override;
};
