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

#include "alembic_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Crée une archive pour lire des objets Alembic.
 *
 * Si l'archive ne peut être ouverte, retourne nul, et rapporte une erreur via le ctx.
 */
struct ArchiveCache *ABC_cree_archive(struct ContexteKuri *ctx_kuri,
                                      struct ContexteOuvertureArchive *ctx);
void ABC_detruit_archive(struct ContexteKuri *ctx, struct ArchiveCache *archive);

void ABC_traverse_archive(struct ContexteKuri *ctx_kuri,
                          struct ArchiveCache *archive,
                          struct ContexteTraverseArchive *ctx);

struct LectriceCache *ABC_cree_lectrice_cache(struct ContexteKuri *ctx_kuri,
                                              struct ArchiveCache *archive,
                                              const char *ptr_nom,
                                              unsigned long taille_nom);
void ABC_detruit_lectrice(struct ContexteKuri *ctx_kuri, struct LectriceCache *lectrice);
void ABC_lectrice_ajourne_donnees(struct LectriceCache *lectrice, void *donnees);
void ABC_lis_objet(struct ContexteKuri *ctx_kuri,
                   struct ContexteLectureCache *contexte,
                   struct LectriceCache *lectrice,
                   double temps);

void ABC_lis_attributs(struct ContexteKuri *ctx_kuri,
                       struct LectriceCache *lectrice,
                       struct ConvertisseuseImportAttributs *convertisseuse,
                       double temps);

#ifdef __cplusplus
}
#endif
