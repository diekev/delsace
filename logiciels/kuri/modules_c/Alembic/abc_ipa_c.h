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

#pragma once

extern "C" {

struct IArchive;
struct IObject;
struct IMaillage;
struct ISubdivision;
struct IXform;
struct ICourbes;
struct IPoints;
struct ICamera;

struct ChaineKuri {
	const char *pointeur;
	long taille;
};

/* interface pour IArchive */

IArchive *ABC_ouvre_archive(ChaineKuri chemin);

void ABC_ferme_archive(IArchive *archive);

IObject *ABC_archive_iobject_racine(IArchive *archive);

/* interface pour IObject */

long ABC_iobject_nombre_enfants(IObject *iobject);

IObject *ABC_iobject_enfant_index(IObject *object, long index);

IObject *ABC_iobject_enfant_nom(IObject *object, ChaineKuri nom);

ChaineKuri ABC_iobject_nom(IObject *object);

ChaineKuri ABC_iobject_chemin(IObject *object);

void ABC_detruit_iobject(IObject *iobject);

bool ABC_est_maillage(IObject *object);

bool ABC_est_subdivision(IObject *object);

bool ABC_est_xform(IObject *object);

bool ABC_est_points(IObject *object);

bool ABC_est_camera(IObject *object);

bool ABC_est_courbes(IObject *object);

IMaillage *ABC_comme_maillage(IObject *object);

ISubdivision *ABC_comme_subdivision(IObject *object);

IXform *ABC_comme_xform(IObject *object);

IPoints *ABC_comme_points(IObject *object);

ICamera *ABC_comme_camera(IObject *object);

ICourbes *ABC_comme_courbes(IObject *object);

}
