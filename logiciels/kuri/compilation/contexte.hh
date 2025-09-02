/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2025 Kévin Dietrich. */

struct AllocatriceNoeud;
struct AssembleuseArbre;
struct EspaceDeTravail;
struct LexèmesExtra;

/* Structure passée à différentes fonctions pour simplifier le passage de paramètres. */
struct Contexte {
    EspaceDeTravail *espace = nullptr;
    AssembleuseArbre *assembleuse = nullptr;
    LexèmesExtra *lexèmes_extra = nullptr;
    AllocatriceNoeud *allocatrice_noeud = nullptr;
};
