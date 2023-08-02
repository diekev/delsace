/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include <variant>

#include "../alembic.h"
#include "../alembic_types.h"

namespace Abc = Alembic::Abc;
namespace AbcGeom = Alembic::AbcGeom;

struct ContexteKuri;
struct ContexteOuvertureArchive;
struct ContexteTraverseArchive;

// En dehors de l'espace de nom AbcKuri afin de ne pas avoir à transtyper en passant par les
// fonctions "extern C".
struct ArchiveCache {
    Abc::IArchive archive{};

    Abc::IArchive &iarchive()
    {
        return archive;
    }

    AbcGeom::IObject racine_lecture()
    {
        return iarchive().getTop();
    }
};

struct AutriceArchive {
    Abc::OArchive archive{};
    ContexteEcritureCache ctx_écriture{};

    EcrivainCache *racine = nullptr;

    AbcGeom::OObject racine_ecriture()
    {
        return archive.getTop();
    }
};

namespace AbcKuri {

ArchiveCache *cree_archive(ContexteKuri *ctx_kuri, ContexteOuvertureArchive *ctx);

void detruit_archive(ContexteKuri *ctx, ArchiveCache *archive);

void traverse_archive(ContexteKuri * /*ctx_kuri*/,
                      ArchiveCache *archive,
                      ContexteTraverseArchive *ctx);

AutriceArchive *crée_autrice_archive(ContexteKuri *ctx_kuri,
                                     ContexteCreationArchive *ctx,
                                     ContexteEcritureCache *ctx_écriture);

void écris_données(AutriceArchive *autrice);

void détruit_autrice(ContexteKuri *ctx, AutriceArchive *autrice);

}  // namespace AbcKuri
