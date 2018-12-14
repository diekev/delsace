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

#include "bsdf.h"

#include "bibliotheques/outils/constantes.h"
#include "bibliotheques/outils/definitions.hh"

#include "gna.h"
#include "koudou.h"
#include "moteur_rendu.h"
#include "nuanceur.h"
#include "scene.h"
#include "structure_acceleration.h"
#include "types.h"

/* ************************************************************************** */

BSDF::BSDF(ContexteNuancage &ctx)
	: contexte(ctx)
{}

/* ************************************************************************** */

BSDFTrivial::BSDFTrivial(ContexteNuancage &ctx)
	: BSDF(ctx)
{}

void BSDFTrivial::evalue_echantillon(GNA &gna, const ParametresRendu &parametres, const dls::math::vec3d &dir, Spectre &L, double &pdf)
{
	INUTILISE(gna);
	INUTILISE(parametres);
	INUTILISE(dir);

	pdf = 0.0;
	L = Spectre(0.0);
}

void BSDFTrivial::genere_echantillon(GNA &gna, const ParametresRendu &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, uint profondeur)
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

void BSDFAngleVue::evalue_echantillon(GNA &gna, const ParametresRendu &parametres, const dls::math::vec3d &dir, Spectre &L, double &pdf)
{
	INUTILISE(gna);
	INUTILISE(parametres);
	INUTILISE(dir);

	pdf = 0.0;
	L = Spectre(0.0);
}

void BSDFAngleVue::genere_echantillon(GNA &gna, const ParametresRendu &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, uint profondeur)
{
	INUTILISE(profondeur);

	dir = get_brdf_ray(gna, contexte.N, -contexte.V);
	pdf = PI;

	auto couleur = std::max(0.0, produit_scalaire(contexte.N, contexte.V));
	L = Spectre(couleur) * spectre_lumiere(parametres, parametres.scene, gna, contexte.P, contexte.N);
}

/* ************************************************************************** */

BSDFDiffus::BSDFDiffus(ContexteNuancage &ctx, Spectre s)
	: BSDF(ctx)
	, spectre(s)
{}

void BSDFDiffus::evalue_echantillon(GNA &gna, const ParametresRendu &parametres, const dls::math::vec3d &dir, Spectre &L, double &pdf)
{
	INUTILISE(gna);
	INUTILISE(parametres);
	INUTILISE(dir);

	pdf = 0.0;
	L = Spectre(0.0);
}

void BSDFDiffus::genere_echantillon(GNA &gna, const ParametresRendu &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, uint profondeur)
{
	INUTILISE(profondeur);

	dir = get_brdf_ray(gna, contexte.N, -contexte.V);
	pdf = PI;
	L = spectre * spectre_lumiere(parametres, parametres.scene, gna, contexte.P, contexte.N);
}

/* ************************************************************************** */

inline auto reflechi(
		const dls::math::vec3d &I,
		const dls::math::vec3d &N)
{
	return I - 2.0 * produit_scalaire(I, N) * N;
}

BSDFReflectance::BSDFReflectance(ContexteNuancage &ctx)
	: BSDF(ctx)
{}

void BSDFReflectance::evalue_echantillon(GNA &gna, const ParametresRendu &parametres, const dls::math::vec3d &dir, Spectre &L, double &pdf)
{
	INUTILISE(gna);
	INUTILISE(parametres);
	INUTILISE(dir);

	pdf = 0.0;
	L = Spectre(0.0);
}

void BSDFReflectance::genere_echantillon(GNA &gna, const ParametresRendu &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, uint profondeur)
{
	dir = -contexte.V;
	pdf = 1.0;

	auto const &rayon = contexte.rayon;
	auto const &R = reflechi(rayon.direction, contexte.N);

	Rayon rayon_local;
	rayon_local.direction = R;
	rayon_local.origine = contexte.P + contexte.N * 1e-4;
	rayon_local.distance_min = rayon.distance_min;
	rayon_local.distance_max = rayon.distance_max;

	for (size_t i = 0; i < 3; ++i) {
		rayon_local.inverse_direction[i] = 1.0 / rayon_local.direction[i];
	}

	L = 0.8f * calcul_spectre(gna, parametres, rayon_local, profondeur + 1);
}

