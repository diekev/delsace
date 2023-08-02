/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GNA GNA;
struct ContexteKuri;

GNA *ALEA_cree_gna(struct ContexteKuri *ctx_kuri, unsigned graine);
void ALEA_reseme_gna(GNA *gna, unsigned graine);
void ALEA_detruit_gna(struct ContexteKuri *ctx_kuri, GNA *gna);

float ALEA_uniforme_r32(GNA *gna, float min, float max);
double ALEA_uniforme_r64(GNA *gna, double min, double max);

float ALEA_normale_r32(GNA *gna, float moyenne, float ecart);
double ALEA_normale_r64(GNA *gna, double moyenne, double ecart);

#ifdef __cplusplus
}
#endif
