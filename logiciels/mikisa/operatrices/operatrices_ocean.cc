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

#include "operatrices_ocean.hh"

#include <cmath>
#include <fftw3.h>

#include "biblinternes/math/complexe.hh"
#include "biblinternes/math/entrepolation.hh"
#include "biblinternes/math/outils.hh"
#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/moultfilage/boucle.hh"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/tableau.hh"

#include "coeur/contexte_evaluation.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "wolika/echantillonnage.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

static fftw_plan cree_plan(
		int M,
		int N,
		dls::tableau<dls::math::complexe<double>> &comp_entree,
		dls::tableau<double> &reel_sortie)
{
	auto entree = reinterpret_cast<double(*)[2]>(comp_entree.donnees());
	auto sortie = reel_sortie.donnees();
	return fftw_plan_dft_c2r_2d(M, N, entree, sortie, FFTW_ESTIMATE);
}

#if 0
struct champs_simulation {
	dls::tableau<dls::math::complexe<double>> entrees{};
	dls::tableau<double> sorties{};
	fftw_plan plan = nullptr;

	champs_simulation() = default;

	champs_simulation(int M, int N)
	{
		auto taille_reel = (M * N);
		auto taille_complexe = (M * (1 + N / 2));
		entrees.redimensionne(taille_complexe);
		sorties.redimensionne(taille_reel);
		plan = cree_plan(M, N, entrees, sorties);
	}

	~champs_simulation()
	{
		fftw_destroy_plan(this->plan);
	}

	dls::math::complexe<double> &complexe(long idx)
	{
		return entrees[idx];
	}

	champs_simulation(champs_simulation const &) = default;
	champs_simulation &operator=(champs_simulation const &) = default;
};

struct ocean {
	/* déplacment */
	champs_simulation disp_y{};

	/* normaux */
	champs_simulation norm_x{};
	champs_simulation norm_z{};

	/* chop */
	champs_simulation chop_x{};
	champs_simulation chop_z{};

	/* jacobien */
	champs_simulation jxx{};
	champs_simulation jzz{};
	champs_simulation jxz{};

	/* autres données */
	fftw_complex *_htilda = nullptr;          /* init w	sim w (only once) */
	/* one dimensional float array */
	float *_kx = nullptr;
	float *_kz = nullptr;

	/* two dimensional complex array */
	fftw_complex *_h0 = nullptr;
	fftw_complex *_h0_minus = nullptr;

	/* two dimensional float array */
	float *_k = nullptr;
};
#endif

/* ************************************************************************** */

struct Ocean {
	/* ********* input parameters to the sim ********* */
	float l = 0.0f;
	float amplitude = 0.0f;
	float reflections_damp = 0.0f;
	float alignement_vent = 0.0f;
	double profondeur = 0.0;

	float vent_x = 0.0f;
	float vent_z = 0.0f;

	float L = 0.0f;

	int graine = 0;

	/* dimensions of computational grid */
	int res_x = 0;
	int res_y = 0;

	/* spatial size of computational grid */
	double taille_spaciale_x = 0.0;
	double taille_spaciale_z = 0.0;

	double facteur_normalisation = 0.0;
	float temps = 0.0f;

	bool calcul_deplacement_y = false;
	bool calcul_normaux = false;
	bool calcul_chop = false;
	bool calcul_ecume = false;

	/* ********* sim data arrays ********* */

	/* two dimensional arrays of complex */
	dls::tableau<dls::math::complexe<double>> fft_in{};
	dls::tableau<dls::math::complexe<double>> fft_in_x{};
	dls::tableau<dls::math::complexe<double>> fft_in_z{};
	dls::tableau<dls::math::complexe<double>> fft_in_jxx{};
	dls::tableau<dls::math::complexe<double>> fft_in_jzz{};
	dls::tableau<dls::math::complexe<double>> fft_in_jxz{};
	dls::tableau<dls::math::complexe<double>> fft_in_nx{};
	dls::tableau<dls::math::complexe<double>> fft_in_nz{};

	/* fftw "plans" */
	fftw_plan disp_y_plan = nullptr;
	fftw_plan disp_x_plan = nullptr;
	fftw_plan disp_z_plan = nullptr;
	fftw_plan N_x_plan = nullptr;
	fftw_plan N_z_plan = nullptr;
	fftw_plan Jxx_plan = nullptr;
	fftw_plan Jxz_plan = nullptr;
	fftw_plan Jzz_plan = nullptr;

	/* two dimensional arrays of float */
	dls::tableau<double> N_x{};
	/* N_y est constant donc inutile de recourir à un tableau. */
	double N_y = 0.0;
	dls::tableau<double> N_z{};
	dls::tableau<double> disp_x{};
	dls::tableau<double> disp_y{};
	dls::tableau<double> disp_z{};

	/* two dimensional arrays of float */
	/* Jacobian and minimum eigenvalue */
	dls::tableau<double> Jxx{};
	dls::tableau<double> Jzz{};
	dls::tableau<double> Jxz{};

	/* one dimensional float array */
	dls::tableau<double> kx{};
	dls::tableau<double> kz{};

	~Ocean();
};

struct OceanResult {
	dls::math::vec3f disp{};
	dls::math::vec3f normal{};
	float foam{};

	/* raw eigenvalues/vectors */
	float Jminus{};
	float Jplus{};
	float Eminus[3];
	float Eplus[3];
};

/* ************************************************************************** */

/* Retourne un nombre aléatoire avec une distribution normale (Gaussienne),
 * avec le sigma spécifié. Ceci utilise la forme polaire de Marsaglia de la
 * méthode de Box-Muller */
/* À FAIRE : fonction similaire dans dls::math pour la 3D */
template <typename T>
static auto gaussRand(GNA &gna, T moyenne = 0,  T sigma = 1)
{
	/* NOTE : pour éviter les problèmes numériques avec des nombres très petit,
	 * on pourrait utiliser des floats pour le calcul de x et y et des doubles
	 * pour le calcul de g.
	 */
	T x;
	T y;
	T longueur_carree;

	do {
		x = gna.uniforme(static_cast<T>(-1), static_cast<T>(1));
		y = gna.uniforme(static_cast<T>(-1), static_cast<T>(1));
		longueur_carree = x * x + y * y;
	} while (longueur_carree >= static_cast<T>(1) || dls::math::est_environ_zero(longueur_carree));

	auto g = std::sqrt(static_cast<T>(-2) * std::log(longueur_carree) / longueur_carree);

	return dls::math::vec2<T>(sigma * x * g + moyenne, sigma * y * g + moyenne);
}

/**
 * Some useful functions
 */
template <typename T>
auto omega(T k, T depth, T gravite)
{
	return std::sqrt(gravite * k * std::tanh(k * depth));
}

/* Spectre de Phillips modifié. */
template <typename T>
auto spectre_phillips(Ocean &o, T kx, T kz)
{
	auto k2 = kx * kx + kz * kz;

	if (dls::math::est_environ_zero(k2)) {
		/* no DC component */
		return 0.0;
	}

	/* damp out the waves going in the direction opposite the wind */
	auto tmp = (static_cast<T>(o.vent_x) * kx + static_cast<T>(o.vent_z) * kz) / std::sqrt(k2);

	if (tmp < 0) {
		tmp *= static_cast<T>(o.reflections_damp);
	}

	return static_cast<T>(o.amplitude) * std::exp(-1.0 / (k2 * static_cast<T>(o.L * o.L))) * std::exp(-k2 * static_cast<T>(o.l * o.l)) *
			std::pow(std::abs(tmp), o.alignement_vent) / (k2 * k2);
}

static void compute_eigenstuff(OceanResult *ocr, float jxx, float jzz, float jxz)
{
	auto a = jxx + jzz;
	auto b = std::sqrt((jxx - jzz) * (jxx - jzz) + 4 * jxz * jxz);

	ocr->Jminus = 0.5f * (a - b);
	ocr->Jplus  = 0.5f * (a + b);

	auto qplus  = (ocr->Jplus  - jxx) / jxz;
	auto qminus = (ocr->Jminus - jxx) / jxz;

	a = std::sqrt(1 + qplus * qplus);
	b = std::sqrt(1 + qminus * qminus);

	ocr->Eplus[0] = 1.0f / a;
	ocr->Eplus[1] = 0.0f;
	ocr->Eplus[2] = qplus / a;

	ocr->Eminus[0] = 1.0f / b;
	ocr->Eminus[1] = 0.0f;
	ocr->Eminus[2] = qminus / b;
}

static float ocean_jminus_vers_ecume(float jminus, float coverage)
{
	float foam = jminus * -0.005f + coverage;
	foam = dls::math::restreint(foam, 0.0f, 1.0f);
	return foam * foam;
}

