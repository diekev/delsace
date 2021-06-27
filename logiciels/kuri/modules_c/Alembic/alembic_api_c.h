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

extern "C" {

ArchiveCache *ABC_ouvre_archive_ex(ContexteKuri *ctx_kuri, ContexteOuvertureArchive *ctx);

void ABC_traverse_archive(ContexteKuri *ctx_kuri, ArchiveCache *archive, ContexteTraverseArchive *ctx);

LectriceCache *ABC_cree_lectrice_cache(ContexteKuri *ctx_kuri, ArchiveCache *archive, const char *ptr_nom, unsigned long taille_nom);
void ABC_detruit_lectrice(ContexteKuri *ctx_kuri, LectriceCache *lectrice);
void ABC_lectrice_ajourne_donnees(LectriceCache *lectrice, void *donnees);
void ABC_lis_objet(ContexteKuri *ctx_kuri, ContexteLectureCache *contexte, LectriceCache *lectrice, double temps);

}
