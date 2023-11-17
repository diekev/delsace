/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#pragma once

#include "biblinternes/moultfilage/synchrone.hh"

#include "parsage/gerante_chaine.hh"
#include "parsage/identifiant.hh"
#include "parsage/modules.hh"

#include "bibliotheque.hh"
#include "gestionnaire_code.hh"
#include "graphe_dependance.hh"
#include "interface_module_kuri.hh"
#include "messagere.hh"
#include "metaprogramme.hh"
#include "operateurs.hh"
#include "structures.hh"
#include "tacheronne.hh"
#include "typage.hh"

#include "structures/chemin_systeme.hh"

class Broyeuse;
struct ContexteLexage;
struct EspaceDeTravail;
struct NoeudCodeEnteteFonction;
struct OptionsDeCompilation;
struct Statistiques;

enum class FormatRapportProfilage : int {
    BRENDAN_GREGG,
    ECHANTILLONS_TOTAL_POUR_FONCTION,
};

namespace kuri {
struct Lexeme;
}

struct GestionnaireChainesAjoutees {
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
};

/* Options passées sur la ligne de commande. */
struct ArgumentsCompilatrice {
    bool active_tests = false;
    bool profile_metaprogrammes = false;
    bool debogue_execution = false;
    FormatRapportProfilage format_rapport_profilage = FormatRapportProfilage::BRENDAN_GREGG;

    /* Fichier où inscrire les fichiers utilisés si --emets_fichiers_utilises fut renseigné. */
    kuri::chemin_systeme chemin_fichier_utilises{};

    /* La liste des arguments en ligne de commande passés après "--". */
    kuri::tableau<kuri::chaine_statique> arguments_pour_métaprogrammes{};
};

struct Compilatrice {
    dls::outils::Synchrone<TableIdentifiant> table_identifiants{};

    dls::outils::Synchrone<OrdonnanceuseTache> ordonnanceuse;

    dls::outils::Synchrone<GeranteChaine> gerante_chaine{};

    dls::outils::Synchrone<Messagère> messagere{};

    dls::outils::Synchrone<GestionnaireCode> gestionnaire_code{};

    dls::outils::Synchrone<GestionnaireBibliotheques> gestionnaire_bibliotheques;

    /* Option pour pouvoir désactivé l'import implicite de Kuri dans les tests unitaires notamment.
     */
    bool importe_kuri = true;
    bool m_possède_erreur = false;
    erreur::Genre m_code_erreur{};

    ArgumentsCompilatrice arguments{};

    dls::outils::Synchrone<GestionnaireChainesAjoutees> chaines_ajoutées_à_la_compilation{};

    kuri::tableau_synchrone<EspaceDeTravail *> espaces_de_travail{};
    EspaceDeTravail *espace_de_travail_defaut = nullptr;

    kuri::chaine racine_kuri{};

    dls::outils::Synchrone<SystèmeModule> sys_module{};

    tableau_page_synchrone<MetaProgramme> metaprogrammes{};

    dls::outils::Synchrone<GrapheDependance> graphe_dependance{};

    dls::outils::Synchrone<RegistreDesOpérateurs> operateurs{};

    Typeuse typeuse;

    dls::outils::Synchrone<InterfaceKuri> interface_kuri{};
    NoeudDeclarationEnteteFonction *fonction_point_d_entree = nullptr;
    NoeudDeclarationEnteteFonction *fonction_point_d_entree_dynamique = nullptr;
    NoeudDeclarationEnteteFonction *fonction_point_de_sortie_dynamique = nullptr;

    /* Globale pour __contexte_fil_principal, définie dans le module Kuri. */
    NoeudDeclarationVariable *globale_contexte_programme = nullptr;

    /* Pour les executions des métaprogrammes. */
    std::mutex mutex_données_constantes_exécutions{};
    DonneesConstantesExecutions données_constantes_exécutions{};

    struct DonneesConstructeurGlobale {
        AtomeGlobale *atome = nullptr;
        NoeudExpression *expression = nullptr;
        TransformationType transformation{};
    };

    using ConteneurConstructeursGlobales = kuri::tableau<DonneesConstructeurGlobale, int>;
    dls::outils::Synchrone<ConteneurConstructeursGlobales> constructeurs_globaux{};

    using TableChaine = kuri::table_hachage<kuri::chaine_statique, AtomeConstante *>;
    dls::outils::Synchrone<TableChaine> table_chaines{"Table des chaines"};

    Module *module_kuri = nullptr;
    Module *module_racine_compilation = nullptr;

    RegistreSymboliqueRI *registre_ri = nullptr;

    /* À FAIRE : nous pourrions stocker les tâcheronnes, et utiliser la première tâcheronne
     * disponible. */
    ConvertisseuseNoeudCode convertisseuse_noeud_code{};

    Broyeuse *broyeuse = nullptr;