/* note that this doesn't wrap properly for i, j < 0, but its not really meant for that being just a way to get
 * the raw data out to save in some image format.
 */
static void evalue_ocean_ij(Ocean &oc, OceanResult *ocr, int i, int j)
{
	i = abs(i) % oc.res_x;
	j = abs(j) % oc.res_y;

	ocr->disp[1] = oc.calcul_deplacement_y ? static_cast<float>(oc.disp_y[i * oc.res_y + j]) : 0.0f;

	if (oc.calcul_chop) {
		ocr->disp[0] = static_cast<float>(oc.disp_x[i * oc.res_y + j]);
		ocr->disp[2] = static_cast<float>(oc.disp_z[i * oc.res_y + j]);
	}
	else {
		ocr->disp[0] = 0.0f;
		ocr->disp[2] = 0.0f;
	}

	if (oc.calcul_normaux) {
		ocr->normal[0] = static_cast<float>(oc.N_x[i * oc.res_y + j]);
		ocr->normal[1] = static_cast<float>(oc.N_y);
		ocr->normal[2] = static_cast<float>(oc.N_z[i * oc.res_y + j]);

		ocr->normal = normalise(ocr->normal);
	}

	if (oc.calcul_ecume) {
		compute_eigenstuff(ocr,
						   static_cast<float>(oc.Jxx[i * oc.res_y + j]),
				static_cast<float>(oc.Jzz[i * oc.res_y + j]), static_cast<float>(oc.Jxz[i * oc.res_y + j]));
	}
}

static void simule_ocean(
		Ocean &o,
		double t,
		double scale,
		double chop_x,
		double chop_z,
		double gravite)
{
	scale *= o.facteur_normalisation;

	o.N_y = 1.0 / scale;

	boucle_parallele(tbb::blocked_range<int>(0, o.res_x),
					 [&](tbb::blocked_range<int> const &plage)
	{
		auto gna = GNA(o.graine + plage.begin());

		for (int i = plage.begin(); i < plage.end(); ++i) {
			/* note the <= _N/2 here, see the fftw doco about the mechanics of the complex->real fft storage */
			for (int j = 0; j <= o.res_y / 2; ++j) {
				auto index = i * (1 + o.res_y / 2) + j;

				auto r1 = gaussRand(gna, 0.0, 1.0);
				auto k = std::sqrt(o.kx[i] * o.kx[i] + o.kz[j] * o.kz[j]);

				auto r1r2 = dls::math::complexe(r1.x, r1.y);

				auto h0 = r1r2 * std::sqrt(spectre_phillips(o, o.kx[i], o.kz[j]) / 2.0);
				auto h0_minus = r1r2 * std::sqrt(spectre_phillips(o, -o.kx[i], -o.kz[j]) / 2.0);

				auto exp_param1 = dls::math::complexe(0.0, omega(k, o.profondeur, gravite) * t);
				auto exp_param2 = dls::math::complexe(0.0, -omega(k, o.profondeur, gravite) * t);
				exp_param1 = dls::math::exp(exp_param1);
				exp_param2 = dls::math::exp(exp_param2);

				auto conj_param = dls::math::conjugue(h0_minus);

				exp_param1 = h0 * exp_param1;
				exp_param2 = conj_param * exp_param2;

				auto htilda = exp_param1 + exp_param2;
				o.fft_in[index] = htilda * scale;

				if (o.calcul_chop) {
					if (k == 0.0) {
						o.fft_in_x[index] = dls::math::complexe(0.0, 0.0);
						o.fft_in_z[index] = dls::math::complexe(0.0, 0.0);
					}
					else {
						if (chop_x == 0.0) {
							o.fft_in_x[index] = dls::math::complexe(0.0, 0.0);
						}
						else {
							auto kx = o.kx[i] / k;

							auto mul_param = dls::math::complexe(0.0, -1.0);
							mul_param *= chop_x;

							auto minus_i = dls::math::complexe(-scale, 0.0);
							mul_param *= minus_i;
							mul_param *= htilda;

							o.fft_in_x[index] = mul_param * kx;
						}

						if (chop_z == 0.0) {
							o.fft_in_z[index] = dls::math::complexe(0.0, 0.0);
						}
						else {
							auto kz = o.kz[i] / k;

							auto mul_param = dls::math::complexe(0.0, -1.0);
							mul_param *= chop_z;

							auto minus_i = dls::math::complexe(-scale, 0.0);
							mul_param *= minus_i;
							mul_param *= htilda;

							o.fft_in_z[index] = mul_param * kz;
						}
					}
				}

				if (o.calcul_normaux) {
					auto mul_param = dls::math::complexe(0.0, -1.0);
					mul_param *= htilda;

					o.fft_in_nx[index] = mul_param * o.kx[i];
					o.fft_in_nz[index] = mul_param * o.kz[i];
				}

				if (o.calcul_ecume) {
					if (k == 0.0) {
						o.fft_in_jxx[index] = dls::math::complexe(0.0, 0.0);
						o.fft_in_jzz[index] = dls::math::complexe(0.0, 0.0);
						o.fft_in_jxz[index] = dls::math::complexe(0.0, 0.0);
					}
					else {
						/* auto mul_param = dls::math::complexe(-scale, 0.0); */

						/* calcul jacobien XX */
						auto mul_param = dls::math::complexe(-1.0, 0.0);
						mul_param *= chop_x;
						mul_param *= htilda;

						auto kxx = o.kx[i] * o.kx[i] / k;
						o.fft_in_jxx[index] = mul_param * kxx;

						/* calcul jacobien ZZ */
						mul_param = dls::math::complexe(-1.0, 0.0);
						mul_param *= chop_z;
						mul_param *= htilda;

						auto kzz = o.kz[j] * o.kz[j] / k;
						o.fft_in_jzz[index] = mul_param * kzz;

						/* calcul jacobien XZ */
						mul_param = dls::math::complexe(-1.0, 0.0);
						mul_param *= (chop_x + chop_z) * 0.5;
						mul_param *= htilda;

						auto kxz = o.kx[i] * o.kz[j] / k;
						o.fft_in_jxz[index] = mul_param * kxz;
					}
				}
			}
		}
	});

	dls::tableau<fftw_plan> plans;

	if (!o.disp_y.est_vide()) {
		plans.pousse(o.disp_y_plan);
	}

	if (o.calcul_chop) {
		plans.pousse(o.disp_x_plan);
		plans.pousse(o.disp_z_plan);
	}

	if (o.calcul_normaux) {
		plans.pousse(o.N_x_plan);
		plans.pousse(o.N_z_plan);
	}

	if (o.calcul_ecume) {
		plans.pousse(o.Jxx_plan);
		plans.pousse(o.Jzz_plan);
		plans.pousse(o.Jxz_plan);
	}

	tbb::parallel_for(0l, plans.taille(),
					  [&](long i)
	{
		fftw_execute(plans[i]);
	});

	if (o.calcul_ecume) {
		for (auto i = 0; i < o.res_x * o.res_y; ++i) {
			o.Jxx[i] += 1.0;
			o.Jzz[i] += 1.0;
		}
	}
}

static void set_height_normalize_factor(Ocean &oc, double gravite)
{
	auto max_h = 0.0;

	if (!oc.calcul_deplacement_y) {
		return;
	}

	oc.facteur_normalisation = 1.0;

	simule_ocean(oc, 0.0, 1.0, 0.0, 0.0, gravite);

	for (int i = 0; i < oc.res_x; ++i) {
		for (int j = 0; j < oc.res_y; ++j) {
			if (max_h < std::fabs(oc.disp_y[i * oc.res_y + j])) {
				max_h = std::fabs(oc.disp_y[i * oc.res_y + j]);
			}
		}
	}

	if (max_h == 0.0) {
		max_h = 0.00001;  /* just in case ... */
	}

	oc.facteur_normalisation = 1.0 / (max_h);
}

