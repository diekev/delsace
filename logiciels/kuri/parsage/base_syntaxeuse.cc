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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "base_syntaxeuse.hh"

#include "structures/enchaineuse.hh"

#include "modules.hh"

BaseSyntaxeuse::BaseSyntaxeuse(Fichier *fichier) : m_lexemes(fichier->lexemes), m_fichier(fichier)
{
    if (m_lexemes.taille() > 0) {
        m_lexeme_courant = &m_lexemes[0];
    }
}

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

    while (!fini() && !possede_erreur()) {
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

kuri::chaine BaseSyntaxeuse::cree_message_erreur(kuri::chaine_statique message)
{
    auto enchaineuse = Enchaineuse();
    auto lexeme = lexeme_courant();

    enchaineuse << "\n";
    enchaineuse << m_fichier->chemin() << ':' << lexeme->ligne + 1 << " : erreur de syntaxage :\n";

    POUR (m_donnees_etat_syntaxage) {
        imprime_ligne_avec_message(enchaineuse, m_fichier, it.lexeme, it.message);
    }

    imprime_ligne_avec_message(enchaineuse, m_fichier, lexeme, message);
    return enchaineuse.chaine();
}

void BaseSyntaxeuse::rapporte_erreur(kuri::chaine_statique message)
{
    m_possede_erreur = true;
    gere_erreur_rapportee(cree_message_erreur(message));
}
