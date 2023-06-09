/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include <variant>

#include "../alembic.h"

namespace Abc = Alembic::Abc;
namespace AbcGeom = Alembic::AbcGeom;

struct ContexteKuri;
struct ContexteOuvertureArchive;
struct ContexteTraverseArchive;

// En dehors de l'espace de nom AbcKuri afin de ne pas avoir à transtyper en passant par les
// fonctions "extern C".
struct ArchiveCache {
    std::variant<Abc::IArchive, Abc::OArchive> archive{};

    bool est_lecture()
    {
        return std::holds_alternative<Abc::IArchive>(archive);
    }

    Abc::IArchive &iarchive()
    {
        return std::get<Abc::IArchive>(archive);
    }

    Abc::OArchive &oarchive()
    {
        return std::get<Abc::OArchive>(archive);
    }

    AbcGeom::IObject racine_lecture()
    {
        return iarchive().getTop();
    }

    AbcGeom::OObject racine_ecriture()
    {
        return oarchive().getTop();
    }
};

namespace AbcKuri {

ArchiveCache *cree_archive(ContexteKuri *ctx_kuri, ContexteOuvertureArchive *ctx);

ArchiveCache *cree_archive_export(ContexteKuri *ctx_kuri, ContexteOuvertureArchive *ctx);

void detruit_archive(ContexteKuri *ctx, ArchiveCache *archive);

void traverse_archive(ContexteKuri * /*ctx_kuri*/,
                      ArchiveCache *archive,
                      ContexteTraverseArchive *ctx);

}  // namespace AbcKuri
