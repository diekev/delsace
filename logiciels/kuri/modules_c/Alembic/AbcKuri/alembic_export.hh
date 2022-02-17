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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

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
                                   unsigned long taille_nom,
                                   eTypeObjetAbc type_objet);

EcrivainCache *cree_instance(ContexteKuri *ctx,
                             EcrivainCache *instance,
                             const char *nom,
                             unsigned long taille_nom);

void ecris_objet(ContexteKuri *ctx, EcrivainCache *ecrivain);

}  // namespace AbcKuri
