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

#include <delsace/math/entrepolation.hh>
#include <delsace/math/outils.hh>

#include "bibliotheques/outils/constantes.h"
#include "bibliotheques/outils/definitions.hh"
#include "bibliotheques/outils/gna.hh"

#include "bibloc/logeuse_memoire.hh"

/* ************************************************************************** */

template <typename T>
static auto est_zero(T v)
{
	if constexpr (std::is_floating_point<T>::value) {
		return std::abs(v) <= std::numeric_limits<T>::epsilon();
	}

	return v == 0;
}

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
	} while (longueur_carree >= static_cast<T>(1) || est_zero(longueur_carree));

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

/* modified Phillips spectrum */
static float Ph(Ocean *o, float kx, float kz)
{
	float tmp;
	float k2 = kx * kx + kz * kz;

	if (k2 == 0.0f) {
		return 0.0f; /* no DC component */
	}

	/* damp out the waves going in the direction opposite the wind */
	tmp = (o->_wx * kx + o->_wz * kz) / sqrtf(k2);
	if (tmp < 0) {
		tmp *= o->_damp_reflections;
	}

	return o->_A * expf(-1.0f / (k2 * (o->_L * o->_L))) * expf(-k2 * (o->_l * o->_l)) *
			powf(fabsf(tmp), o->_wind_alignment) / (k2 * k2);
}

