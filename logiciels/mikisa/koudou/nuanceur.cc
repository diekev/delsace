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

#include "nuanceur.hh"

#include "bsdf.hh"
#include "volume.hh"

namespace kdo {

/* ************************************************************************** */

BSDF *Nuanceur::cree_BSDF(ContexteNuancage &ctx)
{
	return new BSDFTrivial(ctx);
}

bool Nuanceur::a_volume() const
{
	return false;
}

Volume *Nuanceur::cree_volume(ContexteNuancage &/*ctx*/)
{
	return nullptr;
}

/* ************************************************************************** */

Nuanceur *NuanceurAngleVue::defaut()
{
	return new NuanceurAngleVue();
}

BSDF *NuanceurAngleVue::cree_BSDF(ContexteNuancage &ctx)
{
	return new BSDFAngleVue(ctx);
}

/* ************************************************************************** */

Nuanceur *NuanceurDiffus::defaut()
{
	auto nuanceur = new NuanceurDiffus();
	nuanceur->spectre = Spectre(0.8);
	return nuanceur;
}

BSDF *NuanceurDiffus::cree_BSDF(ContexteNuancage &ctx)
{
	return new BSDFDiffus(ctx, spectre);
}

/* ************************************************************************** */

Nuanceur *NuanceurReflection::defaut()
{
	return new NuanceurReflection();
}

BSDF *NuanceurReflection::cree_BSDF(ContexteNuancage &ctx)
{
	return new BSDFReflectance(ctx);
}

/* ************************************************************************** */

Nuanceur *NuanceurRefraction::defaut()
{
	return new NuanceurRefraction();
}

BSDF *NuanceurRefraction::cree_BSDF(ContexteNuancage &ctx)
{
	return new BSDFVerre(ctx);
}

/* ************************************************************************** */

Nuanceur *NuanceurVolume::defaut()
{
	return new NuanceurVolume();
}

BSDF *NuanceurVolume::cree_BSDF(ContexteNuancage &ctx)
{
	return new BSDFVolume(ctx);
}

Volume *NuanceurVolume::cree_volume(ContexteNuancage &ctx)
{
	return new VolumeHeterogeneDiffusionSimple(ctx);
}

bool NuanceurVolume::a_volume() const
{
	return true;
}

/* ************************************************************************** */

NuanceurEmission::NuanceurEmission()
{
	type = TypeNuanceur::EMISSION;
}

Nuanceur *NuanceurEmission::defaut()
{
	return new NuanceurEmission();
}

BSDF *NuanceurEmission::cree_BSDF(ContexteNuancage &ctx)
{
	return new BSDFTrivial(ctx);
}

}  /* namespace kdo */
