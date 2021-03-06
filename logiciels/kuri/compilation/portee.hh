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

#include "biblinternes/structures/tablet.hh"

struct EspaceDeTravail;
struct Fichier;
struct IdentifiantCode;
struct NoeudExpression;
struct NoeudBloc;
struct NoeudDeclaration;

NoeudDeclaration *trouve_dans_bloc(NoeudBloc *bloc, IdentifiantCode *ident);

NoeudDeclaration *trouve_dans_bloc(NoeudBloc *bloc, NoeudDeclaration *decl);

NoeudDeclaration *trouve_dans_bloc_seul(NoeudBloc *bloc, NoeudExpression *noeud);

NoeudDeclaration *trouve_dans_bloc_ou_module(NoeudBloc *bloc,
                                             IdentifiantCode *ident,
                                             Fichier *fichier);

void trouve_declarations_dans_bloc(dls::tablet<NoeudDeclaration *, 10> &declarations,
                                   NoeudBloc *bloc,
                                   IdentifiantCode *ident);

void trouve_declarations_dans_bloc_ou_module(dls::tablet<NoeudDeclaration *, 10> &declarations,
                                             NoeudBloc *bloc,
                                             IdentifiantCode *ident,
                                             Fichier *fichier);

NoeudExpression *bloc_est_dans_boucle(NoeudBloc *bloc, IdentifiantCode *ident_boucle);

NoeudExpression *derniere_instruction(NoeudBloc *b);
