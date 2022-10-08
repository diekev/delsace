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

#pragma once

struct Fichier;
struct Lexeme;

/* Représente une position dans le texte. Utilisée pour les messages d'erreurs.
 * Les index des colonnes sont en octets, dans l'encodage UTF-8.
 */
struct SiteSource {
    Fichier const *fichier = nullptr;
    /* L'index de la ligne où se trouve l'erreur. */
    int index_ligne = -1;

    /* L'index du caractère où se trouve l'erreur. */
    int index_colonne = -1;

    /* Plage de l'erreur, puisque nous ne pouvons la déterminer via un arbre syntaxique, avec
     * index_colonne_min <= index_colonne <= index_colonne_max.
     */
    int index_colonne_min = -1;
    int index_colonne_max = -1;

    SiteSource() = default;

    SiteSource(Fichier const *fichier_, int index_ligne_)
        : fichier(fichier_), index_ligne(index_ligne_)
    {
    }

    static SiteSource cree(Fichier const *fichier, Lexeme const *lexeme);
};
