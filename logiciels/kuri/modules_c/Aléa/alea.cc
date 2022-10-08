/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "alea.h"

#include <random>

#include "../InterfaceCKuri/contexte_kuri.hh"

extern "C" {

struct GNA {
    std::mt19937 mt;
};

GNA *ALEA_cree_gna(ContexteKuri *ctx_kuri, unsigned graine)
{
    auto gna = static_cast<GNA *>(ctx_kuri->loge_memoire(ctx_kuri, sizeof(GNA)));
    gna->mt = std::mt19937(graine);
    return gna;
}

void ALEA_reseme_gna(GNA *gna, unsigned graine)
{
    gna->mt.seed(graine);
}

void ALEA_detruit_gna(ContexteKuri *ctx_kuri, GNA *gna)
{
    ctx_kuri->deloge_memoire(ctx_kuri, gna, sizeof(GNA));
}

float ALEA_uniforme_r32(GNA *gna, float min, float max)
{
    auto dist = std::uniform_real_distribution<float>(min, max);
    return dist(gna->mt);
}

double ALEA_uniforme_r64(GNA *gna, double min, double max)
{
    auto dist = std::uniform_real_distribution<double>(min, max);
    return dist(gna->mt);
}

float ALEA_normale_r32(GNA *gna, float moyenne, float ecart)
{
    auto dist = std::normal_distribution<float>(moyenne, ecart);
    return dist(gna->mt);
}

double ALEA_normale_r64(GNA *gna, double moyenne, double ecart)
{
    auto dist = std::normal_distribution<double>(moyenne, ecart);
    return dist(gna->mt);
}
}
