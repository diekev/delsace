/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "site_source.hh"

#include "lexemes.hh"

SiteSource SiteSource::cree(const Fichier *fichier, const Lexeme *lexeme)
{
    SiteSource site;
    site.fichier = fichier;
    site.index_ligne = lexeme->ligne;
    site.index_colonne = lexeme->colonne;
    site.index_colonne_min = site.index_colonne;
    site.index_colonne_max = static_cast<int>(site.index_colonne + lexeme->chaine.taille());
    return site;
}
