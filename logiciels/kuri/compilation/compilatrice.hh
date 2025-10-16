/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#pragma once

#include "parsage/gerante_chaine.hh"
#include "parsage/identifiant.hh"

#include "bibliotheque.hh"
#include "erreur.h"
#include "gestionnaire_code.hh"
#include "messagere.hh"
#include "metaprogramme.hh"
#include "options.hh"
#include "structures.hh"
#include "tacheronne.hh"

#include "structures/chemin_systeme.hh"
#include "structures/date.hh"

#include "utilitaires/synchrone.hh"

class Broyeuse;
struct ContexteLexage;
struct ConvertisseuseNoeudCode;
struct EspaceDeTravail;
struct NoeudCodeEntêteFonction;
struct OptionsDeCompilation;
struct Sémanticienne;
struct Statistiques;

enum class FormatRapportProfilage : int {
    BRENDAN_GREGG,
    ECHANTILLONS_TOTAL_POUR_FONCTION,
};

namespace kuri {
struct Lexème;
}

struct GestionnaireChainesAjoutées {
  private:
    kuri::tableau<kuri::chaine, int> m_chaines{};

    /* Ceci est utilisé pour trouver la position de la chaine dans le fichier final.
     * Nous commençons à 2, car le fichier est préfixé par la date et l'heure, et
     * d'une ligne vide.
     */
    int64_t nombre_total_de_lignes = 2;

  public:
    int64_t ajoute(kuri::chaine chaine);

    int nombre_de_chaines() const;

    void imprime_dans(std::ostream &os);

    int64_t mémoire_utilisée() const;
};

/* Options passées sur la ligne de commande. */
struct ArgumentsCompilatrice {
    bool active_tests = false;
    bool profile_metaprogrammes = false;
    bool debogue_execution = false;
    bool émets_stats_ops_exécution = false;
    bool préserve_symboles = false;
    bool avec_stats = false;
    bool stats_détaillées = false;
    bool sans_traces_d_appel = false;
    bool émets_ri = false;
    bool émets_code_binaire = false;
    bool importe_kuri = true;
    bool compile_en_mode_parallèle = false;
    bool verbeux = false;
    FormatRapportProfilage format_rapport_profilage = FormatRapportProfilage::BRENDAN_GREGG;

    TypeCoulisse coulisse = TypeCoulisse::C;

    /* Fichier où inscrire les fichiers utilisés si --emets_fichiers_utilises fut renseigné. */
    kuri::chemin_systeme chemin_fichier_utilises{};

    /* La liste des arguments en ligne de commande passés après "--". */
    kuri::tableau<kuri::chaine_statique> arguments_pour_métaprogrammes{};
};

struct Compilatrice {
    kuri::Synchrone<TableIdentifiant> table_identifiants{};

    kuri::Synchrone<OrdonnanceuseTache> ordonnanceuse;

    kuri::Synchrone<GeranteChaine> gérante_chaine{};

    kuri::Synchrone<Messagère> messagère{};

    kuri::Synchrone<GestionnaireCode> gestionnaire_code{};

    kuri::Synchrone<GestionnaireBibliothèques> gestionnaire_bibliothèques;

    /* Option pour pouvoir désactivé l'import implicite de Kuri dans les tests unitaires notamment.
     */
    bool importe_kuri = true;
    bool m_possède_erreur = false;
    erreur::Genre m_code_erreur{};

    ArgumentsCompilatrice arguments{};

    kuri::Synchrone<GestionnaireChainesAjoutées> chaines_ajoutées_à_la_compilation{};

    kuri::tableau_synchrone<EspaceDeTravail *> espaces_de_travail{};
    EspaceDeTravail *espace_de_travail_défaut = nullptr;

    kuri::chemin_systeme racine_kuri{};
    kuri::chemin_systeme racine_modules_kuri{};

    kuri::tableau_page_synchrone<MetaProgramme> métaprogrammes{};

    /* Pour les executions des métaprogrammes. */
    std::mutex mutex_données_constantes_exécutions{};
    DonnéesConstantesExécutions données_constantes_exécutions{};

    kuri::Synchrone<RegistreChainesRI> registre_chaines_ri{};

    Broyeuse *broyeuse = nullptr;

    /* Tous les tableaux créés pour les appels à #compilatrice_fonctions_parsées. */
    kuri::tableau<kuri::tableau<NoeudCodeEntêteFonction *>> m_tableaux_code_fonctions{};

    /* Tous les tableaux créés pour les appels à #compilatrice_lèxe_fichier. */
    kuri::tableau<kuri::tableau<kuri::Lexème>> m_tableaux_lexèmes{};

    kuri::tableau<ÉtatRésolutionAppel *> m_états_libres{};

  private:
    /* Note la date de début de la compilation. Principalement utilisé pour générer les noms des
     * fichiers de logs. */
    Date m_date_début_compilation{};

