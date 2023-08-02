/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

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
