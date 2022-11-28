/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "biblinternes/moultfilage/synchrone.hh"

#include "structures/table_hachage.hh"

#include "representation_intermediaire/instructions.hh"

#include "parsage/modules.hh"

#include "erreur.h"
#include "messagere.hh"
#include "options.hh"
#include "tache.hh"

struct Coulisse;
struct ConstructriceRI;
struct Programme;
struct SiteSource;

/* IPA :
 * - crée_un_espace_de_travail
 * - espace_de_travail_défaut
 *
 * Problèmes :
 * - les modules ne sont ouvert qu'une seule fois
 * - il faudra stocker les modules pour chaque espace de travail, et partager les données
 * constantes des fichiers
 * - séparer les données constantes des données dynamiques
 * -- données constantes : tampon du fichier, lexèmes
 * -- données dynamiques : arbres syntaxiques, types, noeuds dépendances
 */
struct EspaceDeTravail {
  private:
    std::atomic<int> nombre_de_taches[size_t(GenreTache::NOMBRE_ELEMENTS)] = {};

    PhaseCompilation phase = PhaseCompilation::PARSAGE_EN_COURS;

  public:
    kuri::chaine nom{};
    OptionsDeCompilation options{};

    Programme *programme = nullptr;
    UniteCompilation *unite_pour_code_machine = nullptr;

    /* mise en cache de la fonction principale, si vue dans la Syntaxeuse */
    NoeudDeclarationEnteteFonction *fonction_principale = nullptr;

    NoeudDeclarationEnteteFonction *fonction_point_d_entree = nullptr;

    /* Le métaprogramme controlant la compilation dans cette espace. */
    MetaProgramme *metaprogramme = nullptr;

    /* pour activer ou désactiver les optimisations */
    bool optimisations = false;
    mutable std::atomic<bool> possede_erreur{false};

    Compilatrice &m_compilatrice;

    EspaceDeTravail(Compilatrice &compilatrice, OptionsDeCompilation opts, kuri::chaine nom_);

    EMPECHE_COPIE(EspaceDeTravail);

    ~EspaceDeTravail();

    POINTEUR_NUL(EspaceDeTravail)

    long memoire_utilisee() const;

    void rassemble_statistiques(Statistiques &stats) const;

    void tache_ajoutee(GenreTache genre_tache, dls::outils::Synchrone<Messagere> &messagere);
    void tache_terminee(GenreTache genre_tache,
                        dls::outils::Synchrone<Messagere> &messagere,
                        bool peut_envoyer_changement_de_phase);

    void progresse_phase_pour_tache_terminee(GenreTache genre_tache,
                                             dls::outils::Synchrone<Messagere> &messagere,
                                             bool peut_envoyer_changement_de_phase);
    void regresse_phase_pour_tache_ajoutee(GenreTache genre_tache,
                                           dls::outils::Synchrone<Messagere> &messagere);

    bool peut_generer_code_final() const;
    bool parsage_termine() const;

    Message *change_de_phase(dls::outils::Synchrone<Messagere> &messagere,
                             PhaseCompilation nouvelle_phase);

    PhaseCompilation phase_courante() const
    {
        return phase;
    }

    SiteSource site_source_pour(NoeudExpression const *noeud) const;

    void rapporte_avertissement(const NoeudExpression *site, kuri::chaine_statique message) const;
    void rapporte_avertissement(kuri::chaine const &fichier,
                                int ligne,
                                kuri::chaine const &message) const;

    Erreur rapporte_erreur(NoeudExpression const *site,
                           kuri::chaine_statique message,
                           erreur::Genre genre = erreur::Genre::NORMAL) const;
    Erreur rapporte_erreur(kuri::chaine const &chemin_fichier,
                           int ligne,
                           kuri::chaine const &message,
                           erreur::Genre genre = erreur::Genre::NORMAL) const;
    Erreur rapporte_erreur(SiteSource site,
                           kuri::chaine const &message,
                           erreur::Genre genre = erreur::Genre::NORMAL) const;
    Erreur rapporte_erreur_sans_site(const kuri::chaine &message,
                                     erreur::Genre genre = erreur::Genre::NORMAL) const;

    Compilatrice &compilatrice()
    {
        return m_compilatrice;
    }

    Compilatrice &compilatrice() const
    {
        return m_compilatrice;
    }

    void imprime_compte_taches(std::ostream &os) const;
};
