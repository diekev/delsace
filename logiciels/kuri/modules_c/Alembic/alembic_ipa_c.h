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

/* ------------------------------------------------------------------------- */
/** \nom Export
 * \{ */

struct Abc_String {
    const char *characters;
    uint64_t size;
};

struct Abc_Output_Archive_Metadata {
    struct Abc_String application_name;
    struct Abc_String user_description;
    double fps;
};

struct Abc_Output_Archive;

struct Abc_Output_Xform;
struct Abc_Output_Points;

union Abc_Output_Object {
    struct Abc_Output_Xform *xform;
    struct Abc_Output_Points *points;
};

/**
 * @brief abc_output_archive_create Crée une archive Alembic pour y écrire des objets.
 * @param ctx_kuri Le contexte Kuri utilisé pour toutes les allocations.
 * @param path Le chemin où sera écris le fichier. Si un fichier existe déjà à ce chemin, il sera
 * surécrit.
 * @param metadata Les métadonnées de l'archive. DOIT être non-nul.
 * @return Une instance de Abc_Output_Archive.
 */
struct Abc_Output_Archive *abc_output_archive_create(struct ContexteKuri *ctx_kuri,
                                                     struct Abc_String path,
                                                     struct Abc_Output_Archive_Metadata *metadata);

/**
 * @brief abc_output_archive_destroy Détruit l'archive. Rien ne sera écris tant que ceci n'est pas
 * appelé.
 * @param archive L'archive à détruire. Peut être nulle.
 */
void abc_output_archive_destroy(struct Abc_Output_Archive *archive);

/**
 * @brief abc_output_archive_root_object_get Retourne l'objet racine de l'archive.
 */
struct Abc_Output_Xform *abc_output_archive_root_object_get(struct Abc_Output_Archive *archive);

/**
 * @brief abc_output_xform_create Crée un object de type Xform.
 * @param parent Le parent de l'objet.
 * @param nom Le nom de l'objet. Doit être unique au sein du parent.
 * @return L'objet xform créé.
 */
struct Abc_Output_Xform *abc_output_xform_create(struct Abc_Output_Xform *parent,
                                                 struct Abc_String nom);

struct Abc_Output_Xform_Sample;
struct Abc_Output_Xform_Sample *abc_output_xform_sample_create(struct Abc_Output_Archive *archive);
void abc_output_xform_sample_reset(struct Abc_Output_Xform_Sample *sample);
void abc_output_xform_sample_destroy(struct Abc_Output_Xform_Sample *sample);
void abc_output_xform_sample_set_matrix(struct Abc_Output_Xform_Sample *sample, float *matrix);
void abc_output_xform_sample_set_inherits_xform(struct Abc_Output_Xform_Sample *sample,
                                                bool inherits);
void abc_output_xform_sample_set(struct Abc_Output_Xform *xform,
                                 struct Abc_Output_Xform_Sample *sample);

/**
 * @brief abc_output_points_create Crée un object de type Points.
 * @param parent Le parent de l'objet.
 * @param nom Le nom de l'objet. Doit être unique au sein du parent.
 * @return L'objet points créé.
 */
struct Abc_Output_Points *abc_output_points_create(struct Abc_Output_Xform *parent,
                                                   struct Abc_String nom);

struct Abc_Output_Points_Sample;
struct Abc_Output_Points_Sample *abc_output_points_sample_create(
    struct Abc_Output_Archive *archive);
void abc_output_points_sample_reset(struct Abc_Output_Points_Sample *sample);
void abc_output_points_sample_destroy(struct Abc_Output_Points_Sample *sample);
void abc_output_points_sample_positions_set(struct Abc_Output_Points_Sample *sample,
                                            float *positions,
                                            uint64_t num_positions);
void abc_output_points_sample_velocities_set(struct Abc_Output_Points_Sample *sample,
                                             float *velocities,
                                             uint64_t num_velocities);
void abc_output_points_sample_widths_set(struct Abc_Output_Points_Sample *sample,
                                         float *widths,
                                         uint64_t num_widths);
void abc_output_points_sample_ids_set(struct Abc_Output_Points_Sample *sample,
                                      uint64_t *ids,
                                      uint64_t num_ids);
void abc_output_points_sample_set(struct Abc_Output_Points *points,
                                  struct Abc_Output_Points_Sample *sample);

/** \} */

#ifdef __cplusplus
}
#endif