static void initialise_donnees_ocean(
		Ocean &o,
		double gravite)
{
	auto taille = (o.res_x * o.res_y);
	auto taille_complex = (o.res_x * (1 + o.res_y / 2));

	o.kx.redimensionne(o.res_x);
	o.kz.redimensionne(o.res_y);

	/* make this robust in the face of erroneous usage */
	if (o.taille_spaciale_x == 0.0) {
		o.taille_spaciale_x = 0.001;
	}

	if (o.taille_spaciale_z == 0.0) {
		o.taille_spaciale_z = 0.001;
	}

	/* the +ve components and DC */
	for (auto i = 0; i <= o.res_x / 2; ++i) {
		o.kx[i] = constantes<double>::TAU * static_cast<double>(i) / o.taille_spaciale_x;
	}

	/* the -ve components */
	for (auto i = o.res_x - 1, ii = 0; i > o.res_x / 2; --i, ++ii) {
		o.kx[i] = -constantes<double>::TAU * static_cast<double>(ii) / o.taille_spaciale_x;
	}

	/* the +ve components and DC */
	for (auto i = 0; i <= o.res_y / 2; ++i) {
		o.kz[i] = constantes<double>::TAU * static_cast<double>(i) / o.taille_spaciale_z;
	}

	/* the -ve components */
	for (auto i = o.res_y - 1, ii = 0; i > o.res_y / 2; --i, ++ii) {
		o.kz[i] = -constantes<double>::TAU * static_cast<double>(ii) / o.taille_spaciale_z;
	}

	o.fft_in.redimensionne(taille_complex);

	if (o.calcul_deplacement_y) {
		o.disp_y.redimensionne(taille);
		o.disp_y_plan = cree_plan(o.res_x, o.res_y, o.fft_in, o.disp_y);
	}

	if (o.calcul_normaux) {
		o.fft_in_nx.redimensionne(taille_complex);
		o.fft_in_nz.redimensionne(taille_complex);

		o.N_x.redimensionne(taille);
		o.N_z.redimensionne(taille);

		o.N_x_plan = cree_plan(o.res_x, o.res_y, o.fft_in_nx, o.N_x);
		o.N_z_plan = cree_plan(o.res_x, o.res_y, o.fft_in_nz, o.N_z);
	}

	if (o.calcul_chop) {
		o.fft_in_x.redimensionne(taille_complex);
		o.fft_in_z.redimensionne(taille_complex);

		o.disp_x.redimensionne(taille);
		o.disp_z.redimensionne(taille);

		o.disp_x_plan = cree_plan(o.res_x, o.res_y, o.fft_in_x, o.disp_x);
		o.disp_z_plan = cree_plan(o.res_x, o.res_y, o.fft_in_z, o.disp_z);
	}

	if (o.calcul_ecume) {
		o.fft_in_jxx.redimensionne(taille_complex);
		o.fft_in_jzz.redimensionne(taille_complex);
		o.fft_in_jxz.redimensionne(taille_complex);

		o.Jxx.redimensionne(taille);
		o.Jzz.redimensionne(taille);
		o.Jxz.redimensionne(taille);

		o.Jxx_plan = cree_plan(o.res_x, o.res_y, o.fft_in_jxx, o.Jxx);
		o.Jzz_plan = cree_plan(o.res_x, o.res_y, o.fft_in_jzz, o.Jzz);
		o.Jxz_plan = cree_plan(o.res_x, o.res_y, o.fft_in_jxz, o.Jxz);
	}

	set_height_normalize_factor(o, gravite);
}

static void deloge_donnees_ocean(Ocean &oc)
{
	if (oc.calcul_deplacement_y) {
		fftw_destroy_plan(oc.disp_y_plan);
	}

	if (oc.calcul_normaux) {
		fftw_destroy_plan(oc.N_x_plan);
		fftw_destroy_plan(oc.N_z_plan);
	}

	if (oc.calcul_chop) {
		fftw_destroy_plan(oc.disp_x_plan);
		fftw_destroy_plan(oc.disp_z_plan);
	}

	if (oc.calcul_ecume) {
		fftw_destroy_plan(oc.Jxx_plan);
		fftw_destroy_plan(oc.Jzz_plan);
		fftw_destroy_plan(oc.Jxz_plan);
	}
}

/* ************************************************************************** */

Ocean::~Ocean()
{
	deloge_donnees_ocean(*this);
}

/* ************************************************************************** */

class OperatriceSimulationOcean : public OperatriceCorps {
	Ocean m_ocean{};
	bool m_reinit = false;

	calque_image m_ecume_precedente{};

public:
	static constexpr auto NOM = "Océan";
	static constexpr auto AIDE = "";

	explicit OperatriceSimulationOcean(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_ocean.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		if (!valide_corps_entree(*this, &m_corps, true, false)) {
			return EXECUTION_ECHOUEE;
		}

		/* paramètres simulations */
		auto resolution = dls::math::restreint(evalue_entier("résolution"), 4, 11);
		auto velocite_vent = evalue_decimal("vélocité_vent");
		auto echelle_vague = evalue_decimal("échelle_vague");
		auto alignement_vague = evalue_decimal("alignement_vague");
		auto plus_petite_vague = evalue_decimal("plus_petite_vague");
		auto direction_vague = evalue_decimal("direction_vague");
		auto profondeur = evalue_decimal("profondeur");
		auto gravite = evalue_decimal("gravité");
		auto damp = evalue_decimal("damping");
		auto chop_x = evalue_decimal("chop_x");
		auto chop_z = evalue_decimal("chop_z");
		auto graine = evalue_entier("graine");
		auto temps = evalue_decimal("temps", contexte.temps_courant) / static_cast<float>(contexte.cadence);
		auto couverture_ecume = evalue_decimal("couverture_écume");
		auto attenuation_ecume = evalue_decimal("atténuation_écume");
		auto taille = evalue_decimal("taille");
		auto taille_spaciale = static_cast<float>(evalue_entier("taille_spaciale"));

		auto const taille_inverse = 1.0f / (taille * taille_spaciale);

		if (!std::isfinite(taille_inverse)) {
			this->ajoute_avertissement("La taille inverse n'est pas finie");
			return EXECUTION_ECHOUEE;
		}

		m_ocean.res_x = static_cast<int>(std::pow(2.0, resolution));
		m_ocean.res_y = m_ocean.res_x;
		m_ocean.calcul_deplacement_y = true;
		m_ocean.calcul_normaux = true;
		m_ocean.calcul_ecume = true;
		m_ocean.calcul_chop = (chop_x > 0.0f) || (chop_z > 0.0f);
		m_ocean.l = plus_petite_vague;
		m_ocean.amplitude = 1.0f;
		m_ocean.reflections_damp = 1.0f - damp;
		m_ocean.alignement_vent = alignement_vague;
		m_ocean.profondeur = static_cast<double>(profondeur);
		m_ocean.taille_spaciale_x = static_cast<double>(taille_spaciale);
		m_ocean.taille_spaciale_z = static_cast<double>(taille_spaciale);
		m_ocean.vent_x = std::cos(direction_vague);
		m_ocean.vent_z = -std::sin(direction_vague);
		/* plus grosse vague pour une certaine vélocité */
		m_ocean.L = velocite_vent * velocite_vent / gravite;
		m_ocean.temps = temps;
		m_ocean.graine = graine;

		if (contexte.temps_courant == contexte.temps_debut || m_reinit) {
			deloge_donnees_ocean(m_ocean);
			initialise_donnees_ocean(m_ocean, static_cast<double>(gravite));

			auto desc = wlk::desc_grille_2d{};
			desc.etendue.min = dls::math::vec2f(0.0f);
			desc.etendue.max = dls::math::vec2f(1.0f);
			desc.fenetre_donnees = desc.etendue;
			desc.taille_pixel = 1.0 / (static_cast<double>(m_ocean.res_x));

			m_ecume_precedente = calque_image::construit_calque(desc, wlk::type_grille::R32);

			m_reinit = false;
		}

		simule_ocean(m_ocean,
					 static_cast<double>(temps),
					 static_cast<double>(echelle_vague),
					 static_cast<double>(chop_x),
					 static_cast<double>(chop_z),
					 static_cast<double>(gravite));

		/* converti les données en images */
		auto desc = wlk::desc_grille_2d{};
		desc.etendue.min = dls::math::vec2f(0.0f);
		desc.etendue.max = dls::math::vec2f(1.0f);
		desc.fenetre_donnees = desc.etendue;
		desc.taille_pixel = 1.0 / (static_cast<double>(m_ocean.res_x));

		auto calque_depl = calque_image::construit_calque(desc, wlk::type_grille::VEC3);
		auto calque_norm = calque_image::construit_calque(desc, wlk::type_grille::VEC3);

		auto grille_depl = dynamic_cast<wlk::grille_dense_2d<dls::math::vec3f> *>(calque_depl.tampon());
		auto grille_norm = dynamic_cast<wlk::grille_dense_2d<dls::math::vec3f> *>(calque_norm.tampon());
		auto grille_ecume = dynamic_cast<wlk::grille_dense_2d<float> *>(m_ecume_precedente.tampon());

		OceanResult ocr;
		auto gna = GNA{graine};

		for (auto j = 0; j < m_ocean.res_y; ++j) {
			for (auto i = 0; i < m_ocean.res_y; ++i) {
				auto index = grille_depl->calcul_index(dls::math::vec2i(i, j));

				evalue_ocean_ij(m_ocean, &ocr, i, j);

				grille_depl->valeur(index) = ocr.disp;
				grille_norm->valeur(index) = ocr.normal;

				if (m_ocean.calcul_ecume) {
					auto ecume = ocean_jminus_vers_ecume(ocr.Jminus, couverture_ecume);

					/* accumule l'écume précédente pour cette cellule. */
					auto ecume_prec = grille_ecume->valeur(index);

					/* réduit aléatoirement l'écume */
					ecume_prec *= gna.uniforme(0.0f, 1.0f);

					if (ecume_prec < 1.0f) {
						ecume_prec *= ecume_prec;
					}

					/* brise l'écume là où la hauteur (Y) est basse (vallée), et le
					 * déplacement X et Z est au plus haut.
					 */

					auto eplus_neg = ocr.Eplus[2] < 0.0f ? 1.0f + ocr.Eplus[2] : 1.0f;
					eplus_neg = eplus_neg < 0.0f ? 0.0f : eplus_neg;

					ecume_prec *= attenuation_ecume * (0.75f + eplus_neg * 0.25f);

					/* Une restriction pleine ne devrait pas être nécessaire ! */
					auto resultat_ecume = std::min(ecume_prec + ecume, 1.0f);

					grille_ecume->valeur(index) = resultat_ecume;
				}
			}
		}

		/* applique les déplacements à la géométrie d'entrée */
		auto points = m_corps.points_pour_ecriture();

		auto N = m_corps.ajoute_attribut("N", type_attribut::VEC3, portee_attr::POINT);
		N->redimensionne(points->taille());

		auto C = m_corps.ajoute_attribut("C", type_attribut::VEC3, portee_attr::POINT);
		C->redimensionne(points->taille());

		auto const chn_entrep = evalue_enum("entrepolation");
		auto entrepolation = 0;

		if (chn_entrep == "linéaire") {
			entrepolation = 1;
		}
		else if (chn_entrep == "catrom") {
			entrepolation = 2;
		}

		for (auto i = 0; i < points->taille(); ++i) {
			auto p = points->point(i);

			/* converti la position du point en espace grille */
			auto x = p.x * taille_inverse + 0.5f;
			auto y = p.z * taille_inverse + 0.5f;

			switch (entrepolation) {
				case 0:
				{
					p += echantillonne_proche(*grille_depl, x, y);
					points->point(i, p);

					N->vec3(i) = echantillonne_proche(*grille_norm, x, y);

					if (m_ocean.calcul_ecume) {
						auto ecume = echantillonne_proche(*grille_ecume, x, y);
						C->vec3(i) = dls::math::vec3f(ecume, ecume, 1.0f);
					}

					break;
				}
				case 1:
				{
					p += echantillonne_lineaire(*grille_depl, x, y);
					points->point(i, p);

					N->vec3(i) = echantillonne_lineaire(*grille_norm, x, y);

					if (m_ocean.calcul_ecume) {
						auto ecume = echantillonne_lineaire(*grille_ecume, x, y);
						C->vec3(i) = dls::math::vec3f(ecume, ecume, 1.0f);
					}

					break;
				}
				case 2:
				{
					p += echantillonne_catrom(*grille_depl, x, y);
					points->point(i, p);

					N->vec3(i) = echantillonne_catrom(*grille_norm, x, y);

					if (m_ocean.calcul_ecume) {
						auto ecume = echantillonne_catrom(*grille_ecume, x, y);
						C->vec3(i) = dls::math::vec3f(ecume, ecume, 1.0f);
					}

					break;
				}
			}
		}

		return EXECUTION_REUSSIE;
	}

