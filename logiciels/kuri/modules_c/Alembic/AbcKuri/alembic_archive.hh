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
    std::variant<Abc::IArchive, Abc::OArchive> archive;

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

void detruit_archive(ContexteKuri *ctx, ArchiveCache *archive);

void traverse_archive(ContexteKuri * /*ctx_kuri*/,
                      ArchiveCache *archive,
                      ContexteTraverseArchive *ctx);

}  // namespace AbcKuri
