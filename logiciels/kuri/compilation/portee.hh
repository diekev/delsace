/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "structures/ensemblon.hh"
#include "structures/tablet.hh"

struct Fichier;
struct IdentifiantCode;
struct Module;
struct NoeudExpression;
struct NoeudBloc;
struct NoeudDeclaration;
struct NoeudDeclarationEnteteFonction;

struct ContexteRechecheSymbole {
    NoeudBloc const *bloc_racine = nullptr;
    Fichier const *fichier = nullptr;
    NoeudDeclarationEnteteFonction const *fonction_courante = nullptr;
};

NoeudDeclaration *trouve_dans_bloc_seul(NoeudBloc const *bloc, IdentifiantCode const *ident);

NoeudDeclaration *trouve_dans_bloc(
    NoeudBloc const *bloc,
    IdentifiantCode const *ident,
    NoeudBloc const *bloc_final = nullptr,
    NoeudDeclarationEnteteFonction const *fonction_courante = nullptr);

NoeudDeclaration *trouve_dans_bloc(NoeudBloc const *bloc,
                                   NoeudDeclaration const *decl,
                                   NoeudBloc const *bloc_final = nullptr);

NoeudDeclaration *trouve_dans_bloc_ou_module(
    NoeudBloc const *bloc,
    IdentifiantCode const *ident,
    Fichier const *fichier,
    NoeudDeclarationEnteteFonction const *fonction_courante);

NoeudDeclaration *trouve_dans_bloc_ou_module(ContexteRechecheSymbole const contexte,
                                             IdentifiantCode const *ident);

void trouve_declarations_dans_bloc(kuri::tablet<NoeudDeclaration *, 10> &declarations,
                                   NoeudBloc const *bloc,
                                   IdentifiantCode const *ident);

void trouve_declarations_dans_bloc_ou_module(kuri::tablet<NoeudDeclaration *, 10> &declarations,
                                             NoeudBloc const *bloc,
                                             IdentifiantCode const *ident,
                                             Fichier const *fichier);

void trouve_declarations_dans_bloc_ou_module(kuri::tablet<NoeudDeclaration *, 10> &declarations,
                                             kuri::ensemblon<Module const *, 10> &modules_visites,
                                             NoeudBloc const *bloc,
                                             IdentifiantCode const *ident,
                                             Fichier const *fichier);

NoeudExpression *bloc_est_dans_boucle(NoeudBloc const *bloc, IdentifiantCode const *ident_boucle);

bool bloc_est_dans_differe(NoeudBloc const *bloc);

NoeudExpression *derniere_instruction(NoeudBloc const *b);
