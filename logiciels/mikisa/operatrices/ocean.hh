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

#pragma once

#include <fftw3.h>

#include "biblinternes/math/vecteur.hh"

#if 0
#include "biblinternes/memoire/logeuse_memoire.hh"

struct champs_simulation {
	fftw_complex *entrees = nullptr;
	double *sorties = nullptr;
	fftw_plan_s *plan = nullptr;
	long taille = 0;
	long taille_complexe = 0;

	champs_simulation() = default;

	champs_simulation(int M, int N)
		: taille(M * N)
		, taille_complexe(M * (1 + N / 2))
	{
		entrees = memoire::loge_tableau<fftw_complex>(taille_complexe);
		sorties = memoire::loge_tableau<double>(taille);
		plan = fftw_plan_dft_c2r_2d(M, N, entrees, sorties, FFTW_ESTIMATE);
	}

	~champs_simulation()
	{
		memoire::deloge_tableau(entrees, taille_complexe);
		fftw_destroy_plan(this->plan);
		memoire::deloge_tableau(sorties, taille);
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

struct Ocean {
	/* ********* input parameters to the sim ********* */
	float l = 0.0f;
	float amplitude = 0.0f;
	float reflections_damp = 0.0f;
	float alignement_vent = 0.0f;
	float profondeur = 0.0f;

	float vent_x = 0.0f;
	float vent_z = 0.0f;

	float L = 0.0f;

	int graine = 0;

	/* dimensions of computational grid */
	int res_x = 0;
	int res_y = 0;

	/* spatial size of computational grid */
	float taille_spaciale_x = 0.0f;
	float taille_spaciale_z = 0.0f;

	float facteur_normalisation = 0.0f;
	float temps = 0.0f;

	bool calcul_deplacement_y = false;
	bool calcul_normaux = false;
	bool calcul_chop = false;
	bool calcul_ecume = false;

	/* ********* sim data arrays ********* */

	/* two dimensional arrays of complex */
	fftw_complex *fft_in = nullptr;
	fftw_complex *fft_in_x = nullptr;
	fftw_complex *fft_in_z = nullptr;
	fftw_complex *fft_in_jxx = nullptr;
	fftw_complex *fft_in_jzz = nullptr;
	fftw_complex *fft_in_jxz = nullptr;
	fftw_complex *fft_in_nx = nullptr;
	fftw_complex *fft_in_nz = nullptr;

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
	double *disp_y = nullptr;
	double *N_x = nullptr;
	/* N_y est constant donc inutile de recourir à un tableau. */
	double N_y = 0.0;
	double *N_z = nullptr;
	double *disp_x = nullptr;
	double *disp_z = nullptr;

	/* two dimensional arrays of float */
	/* Jacobian and minimum eigenvalue */
	double *Jxx = nullptr;
	double *Jzz = nullptr;
	double *Jxz = nullptr;

	/* one dimensional float array */
	float *kx = nullptr;
	float *kz = nullptr;

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

void deloge_donnees_ocean(struct Ocean *oc);

void initialise_donnees_ocean(struct Ocean *o, float gravite);

/* échantillonnage de la surface de l'océan */
float ocean_jminus_vers_ecume(float jminus, float coverage);
void evalue_ocean_uv(Ocean *oc, OceanResult *ocr, float u, float v);
void evalue_ocean_uv_catrom(Ocean *oc, OceanResult *ocr, float u, float v);
void evalue_ocean_xz(Ocean *oc, OceanResult *ocr, float x, float z);
void evalue_ocean_xz_catrom(Ocean *oc, OceanResult *ocr, float x, float z);
void evalue_ocean_ij(Ocean *oc, OceanResult *ocr, int i, int j);

void simule_ocean(Ocean *o, float t, float scale, float chop_amount, float gravite);
