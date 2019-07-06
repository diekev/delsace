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
#include "bibloc/logeuse_memoire.hh"

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
	float *_kx = nullptr;                     /* init w	sim r */
	float *_kz = nullptr;                     /* init w	sim r */

	/* two dimensional complex array */
	fftw_complex *_h0 = nullptr;              /* init w	sim r */
	fftw_complex *_h0_minus = nullptr;        /* init w	sim r */

	/* two dimensional float array */
	float *_k = nullptr;                      /* init w	sim r */
};
#endif

typedef struct Ocean {
	/* ********* input parameters to the sim ********* */
	float _V = 0.0f;
	float _l = 0.0f;
	float _w = 0.0f;
	float _A = 0.0f;
	float _damp_reflections = 0.0f;
	float _wind_alignment = 0.0f;
	float _depth = 0.0f;

	float _wx = 0.0f;
	float _wz = 0.0f;

	float _L = 0.0f;

	/* dimensions of computational grid */
	int _M = 0;
	int _N = 0;

	/* spatial size of computational grid */
	float _Lx = 0.0f;
	float _Lz = 0.0f;

	float normalize_factor = 0.0f;                /* init w */
	float time = 0.0f;

	short _do_disp_y = 0;
	short _do_normals = 0;
	short _do_chop = 0;
	short _do_jacobian = 0;

	/* À FAIRE */
	//ThreadRWMutex oceanmutex;

	/* ********* sim data arrays ********* */

	/* two dimensional arrays of complex */
	fftw_complex *_fft_in = nullptr;          /* init w	sim w */
	fftw_complex *_fft_in_x = nullptr;        /* init w	sim w */
	fftw_complex *_fft_in_z = nullptr;        /* init w	sim w */
	fftw_complex *_fft_in_jxx = nullptr;      /* init w	sim w */
	fftw_complex *_fft_in_jzz = nullptr;      /* init w	sim w */
	fftw_complex *_fft_in_jxz = nullptr;      /* init w	sim w */
	fftw_complex *_fft_in_nx = nullptr;       /* init w	sim w */
	fftw_complex *_fft_in_nz = nullptr;       /* init w	sim w */
	fftw_complex *_htilda = nullptr;          /* init w	sim w (only once) */

	/* fftw "plans" */
	fftw_plan _disp_y_plan = nullptr;         /* init w	sim r */
	fftw_plan _disp_x_plan = nullptr;         /* init w	sim r */
	fftw_plan _disp_z_plan = nullptr;         /* init w	sim r */
	fftw_plan _N_x_plan = nullptr;            /* init w	sim r */
	fftw_plan _N_z_plan = nullptr;            /* init w	sim r */
	fftw_plan _Jxx_plan = nullptr;            /* init w	sim r */
	fftw_plan _Jxz_plan = nullptr;            /* init w	sim r */
	fftw_plan _Jzz_plan = nullptr;            /* init w	sim r */

	/* two dimensional arrays of float */
	double *_disp_y = nullptr;                /* init w	sim w via plan? */
	double *_N_x = nullptr;                   /* init w	sim w via plan? */
	/* all member of this array has same values, so convert this array to a float to reduce memory usage (MEM01)*/
	/*float * _N_y; */
	double _N_y = 0.0;                    /*			sim w ********* can be rearranged? */
	double *_N_z = nullptr;                   /* init w	sim w via plan? */
	double *_disp_x = nullptr;                /* init w	sim w via plan? */
	double *_disp_z = nullptr;                /* init w	sim w via plan? */

	/* two dimensional arrays of float */
	/* Jacobian and minimum eigenvalue */
	double *_Jxx = nullptr;                   /* init w	sim w */
	double *_Jzz = nullptr;                   /* init w	sim w */
	double *_Jxz = nullptr;                   /* init w	sim w */

	/* one dimensional float array */
	float *_kx = nullptr;                     /* init w	sim r */
	float *_kz = nullptr;                     /* init w	sim r */

	/* two dimensional complex array */
	fftw_complex *_h0 = nullptr;              /* init w	sim r */
	fftw_complex *_h0_minus = nullptr;        /* init w	sim r */

	/* two dimensional float array */
	float *_k = nullptr;                      /* init w	sim r */
} Ocean;

