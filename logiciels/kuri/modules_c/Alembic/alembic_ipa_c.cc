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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "alembic.h"

#include <string_view>
#include <variant>

#include "alembic_ipa_c.h"
#include "alembic_types.h"

#include "AbcKuri/alembic_archive.hh"
#include "AbcKuri/alembic_export.hh"
#include "AbcKuri/alembic_import.hh"

/*
    - [x] Alembic
    - [ ] OpenColorIO (reprend code de Sergey)
    - [x] OpenImageIO
    - [ ] OpenSubDiv (reprend code de Sergey)
    - [ ] PTex

    - [x] Alembic
    - [ ] Bullet
    - [ ] Cycles
    - [ ] CUDA
    - [ ] OpenColorIO (reprend code de Sergey)
    - [ ] OpenCV (ou ffmpeg)
    - [x] OpenImageIO
    - [ ] OpenSubDiv (reprend code de Sergey)
    - [ ] OpenVDB
    - [ ] OptiX
    - [ ] OSL (avec Cycles)
    - [ ] PTex
    - [ ] USD
    - [ ] Vulkan

    Pipeline rendu :
    - charge les objets
    - construction des tampons
    - déforme les positions selon les armatures
    - applique quelconque algorithme de sous-division
    - applique quelconque déplacement
    - recalcule les normaux
 */

extern "C" {

ArchiveCache *ABC_cree_archive(ContexteKuri *ctx_kuri, ContexteOuvertureArchive *ctx)
{
    return AbcKuri::cree_archive(ctx_kuri, ctx);
}

void ABC_detruit_archive(ContexteKuri *ctx, ArchiveCache *archive)
{
    AbcKuri::detruit_archive(ctx, archive);
}

void ABC_traverse_archive(ContexteKuri *ctx_kuri,
                          ArchiveCache *archive,
                          ContexteTraverseArchive *ctx)
{
    AbcKuri::traverse_archive(ctx_kuri, archive, ctx);
}

LectriceCache *ABC_cree_lectrice_cache(ContexteKuri *ctx_kuri,
                                       ArchiveCache *archive,
                                       const char *ptr_nom,
                                       size_t taille_nom)
{
    return AbcKuri::cree_lectrice_cache(ctx_kuri, archive, ptr_nom, taille_nom);
}

void ABC_detruit_lectrice(ContexteKuri *ctx_kuri, LectriceCache *lectrice)
{
    AbcKuri::detruit_lectrice(ctx_kuri, lectrice);
}

void ABC_lectrice_ajourne_donnees(LectriceCache *lectrice, void *donnees)
{
    AbcKuri::lectrice_ajourne_donnees(lectrice, donnees);
}

void ABC_lis_objet(ContexteKuri *ctx_kuri,
                   ContexteLectureCache *contexte,
                   LectriceCache *lectrice,
                   double temps)
{
    AbcKuri::lis_objet(ctx_kuri, contexte, lectrice, temps);
}

// ABC_lis_transformation

/* ABC_lis_attributs
 * - rappel_lis_tous_les_attributs
 * - rappel_nombre_attributs_requis
 * - rappel_nom_attribut_requis_index
 * - rappel_information_portée
 * - reserve_attribut_point
 * - reserve_attribut_polygone
 * - reserve_attribut_point_polygone
 *
 * machine à état pour remplir l'attribut courant ?
 *
 * ajoute_bool
 * ajoute_r32
 * ajoute_r64
 * ajoute_z8
 * ajoute_z16
 * ajoute_z32
 * ajoute_z64
 * ajoute_n8
 * ajoute_n16
 * ajoute_n32
 * ajoute_n64
 * ajoute_matrice_r64
 * ajoute_chaine
 */

// ABC_informations_temporelles_archive

/* ABC_topologie_a_change
 */

// ABC_est_constant

EcrivainCache *ABC_cree_ecrivain_cache_depuis_ref(ContexteKuri *ctx,
                                                  LectriceCache *lectrice,
                                                  EcrivainCache *parent)
{
    return AbcKuri::cree_ecrivain_cache_depuis_ref(ctx, lectrice, parent);
}

EcrivainCache *ABC_cree_ecrivain_cache(ContexteKuri *ctx,
                                       EcrivainCache *parent,
                                       const char *nom,
                                       size_t taille_nom,
                                       eTypeObjetAbc type_objet)
{
    return AbcKuri::cree_ecrivain_cache(ctx, parent, nom, taille_nom, type_objet);
}

EcrivainCache *ABC_cree_instance(ContexteKuri *ctx,
                                 EcrivainCache *instance,
                                 const char *nom,
                                 size_t taille_nom)
{
    return AbcKuri::cree_instance(ctx, instance, nom, taille_nom);
}

void ABC_ecris_objet(ContexteKuri *ctx, EcrivainCache *ecrivain)
{
    AbcKuri::ecris_objet(ctx, ecrivain);
}

void ABC_lis_attributs(ContexteKuri *ctx_kuri,
                       LectriceCache *lectrice,
                       ConvertisseuseImportAttributs *convertisseuse,
                       double temps)
{
    AbcKuri::lis_attributs(ctx_kuri, lectrice, convertisseuse, temps);
}
}
