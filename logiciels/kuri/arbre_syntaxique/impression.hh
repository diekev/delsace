/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#pragma once

#include "prodeclaration.hh"

struct Enchaineuse;

/* ------------------------------------------------------------------------- */
/** \name Impression formattée.
 * Imprime le code textuel d'un arbre syntaxique. Le formattage consiste à
 * standardiser la manière dont le code est écrit.
 * \{ */

void imprime_arbre_formatté_bloc_module(Enchaineuse &enchaineuse, NoeudBloc const *bloc);
void imprime_arbre_formatté(Enchaineuse &enchaineuse, NoeudExpression const *noeud);
void imprime_arbre_formatté(NoeudExpression const *noeud);
void imprime_arbre_canonique_formatté(NoeudExpression const *noeud);

/* Retourne vrai si le noeud possède un bloc qui fut imprimé comme tel dans le
 * code textuel formatté. */
bool expression_eu_bloc(NoeudExpression const *noeud);

/** \} */