/* ************************************************************************** */

auto refracte0(
		const dls::math::vec3d &I,
		const dls::math::vec3d &N,
		const double idr)
{
	auto Nrefr = N;
	auto cos_theta = produit_scalaire(Nrefr, I);
	auto eta_dehors = 1.0;
	auto eta_dedans = idr;

	if (cos_theta < 0.0) {
		/* Nous sommes en dehors de la surface, nous voulons cos(theta) positif. */
		cos_theta = -cos_theta;
	}
	else {
		/* Nous sommes dans la surface, cos(theta) est déjà positif, mais retourne le normal. */
		Nrefr.x = -N.x;
		Nrefr.y = -N.y;
		Nrefr.z = -N.z;

		std::swap(eta_dehors, eta_dedans);
	}

	auto eta = eta_dehors / eta_dedans;
	auto k = 1.0 - eta * eta * (1.0 - cos_theta * cos_theta);

	if (k < 0.0) {
		return dls::math::vec3d(0.0);
	}

	return eta * I + (eta * cos_theta - sqrt(k)) * Nrefr;
}

auto fresnel0(
		const dls::math::vec3d &I,
		const dls::math::vec3d &N,
		const double &idr,
		double &kr)
{
	auto cosi = produit_scalaire(I, N);
	auto eta_dehors = 1.0;
	auto eta_dedans = idr;

	if (cosi > 0) {
		std::swap(eta_dehors, eta_dedans);
	}

	/* Calcul sin_i selon la loi de Snell. */
	auto sint = eta_dehors / eta_dedans * sqrt(std::max(0.0, 1 - cosi * cosi));

	/* Réflection interne totale. */
	if (sint >= 1.0) {
		kr = 1.0;
	}
	else {
		auto cost = sqrt(std::max(0.0, 1.0 - sint * sint));
		cosi = abs(cosi);
		auto Rs = ((eta_dedans * cosi) - (eta_dehors * cost)) / ((eta_dedans * cosi) + (eta_dehors * cost));
		auto Rp = ((eta_dehors * cosi) - (eta_dedans * cost)) / ((eta_dehors * cosi) + (eta_dedans * cost));
		kr = (Rs * Rs + Rp * Rp) / 2;
	}

	/* En conséquence de la conservation d'énergie, la transmittance est donnée
	 * par kt = 1 - kr. */
}

BSDFVerre::BSDFVerre(ContexteNuancage &ctx)
	: BSDF(ctx)
{}

void BSDFVerre::evalue_echantillon(GNA &gna, const ParametresRendu &parametres, const dls::math::vec3d &dir, Spectre &L, double &pdf)
{
	INUTILISE(gna);
	INUTILISE(parametres);
	INUTILISE(dir);
	pdf = 0.0;
	L = Spectre(0.0);
}

