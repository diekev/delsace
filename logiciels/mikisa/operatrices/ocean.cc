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

#include "ocean.hh"

#include <cmath>

#include "biblinternes/math/entrepolation.hh"
#include "biblinternes/math/outils.hh"
#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/moultfilage/boucle.hh"
#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/structures/tableau.hh"

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
		return 0.0f;
	}

	/* damp out the waves going in the direction opposite the wind */
	auto tmp = (o->vent_x * kx + o->vent_z * kz) / std::sqrt(k2);

	if (tmp < 0) {
		tmp *= o->reflections_damp;
	}

	return o->amplitude * std::exp(-1.0f / (k2 * (o->L * o->L))) * std::exp(-k2 * (o->l * o->l)) *
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
static void init_complex(fftw_complex cmpl, float real, float image)
{
	cmpl[0] = static_cast<double>(real);
	cmpl[1] = static_cast<double>(image);
}

static void add_comlex_c(fftw_complex res, fftw_complex cmpl1, fftw_complex cmpl2)
{
	res[0] = cmpl1[0] + cmpl2[0];
	res[1] = cmpl1[1] + cmpl2[1];
}

static void mul_complex_f(fftw_complex res, fftw_complex cmpl, float f)
{
	res[0] = cmpl[0] * static_cast<double>(f);
	res[1] = cmpl[1] * static_cast<double>(f);
}

static void mul_complex_c(fftw_complex res, fftw_complex cmpl1, fftw_complex cmpl2)
{
	fftwf_complex temp;
	temp[0] = static_cast<float>(cmpl1[0] * cmpl2[0] - cmpl1[1] * cmpl2[1]);
	temp[1] = static_cast<float>(cmpl1[0] * cmpl2[1] + cmpl1[1] * cmpl2[0]);
	res[0] = static_cast<double>(temp[0]);
	res[1] = static_cast<double>(temp[1]);
}

static float real_c(fftw_complex cmpl)
{
	return static_cast<float>(cmpl[0]);
}

static float image_c(fftw_complex cmpl)
{
	return static_cast<float>(cmpl[1]);
}

static void conj_complex(fftw_complex res, fftw_complex cmpl1)
{
	res[0] = cmpl1[0];
	res[1] = -cmpl1[1];
}

static void exp_complex(fftw_complex res, fftw_complex cmpl)
{
	auto r = std::exp(cmpl[0]);

	res[0] = std::cos(cmpl[1]) * r;
	res[1] = std::sin(cmpl[1]) * r;
}

float ocean_jminus_vers_ecume(float jminus, float coverage)
{
	float foam = jminus * -0.005f + coverage;
	foam = dls::math::restreint(foam, 0.0f, 1.0f);
	return foam * foam;
}

void evalue_ocean_uv(Ocean *oc, OceanResult *ocr, float u, float v)
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

	auto bilerp = [&](double *m)
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
void evalue_ocean_uv_catrom(Ocean *oc, OceanResult *ocr, float u, float v)
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

	auto interp = [&](double *m)
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

void evalue_ocean_xz(Ocean *oc, OceanResult *ocr, float x, float z)
{
	evalue_ocean_uv(oc, ocr, x / oc->taille_spaciale_x, z / oc->taille_spaciale_z);
}

void evalue_ocean_xz_catrom(Ocean *oc, OceanResult *ocr, float x, float z)
{
	evalue_ocean_uv_catrom(oc, ocr, x / oc->taille_spaciale_x, z / oc->taille_spaciale_z);
}

/* note that this doesn't wrap properly for i, j < 0, but its not really meant for that being just a way to get
 * the raw data out to save in some image format.
 */
void evalue_ocean_ij(Ocean *oc, OceanResult *ocr, int i, int j)
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