    /* Tous les tableaux créés pour les appels à #compilatrice_fonctions_parsées. */
    kuri::tableau<kuri::tableau<NoeudCodeEnteteFonction *>> m_tableaux_code_fonctions{};

    /* Tous les tableaux créés pour les appels à #compilatrice_lèxe_fichier. */
    kuri::tableau<kuri::tableau<kuri::Lexeme>> m_tableaux_lexemes{};

    kuri::tableau<EtatResolutionAppel *> m_états_libres{};

    /* ********************************************************************** */

    Compilatrice(kuri::chaine chemin_racine_kuri, ArgumentsCompilatrice arguments_);

    /* ********************************************************************** */

    /* Désactive la copie, car il ne peut y avoir qu'un seul contexte par
     * compilation. */
    Compilatrice(const Compilatrice &) = delete;
    Compilatrice &operator=(const Compilatrice &) = delete;

    ~Compilatrice();

    /* ********************************************************************** */

    /**
     * Charge le module dont le nom est spécifié.
     *
     * Le nom doit être celui d'un fichier s'appelant '<nom>.kuri' et se trouvant
     * dans le dossier du module racine.
     *
     * Les fonctions contenues dans le module auront leurs noms préfixés par le nom
     * du module, sauf pour le module racine.
     *
     * Le std::ostream est un flux de sortie où sera imprimé le nom du module ouvert
     * pour tenir compte de la progression de la compilation. Si un nom de module ne
     * pointe pas vers un fichier Kuri, ou si le fichier ne peut être ouvert, une
     * exception est lancée.
     *
     * Les Lexeme doivent être celles du nom du module et sont utilisées
     * pour les erreurs lancées.
     *
     * Le paramètre est_racine ne doit être vrai que pour le module racine.
     */
    Module *importe_module(EspaceDeTravail *espace,
                           kuri::chaine_statique nom,
                           NoeudExpression const *site);

    /**
     * Retourne un pointeur vers le module avec le nom et le chemin spécifiés.
     * Si un tel module n'existe pas, un nouveau module est créé.
     */
    Module *trouve_ou_crée_module(IdentifiantCode *nom_module, kuri::chaine_statique chemin);

    /**
     * Retourne un pointeur vers le module dont le nom est spécifié. Si aucun
     * module n'a ce nom, retourne nullptr.
     */
    Module *module(const IdentifiantCode *nom_module) const;

    /**
     * Crée un fichier avec le nom spécifié, et retourne un pointeur vers le
     * fichier ainsi créé ou un pointeur vers un fichier existant.
     */
    ResultatFichier trouve_ou_crée_fichier(Module *module,
                                           kuri::chaine_statique nom_fichier,
                                           kuri::chaine_statique chemin,
                                           bool importe_kuri);

    Fichier *crée_fichier_pour_metaprogramme(MetaProgramme *metaprogramme);

    /**
     * Retourne un pointeur vers le fichier à l'index indiqué. Si l'index est
     * en dehors de portée, le programme crashera.
     */
    Fichier *fichier(int64_t index);
    const Fichier *fichier(int64_t index) const;

    /**
     * Retourne un pointeur vers le module dont le chemin est spécifié. Si aucun
     * fichier n'a ce nom, retourne nullptr.
     */
    Fichier *fichier(kuri::chaine_statique chemin) const;

    MetaProgramme *crée_metaprogramme(EspaceDeTravail *espace);

    /* ********************************************************************** */

    void ajoute_fichier_a_la_compilation(EspaceDeTravail *espace,
                                         kuri::chaine_statique chemin,
                                         Module *module,
                                         NoeudExpression const *site);

    /* ********************************************************************** */

    EspaceDeTravail *demarre_un_espace_de_travail(OptionsDeCompilation const &options,
                                                  kuri::chaine const &nom);

    /* ********************************************************************** */

    ContexteLexage contexte_lexage(EspaceDeTravail *espace);

    int64_t memoire_utilisee() const;

    void rassemble_statistiques(Statistiques &stats) const;

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
    EspaceDeTravail *espace_defaut_compilation();
    kuri::tableau_statique<kuri::Lexeme> lexe_fichier(EspaceDeTravail *espace,
                                                      kuri::chaine_statique chemin_donne,
                                                      const NoeudExpression *site);

    kuri::tableau_statique<NoeudCodeEnteteFonction *> fonctions_parsees(EspaceDeTravail *espace);
    MetaProgramme *metaprogramme_pour_fonction(const NoeudDeclarationEnteteFonction *entete);

    /* Création/suppression d'états pour les résolutions des expressions d'appels. */
    EtatResolutionAppel *crée_ou_donne_état_résolution_appel();
    void libère_état_résolution_appel(EtatResolutionAppel *&état);
};

int fonction_test_variadique_externe(int sentinel, ...);
