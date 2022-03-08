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

#include "../alembic.h"

namespace Abc = Alembic::Abc;

struct ArchiveCache;
struct ContexteKuri;
struct ContexteLectureCache;
struct ConvertisseuseImportAttributs;

struct LectriceCache {
    Abc::IObject iobject{};

#if 0
    std::variant<AbcGeom::IPolyMesh, AbcGeom::ISubD, AbcGeom::ICurves, AbcGeom::IXform> object_;

#endif
    template <typename T>
    bool est_un() const
    {
        return T::matches(iobject.getHeader());
    }

    template <typename T>
    T comme()
    {
        return T(iobject, Abc::kWrapExisting);
    }

    void *donnees = nullptr;
};

namespace AbcKuri {

LectriceCache *cree_lectrice_cache(ContexteKuri *ctx_kuri,
                                   ArchiveCache *archive,
                                   const char *ptr_nom,
                                   size_t taille_nom);
void detruit_lectrice(ContexteKuri *ctx_kuri, LectriceCache *lectrice);

void lectrice_ajourne_donnees(LectriceCache *lectrice, void *donnees);

void lis_objet(ContexteKuri *ctx_kuri,
               ContexteLectureCache *contexte,
               LectriceCache *lectrice,
               double temps);

void lis_attributs(ContexteKuri *ctx_kuri,
                   LectriceCache *lectrice,
                   ConvertisseuseImportAttributs *convertisseuse,
                   double temps);

}  // namespace AbcKuri