void BSDFVerre::genere_echantillon(GNA &gna, const ParametresRendu &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, uint profondeur)
{
	dir = get_brdf_ray(gna, contexte.N, -contexte.V);
	pdf = 1.0;

	auto const &rayon = contexte.rayon;

#ifdef ROULETTE_RUSSE
	/* Calcul le fresnel. */
	double kr;
	fresnel(rayon.direction, contexte.N, index_refraction, kr);

	auto outside = produit_scalaire(rayon.direction, contexte.N) < 0;
	auto bias = outside ? -1e-4 * contexte.N : 1e-4 * contexte.N;

	Rayon rayon_local;
	rayon_local.origine = contexte.P + bias;
	rayon_local.distance_min = rayon.distance_min;
	rayon_local.distance_max = rayon.distance_max;

	/* Calcul la réfraction s'il n'y a pas un cas de réflection interne totale. */
	if (kr <= gna.nombre_aleatoire()) {
		rayon_local.direction = normalise(refracte(rayon.direction, contexte.N, index_refraction));

		for (int i = 0; i < 3; ++i) {
			rayon_local.inverse_direction[i] = 1.0 / rayon_local.direction[i];
		}

		L = calcul_spectre(gna, parametres, rayon_local, profondeur + 1);
		return;
	}

	rayon_local.direction = normalise(reflechi(rayon.direction, contexte.N));

	for (int i = 0; i < 3; ++i) {
		rayon_local.inverse_direction[i] = 1.0 / rayon_local.direction[i];
	}

	L = calcul_spectre(gna, parametres, rayon_local, profondeur + 1);
#else
	auto refractionColor = Spectre(0.0f);

	/* Calcul le fresnel. */
	double kr;
	fresnel0(rayon.direction, contexte.N, index_refraction, kr);
	auto outside = produit_scalaire(rayon.direction, contexte.N) < 0;
	auto bias = outside ? -1e-4 * contexte.N : 1e-4 * contexte.N;

	/* Calcul la réfraction s'il n'y a pas un cas de réflection interne totale. */
	if (kr < 1) {
		auto refractionDirection = normalise(refracte0(rayon.direction, contexte.N, index_refraction));
		auto refractionRayOrig = contexte.P + bias;

		Rayon rayon_local;
		rayon_local.direction = refractionDirection;
		rayon_local.origine = refractionRayOrig;
		rayon_local.distance_min = rayon.distance_min;
		rayon_local.distance_max = rayon.distance_max;

		for (size_t i = 0; i < 3; ++i) {
			rayon_local.inverse_direction[i] = 1.0 / rayon_local.direction[i];
		}

		refractionColor = calcul_spectre(gna, parametres, rayon_local, profondeur + 1);
	}

	Rayon rayon_local;
	rayon_local.direction = normalise(reflechi(rayon.direction, contexte.N));
	rayon_local.origine = contexte.P + bias;
	rayon_local.distance_min = rayon.distance_min;
	rayon_local.distance_max = rayon.distance_max;

	for (size_t i = 0; i < 3; ++i) {
		rayon_local.inverse_direction[i] = 1.0 / rayon_local.direction[i];
	}

	auto reflectionColor = calcul_spectre(gna, parametres, rayon_local, profondeur + 1);

	/* Mélange les deux. */
	L = reflectionColor * static_cast<float>(kr) + refractionColor * static_cast<float>(1.0 - kr);
#endif
}

/* ************************************************************************** */

BSDFVolume::BSDFVolume(ContexteNuancage &ctx)
	: BSDF(ctx)
{}

void BSDFVolume::evalue_echantillon(GNA &gna, const ParametresRendu &parametres, const dls::math::vec3d &dir, Spectre &L, double &pdf)
{
	INUTILISE(gna);
	INUTILISE(parametres);
	INUTILISE(dir);
	pdf = 0.0;
	L = Spectre(0.0);
}