	void parametres_changes() override
	{
		m_reinit = true;
	}

	void performe_versionnage() override
	{
		if (propriete("chop_x") == nullptr) {
			auto prop_chop = propriete("quantité_chop");
			auto chop = 1.0f;

			if (prop_chop != nullptr) {
				chop = std::any_cast<float>(prop_chop->valeur);
			}

			ajoute_propriete("chop_x", danjo::TypePropriete::DECIMAL, chop);
			ajoute_propriete("chop_z", danjo::TypePropriete::DECIMAL, chop);
		}

		if (propriete("entrepolation") == nullptr) {
			ajoute_propriete("entrepolation", danjo::TypePropriete::ENUM, dls::chaine("proche"));
		}
	}
};

/* ************************************************************************** */

#undef VAGUELETTE_OCEAN

#ifdef VAGUELETTE_OCEAN
namespace WaterWavelets {

/*
 * Domain inteprolation - Interpolates function values only for pointS in the
 * domain
 *
 * \tparam Interpolation This is any function satisfying concept Interpolatiobn
 * \tparam Domain This is bool valued function on integers returning true for
 points inside of the domain and false otherwise
 * \param interpolation Interpolation to use.
 * \param domain Function indicating domain.
 */
template <class Interpolation, class Domain>
auto DomainInterpolation(Interpolation interpolation, Domain domain) {
	return [=](auto fun) mutable {

		// The function `dom_fun` collects function values and weights inside of
		// the domain
		auto dom_fun = [=](auto... x) mutable -> dls::math::vec2f {
			dls::math::vec2f val;
			if (domain(x...) == true) {
				val = dls::math::vec2f(fun(x...), 1.0);
			} else {
				val = dls::math::vec2f(0.0f, 0.0f);
			}
			return val;
		};

		// Interpolates `dom_fun`
		auto int_fun = interpolation(dom_fun);

		return [=](auto... x) mutable {
			auto val = int_fun(x...);
			double f = val[0];
			double w = val[1];
			return w != 0.0 ? f / w : 0.0;
		};
	};
}

auto ConstantInterpolation = [](auto fun) {
	using ValueType = std::remove_reference_t<decltype(fun(0))>;
	return [=](auto x) mutable -> ValueType {
		int ix = (int)round(x);
		return fun(ix);
	};
};

auto LinearInterpolation = [](auto fun) {
	// The first two lines are here because Eigen types do
	using ValueType = std::remove_reference_t<decltype(fun(0))>;
	ValueType zero = ValueType(0);
	return [=](auto x) mutable -> ValueType {
		int ix = (int)floor(x);
		auto wx = x - ix;

		return (wx != 0 ? wx * fun(ix + 1) : zero) +
				(wx != 1 ? (1 - wx) * fun(ix) : zero);

	};
};

template <int I, typename... Ts>
using getType = typename std::tuple_element<I, std::tuple<Ts...>>::type;

template <int I, class... Ts> constexpr decltype(auto) get(Ts &&... ts) {
	return std::get<I>(std::forward_as_tuple(ts...));
}

template <int I>
static auto InterpolateNthArgument = [](auto fun, auto interpolation) {
	return [=](auto... x) mutable {
		// static_assert(std::is_invocable<decltype(fun), decltype(x)...>::value,
		//        "Invalid aguments");

		auto foo = [fun, x...](int i) mutable {
			get<I>(x...) = i;
			return fun(x...);
		};

		auto xI = get<I>(x...);
		return interpolation(foo)(xI);
	};
};

template <int I, class Fun, class... Interpolation>
auto InterpolationDimWise_impl(Fun fun, Interpolation... interpolation) {

	if constexpr (I == sizeof...(interpolation)) {
		return fun;
	} else {
		auto interpol = get<I>(interpolation...);
		return InterpolationDimWise_impl<I + 1>(
					InterpolateNthArgument<I>(fun, interpol), interpolation...);
	}
}

template <int I, int... J, class Fun, class... Interpolation>
auto InterpolationDimWise_impl2(Fun fun, Interpolation... interpolation) {

	if constexpr (I == sizeof...(J)) {
		return fun;
	} else {
		constexpr int K = get<I>(J...);
		auto interpol = get<K>(interpolation...);
		return InterpolationDimWise_impl2<I + 1, J...>(
					InterpolateNthArgument<K>(fun, interpol), interpolation...);
	}
}

template <class... Interpolation>
auto InterpolationDimWise(Interpolation... interpolation) {
	return [=](auto fun) {
		return InterpolationDimWise_impl<0>(fun, interpolation...);
	};
}

/**
   @brief Grid
   Four dimensional grid.
 */
class Grid {
public:
	Grid();

	void resize(int n0, int n1, int n2, int n3);

	float &operator()(int i0, int i1, int i2, int i3);

	float const &operator()(int i0, int i1, int i2, int i3) const;

