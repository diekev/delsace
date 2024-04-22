/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "arbre_syntaxique/prodeclaration.hh"

#include "structures/ensemblon.hh"
#include "structures/tablet.hh"

struct Fichier;
struct IdentifiantCode;
struct Module;

struct ContexteRechecheSymbole {
    NoeudBloc const *bloc_racine = nullptr;
    Fichier const *fichier = nullptr;
    NoeudDéclarationEntêteFonction const *fonction_courante = nullptr;
};

NoeudDéclaration *trouve_dans_bloc_seul(NoeudBloc const *bloc, IdentifiantCode const *ident);

NoeudDéclaration *trouve_dans_bloc(
    NoeudBloc const *bloc,
    IdentifiantCode const *ident,
    NoeudBloc const *bloc_final = nullptr,
    NoeudDéclarationEntêteFonction const *fonction_courante = nullptr);

NoeudDéclaration *trouve_dans_bloc(
    NoeudBloc const *bloc,
    NoeudDéclaration const *decl,
    NoeudBloc const *bloc_final = nullptr,
    const NoeudDéclarationEntêteFonction *fonction_courante = nullptr);

NoeudDéclaration *trouve_dans_bloc_ou_module(
    NoeudBloc const *bloc,
    IdentifiantCode const *ident,
    Fichier const *fichier,
    NoeudDéclarationEntêteFonction const *fonction_courante);

NoeudDéclaration *trouve_dans_bloc_ou_module(ContexteRechecheSymbole const contexte,
                                             IdentifiantCode const *ident);

void trouve_declarations_dans_bloc(kuri::tablet<NoeudDéclaration *, 10> &declarations,
                                   NoeudBloc const *bloc,
                                   IdentifiantCode const *ident);

void trouve_declarations_dans_bloc_ou_module(kuri::tablet<NoeudDéclaration *, 10> &declarations,
                                             NoeudBloc const *bloc,
                                             IdentifiantCode const *ident,
                                             Fichier const *fichier);

void trouve_declarations_dans_bloc_ou_module(kuri::tablet<NoeudDéclaration *, 10> &declarations,
                                             kuri::ensemblon<Module const *, 10> &modules_visites,
                                             NoeudBloc const *bloc,
                                             IdentifiantCode const *ident,
                                             Fichier const *fichier);

NoeudExpression *bloc_est_dans_boucle(NoeudBloc const *bloc, IdentifiantCode const *ident_boucle);

bool bloc_est_dans_diffère(NoeudBloc const *bloc);

NoeudExpression *derniere_instruction(NoeudBloc const *b, bool accepte_appels);
