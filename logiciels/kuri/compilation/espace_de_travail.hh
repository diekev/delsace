/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "utilitaires/synchrone.hh"

#include "erreur.h"
#include "graphe_dependance.hh"
#include "interface_module_kuri.hh"
#include "messagere.hh"
#include "metaprogramme.hh"
#include "operateurs.hh"
#include "options.hh"
#include "tache.hh"
#include "typage.hh"

#include "parsage/modules.hh"

#include "representation_intermediaire/constructrice_ri.hh"

struct AtomeGlobale;
struct Compilatrice;
struct MétaProgramme;
struct NoeudDéclarationEntêteFonction;
struct Programme;
struct RegistreSymboliqueRI;
struct SiteSource;
struct Statistiques;
struct UnitéCompilation;

struct Phase {
    PhaseCompilation id{};
    int nombre_de_tâches = 0;
    int messages_en_attente = 0;
};

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
    std::atomic<int> nombre_de_tâches[size_t(GenreTâche::NOMBRE_ELEMENTS)] = {};

    Phase m_phases[NOMBRE_DE_PhaseCompilation] = {};

    PhaseCompilation m_id_phase_courante = PhaseCompilation::PARSAGE_EN_COURS;
    /* Identifiant de la phase de compilation. À chaque fois que nous régressons la phase de
     * compilation, cet identifiant est modifié pour indiquer que la nouvelle phase de compilation
     * est différente de la dernière fois que nous avons eu cette phase. */
    int id_phase = 0;

  public:
    kuri::chaine nom{};
    OptionsDeCompilation options{};
    int id = 0;

    /* Chaque espace a son propre module racine où sont ajoutés les fichiers et les chaines
     * ajoutées lors de la compilation. */
    Module *module = nullptr;

    Programme *programme = nullptr;
    UnitéCompilation *unité_pour_code_machine = nullptr;

    /* mise en cache de la fonction principale, si vue dans la Syntaxeuse */
    NoeudDéclarationEntêteFonction *fonction_principale = nullptr;

    NoeudDéclarationEntêteFonction *fonction_point_d_entrée = nullptr;
    NoeudDéclarationEntêteFonction *fonction_point_d_entrée_dynamique = nullptr;
    NoeudDéclarationEntêteFonction *fonction_point_de_sortie_dynamique = nullptr;

    /* Le métaprogramme controlant la compilation dans cette espace. */
    MétaProgramme *métaprogramme = nullptr;

    /* pour activer ou désactiver les optimisations */
    bool optimisations = false;
    mutable std::atomic<bool> possède_erreur{false};
    mutable std::atomic<int> erreurs_rapportées{0};

    kuri::Synchrone<InterfaceKuri> interface_kuri{};

    kuri::Synchrone<SystèmeModule> sys_module{};

    kuri::Synchrone<GrapheDépendance> graphe_dépendance{};

    kuri::Synchrone<RegistreDesOpérateurs> opérateurs{};

    Typeuse typeuse;

    /* Globale pour __contexte_fil_principal, définie dans le module Kuri. */
    NoeudDéclarationVariable *globale_contexte_programme = nullptr;

    struct DonneesConstructeurGlobale {
        AtomeGlobale *atome = nullptr;
        NoeudExpression *expression = nullptr;
        TransformationType transformation{};
    };

    using ConteneurConstructeursGlobales = kuri::tableau<DonneesConstructeurGlobale, int>;
    kuri::Synchrone<ConteneurConstructeursGlobales> constructeurs_globaux{};

    Module *module_kuri = nullptr;

    // NOTE : données pour la RI.
    // {
    RegistreSymboliqueRI *registre_ri = nullptr;
    kuri::Synchrone<RegistreChainesRI> registre_chaines_ri{};

    /* Globale pour les annotations vides des rubriques des infos-type.
     * Nous n'en créons qu'une seule dans ce cas afin d'économiser de la mémoire.
     */
    AtomeConstante *globale_annotations_vides = nullptr;

    RegistreAnnotations registre_annotations{};

    /* Un seul tableau pour toutes les structures n'ayant pas d'employées. */
    AtomeConstante *tableau_structs_employées_vide = nullptr;
    kuri::trie<AtomeConstante *, AtomeConstante *> trie_structs_employées{};

    /* Trie pour les entrées et sorties des fonctions. Nous partageons les tableaux pour les
     * entrées et sorties. */
    kuri::trie<Type *, AtomeConstante *> trie_types_entrée_sortie{};

    /* Un seul tableau pour toutes les fonctions n'ayant pas d'entrées. */
    AtomeConstante *tableau_types_entrées_vide = nullptr;

    /* Un seul tableau pour toutes les fonctions ne retournant « rien ». */
    AtomeConstante *tableau_types_sorties_rien = nullptr;
    // }

    /* Pour les executions des métaprogrammes. */
    std::mutex mutex_données_constantes_exécutions{};
    DonnéesConstantesExécutions données_constantes_exécutions{};

    NoeudBloc *m_bloc_racine = nullptr;

    Compilatrice &m_compilatrice;

    // Données pour le gestionnaire de code.
    kuri::tableau<UnitéCompilation *> métaprogrammes_en_attente_de_crée_contexte{};
    bool métaprogrammes_en_attente_de_crée_contexte_est_ouvert = true;

    EspaceDeTravail(Compilatrice &compilatrice, OptionsDeCompilation opts, kuri::chaine nom_);

    EMPECHE_COPIE(EspaceDeTravail);

    ~EspaceDeTravail();

    POINTEUR_NUL(EspaceDeTravail)

    Module *donne_module(const IdentifiantCode *nom_module) const;

    Fichier *fichier(int64_t indice);
    const Fichier *fichier(int64_t indice) const;

    Fichier *fichier(kuri::chaine_statique chemin) const;

    int64_t mémoire_utilisée() const;

    void rassemble_statistiques(Statistiques &stats) const;

    void tâche_ajoutée(GenreTâche genre_tâche, kuri::Synchrone<Messagère> &messagère);
    void tâche_terminée(GenreTâche genre_tâche, kuri::Synchrone<Messagère> &messagère);

    void progresse_phase_pour_tâche_terminée(GenreTâche genre_tâche,
                                             kuri::Synchrone<Messagère> &messagère);
    void regresse_phase_pour_tâche_ajoutée(GenreTâche genre_tâche,
                                           kuri::Synchrone<Messagère> &messagère);

    bool peut_génèrer_code_final() const;
    bool parsage_terminé() const;

    Message *change_de_phase(kuri::Synchrone<Messagère> &messagère,
                             PhaseCompilation nouvelle_phase,
                             kuri::chaine_statique fonction_appelante);

    PhaseCompilation phase_courante() const
    {
        return m_id_phase_courante;
    }

    int id_phase_courante() const
    {
        return id_phase;
    }

    SiteSource site_source_pour(NoeudExpression const *noeud) const;

    Erreur rapporte_avertissement(const NoeudExpression *site,
                                  kuri::chaine_statique message) const;
    Erreur rapporte_avertissement(kuri::chaine_statique fichier,
                                  int ligne,
                                  kuri::chaine_statique message) const;
    Erreur rapporte_avertissement_externe(ParamètresErreurExterne const &params) const;

    Erreur rapporte_info(const NoeudExpression *site, kuri::chaine_statique message) const;
    Erreur rapporte_info(kuri::chaine_statique fichier,
                         int ligne,
                         kuri::chaine_statique message) const;
    Erreur rapporte_info(SiteSource site, kuri::chaine_statique message) const;
    Erreur rapporte_info_externe(ParamètresErreurExterne const &params) const;

    Erreur rapporte_erreur(NoeudExpression const *site,
                           kuri::chaine_statique message,
                           erreur::Genre genre = erreur::Genre::NORMAL) const;
    Erreur rapporte_erreur(kuri::chaine_statique chemin_fichier,
                           int ligne,
                           kuri::chaine_statique message,
                           erreur::Genre genre = erreur::Genre::NORMAL) const;
    Erreur rapporte_erreur(SiteSource site,
                           kuri::chaine_statique message,
                           erreur::Genre genre = erreur::Genre::NORMAL) const;
    Erreur rapporte_erreur_sans_site(kuri::chaine_statique message,
                                     erreur::Genre genre = erreur::Genre::NORMAL) const;

    Erreur rapporte_erreur_externe(ParamètresErreurExterne const &params) const;

    Compilatrice &compilatrice()
    {
        return m_compilatrice;
    }

    Compilatrice &compilatrice() const
    {
        return m_compilatrice;
    }

    Phase *donne_phase();
    Phase *donne_phase(GenreTâche genre_tâche);

    void imprime_compte_tâches(std::ostream &os) const;
};
