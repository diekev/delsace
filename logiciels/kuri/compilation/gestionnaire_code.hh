/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "biblinternes/structures/tableau_page.hh"

#include "statistiques/statistiques.hh"

#include "structures/file.hh"
#include "structures/tableau.hh"

#include "arbre_syntaxique/allocatrice.hh"
#include "graphe_dependance.hh"
#include "unite_compilation.hh"

struct Compilatrice;
struct GrapheDependance;
struct OrdonnanceuseTache;
struct Programme;

struct DonnneesResolutionDependances {
    DonneesDependance dependances;
    DonneesDependance dependances_ependues;

    kuri::tableau<NoeudDependance *> noeuds_dependances;

    void reinitialise()
    {
        dependances.efface();
        dependances_ependues.efface();
        noeuds_dependances.efface();
    }
};

/* Le GestionnaireCode a pour tâches de gérer la compilation des programmes. Il crée les unités de
 * compilation et veille à ce qu'elles ne progressent pas dans la compilation tant qu'une de leurs
 * attentes n'est pas satisfaite, le gestionnaire étant notifié si une unité a fini son étape
 * correctement, ou si elle doit attendre que quelque chose soit faite.
 *
 * Pour chaque programme (des espaces de travail ou des métaprogrammes) le gestionnaire veille que
 * seules les dépendances des fonctions racines du programme sont compilées et y incluses.
 *
 * Enfin, le gestionnaire donne à l'OrdonnaceuseTâche les tâches à effectuer pour faire avancer la
 * compilation.
 */
class GestionnaireCode {
    /* Mutex général. */
    std::mutex m_mutex{};

    /* Toutes les unités de compilation créées pour tous les espaces. */
    tableau_page<UniteCompilation> unites{};

    template <typename T>
    using tableau_synchrone = dls::outils::Synchrone<kuri::tableau<T, int>>;
    /* Les unités qui attendent sur quelque chose. */
    tableau_synchrone<UniteCompilation *> unites_en_attente{};
    kuri::tableau<UniteCompilation *> metaprogrammes_en_attente_de_cree_contexte{};
    bool metaprogrammes_en_attente_de_cree_contexte_est_ouvert = true;

    Compilatrice *m_compilatrice = nullptr;

    /* Les programmes en cours de compilation, ils sont ajoutés et retirés de la liste en fonction
     * de leurs états de compilation. Les programmes ne sont retirés que si leur compilation ou
     * exécution est terminée. */
    kuri::file<Programme *> programmes_en_cours{};

    /* Les dépendances d'une déclaration qui sont rassemblées après la fin du typage, nous ne
     * stockons pas définitivement cette information, ce membre ne sers qu'à réutiliser la mémoire
     * allouée précédemment afin de ne pas trop faire d'allocations dynamiques. */
    DonnneesResolutionDependances dependances{};

    AllocatriceNoeud allocatrice_noeud{};
    AssembleuseArbre *m_assembleuse = nullptr;

    /* Toutes les fonctions parsées et typées lors de la compilation, qui ont traversées
     * typage_termine. Accessible via les métaprogrammes, via compilatrice_fonctions_parsées(). */
    kuri::tableau<NoeudDeclarationEnteteFonction *> m_fonctions_parsees{};

    mutable StatistiquesGestion stats{};

    /* Unités dont la dernière tâche a été terminé. */
    std::mutex m_mutex_unités_terminées{};
    kuri::tableau<UniteCompilation *> m_unités_terminées{};

    struct AttenteEspace {
        EspaceDeTravail *espace = nullptr;
        Attente attente{};
    };
    kuri::tableau<AttenteEspace> m_attentes_à_résoudre{};

    struct RequêteCompilationMétaProgramme {
        EspaceDeTravail *espace = nullptr;
        MetaProgramme *métaprogramme = nullptr;
    };
    tableau_synchrone<RequêteCompilationMétaProgramme> m_requêtes_compilations_métaprogrammes{};

    dls::chrono::compte_seconde temps_début_compilation{};
    bool imprime_débogage = true;

  public:
    GestionnaireCode() = default;
    GestionnaireCode(Compilatrice *compilatrice);

    GestionnaireCode(GestionnaireCode const &) = delete;
    GestionnaireCode &operator=(GestionnaireCode const &) = delete;

    ~GestionnaireCode();

    /* Notification qu'un espace fut créé, son programme est ajouté à la liste des programmes en
     * cours de compilation */
    void espace_cree(EspaceDeTravail *espace);

    /* Création des unités pour le typage, etc. */
    void requiers_chargement(EspaceDeTravail *espace, Fichier *fichier);

    void requiers_lexage(EspaceDeTravail *espace, Fichier *fichier);

    void requiers_parsage(EspaceDeTravail *espace, Fichier *fichier);

    void requiers_typage(EspaceDeTravail *espace, NoeudExpression *noeud);

    void requiers_generation_ri(EspaceDeTravail *espace, NoeudExpression *noeud);

