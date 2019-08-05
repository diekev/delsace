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

#include "volume.h"

#include "biblinternes/math/bruit.hh"
#include "biblinternes/outils/gna.hh"

#include "bsdf.h"
#include "koudou.h"
#include "nuanceur.h"
#include "structure_acceleration.h"
#include "types.h"

namespace kdo {

/* ************************************************************************** */

Volume::Volume(ContexteNuancage &ctx)
	: contexte(ctx)
{}

/* ************************************************************************** */

VolumeLoiBeers::VolumeLoiBeers(ContexteNuancage &ctx)
	: Volume(ctx)
{}

bool VolumeLoiBeers::integre(GNA &gna, ParametresRendu const &parametres, Spectre &L, Spectre &tr, Spectre &poids, dls::phys::rayond &wo)
{
	auto rayon_local = contexte.rayon;
	rayon_local.origine = contexte.P;

	auto isect = parametres.acceleratrice->entresecte(parametres.scene, rayon_local, 1000.0);

	if (isect.type == ESECT_OBJET_TYPE_AUCUN) {
		return false;
	}

	auto P = rayon_local.origine + isect.distance * rayon_local.direction;
	L = Spectre(1.0);
	tr = transmittance(gna, parametres, P, contexte.P);
	poids = Spectre(1.0);
	wo.origine = P;
	wo.direction = contexte.rayon.direction;

	return true;
}

Spectre VolumeLoiBeers::transmittance(GNA &gna, ParametresRendu const &/*parametres*/, dls::math::point3d const &P0, dls::math::point3d const &P1)
{
	auto const distance = static_cast<float>(longueur(P0 - P1));

	auto L = Spectre();
	L[0] = std::exp(absorption[0] * -distance);
	L[1] = std::exp(absorption[1] * -distance);
	L[2] = std::exp(absorption[2] * -distance);

	return L;
}

/* ************************************************************************** */

VolumeHeterogeneLoiBeers::VolumeHeterogeneLoiBeers(ContexteNuancage &ctx)
	: Volume(ctx)
{}

bool VolumeHeterogeneLoiBeers::integre(GNA &gna, ParametresRendu const &parametres, Spectre &L, Spectre &tr, Spectre &poids, dls::phys::rayond &wo)
{
	auto rayon_local = contexte.rayon;
	rayon_local.origine = contexte.P;

	auto isect = parametres.acceleratrice->entresecte(parametres.scene, rayon_local, 1000.0);

	if (isect.type == ESECT_OBJET_TYPE_AUCUN) {
		return false;
	}

	auto P = rayon_local.origine + isect.distance * rayon_local.direction;
	L = Spectre(1.0);
	tr = transmittance(gna, parametres, P, contexte.P);
	poids = Spectre(1.0);
	wo.origine = P;
	wo.direction = contexte.rayon.direction;

	return true;
}

Spectre VolumeHeterogeneLoiBeers::transmittance(GNA &gna, ParametresRendu const &/*parametres*/, dls::math::point3d const &P0, dls::math::point3d const &P1)
{
	auto const distance = longueur(P0 - P1);
	auto const dir = (P1 - P0);

	auto termine = false;
	auto t = 0.0;

	/* À FAIRE. */
	auto bruit = dls::math::BruitPerlin3D();

	do {
		auto zeta = gna.uniforme(0.0, 1.0);
		t = t - std::log(1.0 - zeta) / absorption_max;

		if (t > distance) {
			/* Nous n'avons pas terminé dans le volume. */
			break;
		}

		/* Ajourne le contexte de nuançage. */
		auto P = P0 + t * dir;

		/* Calcul l'absorption locale en évaluant le graphe de nuançage. À FAIRE. */
		auto absorp = bruit(static_cast<float>(P.x), static_cast<float>(P.y), static_cast<float>(P.z));

		auto xi = gna.uniforme(0.0, 1.0);

		if (xi < (static_cast<double>(absorp) / absorption_max)) {
			termine = true;
		}
	} while (termine == false);

	if (termine) {
		return Spectre(0.0);
	}

	return Spectre(1.0);
}

/* ************************************************************************** */

VolumeDiffusionSimple::VolumeDiffusionSimple(ContexteNuancage &ctx)
	: Volume(ctx)
{

}

bool VolumeDiffusionSimple::integre(
		GNA &gna,
		ParametresRendu const &parametres,
		Spectre &L,
		Spectre &tr,
		Spectre &poids,
		dls::phys::rayond &wo)
{
	auto rayon_local = contexte.rayon;
	rayon_local.origine = contexte.P;

	auto isect = parametres.acceleratrice->entresecte(parametres.scene, rayon_local, 1000.0);

	if (isect.type == ESECT_OBJET_TYPE_AUCUN) {
		return false;
	}

	auto P = rayon_local.origine + isect.distance * rayon_local.direction;

	/* Transmittance sur tout l'entrevalle. */
	tr = transmittance(gna, parametres, P, contexte.P);

	/* Calcul la position de l'échantillon de diffusion, basé sur la PDF
	 * normalisée selon la transmittance. */
	auto xi = gna.uniforme(0.0, 1.0);
	auto distance_diffusion = -std::log(1.0f - static_cast<float>(xi) * (1.0f - tr.y())) / extinction.y();

	/* Initialise le nuançage au point de diffusion. */
	auto P_diffusion = contexte.P + static_cast<double>(distance_diffusion) * contexte.rayon.direction;
	contexte.P = P_diffusion;

	/* Calcul l'éclairage direct avec l'échantillonage de la lumière et du BSDF */
	BSDFPhaseIsotropique bsdf_phase(contexte);
	L = Spectre(0.0);
	auto L_bsdf = Spectre(0.0);
	auto pdf_bsdf = 0.0;
	auto dir_echantillon = dls::math::vec3d(0.0);

	auto pdf_lumiere = 1.0; // À FAIRE
	auto transmittance_rayon = 1.0; //  À FAIRE
	auto poids_MIS = 1.0; //  À FAIRE
	auto L_lumiere = spectre_lumiere(parametres, parametres.scene, gna, P_diffusion, contexte.N);

	bsdf_phase.evalue_echantillon(gna, parametres, dir_echantillon, L_bsdf, pdf_bsdf);

	L += L_lumiere * L_bsdf * static_cast<float>(transmittance_rayon * poids_MIS / pdf_lumiere);

	auto trd = Spectre::depuis_rgb(
				   std::exp(extinction[0] * -distance_diffusion),
				   std::exp(extinction[1] * -distance_diffusion),
				   std::exp(extinction[2] * -distance_diffusion));

	L *= (extinction * albedo_diffusion * trd);

	/* Le poids est de 1.0 / la PDF normalisée à la transmission totale */
	poids = (Spectre(1.0) - tr) / (trd * extinction);

	wo.origine = P;
	wo.direction = contexte.rayon.direction;

	return true;
}

Spectre VolumeDiffusionSimple::transmittance(
		GNA &gna,
		ParametresRendu const &parametres,
		dls::math::point3d const &P0,
		dls::math::point3d const &P1)
{
	auto const distance = static_cast<float>(longueur(P0 - P1));

	return Spectre::depuis_rgb(
				 std::exp(extinction[0] * -distance),
				 std::exp(extinction[1] * -distance),
				 std::exp(extinction[2] * -distance)
			);
}

/* ************************************************************************** */

VolumeHeterogeneDiffusionSimple::VolumeHeterogeneDiffusionSimple(ContexteNuancage &ctx)
	: Volume(ctx)
{
}

bool VolumeHeterogeneDiffusionSimple::integre(
		GNA &gna,
		ParametresRendu const &parametres,
		Spectre &L,
		Spectre &tr,
		Spectre &poids,
		dls::phys::rayond &wo)
{
	auto rayon_local = contexte.rayon;
	rayon_local.origine = contexte.P;

	auto isect = parametres.acceleratrice->entresecte(parametres.scene, rayon_local, 1000.0);

	if (isect.type == ESECT_OBJET_TYPE_AUCUN) {
		return false;
	}

	auto P = rayon_local.origine + isect.distance * rayon_local.direction;

	auto distance = longueur(P - contexte.P);
	auto termine = false;
	auto t = 0.0;

	/* À FAIRE. */
	auto bruit = dls::math::BruitPerlin3D();

	do {
		auto zeta = gna.uniforme(0.0, 1.0);
		t = t - std::log(1.0 - zeta) / extinction_max;

		if (t > distance) {
			// Nous n'avons pas terminé dans le volume.
			break;
		}

		// Ajourne le contexte de nuançage.
		auto nP = contexte.P + t * contexte.rayon.direction;
		contexte.P = nP;

		/* Calcul l'absorption locale en évaluant le graphe de nuançage. À FAIRE. */
		auto extinct = Spectre(bruit(static_cast<float>(nP.x), static_cast<float>(nP.y), static_cast<float>(nP.z)));

		auto xi = gna.uniforme(0.0, 1.0);

		if (static_cast<float>(xi) < (extinct.y() / static_cast<float>(extinction_max))) {
			termine = true;
		}

	} while (termine == false);

	if (termine) {
		// The shading context has already been advanced to the
		// scatter location. Compute direct lighting after
		// evaluating the local scattering albedo and extinction
		auto albedo_diffusion = Spectre(1.0); // À FAIRE : paramètre UI/texture
		auto extinction = Spectre(1.0); // À FAIRE : paramètre UI/texture

		BSDFPhaseAnisotropique bsdf_phase(contexte);
		L = Spectre(0.0);
		Spectre L_bsdf, beamTransmittance;
		dls::math::vec3d dir_echantillon;
		auto pdf_bsdf = 0.0;
		auto pdf_lumiere = 1.0; // À FAIRE
		auto transmittance_rayon = 1.0; //  À FAIRE
		auto poids_MIS = 1.0; //  À FAIRE
		auto L_lumiere = spectre_lumiere(parametres, parametres.scene, gna, contexte.P, contexte.N);

		bsdf_phase.evalue_echantillon(gna, parametres, dir_echantillon, L_bsdf, pdf_bsdf);

		L += L_lumiere * L_bsdf * static_cast<float>(transmittance_rayon * poids_MIS / pdf_lumiere);

		bsdf_phase.genere_echantillon(gna, parametres, dir_echantillon, L_bsdf, pdf_bsdf, 1);
		//rs.EvaluateLightSample(m_ctx, sampleDirection, L_lumiere, pdf_lumiere, beamTransmittance);

		L += L_lumiere * L_bsdf * beamTransmittance * albedo_diffusion * extinction * static_cast<float>(poids_MIS);

		tr = Spectre(0.0f);
		Spectre pdf = extinction; // Should be extinction * Tr, but Tr is 1
		poids = 1.0 / pdf;
	}
	else {
		tr = Spectre(1.0);
		poids = Spectre(1.0);
	}

	wo.origine = P;
	wo.direction = contexte.rayon.direction;

	return true;
}

Spectre VolumeHeterogeneDiffusionSimple::transmittance(
		GNA &gna,
		ParametresRendu const &parametres,
		dls::math::point3d const &P0,
		dls::math::point3d const &P1)
{
	auto const distance = static_cast<float>(longueur(P0 - P1));

	return Spectre::depuis_rgb(
				 std::exp(-distance),
				 std::exp(-distance),
				 std::exp(-distance)
			);
}

}  /* namespace kdo */
