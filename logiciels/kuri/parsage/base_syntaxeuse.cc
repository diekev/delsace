/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "base_syntaxeuse.hh"

#include "structures/enchaineuse.hh"

#include "lexeuse.hh"
#include "modules.hh"

BaseSyntaxeuse::BaseSyntaxeuse(Fichier *fichier) : m_lexèmes(fichier->lexèmes), m_fichier(fichier)
{
    if (m_lexèmes.taille() > 0) {
        m_lexème_courant = &m_lexèmes[0];
    }
}

BaseSyntaxeuse::~BaseSyntaxeuse() = default;

void BaseSyntaxeuse::analyse()
{
    m_position = 0;

    m_fichier->temps_analyse = 0.0;
    quand_commence();

    if (m_lexèmes.taille() == 0) {
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

void BaseSyntaxeuse::empile_état(kuri::chaine_statique message, Lexeme *lexème)
{
    m_données_état_syntaxage.ajoute({lexème, message});
}

void BaseSyntaxeuse::dépile_état()
{
    m_données_état_syntaxage.pop_back();
}

kuri::chaine BaseSyntaxeuse::crée_message_erreur(kuri::chaine_statique message)
{
    auto enchaineuse = Enchaineuse();
    enchaineuse << '\n';

    POUR (m_données_état_syntaxage) {
        auto site = SiteSource::cree(m_fichier, it.lexème);
        imprime_ligne_avec_message(enchaineuse, site, it.message);
        enchaineuse << '\n';
    }

    auto lexème = lexème_courant();
    auto site = SiteSource::cree(m_fichier, lexème);
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
    gère_erreur_rapportée(crée_message_erreur(message));
}
