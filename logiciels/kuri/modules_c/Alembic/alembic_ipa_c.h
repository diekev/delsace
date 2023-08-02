/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

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
                                              uint64_t taille_nom);
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

struct AutriceArchive *ABC_cree_autrice_archive(struct ContexteKuri *ctx_kuri,
                                                struct ContexteCreationArchive *ctx,
                                                struct ContexteEcritureCache *ctx_ecriture);

void ABC_detruit_autrice(struct ContexteKuri *ctx, struct AutriceArchive *autrice);

struct EcrivainCache *ABC_cree_ecrivain_cache(struct ContexteKuri *ctx,
                                              struct AutriceArchive *archive,
                                              struct EcrivainCache *parent,
                                              const char *nom,
                                              uint64_t taille_nom,
                                              void *données,
                                              enum eTypeObjetAbc type_objet);

/** Crée une instance de `origine` comme enfant de `parent`. Retourne nul s'il est impossible de
 * créer une telle instance. */
struct EcrivainCache *ABC_cree_instance(struct ContexteKuri *ctx,
                                        struct AutriceArchive *archive,
                                        struct EcrivainCache *parent,
                                        struct EcrivainCache *origine,
                                        const char *nom,
                                        uint64_t taille_nom);

void ABC_ecris_donnees(struct AutriceArchive *autrice);

#ifdef __cplusplus
}
#endif
