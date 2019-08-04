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
auto catrom(T p0, T p1, T p2, T p3, T f)
{
	return 0.5 * ((2.0 * p1) +
				  (-p0 + p2) * f +
				  (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * f * f +
				  (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * f * f * f);
}

template <typename T>
auto omega(T k, T depth, T gravite)
{
	return std::sqrt(gravite * k * std::tanh(k * depth));
}

/* Spectre de Phillips modifié. */
template <typename T>
auto spectre_phillips(Ocean *o, T kx, T kz)
{
	auto k2 = kx * kx + kz * kz;

	if (dls::math::est_environ_zero(k2)) {
		/* no DC component */
		return 0.0;
	}

	/* damp out the waves going in the direction opposite the wind */
	auto tmp = (static_cast<T>(o->vent_x) * kx + static_cast<T>(o->vent_z) * kz) / std::sqrt(k2);

	if (tmp < 0) {
		tmp *= static_cast<T>(o->reflections_damp);
	}

	return static_cast<T>(o->amplitude) * std::exp(-1.0 / (k2 * static_cast<T>(o->L * o->L))) * std::exp(-k2 * static_cast<T>(o->l * o->l)) *
			std::pow(std::abs(tmp), o->alignement_vent) / (k2 * k2);
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

/*
 * instead of Complex.h
 * in fftw.h "fftw_complex" typedefed as double[2]
 * below you can see functions are needed to work with such complex numbers.
 * */
static void init_complex(fftw_complex cmpl, double real, double image)
{
	cmpl[0] = real;
	cmpl[1] = image;
}

static float ocean_jminus_vers_ecume(float jminus, float coverage)
{
	float foam = jminus * -0.005f + coverage;
	foam = dls::math::restreint(foam, 0.0f, 1.0f);
	return foam * foam;
}

static void evalue_ocean_uv(Ocean *oc, OceanResult *ocr, float u, float v)
{
	int i0, i1, j0, j1;
	float frac_x, frac_z;
	float uu, vv;

	/* first wrap the texture so 0 <= (u, v) < 1 */
	u = fmodf(u, 1.0f);
	v = fmodf(v, 1.0f);

	if (u < 0) u += 1.0f;
	if (v < 0) v += 1.0f;

	uu = u * static_cast<float>(oc->res_x);
	vv = v * static_cast<float>(oc->res_y);

	i0 = static_cast<int>(std::floor(uu));
	j0 = static_cast<int>(std::floor(vv));

	i1 = (i0 + 1);
	j1 = (j0 + 1);

	frac_x = uu - static_cast<float>(i0);
	frac_z = vv - static_cast<float>(j0);

	i0 = i0 % oc->res_x;
	j0 = j0 % oc->res_y;

	i1 = i1 % oc->res_x;
	j1 = j1 % oc->res_y;

	auto bilerp = [&](dls::tableau<double> const &m)
	{
		return static_cast<float>(dls::math::entrepolation_bilineaire(
									  m[i1 * oc->res_y + j1],
								  m[i0 * oc->res_y + j1],
				m[i1 * oc->res_y + j0],
				m[i0 * oc->res_y + j0], static_cast<double>(frac_x), static_cast<double>(frac_z)));
	};

	{
		if (oc->calcul_deplacement_y) {
			ocr->disp[1] = bilerp(oc->disp_y);
		}

		if (oc->calcul_normaux) {
			ocr->normal[0] = bilerp(oc->N_x);
			ocr->normal[1] = static_cast<float>(oc->N_y);
			ocr->normal[2] = bilerp(oc->N_z);
		}

		if (oc->calcul_chop) {
			ocr->disp[0] = bilerp(oc->disp_x);
			ocr->disp[2] = bilerp(oc->disp_z);
		}
		else {
			ocr->disp[0] = 0.0;
			ocr->disp[2] = 0.0;
		}

		if (oc->calcul_ecume) {
			compute_eigenstuff(ocr, bilerp(oc->Jxx), bilerp(oc->Jzz), bilerp(oc->Jxz));
		}
	}
}

/* use catmullrom interpolation rather than linear */
static void evalue_ocean_uv_catrom(Ocean *oc, OceanResult *ocr, float u, float v)
{
	int i0, i1, i2, i3, j0, j1, j2, j3;
	float frac_x, frac_z;
	float uu, vv;

	/* first wrap the texture so 0 <= (u, v) < 1 */
	u = std::fmod(u, 1.0f);
	v = std::fmod(v, 1.0f);

	if (u < 0) u += 1.0f;
	if (v < 0) v += 1.0f;

	uu = u * static_cast<float>(oc->res_x);
	vv = v * static_cast<float>(oc->res_y);

	i1 = static_cast<int>(std::floor(uu));
	j1 = static_cast<int>(std::floor(vv));

	i2 = (i1 + 1);
	j2 = (j1 + 1);

	frac_x = uu - static_cast<float>(i1);
	frac_z = vv - static_cast<float>(j1);

	i1 = i1 % oc->res_x;
	j1 = j1 % oc->res_y;

	i2 = i2 % oc->res_x;
	j2 = j2 % oc->res_y;

	i0 = (i1 - 1);
	i3 = (i2 + 1);
	i0 = i0 <   0 ? i0 + oc->res_x : i0;
	i3 = i3 >= oc->res_x ? i3 - oc->res_x : i3;

	j0 = (j1 - 1);
	j3 = (j2 + 1);
	j0 = j0 <   0 ? j0 + oc->res_y : j0;
	j3 = j3 >= oc->res_y ? j3 - oc->res_y : j3;

	auto interp = [&](dls::tableau<double> const &m)
	{
		return static_cast<float>(catrom(catrom(m[i0 * oc->res_y + j0], m[i1 * oc->res_y + j0], \
								  m[i2 * oc->res_y + j0], m[i3 * oc->res_y + j0], static_cast<double>(frac_x)), \
				catrom(m[i0 * oc->res_y + j1], m[i1 * oc->res_y + j1], \
				m[i2 * oc->res_y + j1], m[i3 * oc->res_y + j1], static_cast<double>(frac_x)), \
				catrom(m[i0 * oc->res_y + j2], m[i1 * oc->res_y + j2], \
				m[i2 * oc->res_y + j2], m[i3 * oc->res_y + j2], static_cast<double>(frac_x)), \
				catrom(m[i0 * oc->res_y + j3], m[i1 * oc->res_y + j3], \
				m[i2 * oc->res_y + j3], m[i3 * oc->res_y + j3], static_cast<double>(frac_x)), \
				static_cast<double>(frac_z)));
	};

	{
		if (oc->calcul_deplacement_y) {
			ocr->disp[1] = interp(oc->disp_y);
		}
		if (oc->calcul_normaux) {
			ocr->normal[0] = interp(oc->N_x);
			ocr->normal[1] = static_cast<float>(oc->N_y);
			ocr->normal[2] = interp(oc->N_z);
		}
		if (oc->calcul_chop) {
			ocr->disp[0] = interp(oc->disp_x);
			ocr->disp[2] = interp(oc->disp_z);
		}
		else {
			ocr->disp[0] = 0.0;
			ocr->disp[2] = 0.0;
		}

		if (oc->calcul_ecume) {
			compute_eigenstuff(ocr, interp(oc->Jxx), interp(oc->Jzz), interp(oc->Jxz));
		}
	}
}

/* note that this doesn't wrap properly for i, j < 0, but its not really meant for that being just a way to get
 * the raw data out to save in some image format.
 */
static void evalue_ocean_ij(Ocean *oc, OceanResult *ocr, int i, int j)
{
	i = abs(i) % oc->res_x;
	j = abs(j) % oc->res_y;

	ocr->disp[1] = oc->calcul_deplacement_y ? static_cast<float>(oc->disp_y[i * oc->res_y + j]) : 0.0f;

	if (oc->calcul_chop) {
		ocr->disp[0] = static_cast<float>(oc->disp_x[i * oc->res_y + j]);
		ocr->disp[2] = static_cast<float>(oc->disp_z[i * oc->res_y + j]);
	}
	else {
		ocr->disp[0] = 0.0f;
		ocr->disp[2] = 0.0f;
	}

	if (oc->calcul_normaux) {
		ocr->normal[0] = static_cast<float>(oc->N_x[i * oc->res_y + j]);
		ocr->normal[1] = static_cast<float>(oc->N_y);
		ocr->normal[2] = static_cast<float>(oc->N_z[i * oc->res_y + j]);

		ocr->normal = normalise(ocr->normal);
	}

	if (oc->calcul_ecume) {
		compute_eigenstuff(ocr,
						   static_cast<float>(oc->Jxx[i * oc->res_y + j]),
				static_cast<float>(oc->Jzz[i * oc->res_y + j]), static_cast<float>(oc->Jxz[i * oc->res_y + j]));
	}
}

static void simule_ocean(Ocean *o, double t, double scale, double chop_amount, double gravite)
{
	scale *= o->facteur_normalisation;

	o->N_y = 1.0 / static_cast<double>(scale);

	boucle_parallele(tbb::blocked_range<int>(0, o->res_x),
					 [&](tbb::blocked_range<int> const &plage)
	{
		auto gna = GNA(o->graine + plage.begin());

		for (int i = plage.begin(); i < plage.end(); ++i) {
			/* note the <= _N/2 here, see the fftw doco about the mechanics of the complex->real fft storage */
			for (int j = 0; j <= o->res_y / 2; ++j) {
				auto index = i * (1 + o->res_y / 2) + j;

				auto r1 = gaussRand(gna, 0.0, 1.0);
				auto k = std::sqrt(o->kx[i] * o->kx[i] + o->kz[j] * o->kz[j]);

				auto r1r2 = dls::math::complexe(r1.x, r1.y);

				auto h0 = r1r2 * std::sqrt(spectre_phillips(o, o->kx[i], o->kz[j]) / 2.0);
				auto h0_minus = r1r2 * std::sqrt(spectre_phillips(o, -o->kx[i], -o->kz[j]) / 2.0);

				auto exp_param1 = dls::math::complexe(0.0, omega(k, o->profondeur, gravite) * t);
				auto exp_param2 = dls::math::complexe(0.0, -omega(k, o->profondeur, gravite) * t);
				exp_param1 = dls::math::exp(exp_param1);
				exp_param2 = dls::math::exp(exp_param2);

				auto conj_param = dls::math::conjugue(h0_minus);

				exp_param1 = h0 * exp_param1;
				exp_param2 = conj_param * exp_param2;

				auto htilda = exp_param1 + exp_param2;
				o->fft_in[index] = htilda * scale;

				if (o->calcul_chop) {
					if (k == 0.0) {
						o->fft_in_x[index] = dls::math::complexe(0.0, 0.0);
						o->fft_in_z[index] = dls::math::complexe(0.0, 0.0);
					}
					else {
						auto kx = o->kx[i] / k;
						auto kz = o->kz[j] / k;

						auto mul_param = dls::math::complexe(0.0, -1.0);
						mul_param *= chop_amount;

						auto minus_i = dls::math::complexe(-scale, 0.0);
						mul_param *= minus_i;
						mul_param *= htilda;

						o->fft_in_x[index] = mul_param * kx;
						o->fft_in_z[index] = mul_param * kz;
					}
				}

				if (o->calcul_normaux) {
					auto mul_param = dls::math::complexe(0.0, -1.0);
					mul_param *= htilda;

					o->fft_in_nx[index] = mul_param * o->kx[i];
					o->fft_in_nz[index] = mul_param * o->kz[i];
				}

				if (o->calcul_ecume) {
					if (k == 0.0) {
						o->fft_in_jxx[index] = dls::math::complexe(0.0, 0.0);
						o->fft_in_jzz[index] = dls::math::complexe(0.0, 0.0);
						o->fft_in_jxz[index] = dls::math::complexe(0.0, 0.0);
					}
					else {
						/* init_complex(mul_param, -scale, 0); */
						auto mul_param = dls::math::complexe(-1.0, 0.0);
						mul_param *= chop_amount;
						mul_param *= htilda;

						/* calcul jacobien XX */
						auto kxx = o->kx[i] * o->kx[i] / k;
						o->fft_in_jxx[index] = mul_param * kxx;

						/* calcul jacobien ZZ */
						auto kzz = o->kz[j] * o->kz[j] / k;
						o->fft_in_jzz[index] = mul_param * kzz;

						/* calcul jacobien XZ */
						auto kxz = o->kx[i] * o->kz[j] / k;
						o->fft_in_jxz[index] = mul_param * kxz;
					}
				}
			}
		}
	});

	dls::tableau<fftw_plan> plans;

	if (!o->disp_y.est_vide()) {
		plans.pousse(o->disp_y_plan);
	}

	if (o->calcul_chop) {
		plans.pousse(o->disp_x_plan);
		plans.pousse(o->disp_z_plan);
	}

	if (o->calcul_normaux) {
		plans.pousse(o->N_x_plan);
		plans.pousse(o->N_z_plan);
	}

	if (o->calcul_ecume) {
		plans.pousse(o->Jxx_plan);
		plans.pousse(o->Jzz_plan);
		plans.pousse(o->Jxz_plan);
	}

	tbb::parallel_for(0l, plans.taille(),
					  [&](long i)
	{
		fftw_execute(plans[i]);
	});

	if (o->calcul_ecume) {
		for (auto i = 0; i < o->res_x * o->res_y; ++i) {
			o->Jxx[i] += 1.0;
			o->Jzz[i] += 1.0;
		}
	}
}

static void set_height_normalize_factor(Ocean *oc, double gravite)
{
	auto max_h = 0.0;

	if (!oc->calcul_deplacement_y) {
		return;
	}

	oc->facteur_normalisation = 1.0;

	simule_ocean(oc, 0.0, 1.0, 0, gravite);

	for (int i = 0; i < oc->res_x; ++i) {
		for (int j = 0; j < oc->res_y; ++j) {
			if (max_h < std::fabs(oc->disp_y[i * oc->res_y + j])) {
				max_h = std::fabs(oc->disp_y[i * oc->res_y + j]);
			}
		}
	}

	if (max_h == 0.0) {
		max_h = 0.00001;  /* just in case ... */
	}

	oc->facteur_normalisation = 1.0 / (max_h);
}

static void initialise_donnees_ocean(
		Ocean *o,
		double gravite)
{
	auto taille = (o->res_x * o->res_y);
	auto taille_complex = (o->res_x * (1 + o->res_y / 2));

	o->kx.redimensionne(o->res_x);
	o->kz.redimensionne(o->res_y);

	/* make this robust in the face of erroneous usage */
	if (o->taille_spaciale_x == 0.0) {
		o->taille_spaciale_x = 0.001;
	}

	if (o->taille_spaciale_z == 0.0) {
		o->taille_spaciale_z = 0.001;
	}

	/* the +ve components and DC */
	for (auto i = 0; i <= o->res_x / 2; ++i) {
		o->kx[i] = constantes<double>::TAU * static_cast<double>(i) / o->taille_spaciale_x;
	}

	/* the -ve components */
	for (auto i = o->res_x - 1, ii = 0; i > o->res_x / 2; --i, ++ii) {
		o->kx[i] = -constantes<double>::TAU * static_cast<double>(ii) / o->taille_spaciale_x;
	}

	/* the +ve components and DC */
	for (auto i = 0; i <= o->res_y / 2; ++i) {
		o->kz[i] = constantes<double>::TAU * static_cast<double>(i) / o->taille_spaciale_z;
	}

	/* the -ve components */
	for (auto i = o->res_y - 1, ii = 0; i > o->res_y / 2; --i, ++ii) {
		o->kz[i] = -constantes<double>::TAU * static_cast<double>(ii) / o->taille_spaciale_z;
	}

	o->fft_in.redimensionne(taille_complex);

	if (o->calcul_deplacement_y) {
		o->disp_y.redimensionne(taille);
		o->disp_y_plan = cree_plan(o->res_x, o->res_y, o->fft_in, o->disp_y);
	}

	if (o->calcul_normaux) {
		o->fft_in_nx.redimensionne(taille_complex);
		o->fft_in_nz.redimensionne(taille_complex);

		o->N_x.redimensionne(taille);
		o->N_z.redimensionne(taille);

		o->N_x_plan = cree_plan(o->res_x, o->res_y, o->fft_in_nx, o->N_x);
		o->N_z_plan = cree_plan(o->res_x, o->res_y, o->fft_in_nz, o->N_z);
	}

	if (o->calcul_chop) {
		o->fft_in_x.redimensionne(taille_complex);
		o->fft_in_z.redimensionne(taille_complex);

		o->disp_x.redimensionne(taille);
		o->disp_z.redimensionne(taille);

		o->disp_x_plan = cree_plan(o->res_x, o->res_y, o->fft_in_x, o->disp_x);
		o->disp_z_plan = cree_plan(o->res_x, o->res_y, o->fft_in_z, o->disp_z);
	}

	if (o->calcul_ecume) {
		o->fft_in_jxx.redimensionne(taille_complex);
		o->fft_in_jzz.redimensionne(taille_complex);
		o->fft_in_jxz.redimensionne(taille_complex);

		o->Jxx.redimensionne(taille);
		o->Jzz.redimensionne(taille);
		o->Jxz.redimensionne(taille);

		o->Jxx_plan = cree_plan(o->res_x, o->res_y, o->fft_in_jxx, o->Jxx);
		o->Jzz_plan = cree_plan(o->res_x, o->res_y, o->fft_in_jzz, o->Jzz);
		o->Jxz_plan = cree_plan(o->res_x, o->res_y, o->fft_in_jxz, o->Jxz);
	}

	set_height_normalize_factor(o, gravite);
}

static void deloge_donnees_ocean(Ocean *oc)
{
	if (oc == nullptr) {
		return;
	}

	if (oc->calcul_deplacement_y) {
		fftw_destroy_plan(oc->disp_y_plan);
	}

	if (oc->calcul_normaux) {
		fftw_destroy_plan(oc->N_x_plan);
		fftw_destroy_plan(oc->N_z_plan);
	}

	if (oc->calcul_chop) {
		fftw_destroy_plan(oc->disp_x_plan);
		fftw_destroy_plan(oc->disp_z_plan);
	}

	if (oc->calcul_ecume) {
		fftw_destroy_plan(oc->Jxx_plan);
		fftw_destroy_plan(oc->Jzz_plan);
		fftw_destroy_plan(oc->Jxz_plan);
	}
}

/* ************************************************************************** */

Ocean::~Ocean()
{
	deloge_donnees_ocean(this);
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

		if (m_corps.points()->taille() == 0) {
			this->ajoute_avertissement("Le corps d'entrée est vide");
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
		auto quantite_chop = evalue_decimal("quantité_chop");
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
		m_ocean.calcul_chop = (quantite_chop > 0.0f);
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
			deloge_donnees_ocean(&m_ocean);
			initialise_donnees_ocean(&m_ocean, static_cast<double>(gravite));

			auto desc = desc_grille_2d{};
			desc.etendue.min = dls::math::vec2f(0.0f);
			desc.etendue.max = dls::math::vec2f(1.0f);
			desc.fenetre_donnees = desc.etendue;
			desc.taille_pixel = 1.0 / (static_cast<double>(m_ocean.res_x));
			desc.type_donnees = type_grille::R32;

			m_ecume_precedente = calque_image::construit_calque(desc);

			m_reinit = false;
		}

		simule_ocean(&m_ocean, static_cast<double>(temps), static_cast<double>(echelle_vague), static_cast<double>(quantite_chop), static_cast<double>(gravite));

		auto grille_ecume = dynamic_cast<grille_dense_2d<float> *>(m_ecume_precedente.tampon);

		/* applique les déplacements à la géométrie d'entrée */
		auto points = m_corps.points();
		auto res_x = m_ocean.res_x;
		auto res_y = m_ocean.res_y;

		auto N = m_corps.ajoute_attribut("N", type_attribut::VEC3, portee_attr::POINT);
		N->redimensionne(points->taille());

		auto C = m_corps.ajoute_attribut("C", type_attribut::VEC3, portee_attr::POINT);
		C->redimensionne(points->taille());

		OceanResult ocr;

		auto gna = GNA{graine};

		for (auto i = 0; i < points->taille(); ++i) {
			auto p = points->point(i);

			/* converti la position du point en espace grille */
			auto u = std::fmod(p.x * taille_inverse + 0.5f, 1.0f);
			auto v = std::fmod(p.z * taille_inverse + 0.5f, 1.0f);

			if (u < 0.0f) {
				u += 1.0f;
			}

			if (v < 0.0f) {
				v += 1.0f;
			}

			auto x = static_cast<int>(u * (static_cast<float>(res_x)));
			auto y = static_cast<int>(v * (static_cast<float>(res_y)));

			/* À FAIRE : échantillonage bilinéaire. */
			evalue_ocean_ij(&m_ocean, &ocr, x, y);

			p[0] += ocr.disp[0];
			p[1] += ocr.disp[1];
			p[2] += ocr.disp[2];

			points->point(i, p);
			N->vec3(i) = ocr.normal;

			if (m_ocean.calcul_ecume) {
				auto index = grille_ecume->calcul_index(dls::math::vec2i(x, y));
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

				C->vec3(i) = dls::math::vec3f(resultat_ecume, resultat_ecume, 1.0f);
			}
		}

		return EXECUTION_REUSSIE;
	}

	void parametres_changes() override
	{
		m_reinit = true;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_ocean(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceSimulationOcean>());
}

#pragma clang diagnostic pop