    kuri::table_hachage<kuri::chaine, int> m_nombre_occurences_chaines{"noms_uniques"};

    std::mutex m_mutex_noms_valeurs_retours_défaut{};
    kuri::tableau<IdentifiantCode *> m_noms_valeurs_retours_défaut{};

    std::mutex m_mutex_sémanticiennes{};
    kuri::tableau<Sémanticienne *> m_sémanticiennes{};

    std::mutex m_mutex_convertisseuses_noeud_code{};
    kuri::tableau<ConvertisseuseNoeudCode *> m_convertisseuses_noeud_code{};

  public:
    /* ********************************************************************** */

    Compilatrice(kuri::chaine chemin_racine_kuri, ArgumentsCompilatrice arguments_);

    /* ********************************************************************** */

    /* Désactive la copie, car il ne peut y avoir qu'un seul contexte par
     * compilation. */
    Compilatrice(const Compilatrice &) = delete;
    Compilatrice &operator=(const Compilatrice &) = delete;

    ~Compilatrice();

    /* ********************************************************************** */

    Fichier *crée_fichier_pour_metaprogramme(EspaceDeTravail *espace,
                                             MetaProgramme *metaprogramme);

    Fichier *crée_fichier_pour_insère(EspaceDeTravail *espace, NoeudDirectiveInsère *insère);

    MetaProgramme *crée_metaprogramme(EspaceDeTravail *espace);

    /* ********************************************************************** */

    void ajoute_fichier_a_la_compilation(EspaceDeTravail *espace,
                                         kuri::chaine_statique chemin,
                                         Module *module,
                                         NoeudExpression const *site);

    /* ********************************************************************** */

    EspaceDeTravail *démarre_un_espace_de_travail(OptionsDeCompilation const &options,
                                                  kuri::chaine_statique nom,
                                                  kuri::chaine_statique dossier);

    /* ********************************************************************** */

    ContexteLexage contexte_lexage(EspaceDeTravail *espace);

    int64_t memoire_utilisee() const;

    void rassemble_statistiques(Statistiques &stats) const;

    void rapporte_avertissement(kuri::chaine_statique message);

    void rapporte_erreur(EspaceDeTravail const *espace,
                         kuri::chaine_statique message,
                         erreur::Genre genre);

    bool possède_erreur() const
    {
        return m_possède_erreur;
    }

    bool possède_erreur(EspaceDeTravail const *espace) const;

    erreur::Genre code_erreur() const
    {
        return m_code_erreur;
    }

  public:
    OptionsDeCompilation *options_compilation();
    void ajourne_options_compilation(OptionsDeCompilation *options);
    void ajoute_chaine_compilation(EspaceDeTravail *espace,
                                   NoeudExpression const *site,
                                   kuri::chaine_statique c);
    void ajoute_chaine_au_module(EspaceDeTravail *espace,
                                 NoeudExpression const *site,
                                 Module *module,
                                 kuri::chaine_statique c);
    void ajoute_fichier_compilation(EspaceDeTravail *espace,
                                    kuri::chaine_statique c,
                                    const NoeudExpression *site);
    const Message *attend_message();
    EspaceDeTravail *espace_défaut_compilation();
    kuri::tableau_statique<kuri::Lexème> lexe_fichier(EspaceDeTravail *espace,
                                                      kuri::chaine_statique chemin_donne,
                                                      const NoeudExpression *site);

    kuri::tableau_statique<NoeudCodeEntêteFonction *> fonctions_parsees(EspaceDeTravail *espace);
    MetaProgramme *metaprogramme_pour_fonction(const NoeudDéclarationEntêteFonction *entete);

    /* Création/suppression d'états pour les résolutions des expressions d'appels. */
    ÉtatRésolutionAppel *crée_ou_donne_état_résolution_appel();
    void libère_état_résolution_appel(ÉtatRésolutionAppel *&état);

    Date donne_date_début_compilation() const
    {
        return m_date_début_compilation;
    }

    int donne_nombre_occurences_chaine(kuri::chaine_statique chn);

    IdentifiantCode *donne_identifiant_pour_globale(kuri::chaine_statique nom_de_base);

    IdentifiantCode *donne_nom_défaut_valeur_retour(int index);

    Sémanticienne *donne_sémanticienne_disponible(Contexte *contexte);
    void dépose_sémanticienne(Sémanticienne *sémanticienne);
    ConvertisseuseNoeudCode *donne_convertisseuse_noeud_code_disponible();
    void dépose_convertisseuse(ConvertisseuseNoeudCode *convertisseuse);
};

int fonction_test_variadique_externe(int sentinel, ...);

std::optional<kuri::chemin_systeme> determine_chemin_absolu(EspaceDeTravail *espace,
                                                            kuri::chaine_statique chemin,
                                                            NoeudExpression const *site);
