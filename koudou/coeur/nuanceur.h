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

#include "bibliotheques/spectre/spectre.h"

#include "types.h"

struct BSDF;
struct Volume;

struct ContexteNuancage {
	Rayon rayon;
	numero7::math::point3d P;
	numero7::math::vec3d N;
	numero7::math::vec3d V;
};

enum class TypeNuanceur {
	DIFFUS,
	EMISSION,
	ANGLE_VUE,
	REFLECTION,
	REFRACTION,
	VOLUME,
};

struct Nuanceur {
	TypeNuanceur type;

	virtual ~Nuanceur() = default;

	virtual BSDF *cree_BSDF(ContexteNuancage &ctx);

	virtual bool a_volume() const;

	virtual Volume *cree_volume(ContexteNuancage &ctx);
};

struct NuanceurAngleVue : public Nuanceur {
	Spectre spectre = Spectre(0.8);

	NuanceurAngleVue()
	{
		type = TypeNuanceur::ANGLE_VUE;
	}

	static Nuanceur *defaut();

	BSDF *cree_BSDF(ContexteNuancage &ctx) override;
};

struct NuanceurDiffus : public Nuanceur {
	Spectre spectre = Spectre(0.8);

	NuanceurDiffus()
	{
		type = TypeNuanceur::DIFFUS;
	}

	static Nuanceur *defaut();

	BSDF *cree_BSDF(ContexteNuancage &ctx) override;
};

struct NuanceurReflection : public Nuanceur {
	NuanceurReflection()
	{
		type = TypeNuanceur::REFLECTION;
	}

	static Nuanceur *defaut();

	BSDF *cree_BSDF(ContexteNuancage &ctx) override;
};

struct NuanceurRefraction : public Nuanceur {
	double index_refraction = 1.3;

	NuanceurRefraction()
	{
		type = TypeNuanceur::REFRACTION;
	}

	static Nuanceur *defaut();

	BSDF *cree_BSDF(ContexteNuancage &ctx) override;
};

struct NuanceurVolume : public Nuanceur {
	double densite = 1.0;
	Spectre sigma_a = Spectre(1.0);
	Spectre sigma_s = Spectre(1.0);

	NuanceurVolume()
	{
		type = TypeNuanceur::VOLUME;
	}

	static Nuanceur *defaut();

	BSDF *cree_BSDF(ContexteNuancage &ctx) override;

	Volume *cree_volume(ContexteNuancage &ctx) override;

	bool a_volume() const override;
};

struct NuanceurEmission : public Nuanceur {
	Spectre spectre = Spectre(1.0);
	double exposition = 1.0;

	NuanceurEmission();

	static Nuanceur *defaut();

	BSDF *cree_BSDF(ContexteNuancage &ctx) override;
};