static void compute_eigenstuff(OceanResult *ocr, float jxx, float jzz, float jxz)
{
	float a, b, qplus, qminus;
	a = jxx + jzz;
	b = std::sqrt((jxx - jzz) * (jxx - jzz) + 4 * jxz * jxz);

	ocr->Jminus = 0.5f * (a - b);
	ocr->Jplus  = 0.5f * (a + b);

	qplus  = (ocr->Jplus  - jxx) / jxz;
	qminus = (ocr->Jminus - jxx) / jxz;

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

float BKE_ocean_jminus_to_foam(float jminus, float coverage)
{
	float foam = jminus * -0.005f + coverage;
	foam = dls::math::restreint(foam, 0.0f, 1.0f);
	return foam * foam;
}

void BKE_ocean_eval_uv(Ocean *oc, OceanResult *ocr, float u, float v)
{
	int i0, i1, j0, j1;
	float frac_x, frac_z;
	float uu, vv;

	/* first wrap the texture so 0 <= (u, v) < 1 */
	u = fmodf(u, 1.0f);
	v = fmodf(v, 1.0f);

	if (u < 0) u += 1.0f;
	if (v < 0) v += 1.0f;

	/* À FAIRE : mutex océan */
	//BLI_rw_mutex_lock(&oc->oceanmutex, THREAD_LOCK_READ);

	uu = u * static_cast<float>(oc->_M);
	vv = v * static_cast<float>(oc->_N);

	i0 = static_cast<int>(std::floor(uu));
	j0 = static_cast<int>(std::floor(vv));

	i1 = (i0 + 1);
	j1 = (j0 + 1);

	frac_x = uu - static_cast<float>(i0);
	frac_z = vv - static_cast<float>(j0);

	i0 = i0 % oc->_M;
	j0 = j0 % oc->_N;

	i1 = i1 % oc->_M;
	j1 = j1 % oc->_N;

	auto bilerp = [&](double *m)
	{
		return static_cast<float>(dls::math::entrepolation_bilineaire(
									  m[i1 * oc->_N + j1],
								  m[i0 * oc->_N + j1],
				m[i1 * oc->_N + j0],
				m[i0 * oc->_N + j0], static_cast<double>(frac_x), static_cast<double>(frac_z)));
	};

	{
		if (oc->_do_disp_y) {
			ocr->disp[1] = bilerp(oc->_disp_y);
		}

		if (oc->_do_normals) {
			ocr->normal[0] = bilerp(oc->_N_x);
			ocr->normal[1] = static_cast<float>(oc->_N_y) /*BILERP(oc->_N_y) (MEM01)*/;
			ocr->normal[2] = bilerp(oc->_N_z);
		}

		if (oc->_do_chop) {
			ocr->disp[0] = bilerp(oc->_disp_x);
			ocr->disp[2] = bilerp(oc->_disp_z);
		}
		else {
			ocr->disp[0] = 0.0;
			ocr->disp[2] = 0.0;
		}

		if (oc->_do_jacobian) {
			compute_eigenstuff(ocr, bilerp(oc->_Jxx), bilerp(oc->_Jzz), bilerp(oc->_Jxz));
		}
	}

	//BLI_rw_mutex_unlock(&oc->oceanmutex);
}

/* use catmullrom interpolation rather than linear */
void BKE_ocean_eval_uv_catrom(Ocean *oc, OceanResult *ocr, float u, float v)
{
	int i0, i1, i2, i3, j0, j1, j2, j3;
	float frac_x, frac_z;
	float uu, vv;

	/* first wrap the texture so 0 <= (u, v) < 1 */
	u = std::fmod(u, 1.0f);
	v = std::fmod(v, 1.0f);

	if (u < 0) u += 1.0f;
	if (v < 0) v += 1.0f;

	//BLI_rw_mutex_lock(&oc->oceanmutex, THREAD_LOCK_READ);

	uu = u * static_cast<float>(oc->_M);
	vv = v * static_cast<float>(oc->_N);

	i1 = static_cast<int>(std::floor(uu));
	j1 = static_cast<int>(std::floor(vv));

	i2 = (i1 + 1);
	j2 = (j1 + 1);

	frac_x = uu - static_cast<float>(i1);
	frac_z = vv - static_cast<float>(j1);

	i1 = i1 % oc->_M;
	j1 = j1 % oc->_N;

	i2 = i2 % oc->_M;
	j2 = j2 % oc->_N;

	i0 = (i1 - 1);
	i3 = (i2 + 1);
	i0 = i0 <   0 ? i0 + oc->_M : i0;
	i3 = i3 >= oc->_M ? i3 - oc->_M : i3;

	j0 = (j1 - 1);
	j3 = (j2 + 1);
	j0 = j0 <   0 ? j0 + oc->_N : j0;
	j3 = j3 >= oc->_N ? j3 - oc->_N : j3;

	auto interp = [&](double *m)
	{
		return static_cast<float>(catrom(catrom(m[i0 * oc->_N + j0], m[i1 * oc->_N + j0], \
								  m[i2 * oc->_N + j0], m[i3 * oc->_N + j0], static_cast<double>(frac_x)), \
				catrom(m[i0 * oc->_N + j1], m[i1 * oc->_N + j1], \
				m[i2 * oc->_N + j1], m[i3 * oc->_N + j1], static_cast<double>(frac_x)), \
				catrom(m[i0 * oc->_N + j2], m[i1 * oc->_N + j2], \
				m[i2 * oc->_N + j2], m[i3 * oc->_N + j2], static_cast<double>(frac_x)), \
				catrom(m[i0 * oc->_N + j3], m[i1 * oc->_N + j3], \
				m[i2 * oc->_N + j3], m[i3 * oc->_N + j3], static_cast<double>(frac_x)), \
				static_cast<double>(frac_z)));
	};

	{
		if (oc->_do_disp_y) {
			ocr->disp[1] = interp(oc->_disp_y);
		}
		if (oc->_do_normals) {
			ocr->normal[0] = interp(oc->_N_x);
			ocr->normal[1] = static_cast<float>(oc->_N_y) /*INTERP(oc->_N_y) (MEM01)*/;
			ocr->normal[2] = interp(oc->_N_z);
		}
		if (oc->_do_chop) {
			ocr->disp[0] = interp(oc->_disp_x);
			ocr->disp[2] = interp(oc->_disp_z);
		}
		else {
			ocr->disp[0] = 0.0;
			ocr->disp[2] = 0.0;
		}

		if (oc->_do_jacobian) {
			compute_eigenstuff(ocr, interp(oc->_Jxx), interp(oc->_Jzz), interp(oc->_Jxz));
		}
	}

	//BLI_rw_mutex_unlock(&oc->oceanmutex);
}

void BKE_ocean_eval_xz(Ocean *oc, OceanResult *ocr, float x, float z)
{
	BKE_ocean_eval_uv(oc, ocr, x / oc->_Lx, z / oc->_Lz);
}

void BKE_ocean_eval_xz_catrom(Ocean *oc, OceanResult *ocr, float x, float z)
{
	BKE_ocean_eval_uv_catrom(oc, ocr, x / oc->_Lx, z / oc->_Lz);
}

/* note that this doesn't wrap properly for i, j < 0, but its not really meant for that being just a way to get
 * the raw data out to save in some image format.
 */
void BKE_ocean_eval_ij(Ocean *oc, OceanResult *ocr, int i, int j)
{
	//	BLI_rw_mutex_lock(&oc->oceanmutex, THREAD_LOCK_READ);

	i = abs(i) % oc->_M;
	j = abs(j) % oc->_N;

	ocr->disp[1] = oc->_do_disp_y ? static_cast<float>(oc->_disp_y[i * oc->_N + j]) : 0.0f;

	if (oc->_do_chop) {
		ocr->disp[0] = static_cast<float>(oc->_disp_x[i * oc->_N + j]);
		ocr->disp[2] = static_cast<float>(oc->_disp_z[i * oc->_N + j]);
	}
	else {
		ocr->disp[0] = 0.0f;
		ocr->disp[2] = 0.0f;
	}

	if (oc->_do_normals) {
		ocr->normal[0] = static_cast<float>(oc->_N_x[i * oc->_N + j]);
		ocr->normal[1] = static_cast<float>(oc->_N_y)  /* oc->_N_y[i * oc->_N + j] (MEM01) */;
		ocr->normal[2] = static_cast<float>(oc->_N_z[i * oc->_N + j]);

		ocr->normal = normalise(ocr->normal);
	}

	if (oc->_do_jacobian) {
		compute_eigenstuff(ocr,
						   static_cast<float>(oc->_Jxx[i * oc->_N + j]),
				static_cast<float>(oc->_Jzz[i * oc->_N + j]), static_cast<float>(oc->_Jxz[i * oc->_N + j]));
	}

	//BLI_rw_mutex_unlock(&oc->oceanmutex);
}

struct OceanSimulateData {
	Ocean *o;
	float t;
	float scale;
	float chop_amount;
	float pad;
};

static void ocean_compute_htilda(OceanSimulateData *osd, float gravite)
{
	const Ocean *o = osd->o;
	const float scale = osd->scale;
	const float t = osd->t;

	for (int i = 0; i < o->_M; ++i) {
		/* note the <= _N/2 here, see the fftw doco about the mechanics of the complex->real fft storage */
		for (int j = 0; j <= o->_N / 2; ++j) {
			fftw_complex exp_param1;
			fftw_complex exp_param2;
			fftw_complex conj_param;

			init_complex(exp_param1, 0.0, omega(o->_k[i * (1 + o->_N / 2) + j], o->_depth, gravite) * t);
			init_complex(exp_param2, 0.0, -omega(o->_k[i * (1 + o->_N / 2) + j], o->_depth, gravite) * t);
			exp_complex(exp_param1, exp_param1);
			exp_complex(exp_param2, exp_param2);
			conj_complex(conj_param, o->_h0_minus[i * o->_N + j]);

			mul_complex_c(exp_param1, o->_h0[i * o->_N + j], exp_param1);
			mul_complex_c(exp_param2, conj_param, exp_param2);

			add_comlex_c(o->_htilda[i * (1 + o->_N / 2) + j], exp_param1, exp_param2);
			mul_complex_f(o->_fft_in[i * (1 + o->_N / 2) + j], o->_htilda[i * (1 + o->_N / 2) + j], scale);
		}
	}
}

static void ocean_compute_displacement_y(OceanSimulateData *osd)
{
	const Ocean *o = osd->o;

	fftw_execute(o->_disp_y_plan);
}

static void ocean_compute_displacement_x(OceanSimulateData *osd)
{
	const Ocean *o = osd->o;
	const float scale = osd->scale;
	const float chop_amount = osd->chop_amount;
	int i, j;

	for (i = 0; i < o->_M; ++i) {
		for (j = 0; j <= o->_N / 2; ++j) {
			fftw_complex mul_param;
			fftw_complex minus_i;

			init_complex(minus_i, 0.0, -1.0);
			init_complex(mul_param, -scale, 0);
			mul_complex_f(mul_param, mul_param, chop_amount);
			mul_complex_c(mul_param, mul_param, minus_i);
			mul_complex_c(mul_param, mul_param, o->_htilda[i * (1 + o->_N / 2) + j]);
			mul_complex_f(mul_param, mul_param,
						  ((o->_k[i * (1 + o->_N / 2) + j] == 0.0f) ?
							  0.0f :
							  o->_kx[i] / o->_k[i * (1 + o->_N / 2) + j]));
			init_complex(o->_fft_in_x[i * (1 + o->_N / 2) + j], real_c(mul_param), image_c(mul_param));
		}
	}
	fftw_execute(o->_disp_x_plan);
}

static void ocean_compute_displacement_z(OceanSimulateData *osd)
{
	const Ocean *o = osd->o;
	const float scale = osd->scale;
	const float chop_amount = osd->chop_amount;
	int i, j;

	for (i = 0; i < o->_M; ++i) {
		for (j = 0; j <= o->_N / 2; ++j) {
			fftw_complex mul_param;
			fftw_complex minus_i;

			init_complex(minus_i, 0.0, -1.0);
			init_complex(mul_param, -scale, 0);
			mul_complex_f(mul_param, mul_param, chop_amount);
			mul_complex_c(mul_param, mul_param, minus_i);
			mul_complex_c(mul_param, mul_param, o->_htilda[i * (1 + o->_N / 2) + j]);
			mul_complex_f(mul_param, mul_param,
						  ((o->_k[i * (1 + o->_N / 2) + j] == 0.0f) ?
							  0.0f :
							  o->_kz[j] / o->_k[i * (1 + o->_N / 2) + j]));
			init_complex(o->_fft_in_z[i * (1 + o->_N / 2) + j], real_c(mul_param), image_c(mul_param));
		}
	}
	fftw_execute(o->_disp_z_plan);
}

static void ocean_compute_jacobian_jxx(OceanSimulateData *osd)
{
	const Ocean *o = osd->o;
	const float chop_amount = osd->chop_amount;
	int i, j;

	for (i = 0; i < o->_M; ++i) {
		for (j = 0; j <= o->_N / 2; ++j) {
			fftw_complex mul_param;

			/* init_complex(mul_param, -scale, 0); */
			init_complex(mul_param, -1, 0);

			mul_complex_f(mul_param, mul_param, chop_amount);
			mul_complex_c(mul_param, mul_param, o->_htilda[i * (1 + o->_N / 2) + j]);
			mul_complex_f(mul_param, mul_param,
						  ((o->_k[i * (1 + o->_N / 2) + j] == 0.0f) ?
							  0.0f :
							  o->_kx[i] * o->_kx[i] / o->_k[i * (1 + o->_N / 2) + j]));
			init_complex(o->_fft_in_jxx[i * (1 + o->_N / 2) + j], real_c(mul_param), image_c(mul_param));
		}
	}
	fftw_execute(o->_Jxx_plan);

	for (i = 0; i < o->_M; ++i) {
		for (j = 0; j < o->_N; ++j) {
			o->_Jxx[i * o->_N + j] += 1.0;
		}
	}
}

static void ocean_compute_jacobian_jzz(OceanSimulateData *osd)
{
	const Ocean *o = osd->o;
	const float chop_amount = osd->chop_amount;
	int i, j;

	for (i = 0; i < o->_M; ++i) {
		for (j = 0; j <= o->_N / 2; ++j) {
			fftw_complex mul_param;

			/* init_complex(mul_param, -scale, 0); */
			init_complex(mul_param, -1, 0);

			mul_complex_f(mul_param, mul_param, chop_amount);
			mul_complex_c(mul_param, mul_param, o->_htilda[i * (1 + o->_N / 2) + j]);
			mul_complex_f(mul_param, mul_param,
						  ((o->_k[i * (1 + o->_N / 2) + j] == 0.0f) ?
							  0.0f :
							  o->_kz[j] * o->_kz[j] / o->_k[i * (1 + o->_N / 2) + j]));
			init_complex(o->_fft_in_jzz[i * (1 + o->_N / 2) + j], real_c(mul_param), image_c(mul_param));
		}
	}
	fftw_execute(o->_Jzz_plan);

	for (i = 0; i < o->_M; ++i) {
		for (j = 0; j < o->_N; ++j) {
			o->_Jzz[i * o->_N + j] += 1.0;
		}
	}
}

static void ocean_compute_jacobian_jxz(OceanSimulateData *osd)
{
	const Ocean *o = osd->o;
	const float chop_amount = osd->chop_amount;
	int i, j;

	for (i = 0; i < o->_M; ++i) {
		for (j = 0; j <= o->_N / 2; ++j) {
			fftw_complex mul_param;

			/* init_complex(mul_param, -scale, 0); */
			init_complex(mul_param, -1, 0);

			mul_complex_f(mul_param, mul_param, chop_amount);
			mul_complex_c(mul_param, mul_param, o->_htilda[i * (1 + o->_N / 2) + j]);
			mul_complex_f(mul_param, mul_param,
						  ((o->_k[i * (1 + o->_N / 2) + j] == 0.0f) ?
							  0.0f :
							  o->_kx[i] * o->_kz[j] / o->_k[i * (1 + o->_N / 2) + j]));
			init_complex(o->_fft_in_jxz[i * (1 + o->_N / 2) + j], real_c(mul_param), image_c(mul_param));
		}
	}
	fftw_execute(o->_Jxz_plan);
}

static void ocean_compute_normal_x(OceanSimulateData *osd)
{
	const Ocean *o = osd->o;
	int i, j;

	for (i = 0; i < o->_M; ++i) {
		for (j = 0; j <= o->_N / 2; ++j) {
			fftw_complex mul_param;

			init_complex(mul_param, 0.0, -1.0);
			mul_complex_c(mul_param, mul_param, o->_htilda[i * (1 + o->_N / 2) + j]);
			mul_complex_f(mul_param, mul_param, o->_kx[i]);
			init_complex(o->_fft_in_nx[i * (1 + o->_N / 2) + j], real_c(mul_param), image_c(mul_param));
		}
	}
	fftw_execute(o->_N_x_plan);
}

static void ocean_compute_normal_z(OceanSimulateData *osd)
{
	const Ocean *o = osd->o;
	int i, j;

	for (i = 0; i < o->_M; ++i) {
		for (j = 0; j <= o->_N / 2; ++j) {
			fftw_complex mul_param;

			init_complex(mul_param, 0.0, -1.0);
			mul_complex_c(mul_param, mul_param, o->_htilda[i * (1 + o->_N / 2) + j]);
			mul_complex_f(mul_param, mul_param, o->_kz[i]);
			init_complex(o->_fft_in_nz[i * (1 + o->_N / 2) + j], real_c(mul_param), image_c(mul_param));
		}
	}
	fftw_execute(o->_N_z_plan);
}

void BKE_ocean_simulate(Ocean *o, float t, float scale, float chop_amount, float gravite)
{
	OceanSimulateData osd;

	scale *= o->normalize_factor;

	osd.o = o;
	osd.t = t;
	osd.scale = scale;
	osd.chop_amount = chop_amount;

	//	BLI_rw_mutex_lock(&o->oceanmutex, THREAD_LOCK_WRITE);

	/* Note about multi-threading here: we have to run a first set of computations (htilda one) before we can run
	 * all others, since they all depend on it.
	 * So we make a first parallelized forloop run for htilda, and then pack all other computations into
	 * a set of parallel tasks.
	 * This is not optimal in all cases, but remains reasonably simple and should be OK most of the time. */

	/* compute a new htilda */

	ocean_compute_htilda(&osd, gravite);

	if (o->_do_disp_y) {
		ocean_compute_displacement_y(&osd);
	}

	if (o->_do_chop) {
		ocean_compute_displacement_x(&osd);
		ocean_compute_displacement_z(&osd);
	}

	if (o->_do_jacobian) {
		ocean_compute_jacobian_jxx(&osd);
		ocean_compute_jacobian_jzz(&osd);
		ocean_compute_jacobian_jxz(&osd);
	}

	if (o->_do_normals) {
		ocean_compute_normal_x(&osd);
		ocean_compute_normal_z(&osd);
		o->_N_y = 1.0 / static_cast<double>(scale);
	}

	//	BLI_rw_mutex_unlock(&o->oceanmutex);
}

static void set_height_normalize_factor(Ocean *oc, float gravite)
{
	auto res = 1.0;
	auto max_h = 0.0;

	if (!oc->_do_disp_y) {
		return;
	}

	oc->normalize_factor = 1.0;

	BKE_ocean_simulate(oc, 0.0, 1.0, 0, gravite);

	//	BLI_rw_mutex_lock(&oc->oceanmutex, THREAD_LOCK_READ);

	for (int i = 0; i < oc->_M; ++i) {
		for (int j = 0; j < oc->_N; ++j) {
			if (max_h < std::fabs(oc->_disp_y[i * oc->_N + j])) {
				max_h = std::fabs(oc->_disp_y[i * oc->_N + j]);
			}
		}
	}

	//	BLI_rw_mutex_unlock(&oc->oceanmutex);

	if (max_h == 0.0) {
		max_h = 0.00001;  /* just in case ... */
	}

	res = 1.0 / (max_h);

	oc->normalize_factor = static_cast<float>(res);
}

Ocean *BKE_ocean_add(void)
{
	Ocean *oc = memoire::loge<Ocean>("Ocean");

	//	BLI_rw_mutex_init(&oc->oceanmutex);

	return oc;
}

bool BKE_ocean_ensure(OceanModifierData *omd)
{
	if (omd->ocean) {
		return false;
	}

	omd->ocean = BKE_ocean_add();
	BKE_ocean_init_from_modifier(omd->ocean, omd);
	return true;
}

void BKE_ocean_init_from_modifier(Ocean *ocean, OceanModifierData const *omd)
{
	short do_heightfield, do_chop, do_normals, do_jacobian;

	do_heightfield = true;
	do_chop = (omd->chop_amount > 0);
	do_normals = (omd->flag & MOD_OCEAN_GENERATE_NORMALS);
	do_jacobian = (omd->flag & MOD_OCEAN_GENERATE_FOAM);

	BKE_ocean_free_data(ocean);
	BKE_ocean_init(
				ocean,
				omd->resolution * omd->resolution,
				omd->resolution * omd->resolution,
				omd->spatial_size,
				omd->spatial_size,
				omd->wind_velocity,
				omd->smallest_wave,
				1.0,
				omd->wave_direction,
				omd->damp,
				omd->wave_alignment,
				omd->depth,
				omd->time,
				do_heightfield,
				do_chop,
				do_normals,
				do_jacobian,
				omd->seed,
				omd->gravite);
}

void BKE_ocean_init(
		Ocean *o,
		int M,
		int N,
		float Lx,
		float Lz,
		float V,
		float l,
		float A,
		float w,
		float damp,
		float alignment,
		float depth,
		float time,
		short do_height_field,
		short do_chop,
		short do_normals,
		short do_jacobian,
		int seed,
		float gravite)
{
	int i, j, ii;

	//BLI_rw_mutex_lock(&o->oceanmutex, THREAD_LOCK_WRITE);

	o->_M = M;
	o->_N = N;
	o->_V = V;
	o->_l = l;
	o->_A = A;
	o->_w = w;
	o->_damp_reflections = 1.0f - damp;
	o->_wind_alignment = alignment;
	o->_depth = depth;
	o->_Lx = Lx;
	o->_Lz = Lz;
	o->_wx = std::cos(w);
	o->_wz = -std::sin(w); /* wave direction */
	o->_L = V * V / gravite;  /* largest wave for a given velocity V */
	o->time = time;

	o->_do_disp_y = do_height_field;
	o->_do_normals = do_normals;
	o->_do_chop = do_chop;
	o->_do_jacobian = do_jacobian;

	auto taille = (o->_M * o->_N);
	auto taille_complex = (o->_M * (1 + o->_N / 2));

	o->_k = memoire::loge_tableau<float>("_k", taille_complex);
	o->_h0 = memoire::loge_tableau<fftw_complex>("_h0", taille);
	o->_h0_minus = memoire::loge_tableau<fftw_complex>("_h0_minus", taille);
	o->_kx = memoire::loge_tableau<float>("_kx", o->_M);
	o->_kz = memoire::loge_tableau<float>("_kz", o->_N);

	/* make this robust in the face of erroneous usage */
	if (o->_Lx == 0.0f)
		o->_Lx = 0.001f;

	if (o->_Lz == 0.0f)
		o->_Lz = 0.001f;

	/* the +ve components and DC */
	for (i = 0; i <= o->_M / 2; ++i)
		o->_kx[i] = constantes<float>::TAU * static_cast<float>(i) / o->_Lx;

	/* the -ve components */
	for (i = o->_M - 1, ii = 0; i > o->_M / 2; --i, ++ii)
		o->_kx[i] = -constantes<float>::TAU * static_cast<float>(ii) / o->_Lx;

	/* the +ve components and DC */
	for (i = 0; i <= o->_N / 2; ++i)
		o->_kz[i] = constantes<float>::TAU * static_cast<float>(i) / o->_Lz;

	/* the -ve components */
	for (i = o->_N - 1, ii = 0; i > o->_N / 2; --i, ++ii)
		o->_kz[i] = -constantes<float>::TAU * static_cast<float>(ii) / o->_Lz;

	/* pre-calculate the k matrix */
	for (i = 0; i < o->_M; ++i)
		for (j = 0; j <= o->_N / 2; ++j)
			o->_k[i * (1 + o->_N / 2) + j] = std::sqrt(o->_kx[i] * o->_kx[i] + o->_kz[j] * o->_kz[j]);

	auto gna = GNA(seed);

	for (i = 0; i < o->_M; ++i) {
		for (j = 0; j < o->_N; ++j) {
			auto r1 = gaussRand(gna, 0.0f, 1.0f);

			fftw_complex r1r2;
			init_complex(r1r2, r1.x, r1.y);
			mul_complex_f(o->_h0[i * o->_N + j], r1r2, std::sqrt(Ph(o, o->_kx[i], o->_kz[j]) / 2.0f));
			mul_complex_f(o->_h0_minus[i * o->_N + j], r1r2, std::sqrt(Ph(o, -o->_kx[i], -o->_kz[j]) / 2.0f));
		}
	}

	o->_fft_in = memoire::loge_tableau<fftw_complex>("_fft_in", taille_complex);
	o->_htilda = memoire::loge_tableau<fftw_complex>("_htilda", taille_complex);

	//BLI_thread_lock(LOCK_FFTW);

	if (o->_do_disp_y) {
		o->_disp_y = memoire::loge_tableau<double>("_disp_y", taille);
		o->_disp_y_plan = fftw_plan_dft_c2r_2d(o->_M, o->_N, o->_fft_in, o->_disp_y, FFTW_ESTIMATE);
	}

	if (o->_do_normals) {
		o->_fft_in_nx = memoire::loge_tableau<fftw_complex>("_fft_in_nx", taille_complex);
		o->_fft_in_nz = memoire::loge_tableau<fftw_complex>("_fft_in_nz", taille_complex);

		o->_N_x = memoire::loge_tableau<double>("_N_x", taille);
		/* o->_N_y = (float *) fftwf_malloc(o->_M * o->_N * sizeof(float)); (MEM01) */
		o->_N_z = memoire::loge_tableau<double>("_N_z", taille);

		o->_N_x_plan = fftw_plan_dft_c2r_2d(o->_M, o->_N, o->_fft_in_nx, o->_N_x, FFTW_ESTIMATE);
		o->_N_z_plan = fftw_plan_dft_c2r_2d(o->_M, o->_N, o->_fft_in_nz, o->_N_z, FFTW_ESTIMATE);
	}

	if (o->_do_chop) {
		o->_fft_in_x = memoire::loge_tableau<fftw_complex>("_fft_in_x", taille_complex);
		o->_fft_in_z = memoire::loge_tableau<fftw_complex>("_fft_in_z", taille_complex);

		o->_disp_x = memoire::loge_tableau<double>("_disp_x", taille);
		o->_disp_z = memoire::loge_tableau<double>("_disp_z", taille);

		o->_disp_x_plan = fftw_plan_dft_c2r_2d(o->_M, o->_N, o->_fft_in_x, o->_disp_x, FFTW_ESTIMATE);
		o->_disp_z_plan = fftw_plan_dft_c2r_2d(o->_M, o->_N, o->_fft_in_z, o->_disp_z, FFTW_ESTIMATE);
	}

	if (o->_do_jacobian) {
		o->_fft_in_jxx = memoire::loge_tableau<fftw_complex>("_fft_in_jxx", taille_complex);
		o->_fft_in_jzz = memoire::loge_tableau<fftw_complex>("_fft_in_jzz", taille_complex);
		o->_fft_in_jxz = memoire::loge_tableau<fftw_complex>("_fft_in_jxz", taille_complex);

		o->_Jxx = memoire::loge_tableau<double>("_Jxx", taille);
		o->_Jzz = memoire::loge_tableau<double>("_Jzz", taille);
		o->_Jxz = memoire::loge_tableau<double>("_Jxz", taille);

		o->_Jxx_plan = fftw_plan_dft_c2r_2d(o->_M, o->_N, o->_fft_in_jxx, o->_Jxx, FFTW_ESTIMATE);
		o->_Jzz_plan = fftw_plan_dft_c2r_2d(o->_M, o->_N, o->_fft_in_jzz, o->_Jzz, FFTW_ESTIMATE);
		o->_Jxz_plan = fftw_plan_dft_c2r_2d(o->_M, o->_N, o->_fft_in_jxz, o->_Jxz, FFTW_ESTIMATE);
	}

	//BLI_thread_unlock(LOCK_FFTW);

	//BLI_rw_mutex_unlock(&o->oceanmutex);

	set_height_normalize_factor(o, gravite);
}

void BKE_ocean_free_data(Ocean *oc)
{
	if (oc == nullptr) {
		return;
	}

	//	BLI_rw_mutex_lock(&oc->oceanmutex, THREAD_LOCK_WRITE);

	//	BLI_thread_lock(LOCK_FFTW);


	auto taille = (oc->_M * oc->_N);
	auto taille_complex = (oc->_M * (1 + oc->_N / 2));

	if (oc->_do_disp_y) {
		fftw_destroy_plan(oc->_disp_y_plan);
		memoire::deloge_tableau("_disp_y", oc->_disp_y, taille);
	}

	if (oc->_do_normals) {
		memoire::deloge_tableau("_fft_in_nx", oc->_fft_in_nx, taille_complex);
		memoire::deloge_tableau("_fft_in_nz", oc->_fft_in_nz, taille_complex);
		fftw_destroy_plan(oc->_N_x_plan);
		fftw_destroy_plan(oc->_N_z_plan);
		memoire::deloge_tableau("_N_x", oc->_N_x, taille);
		/*fftwf_free(oc->_N_y); (MEM01)*/
		memoire::deloge_tableau("_N_z", oc->_N_z, taille);
	}

	if (oc->_do_chop) {
		memoire::deloge_tableau("_fft_in_x", oc->_fft_in_x, taille_complex);
		memoire::deloge_tableau("_fft_in_z", oc->_fft_in_z, taille_complex);
		fftw_destroy_plan(oc->_disp_x_plan);
		fftw_destroy_plan(oc->_disp_z_plan);
		memoire::deloge_tableau("_disp_x", oc->_disp_x, taille);
		memoire::deloge_tableau("_disp_z", oc->_disp_z, taille);
	}

	if (oc->_do_jacobian) {
		memoire::deloge_tableau("_fft_in_jxx", oc->_fft_in_jxx, taille_complex);
		memoire::deloge_tableau("_fft_in_jzz", oc->_fft_in_jzz, taille_complex);
		memoire::deloge_tableau("_fft_in_jxz", oc->_fft_in_jxz, taille_complex);
		fftw_destroy_plan(oc->_Jxx_plan);
		fftw_destroy_plan(oc->_Jzz_plan);
		fftw_destroy_plan(oc->_Jxz_plan);
		memoire::deloge_tableau("_Jxx", oc->_Jxx, taille);
		memoire::deloge_tableau("_Jzz,", oc->_Jzz, taille);
		memoire::deloge_tableau("_Jxz", oc->_Jxz, taille);
	}

	//	BLI_thread_unlock(LOCK_FFTW);

	if (oc->_fft_in) {
		memoire::deloge_tableau("oc->_fft_in", oc->_fft_in, taille_complex);
	}

	/* check that ocean data has been initialized */
	if (oc->_htilda) {
		memoire::deloge_tableau("_htilda", oc->_htilda, taille_complex);
		memoire::deloge_tableau("_k", oc->_k, taille_complex);
		memoire::deloge_tableau("_h0", oc->_h0, taille);
		memoire::deloge_tableau("_h0_minus", oc->_h0_minus, taille);
		memoire::deloge_tableau("_kx", oc->_kx, oc->_M);
		memoire::deloge_tableau("_kz", oc->_kz, oc->_N);
	}

	//	BLI_rw_mutex_unlock(&oc->oceanmutex);
}

void BKE_ocean_free(Ocean *oc)
{
	if (!oc) return;

	BKE_ocean_free_data(oc);
	//BLI_rw_mutex_end(&oc->oceanmutex);

	memoire::deloge("Ocean", oc);
}

/* ********* Baking/Caching ********* */


#define CACHE_TYPE_DISPLACE 1
#define CACHE_TYPE_FOAM     2
#define CACHE_TYPE_NORMAL   3
#define FILE_MAX 1024

//static void cache_filename(char *string, const char *path, const char *relbase, int frame, int type)
//{
//	char cachepath[FILE_MAX];
//	const char *fname;

//	INUTILISE(string);
//	INUTILISE(path);
//	INUTILISE(relbase);
//	INUTILISE(frame);
//	INUTILISE(cachepath);

//	switch (type) {
//		case CACHE_TYPE_FOAM:
//			fname = "foam_";
//			break;
//		case CACHE_TYPE_NORMAL:
//			fname = "normal_";
//			break;
//		case CACHE_TYPE_DISPLACE:
//		default:
//			fname = "disp_";
//			break;
//	}

//	BLI_join_dirfile(cachepath, sizeof(cachepath), path, fname);

//	BKE_image_path_from_imtype(string, cachepath, relbase, frame, R_IMF_IMTYPE_OPENEXR, true, true, "");
//}

/* silly functions but useful to inline when the args do a lot of indirections */
void rgb_to_rgba_unit_alpha(float r_rgba[4], const float rgb[3])
{
	r_rgba[0] = rgb[0];
	r_rgba[1] = rgb[1];
	r_rgba[2] = rgb[2];
	r_rgba[3] = 1.0f;
}

void value_to_rgba_unit_alpha(float r_rgba[4], const float value)
{
	r_rgba[0] = value;
	r_rgba[1] = value;
	r_rgba[2] = value;
	r_rgba[3] = 1.0f;
}

void BKE_ocean_free_cache(OceanCache *och)
{
	int i, f = 0;

	if (!och) return;

	if (och->ibufs_disp) {
		for (i = och->start, f = 0; i <= och->end; i++, f++) {
			if (och->ibufs_disp[f]) {
				//delete och->ibufs_disp[f];
			}
		}

		memoire::deloge_tableau("ibufs_disp", och->ibufs_disp, 0);
	}

	if (och->ibufs_foam) {
		for (i = och->start, f = 0; i <= och->end; i++, f++) {
			if (och->ibufs_foam[f]) {
				//delete och->ibufs_foam[f];
			}
		}

		memoire::deloge_tableau("ibufs_foam", och->ibufs_foam, 0);
	}

	if (och->ibufs_norm) {
		for (i = och->start, f = 0; i <= och->end; i++, f++) {
			if (och->ibufs_norm[f]) {
				//delete och->ibufs_norm[f];
			}
		}

		memoire::deloge_tableau("ibufs_norm", och->ibufs_norm, 0);
	}

	if (och->time) {
		memoire::deloge("time", och->time);
	}

	memoire::deloge("OceanCache", och);
}

#if 0
void BKE_ocean_cache_eval_uv(OceanCache *och, OceanResult *ocr, int f, float u, float v)
{
	int res_x = och->resolution_x;
	int res_y = och->resolution_y;
	float result[4];

	u = std::fmod(u, 1.0f);
	v = std::fmod(v, 1.0f);

	if (u < 0) u += 1.0f;
	if (v < 0) v += 1.0f;

	if (och->ibufs_disp[f]) {
		ibuf_sample(och->ibufs_disp[f], u, v, (1.0f / (float)res_x), (1.0f / (float)res_y), result);
		copy_v3_v3(ocr->disp, result);
	}

	if (och->ibufs_foam[f]) {
		ibuf_sample(och->ibufs_foam[f], u, v, (1.0f / (float)res_x), (1.0f / (float)res_y), result);
		ocr->foam = result[0];
	}

	if (och->ibufs_norm[f]) {
		ibuf_sample(och->ibufs_norm[f], u, v, (1.0f / (float)res_x), (1.0f / (float)res_y), result);
		copy_v3_v3(ocr->normal, result);
	}
}

void BKE_ocean_cache_eval_ij(OceanCache *och, OceanResult *ocr, int f, int i, int j)
{
	const int res_x = och->resolution_x;
	const int res_y = och->resolution_y;

	if (i < 0) i = -i;
	if (j < 0) j = -j;

	i = i % res_x;
	j = j % res_y;

	if (och->ibufs_disp[f]) {
		copy_v3_v3(ocr->disp, &och->ibufs_disp[f]->rect_float[4 * (res_x * j + i)]);
	}

	if (och->ibufs_foam[f]) {
		ocr->foam = och->ibufs_foam[f]->rect_float[4 * (res_x * j + i)];
	}

	if (och->ibufs_norm[f]) {
		copy_v3_v3(ocr->normal, &och->ibufs_norm[f]->rect_float[4 * (res_x * j + i)]);
	}
}
#endif

OceanCache *BKE_ocean_init_cache(const char *bakepath, const char *relbase, int start, int end, float wave_scale,
								 float chop_amount, float foam_coverage, float foam_fade, int resolution)
{
	OceanCache *och = new OceanCache;

	och->bakepath = bakepath;
	och->relbase = relbase;

	och->start = start;
	och->end = end;
	och->duration = (end - start) + 1;
	och->wave_scale = wave_scale;
	och->chop_amount = chop_amount;
	och->foam_coverage = foam_coverage;
	och->foam_fade = foam_fade;
	och->resolution_x = resolution * resolution;
	och->resolution_y = resolution * resolution;

	/* À FAIRE : images. */
	och->ibufs_disp = nullptr; // MEM_callocN(sizeof(ImBuf *) * och->duration, "displacement imbuf pointer array");
	och->ibufs_foam = nullptr; // MEM_callocN(sizeof(ImBuf *) * och->duration, "foam imbuf pointer array");
	och->ibufs_norm = nullptr; // MEM_callocN(sizeof(ImBuf *) * och->duration, "normal imbuf pointer array");

	och->time = nullptr;

	return och;
}

void BKE_ocean_simulate_cache(OceanCache *och, int frame)
{
	//	char string[FILE_MAX];

	/* ibufs array is zero based, but filenames are based on frame numbers */
	/* still need to clamp frame numbers to valid range of images on disk though */
	auto f = dls::math::restreint(frame, och->start, och->end) - och->start; /* shift to 0 based */

	/* if image is already loaded in mem, return */
	if (och->ibufs_disp[f] != nullptr) {
		return;
	}

	/* use default color spaces since we know for sure cache files were saved with default settings too */

	//	cache_filename(string, och->bakepath, och->relbase, frame, CACHE_TYPE_DISPLACE);
	//	och->ibufs_disp[f] = IMB_loadiffname(string, 0, nullptr);

	//	cache_filename(string, och->bakepath, och->relbase, frame, CACHE_TYPE_FOAM);
	//	och->ibufs_foam[f] = IMB_loadiffname(string, 0, nullptr);

	//	cache_filename(string, och->bakepath, och->relbase, frame, CACHE_TYPE_NORMAL);
	//	och->ibufs_norm[f] = IMB_loadiffname(string, 0, nullptr);
}

#if 0
void BKE_ocean_bake(Ocean *o, OceanCache *och, void (*update_cb)(void *, float progress, int *cancel),
					void *update_cb_data)
{
	/* note: some of these values remain uninitialized unless certain options
	 * are enabled, take care that BKE_ocean_eval_ij() initializes a member
	 * before use - campbell */
	OceanResult ocr;

	//	ImageFormatData imf = {0};

	int f, i = 0, x, y, cancel = 0;
	float progress;

	ImBuf *ibuf_foam, *ibuf_disp, *ibuf_normal;
	float *prev_foam;
	int res_x = och->resolution_x;
	int res_y = och->resolution_y;
	char string[FILE_MAX];
	//RNG *rng;

	if (!o) return;

	if (o->_do_jacobian) prev_foam = MEM_callocN(res_x * res_y * sizeof(float), "previous frame foam bake data");
	else prev_foam = nullptr;

	//rng = BLI_rng_new(0);

	/* setup image format */
	imf.imtype = R_IMF_IMTYPE_OPENEXR;
	imf.depth =  R_IMF_CHAN_DEPTH_16;
	imf.exr_codec = R_IMF_EXR_CODEC_ZIP;

	for (f = och->start, i = 0; f <= och->end; f++, i++) {

		/* create a new imbuf to store image for this frame */
		ibuf_foam = IMB_allocImBuf(res_x, res_y, 32, IB_rectfloat);
		ibuf_disp = IMB_allocImBuf(res_x, res_y, 32, IB_rectfloat);
		ibuf_normal = IMB_allocImBuf(res_x, res_y, 32, IB_rectfloat);

		BKE_ocean_simulate(o, och->time[i], och->wave_scale, och->chop_amount);

		/* add new foam */
		for (y = 0; y < res_y; y++) {
			for (x = 0; x < res_x; x++) {

				BKE_ocean_eval_ij(o, &ocr, x, y);

				/* add to the image */
				rgb_to_rgba_unit_alpha(&ibuf_disp->rect_float[4 * (res_x * y + x)], ocr.disp);

				if (o->_do_jacobian) {
					/* TODO, cleanup unused code - campbell */

					float /*r, */ /* UNUSED */ pr = 0.0f, foam_result;
					float neg_disp, neg_eplus;

					ocr.foam = BKE_ocean_jminus_to_foam(ocr.Jminus, och->foam_coverage);

					/* accumulate previous value for this cell */
					if (i > 0) {
						pr = prev_foam[res_x * y + x];
					}

					/* r = BLI_rng_get_float(rng); */ /* UNUSED */ /* randomly reduce foam */

					/* pr = pr * och->foam_fade; */		/* overall fade */

					/* remember ocean coord sys is Y up!
					 * break up the foam where height (Y) is low (wave valley), and X and Z displacement is greatest
					 */

					neg_disp = ocr.disp[1] < 0.0f ? 1.0f + ocr.disp[1] : 1.0f;
					neg_disp = neg_disp < 0.0f ? 0.0f : neg_disp;

					/* foam, 'ocr.Eplus' only initialized with do_jacobian */
					neg_eplus = ocr.Eplus[2] < 0.0f ? 1.0f + ocr.Eplus[2] : 1.0f;
					neg_eplus = neg_eplus < 0.0f ? 0.0f : neg_eplus;

					if (pr < 1.0f)
						pr *= pr;

					pr *= och->foam_fade * (0.75f + neg_eplus * 0.25f);

					/* A full clamping should not be needed! */
					foam_result = min_ff(pr + ocr.foam, 1.0f);

					prev_foam[res_x * y + x] = foam_result;

					/*foam_result = min_ff(foam_result, 1.0f); */

					value_to_rgba_unit_alpha(&ibuf_foam->rect_float[4 * (res_x * y + x)], foam_result);
				}

				if (o->_do_normals) {
					rgb_to_rgba_unit_alpha(&ibuf_normal->rect_float[4 * (res_x * y + x)], ocr.normal);
				}
			}
		}

		/* write the images */
		cache_filename(string, och->bakepath, och->relbase, f, CACHE_TYPE_DISPLACE);
		if (0 == BKE_imbuf_write(ibuf_disp, string, &imf))
			printf("Cannot save Displacement File Output to %s\n", string);

		if (o->_do_jacobian) {
			cache_filename(string, och->bakepath, och->relbase, f, CACHE_TYPE_FOAM);
			if (0 == BKE_imbuf_write(ibuf_foam, string, &imf))
				printf("Cannot save Foam File Output to %s\n", string);
		}

		if (o->_do_normals) {
			cache_filename(string, och->bakepath, och->relbase, f, CACHE_TYPE_NORMAL);
			if (0 == BKE_imbuf_write(ibuf_normal, string, &imf))
				printf("Cannot save Normal File Output to %s\n", string);
		}

		IMB_freeImBuf(ibuf_disp);
		IMB_freeImBuf(ibuf_foam);
		IMB_freeImBuf(ibuf_normal);

		progress = (f - och->start) / (float)och->duration;

		update_cb(update_cb_data, progress, &cancel);

		if (cancel) {
			if (prev_foam) MEM_freeN(prev_foam);
			//BLI_rng_free(rng);
			return;
		}
	}

	//BLI_rng_free(rng);
	if (prev_foam) MEM_freeN(prev_foam);
	och->baked = 1;
}
#endif

void BKE_ocean_free_modifier_cache(OceanModifierData *omd)
{
	BKE_ocean_free_cache(omd->oceancache);
}