	int dimension(int dim) const;

private:
	std::vector<float>  m_data;
	std::array<int, 4> m_dimensions;
};

Grid::Grid() : m_dimensions{0, 0, 0, 0}, m_data{0} {}

void Grid::resize(int n0, int n1, int n2, int n3) {
	m_dimensions = std::array<int, 4>{n0, n1, n2, n3};
	m_data.resize(n0 * n1 * n2 * n3);
}

float &Grid::operator()(int i0, int i1, int i2, int i3) {
	assert(i0 >= 0 && i0 < dimension(0) && i1 >= 0 && i1 < dimension(1) &&
		   i2 >= 0 && i2 < dimension(2) && i3 >= 0 && i3 < dimension(3));
	return m_data[i3 +
			dimension(3) * (i2 + dimension(2) * (i1 + dimension(1) * i0))];
}

float const &Grid::operator()(int i0, int i1, int i2, int i3) const {
	return m_data[i3 +
			dimension(3) * (i2 + dimension(2) * (i1 + dimension(1) * i0))];
}

int Grid::dimension(int dim) const { return m_dimensions[dim]; }


class Spectrum {
public:
	Spectrum(double windSpeed);

	/*
   * Maximal reasonable value of zeta to consider
   */
	double maxZeta() const;

	/*
   * Minamal resonable value of zeta to consider
   */
	double minZeta() const;

	/*
   * Returns density of wave for given zeta(=log2(wavelength))
   */
	double operator()(double zeta) const;

public:
	double m_windSpeed = 1;
};

Spectrum::Spectrum(double windSpeed) : m_windSpeed(windSpeed) {}

double Spectrum::maxZeta() const { return log2(10); }

double Spectrum::minZeta() const { return log2(0.03); };

double Spectrum::operator()(double zeta) const {
	double A = pow(1.1, 1.5 * zeta); // original pow(2, 1.5*zeta)
	double B = exp(-1.8038897788076411 * pow(4, zeta) / pow(m_windSpeed, 4));
	return 0.139098 * sqrt(A * B);
}

class Environment {
public:
	Environment(float size);

	bool inDomain(dls::math::vec2f pos) const;
	float levelset(dls::math::vec2f pos) const;
	dls::math::vec2f levelsetGrad(dls::math::vec2f pos) const;

public:
	float _dx;
};

template <typename Fun>
auto integrate(int integration_nodes, double x_min, double x_max, Fun const &fun) {

	float dx = (x_max - x_min) / integration_nodes;
	double x  = x_min + 0.5f * dx;

	auto result = dx * fun(x); // the first integration node
	for (int i = 1; i < integration_nodes; i++) { // proceed with other nodes, notice `int i= 1`
		x += dx;
		result += dx * fun(x);
	}

	return result;
}

#include "harbor_data.cpp"

auto &raw_data = harbor_data;

const int N = sqrt(raw_data.size());

auto data_grid = [](int i, int j) -> float {
	// outside of the data grid just return some high arbitrary number
	if (i < 0 || i >= N || j < 0 || j >= N)
		return 100;

	return raw_data[j + i * N];
};

auto dx_data_grid = [](int i, int j) -> float {
	if (i < 0 || i >= (N - 1) || j < 0 || j >= N)
		return 0;

	return raw_data[j + (i + 1) * N] - raw_data[j + i * N];
};

auto dy_data_grid = [](int i, int j) -> float {
	if (i < 0 || i >= N || j < 0 || j >= (N - 1))
		return 0;

	return raw_data[(j + 1) + i * N] - raw_data[j + i * N];
};

auto igrid =
		InterpolationDimWise(LinearInterpolation, LinearInterpolation)(data_grid);

auto igrid_dx = InterpolationDimWise(LinearInterpolation,
									 LinearInterpolation)(dx_data_grid);

auto igrid_dy = InterpolationDimWise(LinearInterpolation,
									 LinearInterpolation)(dy_data_grid);

auto grid = [](dls::math::vec2f pos, float dx) -> float {
	pos *= 1 / dx;
	pos += dls::math::vec2f(N / 2 - 0.5f, N / 2 - 0.5f);
	return igrid(pos[0], pos[1]) * dx;
};

auto grid_dx = [](dls::math::vec2f pos, float dx) -> float {
	pos *= 1 / dx;
	pos += dls::math::vec2f(N / 2 - 1.0f, N / 2 - 0.5f);
	return igrid_dx(pos[0], pos[1]);
};

auto grid_dy = [](dls::math::vec2f pos, float dx) -> float {
	pos *= 1 / dx;
	pos += dls::math::vec2f(N / 2 - 0.5f, N / 2 - 1.0f);
	return igrid_dy(pos[0], pos[1]);
};

Environment::Environment(float size) : _dx((2 * size) / N) {}

bool Environment::inDomain(dls::math::vec2f pos) const { return levelset(pos) >= 0; }

float Environment::levelset(dls::math::vec2f pos) const { return grid(pos, _dx); }

dls::math::vec2f Environment::levelsetGrad(dls::math::vec2f pos) const {
	auto grad = dls::math::vec2f{grid_dx(pos, _dx), grid_dy(pos, _dx)};
	return normalise(grad);
}

/*
 * The class ProfileBuffer is representation of the integral (21) from the
 * paper.
 *
 * It provides two functionalities:
 * 1. Function `precompute` precomputes values of the integral for main input
 * valus of `p` (see paper for the meaning of p)
 * 2. Call operator evaluates returns the profile value at a given point.
 * This is done by interpolating from precomputed values.
 *
 * TODO: Add note about making (21) pariodic over precomputed interval.
 */
class ProfileBuffer {
public:
	/**
   * Precomputes profile buffer for the give spectrum.
   *
   * This function numerically precomputes integral (21) from the paper.
   *
   * @param spectrum A function which accepts zeta(=log2(wavelength)) and retuns
   * wave density.
   * @param time Precompute the profile for a given time.
   * @param zeta_min Lower bound of the integral. zeta_min == log2('minimal
   * wavelength')
   * @param zeta_min Upper bound of the integral. zeta_max == log2('maximal
   * wavelength')
   * @param resolution number of nodes for which the .
   * @param periodicity The period of the final function is determined as
   * `periodicity*pow(2,zeta_max)`
   * @param integration_nodes Number of integraion nodes
   */
	template <typename Spectrum>
	void precompute(Spectrum &spectrum, float time, float zeta_min,
					float zeta_max, int resolution = 4096, int periodicity = 2,
					int integration_nodes = 100) {
		m_data.resize(resolution);
		m_period = periodicity * pow(2, zeta_max);

		//#pragma omp parallel for
		for (int i = 0; i < resolution; i++) {

			constexpr float tau = 6.28318530718;
			float           p   = (i * m_period) / resolution;

			m_data[i] =
					integrate(integration_nodes, zeta_min, zeta_max, [&](float zeta) {

				float waveLength = pow(2, zeta);
				float waveNumber = tau / waveLength;
				float phase1 =
						waveNumber * p - dispersionRelation(waveNumber) * time;
				float phase2 = waveNumber * (p - m_period) -
						dispersionRelation(waveNumber) * time;

				float weight1 = p / m_period;
				float weight2 = 1 - weight1;
				return waveLength * static_cast<float>(spectrum(zeta)) *
						(cubic_bump(weight1) * gerstner_wave(phase1, waveNumber) +
						 cubic_bump(weight2) * gerstner_wave(phase2, waveNumber));
			});
		}
	}

	/**
   * Evaluate profile at point p by doing linear interpolation over precomputed
   * data
   * @param p evaluation position, it is usually p=dot(position, wavedirection)
   */
	dls::math::vec4f operator()(float p) const;

private:
	/**
   * Dispersion relation in infinite depth -
   * https://en.wikipedia.org/wiki/Dispersion_(water_waves)
   */
	float dispersionRelation(float k) const;

	/**
   * Gerstner wave - https://en.wikipedia.org/wiki/Trochoidal_wave
   *
   * @return Array of the following values:
	  1. horizontal position offset
	  2. vertical position offset
	  3. position derivative of horizontal offset
	  4. position derivative of vertical offset
   */
	dls::math::vec4f gerstner_wave(float phase /*=knum*x*/, float knum) const;

	/** bubic_bump is based on $p_0$ function from
   * https://en.wikipedia.org/wiki/Cubic_Hermite_spline
   */
	float cubic_bump(float x) const;

public:
	float m_period;