void simule_ocean(Ocean *o, float t, float scale, float chop_amount, float gravite)
{
	scale *= o->facteur_normalisation;

	o->N_y = 1.0 / static_cast<double>(scale);

	boucle_parallele(tbb::blocked_range<int>(0, o->res_x),
					 [&](tbb::blocked_range<int> const &plage)
	{
		for (int i = plage.begin(); i < plage.end(); ++i) {
			/* note the <= _N/2 here, see the fftw doco about the mechanics of the complex->real fft storage */
			for (int j = 0; j <= o->res_y / 2; ++j) {
				auto index = i * (1 + o->res_y / 2) + j;

				fftw_complex exp_param1;
				fftw_complex exp_param2;
				fftw_complex conj_param;

				init_complex(exp_param1, 0.0, omega(o->k[index], o->profondeur, gravite) * t);
				init_complex(exp_param2, 0.0, -omega(o->k[index], o->profondeur, gravite) * t);
				exp_complex(exp_param1, exp_param1);
				exp_complex(exp_param2, exp_param2);
				conj_complex(conj_param, o->h0_minus[i * o->res_y + j]);

				mul_complex_c(exp_param1, o->h0[i * o->res_y + j], exp_param1);
				mul_complex_c(exp_param2, conj_param, exp_param2);

				add_comlex_c(o->htilda[index], exp_param1, exp_param2);
				mul_complex_f(o->fft_in[index], o->htilda[index], scale);

				if (o->calcul_chop) {
					fftw_complex mul_param;
					fftw_complex minus_i;

					init_complex(minus_i, 0.0, -1.0);
					init_complex(mul_param, -scale, 0);
					mul_complex_f(mul_param, mul_param, chop_amount);
					mul_complex_c(mul_param, mul_param, minus_i);
					mul_complex_c(mul_param, mul_param, o->htilda[index]);

					auto kx = ((o->k[index] == 0.0f) ? 0.0f : o->kx[i] / o->k[index]);
					auto kz = ((o->k[index] == 0.0f) ? 0.0f : o->kz[j] / o->k[index]);

					init_complex(o->fft_in_x[index], real_c(mul_param) * kx, image_c(mul_param) * kx);
					init_complex(o->fft_in_z[index], real_c(mul_param) * kz, image_c(mul_param) * kz);
				}

				if (o->calcul_normaux) {
					fftw_complex mul_param;

					init_complex(mul_param, 0.0, -1.0);
					mul_complex_c(mul_param, mul_param, o->htilda[index]);

					init_complex(o->fft_in_nx[index], real_c(mul_param) * o->kx[i], image_c(mul_param) * o->kx[i]);
					init_complex(o->fft_in_nz[index], real_c(mul_param) * o->kz[i], image_c(mul_param) * o->kz[i]);
				}

				if (o->calcul_ecume) {
					fftw_complex mul_param;

					/* init_complex(mul_param, -scale, 0); */
					init_complex(mul_param, -1, 0);

					mul_complex_f(mul_param, mul_param, chop_amount);
					mul_complex_c(mul_param, mul_param, o->htilda[index]);

					/* calcul jacobien XX */
					auto kxx = ((o->k[index] == 0.0f) ?
									0.0f :
									o->kx[i] * o->kx[i] / o->k[index]);

					init_complex(o->fft_in_jxx[index], real_c(mul_param) * kxx, image_c(mul_param) * kxx);

					/* calcul jacobien ZZ */
					auto kzz = ((o->k[index] == 0.0f) ?
									0.0f :
									o->kz[j] * o->kz[j] / o->k[index]);

					init_complex(o->fft_in_jzz[index], real_c(mul_param) * kzz, image_c(mul_param) * kzz);

					/* calcul jacobien XZ */
					auto kxz = ((o->k[index] == 0.0f) ?
									0.0f :
									o->kx[i] * o->kz[j] / o->k[index]);

					init_complex(o->fft_in_jxz[index], real_c(mul_param) * kxz, image_c(mul_param) * kxz);
				}
			}
		}
	});

	dls::tableau<fftw_plan> plans;

	if (o->disp_y) {
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

static void set_height_normalize_factor(Ocean *oc, float gravite)
{
	auto res = 1.0;
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

	res = 1.0 / (max_h);

	oc->facteur_normalisation = static_cast<float>(res);
}

void initialise_donnees_ocean(
		Ocean *o,
		int seed,
		float gravite)
{
	auto taille = (o->res_x * o->res_y);
	auto taille_complex = (o->res_x * (1 + o->res_y / 2));

	o->k = memoire::loge_tableau<float>("k", taille_complex);
	o->h0 = memoire::loge_tableau<fftw_complex>("h0", taille);
	o->h0_minus = memoire::loge_tableau<fftw_complex>("h0_minus", taille);
	o->kx = memoire::loge_tableau<float>("kx", o->res_x);
	o->kz = memoire::loge_tableau<float>("kz", o->res_y);

	/* make this robust in the face of erroneous usage */
	if (o->taille_spaciale_x == 0.0f) {
		o->taille_spaciale_x = 0.001f;
	}

	if (o->taille_spaciale_z == 0.0f) {
		o->taille_spaciale_z = 0.001f;
	}

	/* the +ve components and DC */
	for (auto i = 0; i <= o->res_x / 2; ++i) {
		o->kx[i] = constantes<float>::TAU * static_cast<float>(i) / o->taille_spaciale_x;
	}

	/* the -ve components */
	for (auto i = o->res_x - 1, ii = 0; i > o->res_x / 2; --i, ++ii) {
		o->kx[i] = -constantes<float>::TAU * static_cast<float>(ii) / o->taille_spaciale_x;
	}

	/* the +ve components and DC */
	for (auto i = 0; i <= o->res_y / 2; ++i) {
		o->kz[i] = constantes<float>::TAU * static_cast<float>(i) / o->taille_spaciale_z;
	}

	/* the -ve components */
	for (auto i = o->res_y - 1, ii = 0; i > o->res_y / 2; --i, ++ii) {
		o->kz[i] = -constantes<float>::TAU * static_cast<float>(ii) / o->taille_spaciale_z;
	}

	/* pre-calculate the k matrix */
	for (auto i = 0; i < o->res_x; ++i) {
		for (auto j = 0; j <= o->res_y / 2; ++j) {
			o->k[i * (1 + o->res_y / 2) + j] = std::sqrt(o->kx[i] * o->kx[i] + o->kz[j] * o->kz[j]);
		}
	}

	auto gna = GNA(seed);

	for (auto i = 0; i < o->res_x; ++i) {
		for (auto j = 0; j < o->res_y; ++j) {
			auto r1 = gaussRand(gna, 0.0f, 1.0f);

			fftw_complex r1r2;
			init_complex(r1r2, r1.x, r1.y);
			mul_complex_f(o->h0[i * o->res_y + j], r1r2, std::sqrt(spectre_phillips(o, o->kx[i], o->kz[j]) / 2.0f));
			mul_complex_f(o->h0_minus[i * o->res_y + j], r1r2, std::sqrt(spectre_phillips(o, -o->kx[i], -o->kz[j]) / 2.0f));
		}
	}

	o->fft_in = memoire::loge_tableau<fftw_complex>("fft_in", taille_complex);
	o->htilda = memoire::loge_tableau<fftw_complex>("htilda", taille_complex);

	if (o->calcul_deplacement_y) {
		o->disp_y = memoire::loge_tableau<double>("disp_y", taille);
		o->disp_y_plan = fftw_plan_dft_c2r_2d(o->res_x, o->res_y, o->fft_in, o->disp_y, FFTW_ESTIMATE);
	}

	if (o->calcul_normaux) {
		o->fft_in_nx = memoire::loge_tableau<fftw_complex>("fft_in_nx", taille_complex);
		o->fft_in_nz = memoire::loge_tableau<fftw_complex>("fft_in_nz", taille_complex);

		o->N_x = memoire::loge_tableau<double>("N_x", taille);
		o->N_z = memoire::loge_tableau<double>("N_z", taille);

		o->N_x_plan = fftw_plan_dft_c2r_2d(o->res_x, o->res_y, o->fft_in_nx, o->N_x, FFTW_ESTIMATE);
		o->N_z_plan = fftw_plan_dft_c2r_2d(o->res_x, o->res_y, o->fft_in_nz, o->N_z, FFTW_ESTIMATE);
	}

	if (o->calcul_chop) {
		o->fft_in_x = memoire::loge_tableau<fftw_complex>("fft_in_x", taille_complex);
		o->fft_in_z = memoire::loge_tableau<fftw_complex>("fft_in_z", taille_complex);

		o->disp_x = memoire::loge_tableau<double>("disp_x", taille);
		o->disp_z = memoire::loge_tableau<double>("disp_z", taille);

		o->disp_x_plan = fftw_plan_dft_c2r_2d(o->res_x, o->res_y, o->fft_in_x, o->disp_x, FFTW_ESTIMATE);
		o->disp_z_plan = fftw_plan_dft_c2r_2d(o->res_x, o->res_y, o->fft_in_z, o->disp_z, FFTW_ESTIMATE);
	}

	if (o->calcul_ecume) {
		o->fft_in_jxx = memoire::loge_tableau<fftw_complex>("fft_in_jxx", taille_complex);
		o->fft_in_jzz = memoire::loge_tableau<fftw_complex>("fft_in_jzz", taille_complex);
		o->fft_in_jxz = memoire::loge_tableau<fftw_complex>("fft_in_jxz", taille_complex);

		o->Jxx = memoire::loge_tableau<double>("Jxx", taille);
		o->Jzz = memoire::loge_tableau<double>("Jzz", taille);
		o->Jxz = memoire::loge_tableau<double>("Jxz", taille);

		o->Jxx_plan = fftw_plan_dft_c2r_2d(o->res_x, o->res_y, o->fft_in_jxx, o->Jxx, FFTW_ESTIMATE);
		o->Jzz_plan = fftw_plan_dft_c2r_2d(o->res_x, o->res_y, o->fft_in_jzz, o->Jzz, FFTW_ESTIMATE);
		o->Jxz_plan = fftw_plan_dft_c2r_2d(o->res_x, o->res_y, o->fft_in_jxz, o->Jxz, FFTW_ESTIMATE);
	}

	set_height_normalize_factor(o, gravite);
}

void deloge_donnees_ocean(Ocean *oc)
{
	if (oc == nullptr) {
		return;
	}

	auto taille = (oc->res_x * oc->res_y);
	auto taille_complex = (oc->res_x * (1 + oc->res_y / 2));

	if (oc->calcul_deplacement_y) {
		fftw_destroy_plan(oc->disp_y_plan);
		memoire::deloge_tableau("disp_y", oc->disp_y, taille);
	}

	if (oc->calcul_normaux) {
		memoire::deloge_tableau("fft_in_nx", oc->fft_in_nx, taille_complex);
		memoire::deloge_tableau("fft_in_nz", oc->fft_in_nz, taille_complex);
		fftw_destroy_plan(oc->N_x_plan);
		fftw_destroy_plan(oc->N_z_plan);
		memoire::deloge_tableau("N_x", oc->N_x, taille);
		memoire::deloge_tableau("N_z", oc->N_z, taille);
	}

	if (oc->calcul_chop) {
		memoire::deloge_tableau("fft_in_x", oc->fft_in_x, taille_complex);
		memoire::deloge_tableau("fft_in_z", oc->fft_in_z, taille_complex);
		fftw_destroy_plan(oc->disp_x_plan);
		fftw_destroy_plan(oc->disp_z_plan);
		memoire::deloge_tableau("disp_x", oc->disp_x, taille);
		memoire::deloge_tableau("disp_z", oc->disp_z, taille);
	}

	if (oc->calcul_ecume) {
		memoire::deloge_tableau("fft_in_jxx", oc->fft_in_jxx, taille_complex);
		memoire::deloge_tableau("fft_in_jzz", oc->fft_in_jzz, taille_complex);
		memoire::deloge_tableau("fft_in_jxz", oc->fft_in_jxz, taille_complex);
		fftw_destroy_plan(oc->Jxx_plan);
		fftw_destroy_plan(oc->Jzz_plan);
		fftw_destroy_plan(oc->Jxz_plan);
		memoire::deloge_tableau("Jxx", oc->Jxx, taille);
		memoire::deloge_tableau("Jzz,", oc->Jzz, taille);
		memoire::deloge_tableau("Jxz", oc->Jxz, taille);
	}

	if (oc->fft_in) {
		memoire::deloge_tableau("fft_in", oc->fft_in, taille_complex);
	}

	/* check that ocean data has been initialized */
	if (oc->htilda) {
		memoire::deloge_tableau("htilda", oc->htilda, taille_complex);
		memoire::deloge_tableau("k", oc->k, taille_complex);
		memoire::deloge_tableau("h0", oc->h0, taille);
		memoire::deloge_tableau("h0_minus", oc->h0_minus, taille);
		memoire::deloge_tableau("kx", oc->kx, oc->res_x);
		memoire::deloge_tableau("kz", oc->kz, oc->res_y);
	}
}

Ocean::~Ocean()
{
	deloge_donnees_ocean(this);
}
