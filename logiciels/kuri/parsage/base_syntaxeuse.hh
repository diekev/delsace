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

#pragma once

#include "biblinternes/chrono/outils.hh"

#include "structures/chaine.hh"
#include "structures/tableau.hh"
#include "structures/tablet.hh"

#include "lexemes.hh"

struct Fichier;

struct BaseSyntaxeuse {
  protected:
    /* Pour les messages d'erreurs. */
    struct DonneesEtatSyntaxage {
        Lexeme *lexeme = nullptr;
        kuri::chaine_statique message{};
    };

    kuri::tableau<Lexeme, int> &m_lexemes;
    Fichier *m_fichier = nullptr;
    Lexeme *m_lexeme_courant = nullptr;
    int m_position = 0;
    bool m_possede_erreur = false;
    char _pad[3];

    dls::chrono::metre_seconde m_chrono_analyse{};

    kuri::tablet<DonneesEtatSyntaxage, 33> m_donnees_etat_syntaxage{};

  public:
    BaseSyntaxeuse(Fichier *fichier);

    COPIE_CONSTRUCT(BaseSyntaxeuse);

    virtual ~BaseSyntaxeuse() = default;

    void analyse();

    bool possede_erreur() const
    {
        return m_possede_erreur;
    }

  protected:
    // Interface pour les classes dérivées.

    // Cette fonction sera appelée à chaque itération de la boucle principale
    // tant qu'il reste des léxèmes ou qu'il n'y a aucune erreur.
    virtual void analyse_une_chose() = 0;

    // Cette fonction optionnelle sera appelée avant de commencer la boucle principale.
    virtual void quand_commence()
    {
    }

    // Cette fonction optionnelle sera appelée à la fin de la boucle principale, ou s'il
    // n'a pas de lexèmes dans la source
    virtual void quand_termine()
    {
    }

    // Cette fonction est appelée quand une erreur est rapportée via rapporte_erreur
    virtual void gere_erreur_rapportee(kuri::chaine const &message_erreur) = 0;

    // Interface pour la consommation et l'appariement de lexèmes

    inline void consomme()
    {
        m_position += 1;

        if (!fini()) {
            m_lexeme_courant += 1;
        }
    }

    inline void consomme(GenreLexeme genre_lexeme, kuri::chaine_statique message)
    {
        if (m_lexemes[m_position].genre != genre_lexeme) {
            rapporte_erreur(message);
            return;
        }

        return consomme();
    }

    inline void recule()
    {
        m_position -= 1;

        if (m_position >= 0) {
            m_lexeme_courant = &m_lexemes[m_position];
        }
    }

    inline Lexeme *lexeme_courant()
    {
        return m_lexeme_courant;
    }

    inline Lexeme const *lexeme_courant() const
    {
        return m_lexeme_courant;
    }

    inline bool fini() const
    {
        return m_position >= m_lexemes.taille();
    }

    inline bool apparie(GenreLexeme genre_lexeme) const
    {
        return m_lexeme_courant->genre == genre_lexeme;
    }

    inline bool apparie(kuri::chaine_statique chaine) const
    {
        const auto chaine_lexeme = m_lexeme_courant->chaine;
        const auto chaine_statique_lexeme = kuri::chaine_statique{chaine_lexeme.pointeur(),
                                                                  chaine_lexeme.taille()};
        return chaine_statique_lexeme == chaine;
    }

    inline bool apparie(const IdentifiantCode *ident) const
    {
        return m_lexeme_courant->ident == ident;
    }

    // Interface pour la gestion d'erreurs

    void empile_etat(kuri::chaine_statique message, Lexeme *lexeme);

    void depile_etat();

    kuri::chaine cree_message_erreur(kuri::chaine_statique message);

    void rapporte_erreur(kuri::chaine_statique message);
};
