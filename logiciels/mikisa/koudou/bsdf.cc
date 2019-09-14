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

#include "bsdf.hh"

#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/phys/rayon.hh"

#include "koudou.hh"
#include "moteur_rendu.hh"
#include "nuanceur.hh"
#include "scene.hh"
#include "structure_acceleration.hh"
#include "types.hh"

namespace kdo {

/* ************************************************************************** */

BSDF::BSDF(ContexteNuancage &ctx)
	: contexte(ctx)
{}

/* ************************************************************************** */

BSDFTrivial::BSDFTrivial(ContexteNuancage &ctx)
	: BSDF(ctx)
{}

void BSDFTrivial::evalue_echantillon(GNA &gna, ParametresRendu const &parametres, dls::math::vec3d const &dir, Spectre &L, double &pdf)
{
	INUTILISE(gna);
	INUTILISE(parametres);
	INUTILISE(dir);

	pdf = 0.0;
	L = Spectre(0.0);
}

void BSDFTrivial::genere_echantillon(GNA &gna, ParametresRendu const &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, uint profondeur)
{
	INUTILISE(gna);
	INUTILISE(parametres);
	INUTILISE(profondeur);

	dir = -contexte.V;
	pdf = 1.0;
	L = Spectre(1.0);
}

/* ************************************************************************** */

BSDFAngleVue::BSDFAngleVue(ContexteNuancage &ctx)
	: BSDF(ctx)
{}

void BSDFAngleVue::evalue_echantillon(GNA &gna, ParametresRendu const &parametres, dls::math::vec3d const &dir, Spectre &L, double &pdf)
{
	INUTILISE(gna);
	INUTILISE(parametres);
	INUTILISE(dir);

	pdf = 0.0;
	L = Spectre(0.0);
}

void BSDFAngleVue::genere_echantillon(GNA &gna, ParametresRendu const &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, uint profondeur)
{
	INUTILISE(profondeur);

	dir = get_brdf_ray(gna, contexte.N, -contexte.V);
	pdf = constantes<double>::PI;

	auto couleur = std::max(0.0, produit_scalaire(contexte.N, contexte.V));
	L = Spectre(couleur) * spectre_lumiere(parametres, parametres.scene, gna, contexte.P, contexte.N);
}

/* ************************************************************************** */

BSDFDiffus::BSDFDiffus(ContexteNuancage &ctx, Spectre s)
	: BSDF(ctx)
	, spectre(s)
{}

void BSDFDiffus::evalue_echantillon(GNA &gna, ParametresRendu const &parametres, dls::math::vec3d const &dir, Spectre &L, double &pdf)
{
	INUTILISE(gna);
	INUTILISE(parametres);
	INUTILISE(dir);

	pdf = 0.0;
	L = Spectre(0.0);
}

void BSDFDiffus::genere_echantillon(GNA &gna, ParametresRendu const &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, uint profondeur)
{
	INUTILISE(profondeur);

	dir = get_brdf_ray(gna, contexte.N, -contexte.V);
	pdf = constantes<double>::PI;
	L = spectre * spectre_lumiere(parametres, parametres.scene, gna, contexte.P, contexte.N);
}

/* ************************************************************************** */

BSDFReflectance::BSDFReflectance(ContexteNuancage &ctx)
	: BSDF(ctx)
{}

void BSDFReflectance::evalue_echantillon(GNA &gna, ParametresRendu const &parametres, dls::math::vec3d const &dir, Spectre &L, double &pdf)
{
	INUTILISE(gna);
	INUTILISE(parametres);
	INUTILISE(dir);

	pdf = 0.0;
	L = Spectre(0.0);
}

void BSDFReflectance::genere_echantillon(GNA &gna, ParametresRendu const &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, uint profondeur)
{
	dir = -contexte.V;
	pdf = 1.0;

	auto const &rayon = contexte.rayon;
	auto const &R = reflechi(rayon.direction, contexte.N);

	dls::phys::rayond rayon_local;
	rayon_local.direction = R;
	rayon_local.origine = contexte.P + contexte.N * 1e-4;
	rayon_local.distance_min = rayon.distance_min;
	rayon_local.distance_max = rayon.distance_max;
	calcul_direction_inverse(rayon_local);

	L = 0.8f * calcul_spectre(gna, parametres, rayon_local, profondeur + 1);
}

/* ************************************************************************** */

#undef ROULETTE_RUSSE

BSDFVerre::BSDFVerre(ContexteNuancage &ctx)
	: BSDF(ctx)
{}

void BSDFVerre::evalue_echantillon(GNA &gna, ParametresRendu const &parametres, dls::math::vec3d const &dir, Spectre &L, double &pdf)
{
	INUTILISE(gna);
	INUTILISE(parametres);
	INUTILISE(dir);
	pdf = 0.0;
	L = Spectre(0.0);
}

void BSDFVerre::genere_echantillon(GNA &gna, ParametresRendu const &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, uint profondeur)
{
	dir = get_brdf_ray(gna, contexte.N, -contexte.V);
	pdf = 1.0;

	auto const &rayon = contexte.rayon;

#ifdef ROULETTE_RUSSE
	/* Calcul le fresnel. */
	auto kr = fresnel(rayon.direction, contexte.N, index_refraction);
	auto outside = produit_scalaire(rayon.direction, contexte.N) < 0;
	auto bias = outside ? -1e-4 * contexte.N : 1e-4 * contexte.N;

	dls::phys::rayond rayon_local;
	rayon_local.origine = contexte.P + bias;
	rayon_local.distance_min = rayon.distance_min;
	rayon_local.distance_max = rayon.distance_max;

	/* Calcul la réfraction s'il n'y a pas un cas de réflection interne totale. */
	if (kr <= gna.nombre_aleatoire()) {
		rayon_local.direction = normalise(refracte(rayon.direction, contexte.N, index_refraction));
		calcul_direction_inverse(rayon_local);

		L = calcul_spectre(gna, parametres, rayon_local, profondeur + 1);
		return;
	}

	rayon_local.direction = normalise(reflechi(rayon.direction, contexte.N));
	calcul_direction_inverse(rayon_local);

	L = calcul_spectre(gna, parametres, rayon_local, profondeur + 1);
#else
	auto refractionColor = Spectre(0.0f);

	/* Calcul le fresnel. */
	auto kr = fresnel(rayon.direction, contexte.N, index_refraction);
	auto outside = produit_scalaire(rayon.direction, contexte.N) < 0;
	auto bias = outside ? -1e-4 * contexte.N : 1e-4 * contexte.N;

	/* Calcul la réfraction s'il n'y a pas un cas de réflection interne totale. */
	if (kr < 1) {
		auto refractionDirection = normalise(refracte(rayon.direction, contexte.N, index_refraction));
		auto refractionRayOrig = contexte.P + bias;

		dls::phys::rayond rayon_local;
		rayon_local.direction = refractionDirection;
		rayon_local.origine = refractionRayOrig;
		rayon_local.distance_min = rayon.distance_min;
		rayon_local.distance_max = rayon.distance_max;
		calcul_direction_inverse(rayon_local);

		refractionColor = calcul_spectre(gna, parametres, rayon_local, profondeur + 1);
	}

	dls::phys::rayond rayon_local;
	rayon_local.direction = normalise(reflechi(rayon.direction, contexte.N));
	rayon_local.origine = contexte.P + bias;
	rayon_local.distance_min = rayon.distance_min;
	rayon_local.distance_max = rayon.distance_max;
	calcul_direction_inverse(rayon_local);

	auto reflectionColor = calcul_spectre(gna, parametres, rayon_local, profondeur + 1);

	/* Mélange les deux. */
	L = reflectionColor * static_cast<float>(kr) + refractionColor * static_cast<float>(1.0 - kr);
#endif
}

/* ************************************************************************** */

BSDFVolume::BSDFVolume(ContexteNuancage &ctx)
	: BSDF(ctx)
{}

void BSDFVolume::evalue_echantillon(GNA &gna, ParametresRendu const &parametres, dls::math::vec3d const &dir, Spectre &L, double &pdf)
{
	INUTILISE(gna);
	INUTILISE(parametres);
	INUTILISE(dir);
	pdf = 0.0;
	L = Spectre(0.0);
}

void BSDFVolume::genere_echantillon(GNA &gna, ParametresRendu const &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, uint profondeur)
{
	INUTILISE(gna);
	INUTILISE(profondeur);

	dir = -contexte.V;
	pdf = 1.0;

	/* Lance un rayon pour définir le point de sortie du volume. */
	auto rayon_local = contexte.rayon;
	rayon_local.origine = contexte.P;

	auto isect = parametres.scene.traverse(rayon_local);

	if (isect.type == ESECT_OBJET_TYPE_AUCUN) {
		L = Spectre(0.0);
		return;
	}

	auto P = rayon_local.origine + isect.distance * rayon_local.direction;
	auto taille = longueur(P - contexte.P);

	auto n = static_cast<size_t>(taille / 0.001);
	auto accumulation = Spectre(1.0);

	while (n--) {
		accumulation[0] *= static_cast<float>(std::exp(-densite * static_cast<double>(sigma_a[0] + sigma_s[0]) * 0.001));
		accumulation[1] *= static_cast<float>(std::exp(-densite * static_cast<double>(sigma_a[1] + sigma_s[1]) * 0.001));
		accumulation[2] *= static_cast<float>(std::exp(-densite * static_cast<double>(sigma_a[2] + sigma_s[2]) * 0.001));
	}

	L = accumulation;

	/* Met à jour le point d'où doit partir le rayon après l'évaluation. */
	contexte.P = P;
}

/* ************************************************************************** */

BSDFPhaseIsotropique::BSDFPhaseIsotropique(ContexteNuancage &ctx)
	: BSDF(ctx)
{}

void BSDFPhaseIsotropique::evalue_echantillon(GNA &gna, ParametresRendu const &parametres, dls::math::vec3d const &dir, Spectre &L, double &pdf)
{
	INUTILISE(gna);
	INUTILISE(parametres);
	INUTILISE(dir);

	pdf = 0.25 * constantes<double>::PI_INV;
	L = Spectre(pdf);
}

void BSDFPhaseIsotropique::genere_echantillon(GNA &gna, ParametresRendu const &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, uint profondeur)
{
	INUTILISE(parametres);
	INUTILISE(profondeur);

	auto xi = gna.uniforme(0.0, 1.0);
	dir.z = xi * 2.0 - 1.0; // cos_theta;

	auto sin_theta = 1.0 - dir.z * dir.z; // carré de sin_theta

	if (sin_theta > 1.0) {
		sin_theta = std::sqrt(sin_theta);
		xi = gna.uniforme(0.0, 1.0);

		auto phi = xi * constantes<double>::TAU;
		dir.x = sin_theta * std::cos(phi);
		dir.y = sin_theta * std::sin(phi);
	}
	else {
		dir.x = 0.0;
		dir.y = 0.0;
	}

	pdf = 0.25 * constantes<double>::PI_INV;
	L = Spectre(pdf);
}

/* ************************************************************************** */

BSDFPhaseAnisotropique::BSDFPhaseAnisotropique(ContexteNuancage &ctx, double g_)
	: BSDF(ctx)
	, g(g_)
{
	if (g < -1.0) {
		g = -1.0;
	}
	if (g > 1.0) {
		g = 1.0;
	}

	isotropique = std::abs(g) < 0.00001;

	if (isotropique == false) {
		un_plus_g2 = 1.0 + g * g;
		un_moins_g2 = 1.0 - g * g;
		un_sur_2g = 0.5 / g;
	}
}

void BSDFPhaseAnisotropique::evalue_echantillon(GNA &gna, ParametresRendu const &parametres, dls::math::vec3d const &dir, Spectre &L, double &pdf)
{
	INUTILISE(gna);
	INUTILISE(parametres);
	INUTILISE(dir);

	if (isotropique) {
		pdf = 0.25 * constantes<double>::PI_INV;
	}
	else {
		auto cos_theta = produit_scalaire(-contexte.V, dir);
		pdf = calcul_pdf(cos_theta);
	}

	L = Spectre(pdf);
}

void BSDFPhaseAnisotropique::genere_echantillon(GNA &gna, ParametresRendu const &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, uint profondeur)
{
	INUTILISE(parametres);
	INUTILISE(profondeur);

	if (isotropique) {
		/* Pareil que BSDFPhaseIsotropique::genere_echantillon. */
		auto xi = gna.uniforme(0.0, 1.0);
		dir.z = xi * 2.0 - 1.0; // cos_theta;

		auto sin_theta = 1.0 - dir.z * dir.z; // carré de sin_theta

		if (sin_theta > 1.0) {
			sin_theta = std::sqrt(sin_theta);
			xi = gna.uniforme(0.0, 1.0);

			auto phi = xi * constantes<double>::TAU;
			dir.x = sin_theta * std::cos(phi);
			dir.y = sin_theta * std::sin(phi);
		}
		else {
			dir.x = 0.0;
			dir.y = 0.0;
		}

		pdf = 0.25 * constantes<double>::PI_INV;
	}
	else {
		auto phi = gna.uniforme(0.0, 1.0) * constantes<double>::TAU;
		auto cos_theta = inverse_cdf(gna.uniforme(0.0, 1.0));
		auto sin_theta = std::sqrt(1.0 - cos_theta * cos_theta); // carré de sin_theta
		auto in = -contexte.V;

		dls::math::vec3d t0, t1;
		cree_base_orthonormale(in, t0, t1);

		dir = sin_theta * std::sin(phi) * t0 + sin_theta * std::cos(phi) * t1 + cos_theta * in;
		pdf = calcul_pdf(cos_theta);
	}

	L = Spectre(pdf);
}

double BSDFPhaseAnisotropique::calcul_pdf(double cos_theta) const
{
	return 0.25 * un_moins_g2 / (constantes<double>::PI * std::pow(un_plus_g2 - 2.0 * g * cos_theta, 1.5f));
}

double BSDFPhaseAnisotropique::inverse_cdf(double xi) const
{
	auto const t = (un_moins_g2) / (1.0 - g + 2.0 * g * xi);
	return un_sur_2g * (un_plus_g2 - t * t);
}

}  /* namespace kdo */