typedef struct OceanResult {
	float disp[3];
	dls::math::vec3f normal{};
	float foam{};

	/* raw eigenvalues/vectors */
	float Jminus{};
	float Jplus{};
	float Eminus[3];
	float Eplus[3];
} OceanResult;
typedef struct OceanCache {
	struct ImBuf **ibufs_disp;
	struct ImBuf **ibufs_foam;
	struct ImBuf **ibufs_norm;

	const char *bakepath;
	const char *relbase;

	/* precalculated for time range */
	float *time;

	/* constant for time range */
	float wave_scale;
	float chop_amount;
	float foam_coverage;
	float foam_fade;

	int start;
	int end;
	int duration;
	int resolution_x;
	int resolution_y;

	int baked;
} OceanCache;

struct Ocean *BKE_ocean_add(void);
void BKE_ocean_free_data(struct Ocean *oc);
void BKE_ocean_free(struct Ocean *oc);
bool BKE_ocean_ensure(struct OceanModifierData *omd);
void BKE_ocean_init_from_modifier(struct Ocean *ocean, struct OceanModifierData const *omd);

void BKE_ocean_init(struct Ocean *o, int M, int N, float Lx, float Lz, float V, float l, float A, float w, float damp,
		float alignment, float depth, float time, short do_height_field, short do_chop, short do_normals, short do_jacobian, int seed, float gravite);
void BKE_ocean_simulate(struct Ocean *o, float t, float scale, float chop_amount, float gravite);

/* sampling the ocean surface */
float BKE_ocean_jminus_to_foam(float jminus, float coverage);
void  BKE_ocean_eval_uv(struct Ocean *oc, struct OceanResult *ocr, float u, float v);
void  BKE_ocean_eval_uv_catrom(struct Ocean *oc, struct OceanResult *ocr, float u, float v);
void  BKE_ocean_eval_xz(struct Ocean *oc, struct OceanResult *ocr, float x, float z);
void  BKE_ocean_eval_xz_catrom(struct Ocean *oc, struct OceanResult *ocr, float x, float z);
void  BKE_ocean_eval_ij(struct Ocean *oc, struct OceanResult *ocr, int i, int j);


/* ocean cache handling */
struct OceanCache *BKE_ocean_init_cache(
		const char *bakepath, const char *relbase,
		int start, int end, float wave_scale,
		float chop_amount, float foam_coverage, float foam_fade, int resolution);
void BKE_ocean_simulate_cache(struct OceanCache *och, int frame);

void BKE_ocean_bake(struct Ocean *o, struct OceanCache *och, void (*update_cb)(void *, float progress, int *cancel), void *update_cb_data);
void BKE_ocean_cache_eval_uv(struct OceanCache *och, struct OceanResult *ocr, int f, float u, float v);
void BKE_ocean_cache_eval_ij(struct OceanCache *och, struct OceanResult *ocr, int f, int i, int j);

void BKE_ocean_free_cache(struct OceanCache *och);
void BKE_ocean_free_modifier_cache(struct OceanModifierData *omd);

typedef struct OceanModifierData {
	struct Ocean *ocean;
	struct OceanCache *oceancache;

	int resolution;
	float spatial_size;

	float wind_velocity;

	float damp;
	float smallest_wave;
	float depth;

	float wave_alignment;
	float wave_direction;
	float wave_scale;

	float chop_amount;
	float foam_coverage;
	float time;

	char flag;
	char pad2;

	int seed;

	float foam_fade;

	float gravite;
} OceanModifierData;

enum {
	MOD_OCEAN_GEOM_GENERATE = 0,
	MOD_OCEAN_GEOM_DISPLACE = 1,
	MOD_OCEAN_GEOM_SIM_ONLY = 2,
};


enum {
	MOD_OCEAN_GENERATE_FOAM     = (1 << 0),
	MOD_OCEAN_GENERATE_NORMALS  = (1 << 1),
};