	std::vector<dls::math::vec4f> m_data;
};

constexpr int pos_modulo(int n, int d) { return (n % d + d) % d; }

dls::math::vec4f ProfileBuffer::operator()(float p) const {

	const int N = m_data.size();

	// Guard from acessing outside of the buffer by wrapping
	auto extended_buffer = [=](int i) -> dls::math::vec4f {
		return m_data[pos_modulo(i, N)];
	};

	// Preform linear interpolation
	auto interpolated_buffer = LinearInterpolation(extended_buffer);

	// rescale `p` to interval [0,1)
	return interpolated_buffer(N * p / m_period);
}

float ProfileBuffer::dispersionRelation(float k) const {
	constexpr float g = 9.81;
	return sqrt(k * g);
};

dls::math::vec4f ProfileBuffer::gerstner_wave(float phase /*=knum*x*/,
											  float knum) const {
	float s = sin(phase);
	float c = cos(phase);
	return dls::math::vec4f{-s, c, -knum * c, -knum * s};
};

float ProfileBuffer::cubic_bump(float x) const {
	if (abs(x) >= 1)
		return 0.0f;
	else
		return x * x * (2 * abs(x) - 3) + 1;
};


/**
@ WaveGrid main class representing water surface.
@section Usage
@section Discretization
explain that \theta_0 = 0.5*dtheta
  The location is determined by four numbers: x,y,theta,zeta
  $x \in [-size,size]$ - the first spatial coordinate
  $y \in [-size,size]$ - the second spatial coordinate
  $theta \in [0,2 \pi)$  - direction of wavevector, theta==0 corresponds to
 wavevector in +x direction
  $zeta \in [\log_2(minWavelength),\log_2(maxWavelength)]$ - zeta is log2 of
 wavelength
  The reason behind using zeta instead of wavenumber or wavelength is the
 nature of wavelengths and it has better discretization properties. We want
 to have a nice cascade of waves with exponentially increasing wavelengths.
*/

class WaveGrid {
public:
	using Idx = std::array<int, 4>;

	enum Coord { X = 0, Y = 1, Theta = 2, Zeta = 3 };

public:
	/**
	 @brief Settings to set up @ref WaveGrid
	 The physical size of the resulting domain is:
	 [-size,size]x[-size,size]x[0,2pi)x[min_zeta,max_zeta]
	 The final grid resolution is: n_x*n_x*n_theta*n_zeta
   */
	struct Settings {
		/** Spatial size of the domain will be [-size,size]x[-size,size] */
		float size = 50;
		/** Maximal zeta to simulate. */
		float max_zeta = 0.01;
		/** Minimal zeta to simulate. */
		float min_zeta = 10;

		/** Number of nodes per spatial dimension. */
		int n_x = 100;
		/** Number of discrete wave directions. */
		int n_theta = 16;
		/** Number of nodes in zeta. This determines resolution in wave number @see
	 * @ref Discretization. */
		int n_zeta = 1;

		/** Set up initial time. Default value is 100 because at time zero you get
	 * wierd coherency patterns */
		float initial_time = 100;

		/** Select spectrum type. Currently only PiersonMoskowitz is supported. */
		enum SpectrumType {
			LinearBasis,
			PiersonMoskowitz
		} spectrumType = PiersonMoskowitz;
	};

public:
	/**
   * @brief Construct WaveGrid based on supplied @ref Settings
   * @param s settings to initialize WaveGrid
   */
	WaveGrid(Settings s);

	/** @brief Preform one time step.
   * @param dt time step
   * @param fullUpdate If true preform standard time step. If false, update only profile buffers.
   *
   * One time step consists of doing an advection step, diffusion step and
   * computation of profile buffers.
   *
   * To choose a reasonable dt we provide function @ref cflTimeStep()
   */
	void timeStep(const float dt, bool fullUpdate =true);

	/**
   * @brief Position and normal of water surface
   * @param pos Position where you want to know position and normal
   * @return A pair: position, normal
   *
   * Returned position is not only a vertical displacement but because we use
   * Gerstner waves we get also a horizontal displacement.
   */
	std::pair<dls::math::vec3f, dls::math::vec3f> waterSurface(dls::math::vec2f pos) const;

	/**
   * @brief Time step based on CFL conditions
   *
   * Returns time in which the fastest waves move one grid cell across.
   * It is usefull for setting reasonable time step in @ref timeStep()
   */
	float cflTimeStep() const;

	/**
   * @brief Amplitude at a point
   * @param pos4 Position in physical coordinates
   * @return Returns interpolated amplitude at a given point
   *
   * The point pos4 has to be physical coordinates: i.e. in box
   * [-size,size]x[-size,size]x[0,2pi)x[min_zeta,max_zeta] @ref Settings
   * Default value(@ref defaultAmplitude()) is returned is point is outside of
   * this box.
   */
	float amplitude(dls::math::vec4f pos4) const;

	/**
   * @brief Amplitude value at a grid not
   * @param idx4 Node index to get amplitude at
   * @return Amplitude value at the give node.
   *
   * If idx4 is outside of discretized grid
   * {0,...,n_x-1}x{0,...,n_x-1}x{0,...,n_theta}x{0,...,n_zeta} then the default
   * value is returned,@ref defaultAmplitude().
   */
	float gridValue(Idx idx4) const;

	/**
   * @brief Wave trajectory
   * @param pos4 Starting location
   * @param length Trajectory length
   * @return Trajectory of a wave starting at position pos4.
   *
   * This method was used for debugin purposes, mainly checking that boudary
   * reflection works correctly.
   */
	std::vector<dls::math::vec4f> trajectory(dls::math::vec4f pos4, float length) const;

	/**
   * @brief Adds point disturbance to a point
   * @param pos Position of disturbance, in physical coordinates.
   * @val Strength of disturbance
   *
   * This function basically increases amplitude for all directions at a certain
   * point.
   */
	void addPointDisturbance(dls::math::vec2f pos, float val);

public:
	/**
   * @brief Preforms advection step
   * @param dt Time for one step.
   */
	void advectionStep(float dt);

	/**
   * @brief Preforms diffusion step
   * @param dt Time for one step.
   */
	void diffusionStep(float dt);

	/**
   * @brief Precomputes profile buffers
   *
   * The "parameter" to this function is the internal time(@ref m_time) at which
   * the profile buffers are precomputed.
   */
	void precomputeProfileBuffers();

	/**
   * @brief Precomputed group speed.
   *
   * This basically computes the "expected group speed" for the currently chosen
   * wave spectrum.
   */
	void precomputeGroupSpeeds();

	/**
   * @brief Boundary checking.
   * @param pos4 Point to be checked
   * @return If the intput point was inside of a boudary then this function
   * returns reflected point.
   *
   * If the point pos4 is not inside of the boundary then it is returned.
   *
   * If the point is inside of the boundary then it is reflected. This means
   * that the spatial position and also the wave direction is reflected w.t.r.
   * to the boundary.
   */
	dls::math::vec4f boundaryReflection(dls::math::vec4f pos4) const;

	/**
   * @brief Extends discrete grid with default values
   * @return Returns a function with signature(Int × Int × Int × Int -> float).
   *
   * We store amplitudes on a 4D grid of the size (n_x*n_x*n_theta*n_zeta)(@ref
   * Settings). Sometimes it is usefull not to worry about grid bounds and get
   * default value for points outside of the grid or it wrapps arround for the
   * theta-coordinate.
   *
   * This function is doing exactly this, it returns a function which accepts
   * four integers and returns an amplitude. You do not have to worry about
   * the bounds.
   */
	auto extendedGrid() const;

	/**
   * @brief Preforms linear interpolation on the grid
   * @return Returns a function(dls::math::vec4f -> float): accepting a point in physical
   * coordinates and returns interpolated amplitude.
   *
   * This function preforms a linear interpolation on the computational grid in
   * all four coordinates. It returns a function accepting @ref dls::math::vec4f and returns
   * interpolated amplitude. Any input point is valid because the interpolation
   * is done on @ref extendedGrid().
   */
	auto interpolatedAmplitude() const;

public:
	float idxToPos(int idx, int dim) const;
	dls::math::vec4f idxToPos(Idx idx) const;

	float posToGrid(float pos, int dim) const;
	dls::math::vec4f posToGrid(dls::math::vec4f pos4) const;

	int posToIdx(float pos, int dim) const;
	Idx posToIdx(dls::math::vec4f pos4) const;

	dls::math::vec2f nodePosition(int ix, int iy) const;

	float waveLength(int izeta) const;
	float waveNumber(int izeta) const;

	float dispersionRelation(float k) const;
	float dispersionRelation(dls::math::vec4f pos4) const;
	float groupSpeed(int izeta) const;
	// float groupSpeed(dls::math::vec4f pos4) const;

	// dls::math::vec2f groupVelocity(int izeta) const;
	dls::math::vec2f groupVelocity(dls::math::vec4f pos4) const;
	float defaultAmplitude(int itheta, int izeta) const;

