/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "base_syntaxeuse.hh"

#include "structures/enchaineuse.hh"

#include "lexeuse.hh"
#include "modules.hh"

BaseSyntaxeuse::BaseSyntaxeuse(Fichier *fichier) : m_lexemes(fichier->lexèmes), m_fichier(fichier)
{
    if (m_lexemes.taille() > 0) {
        m_lexeme_courant = &m_lexemes[0];
    }
}

BaseSyntaxeuse::~BaseSyntaxeuse() = default;

void BaseSyntaxeuse::analyse()
{
    m_position = 0;

    m_fichier->temps_analyse = 0.0;
    quand_commence();

    if (m_lexemes.taille() == 0) {
        quand_termine();
        return;
    }

    m_chrono_analyse.commence();

    while (!fini() && !possède_erreur()) {
        analyse_une_chose();
    }

    m_fichier->temps_analyse += m_chrono_analyse.arrete();
    quand_termine();
}

void BaseSyntaxeuse::empile_etat(kuri::chaine_statique message, Lexeme *lexeme)
{
    m_donnees_etat_syntaxage.ajoute({lexeme, message});
}

void BaseSyntaxeuse::depile_etat()
{
    m_donnees_etat_syntaxage.pop_back();
}

kuri::chaine BaseSyntaxeuse::crée_message_erreur(kuri::chaine_statique message)
{
    auto enchaineuse = Enchaineuse();
    enchaineuse << '\n';

    POUR (m_donnees_etat_syntaxage) {
        auto site = SiteSource::cree(m_fichier, it.lexeme);
        imprime_ligne_avec_message(enchaineuse, site, it.message);
        enchaineuse << '\n';
    }

    auto lexeme = lexeme_courant();
    auto site = SiteSource::cree(m_fichier, lexeme);
    imprime_ligne_avec_message(enchaineuse, site, message);
    return enchaineuse.chaine();
}

void BaseSyntaxeuse::rapporte_erreur(kuri::chaine_statique message)
{
    if (m_possède_erreur) {
        /* Avance pour ne pas être bloqué. */
        consomme();
        return;
    }

    m_possède_erreur = true;
    gere_erreur_rapportee(crée_message_erreur(message));
}
