/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#pragma once

#include "biblinternes/moultfilage/synchrone.hh"

#include "structures/chaine.hh"

#include "utilitaires/macros.hh"

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
    dls::outils::Synchrone<GeranteChaine> &m_gérante_chaine;
    dls::outils::Synchrone<TableIdentifiant> &m_table_identifiants;
    Fichier *m_données;

    const char *m_début_mot = nullptr;
    const char *m_début = nullptr;
    const char *m_fin = nullptr;

    int m_position_ligne = 0;
    int m_compte_ligne = 0;
    int m_pos_mot = 0;
    int m_taille_mot_courant = 0;

    int m_drapeaux = 0;
    GenreLexeme m_dernier_id = GenreLexeme::INCONNU;
    TypeRappelErreur m_rappel_erreur{};
    bool m_possède_erreur = false;

  public:
    Lexeuse(ContexteLexage contexte, Fichier *données, int drapeaux = 0);

    Lexeuse(Lexeuse const &) = delete;
    Lexeuse &operator=(Lexeuse const &) = delete;

    void performe_lexage();

    bool possède_erreur() const
    {
        return m_possède_erreur;
    }

  private:
    TOUJOURS_ENLIGNE bool fini() const
    {
        return m_possède_erreur || m_début >= m_fin;
    }

    template <int N>
    void avance_fixe()
    {
        m_position_ligne += N;
        m_début += N;
    }

    void avance_sans_nouvelle_ligne(int n)
    {
        m_position_ligne += n;
        m_début += n;
    }

    void avance(int n = 1);

    TOUJOURS_ENLIGNE char caractère_courant() const
    {
        return *m_début;
    }

    TOUJOURS_ENLIGNE char caractère_voisin(int n = 1) const
    {
        return *(m_début + n);
    }

    dls::vue_chaine_compacte mot_courant() const;

    void rapporte_erreur(const kuri::chaine &quoi, int centre, int min, int max);

    TOUJOURS_ENLIGNE void ajoute_caractère(int n = 1)
    {
        m_taille_mot_courant += n;
    }

    Lexeme donne_lexème_suivant();
    void ajoute_lexème(GenreLexeme identifiant);
    void ajoute_lexème(Lexeme lexème);

    TOUJOURS_ENLIGNE void enregistre_pos_mot()
    {
        m_pos_mot = m_position_ligne;
        m_début_mot = m_début;
    }

    void consomme_espaces_blanches();

    void rapporte_erreur(kuri::chaine const &quoi);
    void rapporte_erreur_caractère_unicode();

    Lexeme crée_lexème_opérateur(int nombre_de_caractère, GenreLexeme genre_lexème);
    Lexeme crée_lexème_littérale_entier(uint64_t valeur);
    Lexeme crée_lexème_littérale_réelle(double valeur);

    Lexeme lèxe_chaine_littérale();
    Lexeme lèxe_chaine_littérale_guillemet();
    Lexeme lèxe_caractère_littérale();
    unsigned lèxe_caractère_littéral(kuri::chaine *chaine);
    Lexeme lèxe_commentaire();
    Lexeme lèxe_commentaire_bloc();
    Lexeme lèxe_littérale_nombre();
    Lexeme lèxe_identifiant();

    template <bool INCLUS_COMMENTAIRE>
    Lexeme lèxe_commentaire_impl();

    template <bool INCLUS_COMMENTAIRE>
    Lexeme lèxe_commentaire_bloc_impl();

    Lexeme lèxe_nombre_décimal();
    Lexeme lèxe_nombre_hexadécimal();
    Lexeme lèxe_nombre_binaire();
    Lexeme lèxe_nombre_octal();
    Lexeme lèxe_nombre_reel_hexadécimal();
};