	int  gridDim(int dim) const;
	float dx(int dim) const;

public:
	Grid     m_amplitude, m_newAmplitude;
	Spectrum m_spectrum;

	std::vector<ProfileBuffer> m_profileBuffers;

	std::array<float, 4> m_xmin;
	std::array<float, 4> m_xmax;
	std::array<float, 4> m_dx;
	std::array<float, 4> m_idx;

	std::vector<float> m_groupSpeeds;

	float m_time;

	Environment m_enviroment;
};

constexpr float tau = 6.28318530718; // https://tauday.com/tau-manifesto

WaveGrid::WaveGrid(Settings s) : m_spectrum(10),m_enviroment(s.size) {

	m_amplitude.resize(s.n_x, s.n_x, s.n_theta, s.n_zeta);
	m_newAmplitude.resize(s.n_x, s.n_x, s.n_theta, s.n_zeta);

	float zeta_min = m_spectrum.minZeta();
	float zeta_max = m_spectrum.maxZeta();

	m_xmin = {-s.size, -s.size, 0.0, zeta_min};
	m_xmax = {s.size, s.size, tau, zeta_max};

	for (int i = 0; i < 4; i++) {
		m_dx[i]  = (m_xmax[i] - m_xmin[i]) / m_amplitude.dimension(i);
		m_idx[i] = 1.0 / m_dx[i];
	}

	m_time = s.initial_time;

	m_profileBuffers.resize(s.n_zeta);
	precomputeGroupSpeeds();
}

void WaveGrid::timeStep(const float dt, bool fullUpdate) {
	if (fullUpdate) {
		advectionStep(dt);
		diffusionStep(dt);
	}
	precomputeProfileBuffers();
	m_time += dt;
}

float WaveGrid::cflTimeStep() const {
	return std::min(m_dx[X], m_dx[Y]) / groupSpeed(gridDim(Zeta) - 1);
}

std::pair<dls::math::vec3f, dls::math::vec3f> WaveGrid::waterSurface(dls::math::vec2f pos) const {

	auto surface = dls::math::vec3f{0, 0, 0};
	auto tx      = dls::math::vec3f{0, 0, 0};
	auto ty      = dls::math::vec3f{0, 0, 0};

	for (int izeta = 0; izeta < gridDim(Zeta); izeta++) {
		float  zeta    = idxToPos(izeta, Zeta);
		auto &profile = m_profileBuffers[izeta];

		int  DIR_NUM = gridDim(Theta);
		int  N       = 4 * DIR_NUM;
		float da      = 1.0 / N;
		float dx      = DIR_NUM * tau / N;
		for (float a = 0; a < 1; a += da) {

			float angle  = a * tau;
			auto kdir   = dls::math::vec2f{cosf(angle), sinf(angle)};
			float kdir_x = produit_scalaire(kdir, pos);

			dls::math::vec4f wave_data =
					dx * amplitude(dls::math::vec4f{pos[X], pos[Y], angle, zeta}) * profile(kdir_x);

			surface +=
					dls::math::vec3f{kdir[0] * wave_data[0], kdir[1] * wave_data[0], wave_data[1]};

		tx += kdir[0] * dls::math::vec3f{wave_data[2], 0, wave_data[3]};
		ty += kdir[1] * dls::math::vec3f{0, wave_data[2], wave_data[3]};
	}
}

auto normal = normalise(produit_croix(tx, ty));

return {surface, normal};
}

auto WaveGrid::extendedGrid() const {
	return [this](int ix, int iy, int itheta, int izeta) {
		// wrap arround for angle
		itheta = pos_modulo(itheta, gridDim(Theta));

		// return zero for wavenumber outside of a domain
		if (izeta < 0 || izeta >= gridDim(Zeta)) {
			return 0.0f;
		}

		// return a default value for points outside of the simulation box
		if (ix < 0 || ix >= gridDim(X) || iy < 0 || iy >= gridDim(Y)) {
			return defaultAmplitude(itheta, izeta);
		}

		// if the point is in the domain the return the actual value of the grid
		return m_amplitude(ix, iy, itheta, izeta);
	};
}

auto WaveGrid::interpolatedAmplitude() const {

	auto extended_grid = extendedGrid();

	// This function indicated which grid points are in domain and which are not
	auto domain = [this](int ix, int iy, int itheta, int izeta) -> bool {
		return m_enviroment.inDomain(nodePosition(ix, iy));
	};

	auto interpolation = InterpolationDimWise(
				// CubicInterpolation, CubicInterpolation,
				LinearInterpolation, LinearInterpolation, LinearInterpolation,
				ConstantInterpolation);

	auto interpolated_grid =
			DomainInterpolation(interpolation, domain)(extended_grid);

	return [interpolated_grid, this](dls::math::vec4f pos4) mutable {
		dls::math::vec4f ipos4 = posToGrid(pos4);
		return interpolated_grid(ipos4[X], ipos4[Y], ipos4[Theta], ipos4[Zeta]);
	};
}

float WaveGrid::amplitude(dls::math::vec4f pos4) const
{
	return interpolatedAmplitude()(pos4);
}

float WaveGrid::gridValue(Idx idx4) const {
	return extendedGrid()(idx4[X], idx4[Y], idx4[Theta], idx4[Zeta]);
}

std::vector<dls::math::vec4f> WaveGrid::trajectory(dls::math::vec4f pos4, float length) const {

	std::vector<dls::math::vec4f> trajectory;
	float              dist = 0;

	for (float dist = 0; dist <= length;) {

		trajectory.push_back(pos4);

		dls::math::vec2f vel = groupVelocity(pos4);
		float dt  = dx(X) / longueur(vel);

		pos4[X] += dt * vel[X];
		pos4[Y] += dt * vel[Y];

		pos4 = boundaryReflection(pos4);

		dist += dt * longueur(vel);
	}
	trajectory.push_back(pos4);
	return trajectory;
}

void WaveGrid::addPointDisturbance(const dls::math::vec2f pos, const float val) {
	// Find the closest point on the grid to the point `pos`
	int ix = posToIdx(pos[X], X);
	int iy = posToIdx(pos[Y], Y);
	if (ix >= 0 && ix < gridDim(X) && iy >= 0 && iy < gridDim(Y)) {

		for (int itheta = 0; itheta < gridDim(Theta); itheta++) {
			m_amplitude(ix, iy, itheta, 0) += val;
		}
	}
}

void WaveGrid::advectionStep(const float dt) {

	auto amplitude = interpolatedAmplitude();

	//#pragma omp parallel for collapse(2)
	for (int ix = 0; ix < gridDim(X); ix++) {
		for (int iy = 0; iy < gridDim(Y); iy++) {

			dls::math::vec2f pos = nodePosition(ix, iy);

			// update only points in the domain
			if (m_enviroment.inDomain(pos)) {

				for (int itheta = 0; itheta < gridDim(Theta); itheta++) {
					for (int izeta = 0; izeta < gridDim(Zeta); izeta++) {

						dls::math::vec4f pos4 = idxToPos({ix, iy, itheta, izeta});
						dls::math::vec2f vel  = groupVelocity(pos4);

						// Tracing back in Semi-Lagrangian
						dls::math::vec4f trace_back_pos4 = pos4;
						trace_back_pos4[X] -= dt * vel[X];
						trace_back_pos4[Y] -= dt * vel[Y];

						// Take care of boundaries
						trace_back_pos4 = boundaryReflection(trace_back_pos4);

						m_newAmplitude(ix, iy, itheta, izeta) = amplitude(trace_back_pos4);
					}
				}
			}
		}
	}

	std::swap(m_newAmplitude, m_amplitude);
}

dls::math::vec4f WaveGrid::boundaryReflection(const dls::math::vec4f pos4) const {
	dls::math::vec2f pos = dls::math::vec2f{pos4[X], pos4[Y]};
	float ls  = m_enviroment.levelset(pos);
	if (ls >= 0) // no reflection is needed if point is in the domain
		return pos4;

	// Boundary normal is approximatex by the levelset gradient
	dls::math::vec2f n = m_enviroment.levelsetGrad(pos);

	float theta = pos4[Theta];
	dls::math::vec2f kdir  = dls::math::vec2f{cosf(theta), sinf(theta)};

	// Reflect point and wave-vector direction around boundary
	// Here we rely that `ls` is equal to the signed distance from the boundary
	pos  = pos - 2.0f * ls * n;
	kdir = kdir - 2.0f * (kdir * n) * n;

	float reflected_theta = atan2(kdir[Y], kdir[X]);

	// We are assuming that after one reflection you are back in the domain. This
	// assumption is valid if you boundary is not so crazy.
	// This assert tests this assumption.
	assert(m_enviroment.inDomain(pos));

	return dls::math::vec4f{pos[X], pos[Y], reflected_theta, pos4[Zeta]};
}

void WaveGrid::diffusionStep(const float dt) {

	auto grid = extendedGrid();

	//#pragma omp parallel for collapse(2)
	for (int ix = 0; ix < gridDim(X); ix++) {
		for (int iy = 0; iy < gridDim(Y); iy++) {

			float ls = m_enviroment.levelset(nodePosition(ix, iy));

			for (int itheta = 0; itheta < gridDim(Theta); itheta++) {
				for (int izeta = 0; izeta < gridDim(Zeta); izeta++) {

					dls::math::vec4f pos4  = idxToPos({ix, iy, itheta, izeta});
					float gamma = 2*0.025 * groupSpeed(izeta) * dt * m_idx[X];

					// do diffusion only if you are 2 grid nodes away from boudnary
					if (ls >= 4 * dx(X)) {
						m_newAmplitude(ix, iy, itheta, izeta) =
								(1 - gamma) * grid(ix, iy, itheta, izeta) +
								gamma * 0.5 *
								(grid(ix, iy, itheta + 1, izeta) +
								 grid(ix, iy, itheta - 1, izeta));
					} else {
						m_newAmplitude(ix, iy, itheta, izeta) = grid(ix, iy, itheta, izeta);
					}
					// auto dispersion = [](int i) { return 1.0; };
					// float delta =
					//     1e-5 * dt * pow(m_dx[3], 2) * dispersion(waveNumber(izeta));
					// 0.5 * delta *
					//     (m_amplitude(ix, iy, itheta, izeta + 1) +
					//      m_amplitude(ix, iy, itheta, izeta + 1));
				}
			}
		}
	}
	std::swap(m_newAmplitude, m_amplitude);
}

void WaveGrid::precomputeProfileBuffers() {

	for (int izeta = 0; izeta < gridDim(Zeta); izeta++) {

		float zeta_min = idxToPos(izeta, Zeta) - 0.5 * dx(Zeta);
		float zeta_max = idxToPos(izeta, Zeta) + 0.5 * dx(Zeta);

		// define spectrum

		m_profileBuffers[izeta].precompute(m_spectrum, m_time, zeta_min, zeta_max);
	}
}

void WaveGrid::precomputeGroupSpeeds() {
	m_groupSpeeds.resize(gridDim(Zeta));
	for (int izeta = 0; izeta < gridDim(Zeta); izeta++) {

		float zeta_min = idxToPos(izeta, Zeta) - 0.5 * dx(Zeta);
		float zeta_max = idxToPos(izeta, Zeta) + 0.5 * dx(Zeta);

		auto result = integrate(100, zeta_min, zeta_max, [&](float zeta) -> dls::math::vec2f {
			float waveLength = pow(2, zeta);
			float waveNumber = tau / waveLength;
			float cg         = 0.5 * sqrt(9.81 / waveNumber);
			float density    = m_spectrum(zeta);
			return dls::math::vec2f{cg * density, density};
		});

		m_groupSpeeds[izeta] =
				3 /*the 3 should not be here !!!*/ * result[0] / result[1];
	}
}

float WaveGrid::idxToPos(const int idx, const int dim) const {
	return m_xmin[dim] + (idx + 0.5) * m_dx[dim];
}

dls::math::vec4f WaveGrid::idxToPos(const Idx idx) const {
	return dls::math::vec4f{idxToPos(idx[X], X), idxToPos(idx[Y], Y),
				idxToPos(idx[Theta], Theta), idxToPos(idx[Zeta], Zeta)};
}

float WaveGrid::posToGrid(const float pos, const int dim) const {
	return (pos - m_xmin[dim]) * m_idx[dim] - 0.5;
}

dls::math::vec4f WaveGrid::posToGrid(const dls::math::vec4f pos4) const {
	return dls::math::vec4f{posToGrid(pos4[X], X), posToGrid(pos4[Y], Y),
				posToGrid(pos4[Theta], Theta), posToGrid(pos4[Zeta], Zeta)};
}

int WaveGrid::posToIdx(const float pos, const int dim) const {
	return round(posToGrid(pos, dim));
}

WaveGrid::Idx WaveGrid::posToIdx(const dls::math::vec4f pos4) const {
	return Idx{posToIdx(pos4[X], X), posToIdx(pos4[Y], Y),
				posToIdx(pos4[Theta], Theta), posToIdx(pos4[Zeta], Zeta)};
}

dls::math::vec2f WaveGrid::nodePosition(int ix, int iy) const {
	return dls::math::vec2f{idxToPos(ix, 0), idxToPos(iy, 1)};
}

float WaveGrid::waveLength(int izeta) const {
	float zeta = idxToPos(izeta, Zeta);
	return pow(2, zeta);
}

float WaveGrid::waveNumber(int izeta) const { return tau / waveLength(izeta); }

float WaveGrid::dispersionRelation(float knum) const {
	const float g = 9.81;
	return sqrt(knum * g);
}

float WaveGrid::dispersionRelation(dls::math::vec4f pos4) const {
	float knum = waveNumber(pos4[Zeta]);
	return dispersionRelation(knum);
}

// float WaveGrid::groupSpeed(dls::math::vec4f pos4) const {
//   float       knum = waveNumber(pos4[Zeta]);
//   const float g    = 9.81;
//   return 0.5 * sqrt(g / knum);
// }

float WaveGrid::groupSpeed(int izeta) const { return m_groupSpeeds[izeta]; }

dls::math::vec2f WaveGrid::groupVelocity(dls::math::vec4f pos4) const {
	int  izeta = posToIdx(pos4[Zeta], Zeta);
	float cg    = groupSpeed(izeta);
	float theta = pos4[Theta];
	return cg * dls::math::vec2f{cosf(theta), sinf(theta)};
}

float WaveGrid::defaultAmplitude(const int itheta, const int izeta) const {
	if (itheta == 5 * gridDim(Theta) / 16)
		return 0.1;
	return 0.0;
}

int WaveGrid::gridDim(const int dim) const {
	return m_amplitude.dimension(dim);
}

float WaveGrid::dx(int dim) const { return m_dx[dim]; }

} // namespace WaterWavelets
#endif

