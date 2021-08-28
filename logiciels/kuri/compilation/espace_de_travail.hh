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

#include "biblinternes/moultfilage/synchrone.hh"
#include "biblinternes/structures/dico.hh"

#include "structures/table_hachage.hh"

#include "representation_intermediaire/instructions.hh"

#include "parsage/modules.hh"

#include "bibliotheque.hh"
#include "erreur.h"
#include "graphe_dependance.hh"
#include "interface_module_kuri.hh"
#include "messagere.hh"
#include "metaprogramme.hh"
#include "operateurs.hh"
#include "options.hh"
#include "typage.hh"

struct Coulisse;
struct ConstructriceRI;
struct Programme;

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
    std::atomic<int> nombre_taches_chargement = 0;
    std::atomic<int> nombre_taches_lexage = 0;
    std::atomic<int> nombre_taches_parsage = 0;
    std::atomic<int> nombre_taches_typage = 0;
    std::atomic<int> nombre_taches_ri = 0;
    std::atomic<int> nombre_taches_execution = 0;
    std::atomic<int> nombre_taches_optimisation = 0;

    PhaseCompilation phase = PhaseCompilation::PARSAGE_EN_COURS;

  public:
    kuri::chaine nom{};
    OptionsDeCompilation options{};

    template <typename T>
    using tableau_page_synchrone = dls::outils::Synchrone<tableau_page<T>>;

    tableau_page_synchrone<Module> modules{};
    tableau_page_synchrone<Fichier> fichiers{};
    tableau_page_synchrone<MetaProgramme> metaprogrammes{};

    kuri::tableau<Fichier *> table_fichiers{};

    dls::outils::Synchrone<GrapheDependance> graphe_dependance{};

    dls::outils::Synchrone<Operateurs> operateurs{};

    Typeuse typeuse;

    dls::outils::Synchrone<InterfaceKuri> interface_kuri{};

    Programme *programme = nullptr;

    tableau_page<AtomeFonction> fonctions{};
    tableau_page<AtomeGlobale> globales{};

    struct DonneesConstructeurGlobale {
        AtomeGlobale *atome = nullptr;
        NoeudExpression *expression = nullptr;
        TransformationType transformation{};
    };

    using ConteneurConstructeursGlobales = kuri::tableau<DonneesConstructeurGlobale, int>;
    dls::outils::Synchrone<ConteneurConstructeursGlobales> constructeurs_globaux{};

    using TableChaine = kuri::table_hachage<kuri::chaine_statique, AtomeConstante *>;
    dls::outils::Synchrone<TableChaine> table_chaines{};

    std::mutex mutex_atomes_fonctions{};
    std::mutex mutex_atomes_globales{};

    /* mise en cache de la fonction principale, si vue dans la Syntaxeuse */
    NoeudDeclarationEnteteFonction *fonction_principale = nullptr;

    NoeudDeclarationEnteteFonction *fonction_point_d_entree = nullptr;

    /* Le métaprogramme controlant la compilation dans cette espace. */
    MetaProgramme *metaprogramme = nullptr;

    Coulisse *coulisse = nullptr;

    Module *module_kuri = nullptr;

    dls::outils::Synchrone<GestionnaireBibliotheques> gestionnaire_bibliotheques;

    /* pour activer ou désactiver les optimisations */
    bool optimisations = false;
    mutable std::atomic<bool> possede_erreur{false};

    Compilatrice &m_compilatrice;

    /* Pour les executions des métaprogrammes. */
    std::mutex mutex_donnees_constantes_executions{};
    DonneesConstantesExecutions donnees_constantes_executions{};

    EspaceDeTravail(Compilatrice &compilatrice, OptionsDeCompilation opts, kuri::chaine nom_);

    COPIE_CONSTRUCT(EspaceDeTravail);

    ~EspaceDeTravail();

    POINTEUR_NUL(EspaceDeTravail)

    /**
     * Retourne un pointeur vers le module avec le nom et le chemin spécifiés.
     * Si un tel module n'existe pas, un nouveau module est créé.
     */
    Module *trouve_ou_cree_module(dls::outils::Synchrone<SystemeModule> &sys_module,
                                  IdentifiantCode *nom_module,
                                  kuri::chaine_statique chemin);

    /**
     * Retourne un pointeur vers le module dont le nom est spécifié. Si aucun
     * module n'a ce nom, retourne nullptr.
     */
    Module *module(const IdentifiantCode *nom_module) const;

    /**
     * Crée un fichier avec le nom spécifié, et retourne un pointeur vers le
     * fichier ainsi créé ou un pointeur vers un fichier existant.
     */
    ResultatFichier trouve_ou_cree_fichier(dls::outils::Synchrone<SystemeModule> &sys_module,
                                           Module *module,
                                           kuri::chaine_statique nom_fichier,
                                           kuri::chaine_statique chemin,
                                           bool importe_kuri);

    Fichier *cree_fichier_pour_metaprogramme(MetaProgramme *metaprogramme);

    /**
     * Retourne un pointeur vers le fichier à l'index indiqué. Si l'index est
     * en dehors de portée, le programme crashera.
     */
    Fichier *fichier(long index) const;

    /**
     * Retourne un pointeur vers le module dont le chemin est spécifié. Si aucun
     * fichier n'a ce nom, retourne nullptr.
     */
    Fichier *fichier(const dls::vue_chaine_compacte &chemin) const;

    AtomeFonction *cree_fonction(Lexeme const *lexeme, kuri::chaine const &nom_fonction);
    AtomeFonction *cree_fonction(Lexeme const *lexeme,
                                 kuri::chaine const &nom_fonction,
                                 kuri::tableau<Atome *, int> &&params);
    AtomeFonction *trouve_ou_insere_fonction(ConstructriceRI &constructrice,
                                             NoeudDeclarationEnteteFonction *decl);
    AtomeFonction *trouve_fonction(kuri::chaine const &nom_fonction);

    AtomeGlobale *cree_globale(Type *type,
                               AtomeConstante *valeur,
                               bool initialisateur,
                               bool est_constante);
    AtomeGlobale *trouve_globale(NoeudDeclaration *decl);
    AtomeGlobale *trouve_ou_insere_globale(NoeudDeclaration *decl);

    long memoire_utilisee() const;

    void rassemble_statistiques(Statistiques &stats) const;

    MetaProgramme *cree_metaprogramme();

    void tache_chargement_ajoutee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_lexage_ajoutee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_parsage_ajoutee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_typage_ajoutee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_ri_ajoutee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_optimisation_ajoutee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_execution_ajoutee(dls::outils::Synchrone<Messagere> &messagere);

    void tache_chargement_terminee(dls::outils::Synchrone<Messagere> &messagere, Fichier *fichier);
    void tache_lexage_terminee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_parsage_terminee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_typage_terminee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_ri_terminee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_optimisation_terminee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_execution_terminee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_generation_objet_terminee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_liaison_executable_terminee(dls::outils::Synchrone<Messagere> &messagere);

    bool peut_generer_code_final() const;
    bool parsage_termine() const;

    void change_de_phase(dls::outils::Synchrone<Messagere> &messagere,
                         PhaseCompilation nouvelle_phase);
    PhaseCompilation phase_courante() const;

    void rapporte_avertissement(NoeudExpression *site, kuri::chaine_statique message) const;
    void rapporte_avertissement(kuri::chaine const &fichier,
                                int ligne,
                                kuri::chaine const &message) const;

    Erreur rapporte_erreur(NoeudExpression const *site,
                           kuri::chaine_statique message,
                           erreur::Genre genre = erreur::Genre::NORMAL) const;
    Erreur rapporte_erreur(kuri::chaine const &fichier,
                           int ligne,
                           kuri::chaine const &message) const;
    Erreur rapporte_erreur_sans_site(const kuri::chaine &message,
                                     erreur::Genre genre = erreur::Genre::NORMAL) const;

    /* Imprime la RI de toutes les fonctions de l'espace de travail. */
    void imprime_programme() const;

    Compilatrice &compilatrice()
    {
        return m_compilatrice;
    }

    void imprime_compte_taches(std::ostream &os) const;
};