void BSDFVolume::genere_echantillon(GNA &gna, const ParametresRendu &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, uint profondeur)
{
	INUTILISE(gna);
	INUTILISE(profondeur);

	dir = -contexte.V;
	pdf = 1.0;

	/* Lance un rayon pour définir le point de sortie du volume. */
	auto rayon_local = contexte.rayon;
	rayon_local.origine = contexte.P;

	auto isect = parametres.acceleratrice->entresecte(parametres.scene, rayon_local, 1000.0);

	if (isect.type_objet == OBJET_TYPE_AUCUN) {
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

void BSDFPhaseIsotropique::evalue_echantillon(GNA &gna, const ParametresRendu &parametres, const dls::math::vec3d &dir, Spectre &L, double &pdf)
{
	INUTILISE(gna);
	INUTILISE(parametres);
	INUTILISE(dir);

	pdf = 0.25 * PI_INV;
	L = Spectre(pdf);
}

void BSDFPhaseIsotropique::genere_echantillon(GNA &gna, const ParametresRendu &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, uint profondeur)
{
	INUTILISE(parametres);
	INUTILISE(profondeur);

	auto xi = gna.nombre_aleatoire();
	dir.z = xi * 2.0 - 1.0; // cos_theta;

	auto sin_theta = 1.0 - dir.z * dir.z; // carré de sin_theta

	if (sin_theta > 1.0) {
		sin_theta = std::sqrt(sin_theta);
		xi = gna.nombre_aleatoire();

		auto phi = xi * TAU;
		dir.x = sin_theta * std::cos(phi);
		dir.y = sin_theta * std::sin(phi);
	}
	else {
		dir.x = 0.0;
		dir.y = 0.0;
	}

	pdf = 0.25 * PI_INV;
	L = Spectre(pdf);
}

/* ************************************************************************** */

/**
 * "Building an orthonormal basis, revisited"
 * http://graphics.pixar.com/library/OrthonormalB/paper.pdf
 */
template <typename T>
static void cree_base_orthonormal(
		const dls::math::vec3<T> &n,
		dls::math::vec3<T> &b0,
		dls::math::vec3<T> &b1)
{
	auto const sign = std::copysign(static_cast<T>(1.0), n.z);
	auto const a = static_cast<T>(-1.0) / (sign + n.z);
	auto const b = n.x * n.y * a;
	b0 = dls::math::vec3<T>(1.0 + sign * n.x * n.x * a, sign * b, -sign * n.x);
	b1 = dls::math::vec3<T>(b, sign + n.y * n.y * a, -n.y);
}

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

void BSDFPhaseAnisotropique::evalue_echantillon(GNA &gna, const ParametresRendu &parametres, const dls::math::vec3d &dir, Spectre &L, double &pdf)
{
	INUTILISE(gna);
	INUTILISE(parametres);
	INUTILISE(dir);

	if (isotropique) {
		pdf = 0.25 * PI_INV;
	}
	else {
		auto cos_theta = produit_scalaire(-contexte.V, dir);
		pdf = calcul_pdf(cos_theta);
	}

	L = Spectre(pdf);
}

void BSDFPhaseAnisotropique::genere_echantillon(GNA &gna, const ParametresRendu &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, uint profondeur)
{
	INUTILISE(parametres);
	INUTILISE(profondeur);

	if (isotropique) {
		/* Pareil que BSDFPhaseIsotropique::genere_echantillon. */
		auto xi = gna.nombre_aleatoire();
		dir.z = xi * 2.0 - 1.0; // cos_theta;

		auto sin_theta = 1.0 - dir.z * dir.z; // carré de sin_theta

		if (sin_theta > 1.0) {
			sin_theta = std::sqrt(sin_theta);
			xi = gna.nombre_aleatoire();

			auto phi = xi * TAU;
			dir.x = sin_theta * std::cos(phi);
			dir.y = sin_theta * std::sin(phi);
		}
		else {
			dir.x = 0.0;
			dir.y = 0.0;
		}

		pdf = 0.25 * PI_INV;
	}
	else {
		auto phi = gna.nombre_aleatoire() * TAU;
		auto cos_theta = inverse_cdf(gna.nombre_aleatoire());
		auto sin_theta = std::sqrt(1.0 - cos_theta * cos_theta); // carré de sin_theta
		auto in = -contexte.V;

		dls::math::vec3d t0, t1;
		cree_base_orthonormal(in, t0, t1);

		dir = sin_theta * std::sin(phi) * t0 + sin_theta * std::cos(phi) * t1 + cos_theta * in;
		pdf = calcul_pdf(cos_theta);
	}

	L = Spectre(pdf);
}

double BSDFPhaseAnisotropique::calcul_pdf(double cos_theta) const
{
	return 0.25 * un_moins_g2 / (M_PI * std::pow(un_plus_g2 - 2.0 * g * cos_theta, 1.5f));
}

double BSDFPhaseAnisotropique::inverse_cdf(double xi) const
{
	auto const t = (un_moins_g2) / (1.0 - g + 2.0 * g * xi);
	return un_sur_2g * (un_plus_g2 - t * t);
}