    /* Crée une unité de compilation pour le métaprogramme.
     * Si peut_planifier_compilation est vrai, l'unité est ajoutée à la liste d'unités en attente.
     * Sinon, l'unité est ajoutée à la liste des métaprogrammes en attentes de la disponibilité de
     * la RI pour #crée_contexte. */
    void requiers_generation_ri_principale_metaprogramme(EspaceDeTravail *espace,
                                                         MetaProgramme *metaprogramme,
                                                         bool peut_planifier_compilation);

    void requiers_compilation_metaprogramme(EspaceDeTravail *espace, MetaProgramme *metaprogramme);

    UniteCompilation *requiers_generation_code_machine(EspaceDeTravail *espace,
                                                       Programme *programme);

    void requiers_liaison_executable(EspaceDeTravail *espace, Programme *programme);

    void requiers_initialisation_type(EspaceDeTravail *espace, Type *type);

    /* Attente sur quelque chose. */
    void mets_en_attente(UniteCompilation *unite_attendante, Attente attente);
    void mets_en_attente(UniteCompilation *unite_attendante,
                         kuri::tableau_statique<Attente> attentes);

    /* Fin d'une tâche. */
    void tâche_unité_terminée(UniteCompilation *unité);

    void envoi_message_termine(UniteCompilation *unité);

    void message_recu(Message const *message);

    const kuri::tableau<NoeudDeclarationEnteteFonction *> &fonctions_parsees() const
    {
        return m_fonctions_parsees;
    }

    /* Appelé par la MachineVirtuelle quand l'interception de messages est terminée. Ceci notifie à
     * son tour la Messagère.
     * Toutes les unités d'envoie de messages sont annulées, et toutes les unités attendant sur un
     * message sont marquées comme prêtes. */
    void interception_message_terminee(EspaceDeTravail *espace);

    void imprime_stats() const;

    void démarre_boucle_compilation();

    void ajourne_espace_pour_nouvelles_options(EspaceDeTravail *espace);

  private:
    /* Notification qu'un espace fut créé, son programme est ajouté à la liste des programmes en
     * cours de compilation */
    void metaprogramme_cree(MetaProgramme *metaprogramme);

    void requiers_compilation_metaprogramme_impl(EspaceDeTravail *espace,
                                                 MetaProgramme *metaprogramme);

    UniteCompilation *cree_unite(EspaceDeTravail *espace, RaisonDEtre raison, bool met_en_attente);
    void cree_unite_pour_fichier(EspaceDeTravail *espace, Fichier *fichier, RaisonDEtre raison);
    UniteCompilation *cree_unite_pour_noeud(EspaceDeTravail *espace,
                                            NoeudExpression *noeud,
                                            RaisonDEtre raison,
                                            bool met_en_attente);

    UniteCompilation *cree_unite_pour_message(EspaceDeTravail *espace, Message *message);

    UniteCompilation *requiers_noeud_code(EspaceDeTravail *espace, NoeudExpression *noeud);

    /* Ajoute l'unité à la liste d'attente, et change son état vers EN_ATTENTE. */
    void ajoute_unité_à_liste_attente(UniteCompilation *unité);

    void requiers_execution(EspaceDeTravail *espace, MetaProgramme *metaprogramme);

    void ajoute_programme(Programme *programme);

    void enleve_programme(Programme *programme);

    void determine_dependances(NoeudExpression *noeud,
                               EspaceDeTravail *espace,
                               GrapheDependance &graphe);

    bool plus_rien_n_est_a_faire();
    bool tente_de_garantir_presence_creation_contexte(EspaceDeTravail *espace,
                                                      Programme *programme,
                                                      GrapheDependance &graphe);

    void tente_de_garantir_fonction_point_d_entree(EspaceDeTravail *espace);

    void finalise_programme_avant_generation_code_machine(EspaceDeTravail *espace,
                                                          Programme *programme);

    void flush_metaprogrammes_en_attente_de_cree_contexte();

    void ajoute_requêtes_pour_attente(EspaceDeTravail *espace, Attente attente);

    void gère_choses_terminées();

    /* Ajoute le contenu de Typeuse.types_à_insérer_dans_graphe dans le graphe de dépendance. */
    void ajoute_types_dans_graphe();

    void gère_requête_compilations_métaprogrammes();

    void chargement_fichier_termine(UniteCompilation *unite);

    void lexage_fichier_termine(UniteCompilation *unite);

    void parsage_fichier_termine(UniteCompilation *unite);

    void typage_termine(UniteCompilation *unite);

    void generation_ri_terminee(UniteCompilation *unite);

    void execution_terminee(UniteCompilation *unite);

    void generation_code_machine_terminee(UniteCompilation *unite);

    void liaison_programme_terminee(UniteCompilation *unite);

    void conversion_noeud_code_terminee(UniteCompilation *unite);

    void fonction_initialisation_type_creee(UniteCompilation *unite);

    void optimisation_terminee(UniteCompilation *unite);

    void crée_tâches_pour_ordonnanceuse();
};
