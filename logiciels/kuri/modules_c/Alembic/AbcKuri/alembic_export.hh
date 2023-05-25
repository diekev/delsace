/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "../alembic_types.h"

struct ContexteKuri;
struct EcrivainCache;
struct LectriceCache;

namespace AbcKuri {

EcrivainCache *cree_ecrivain_cache_depuis_ref(ContexteKuri *ctx,
                                              LectriceCache *lectrice,
                                              EcrivainCache *parent);

EcrivainCache *cree_ecrivain_cache(ContexteKuri *ctx,
                                   EcrivainCache *parent,
                                   const char *nom,
                                   uint64_t taille_nom,
                                   eTypeObjetAbc type_objet);

EcrivainCache *cree_instance(ContexteKuri *ctx,
                             EcrivainCache *instance,
                             const char *nom,
                             uint64_t taille_nom);

void ecris_objet(ContexteKuri *ctx, EcrivainCache *ecrivain);

}  // namespace AbcKuri
