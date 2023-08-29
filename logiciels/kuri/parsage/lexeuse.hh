/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#pragma once

#include "biblinternes/moultfilage/synchrone.hh"
#include "biblinternes/outils/definitions.h"

#include "structures/chaine.hh"

#include "lexemes.hh"
#include "site_source.hh"

struct Fichier;
struct GeranteChaine;
struct TableIdentifiant;

enum {
    INCLUS_CARACTERES_BLANC = (1 << 0),
    INCLUS_COMMENTAIRES = (1 << 1),
};

using TypeRappelErreur = std::function<void(SiteSource, kuri::chaine)>;

struct ContexteLexage {
    dls::outils::Synchrone<GeranteChaine> &gerante_chaine;
    dls::outils::Synchrone<TableIdentifiant> &table_identifiants;
    TypeRappelErreur rappel_erreur;
};

struct Lexeuse {
  private:
    dls::outils::Synchrone<GeranteChaine> &m_gerante_chaine;
    dls::outils::Synchrone<TableIdentifiant> &m_table_identifiants;
    Fichier *m_donnees;

    const char *m_debut_mot = nullptr;
    const char *m_debut = nullptr;
    const char *m_fin = nullptr;

    int m_position_ligne = 0;
    int m_compte_ligne = 0;
    int m_pos_mot = 0;
    int m_taille_mot_courant = 0;

    int m_drapeaux = 0;
    GenreLexeme m_dernier_id = GenreLexeme::INCONNU;
    TypeRappelErreur m_rappel_erreur{};
    bool m_possede_erreur = false;

  public:
    Lexeuse(ContexteLexage contexte, Fichier *donnees, int drapeaux = 0);

    Lexeuse(Lexeuse const &) = delete;
    Lexeuse &operator=(Lexeuse const &) = delete;

    void performe_lexage();

    bool possede_erreur() const
    {
        return m_possede_erreur;
    }

  private:
    ENLIGNE_TOUJOURS bool fini() const
    {
        return m_possede_erreur || m_debut >= m_fin;
    }

    template <int N>
    void avance_fixe()
    {
        m_position_ligne += N;
        m_debut += N;
    }

    void avance(int n = 1);

    ENLIGNE_TOUJOURS char caractère_courant() const
    {
        return *m_debut;
    }

    ENLIGNE_TOUJOURS char caractère_voisin(int n = 1) const
    {
        return *(m_debut + n);
    }

    dls::vue_chaine_compacte mot_courant() const;

    void rapporte_erreur(const kuri::chaine &quoi, int centre, int min, int max);

    ENLIGNE_TOUJOURS void ajoute_caractère(int n = 1)
    {
        m_taille_mot_courant += n;
    }

    void ajoute_mot(GenreLexeme identifiant);
    void ajoute_mot(GenreLexeme identifiant, unsigned valeur);

    ENLIGNE_TOUJOURS void enregistre_pos_mot()
    {
        m_pos_mot = m_position_ligne;
        m_debut_mot = m_debut;
    }

    void lexe_commentaire();

    void lexe_commentaire_bloc();

    template <bool INCLUS_COMMENTAIRE>
    void lexe_commentaire_impl();

    template <bool INCLUS_COMMENTAIRE>
    void lexe_commentaire_bloc_impl();

    void lexe_nombre();
    void lexe_nombre_decimal();
    void lexe_nombre_hexadecimal();
    void lexe_nombre_binaire();
    void lexe_nombre_octal();
    void lexe_nombre_reel_hexadecimal();

    unsigned lexe_caractère_litteral(kuri::chaine *chaine);

    void ajoute_lexeme_entier(uint64_t valeur);
    void ajoute_lexeme_reel(double valeur);
};
