/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "site_source.hh"

#include "lexemes.hh"

SiteSource SiteSource::cree(const Fichier *fichier, const Lexème *lexeme)
{
    SiteSource site;
    site.fichier = fichier;
    site.indice_ligne = lexeme->ligne;
    site.indice_colonne = lexeme->colonne;
    site.indice_colonne_min = site.indice_colonne;
    site.indice_colonne_max = static_cast<int>(site.indice_colonne + lexeme->chaine.taille());
    return site;
}
