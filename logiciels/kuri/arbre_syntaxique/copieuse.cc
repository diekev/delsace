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
 * The Original Code is Copyright (C) 2022 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "copieuse.hh"

Copieuse::Copieuse(AssembleuseArbre *assembleuse) : assem(assembleuse)
{
}

NoeudExpression *Copieuse::trouve_copie(const NoeudExpression *racine)
{
    auto trouve = false;
    auto copie = noeuds_copies.trouve(racine, trouve);
    if (trouve) {
        return copie;
    }
    return nullptr;
}

void Copieuse::insere_copie(const NoeudExpression *racine, NoeudExpression *copie)
{
    noeuds_copies.insere(racine, copie);
}

NoeudExpression *copie_noeud(AssembleuseArbre *assem,
                             const NoeudExpression *racine,
                             NoeudBloc *bloc_parent)
{
    Copieuse copieuse(assem);
    return copieuse.copie_noeud(racine, bloc_parent);
}
