/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

struct Fichier;
struct Lexème;

/* Représente une position dans le texte. Utilisée pour les messages d'erreurs.
 * Les index des colonnes sont en octets, dans l'encodage UTF-8.
 */
struct SiteSource {
    Fichier const *fichier = nullptr;
    /* L'index de la ligne où se trouve l'erreur. */
    int indice_ligne = -1;

    /* L'index du caractère où se trouve l'erreur. */
    int indice_colonne = -1;

    /* Plage de l'erreur, puisque nous ne pouvons la déterminer via un arbre syntaxique, avec
     * indice_colonne_min <= indice_colonne <= indice_colonne_max.
     */
    int indice_colonne_min = -1;
    int indice_colonne_max = -1;

    SiteSource() = default;

    SiteSource(Fichier const *fichier_, int indice_ligne_)
        : fichier(fichier_), indice_ligne(indice_ligne_)
    {
    }

    static SiteSource cree(Fichier const *fichier, Lexème const *lexeme);
};
