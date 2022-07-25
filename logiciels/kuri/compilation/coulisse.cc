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

#include "coulisse.hh"

#include "options.hh"

#include "coulisse_asm.hh"
#include "coulisse_c.hh"
#include "coulisse_llvm.hh"
#include "coulisse_mv.hh"

Coulisse *Coulisse::cree_pour_options(OptionsDeCompilation options)
{
    switch (options.coulisse) {
        case TypeCoulisse::C:
        {
            return memoire::loge<CoulisseC>("CoulisseC");
        }
        case TypeCoulisse::LLVM:
        {
#ifdef AVEC_LLVM
            return memoire::loge<CoulisseLLVM>("CoulisseLLVM");
#else
            return nullptr;
#endif
        }
        case TypeCoulisse::ASM:
        {
            return memoire::loge<CoulisseASM>("CoulisseASM");
        }
    }

    assert(false);
    return nullptr;
}

Coulisse *Coulisse::cree_pour_metaprogramme()
{
    return memoire::loge<CoulisseMV>("CoulisseMV");
}

void Coulisse::detruit(Coulisse *coulisse)
{
    if (dynamic_cast<CoulisseC *>(coulisse)) {
        auto c = dynamic_cast<CoulisseC *>(coulisse);
        memoire::deloge("CoulisseC", c);
        coulisse = nullptr;
    }
#ifdef AVEC_LLVM
    else if (dynamic_cast<CoulisseLLVM *>(coulisse)) {
        auto c = dynamic_cast<CoulisseLLVM *>(coulisse);
        memoire::deloge("CoulisseLLVM", c);
        coulisse = nullptr;
    }
#endif
    else if (dynamic_cast<CoulisseASM *>(coulisse)) {
        auto c = dynamic_cast<CoulisseASM *>(coulisse);
        memoire::deloge("CoulisseASM", c);
        coulisse = nullptr;
    }
    else if (dynamic_cast<CoulisseMV *>(coulisse)) {
        auto c = dynamic_cast<CoulisseMV *>(coulisse);
        memoire::deloge("CoulisseMV", c);
        coulisse = nullptr;
    }
}