class OperatriceVagueletteOcean : public OperatriceCorps {
#ifdef VAGUELETTE_OCEAN
	WaterWavelets::WaveGrid *m_grille = nullptr;
#endif

public:
	static constexpr auto NOM = "Vaguelette Océan";
	static constexpr auto AIDE = "";

	explicit OperatriceVagueletteOcean(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	~OperatriceVagueletteOcean() override
	{
#ifdef VAGUELETTE_OCEAN
		delete m_grille;
#endif
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		if (m_corps.points_pour_lecture()->taille() == 0) {
			this->ajoute_avertissement("Le corps d'entrée est vide");
			return EXECUTION_ECHOUEE;
		}

		if (contexte.temps_courant == contexte.temps_debut) {
			reinitialise();
		}

		this->ajoute_avertissement("Implémentation non-terminée");

#ifdef VAGUELETTE_OCEAN
		auto dt = m_grille->cflTimeStep();
		m_grille->timeStep(dt);

		auto points = m_corps.points();

		for (auto i = 0; i < points->taille(); ++i) {
			auto p = points->point(i);

			for (auto i = 0; i < 8; ++i) {
				float theta = m_grille->idxToPos(i, WaterWavelets::WaveGrid::Theta);
				auto pos4 = dls::math::vec4f(p.x / 50.0f, p.z / 50.0f, theta, m_grille->idxToPos(0, WaterWavelets::WaveGrid::Zeta));
				p.y += m_grille->amplitude(pos4);
			}

			points->point(i, p);
		}
#else
		this->ajoute_avertissement("Compilé sans Vaguelette Océan");
#endif

		return EXECUTION_REUSSIE;
	}

	void parametres_changes() override
	{
		reinitialise();
	}

	void reinitialise()
	{
#ifdef VAGUELETTE_OCEAN
		delete m_grille;

		WaterWavelets::WaveGrid::Settings s;
		s.size = 50;

		s.n_x = 100;
		s.n_theta = 8;
		s.n_zeta = 1;

		s.spectrumType = WaterWavelets::WaveGrid::Settings::PiersonMoskowitz;

		m_grille = new WaterWavelets::WaveGrid(s);
#endif
	}
};

/* ************************************************************************** */

void enregistre_operatrices_ocean(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceSimulationOcean>());
	usine.enregistre_type(cree_desc<OperatriceVagueletteOcean>());
}

#pragma clang diagnostic pop
