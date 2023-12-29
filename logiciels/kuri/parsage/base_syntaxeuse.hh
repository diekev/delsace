/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

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
        Lexeme *lexème = nullptr;
        kuri::chaine_statique message{};
    };

    kuri::tableau<Lexeme, int> &m_lexèmes;
    Fichier *m_fichier = nullptr;
    Lexeme *m_lexème_courant = nullptr;
    Lexeme *m_lexème_sauvegardé = nullptr;
    int m_position = 0;
    int32_t m_position_sauvegardée = 0;
    bool m_position_fut_sauvegardée = false;
    bool m_possède_erreur = false;
    char _pad[2];

    dls::chrono::metre_seconde m_chrono_analyse{};

    kuri::tablet<DonneesEtatSyntaxage, 33> m_données_état_syntaxage{};

  public:
    BaseSyntaxeuse(Fichier *fichier);

    EMPECHE_COPIE(BaseSyntaxeuse);

    virtual ~BaseSyntaxeuse();

    void analyse();

    bool possède_erreur() const
    {
        return m_possède_erreur;
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
    virtual void gère_erreur_rapportée(kuri::chaine const &message_erreur) = 0;

    // Interface pour la consommation et l'appariement de lexèmes

    inline void consomme()
    {
        m_position += 1;

        if (!fini()) {
            m_lexème_courant += 1;
        }
    }

    inline void consomme(GenreLexeme genre_lexème, kuri::chaine_statique message)
    {
        if (fini() || m_lexèmes[m_position].genre != genre_lexème) {
            rapporte_erreur(message);
            /* Ne retournons pas ici, afin que consomme() fasse progresser la compilation. */
        }

        return consomme();
    }

    inline void recule()
    {
        m_position -= 1;

        if (m_position >= 0) {
            m_lexème_courant = &m_lexèmes[m_position];
        }
    }

    inline Lexeme *lexème_courant()
    {
        return m_lexème_courant;
    }

    inline Lexeme const *lexème_courant() const
    {
        return m_lexème_courant;
    }

    inline bool fini() const
    {
        return m_position >= m_lexèmes.taille();
    }

    inline bool apparie(GenreLexeme genre_lexème) const
    {
        return m_lexème_courant->genre == genre_lexème;
    }

    inline bool apparie(kuri::chaine_statique chaine) const
    {
        const auto chaine_lexème = m_lexème_courant->chaine;
        const auto chaine_statique_lexème = kuri::chaine_statique{chaine_lexème.pointeur(),
                                                                  chaine_lexème.taille()};
        return chaine_statique_lexème == chaine;
    }

    inline bool apparie(const IdentifiantCode *ident) const
    {
        return m_lexème_courant->ident == ident;
    }

    // Interface pour la gestion d'erreurs

    void empile_état(kuri::chaine_statique message, Lexeme *lexème);

    void dépile_état();

    kuri::chaine crée_message_erreur(kuri::chaine_statique message);

    void rapporte_erreur(kuri::chaine_statique message);

    void sauvegarde_position_lexème();

    void restaure_position_lexème();
};
