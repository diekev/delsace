/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "statistiques/statistiques.hh"

#include "structures/file.hh"
#include "structures/tableau.hh"
#include "structures/tableau_page.hh"

#include "arbre_syntaxique/allocatrice.hh"
#include "graphe_dependance.hh"
#include "unite_compilation.hh"

struct Compilatrice;
struct GrapheDépendance;
struct OrdonnanceuseTache;
struct Programme;

struct DonnéesRésolutionDépendances {
    DonnéesDépendance dépendances;
    DonnéesDépendance dépendances_épendues;

    void reinitialise()
    {
        dépendances.efface();
        dépendances_épendues.efface();
    }
};

/* ------------------------------------------------------------------------- */
/** \name État chargement fichiers
 * \{ */

/* Cette structure tient trace du nombre d'unités pour une certaine raison d'être relative à un
 * chargement de fichier. */
struct ÉtatFileUnitésChargementFile {
    int compte = 0;
};

/**
 * Cette structure tient trace de l'état de chargement des fichiers (du chargement depuis le disque
 * jusqu'au parsage).
 *
 * Nous ne voulons pas envoyer vers la validation sémantique les unités de compilation tant que
 * tous les fichiers ne furent pas parsés et chargés afin d'éviter des fausses erreurs de
 * compilation car des unités attendèrent trop longtemps sur quelque chose qui se trouve dans un
 * fichier non encore traité.
 *
 * Pour l'instant, nous ne bloquons les unités que lors du chargement initial conséquent au
 * démarrage de la compilation depuis un fichier : nous ne bloquons pas les unités si un
 * métaprogramme ajoute du code à la compilation.
 *
 * Ceci tient trace de deux types d'unités :
 * - les unités pour le chargement/lexage/parsage,
 * - les unités pour les instruction « charge » ou « importe ».
 *
 * Nous bloquons toutes les autres unités tant que toutes les unités sus-citées ne furent pas
 * traitées.
 */
struct ÉtatChargementFichiers {
  private:
    /* Toutes les unités de compilation pour les instructions charge/importe sont ici.
     */
    UniteCompilation *file_unités_charge_ou_importe = nullptr;

    /* Le nombre d'unité pour chaque raison d'être relative à des fichiers. Nous avons un nombre
     * pour chaque raison d'être mais seules les raisons de chargement/lexage/syntaxage sont
     * utilisées. */
    ÉtatFileUnitésChargementFile nombre_d_unités_pour_raison[NOMBRE_DE_RAISON_D_ETRE] = {{}};

  public:
    /* Unités correspondants à des « charge » ou « importe ». */
    void ajoute_unité_pour_charge_ou_importe(UniteCompilation *unité);
    void supprime_unité_pour_charge_ou_importe(UniteCompilation *unité);

    /* Unités correspondants à des tâches de chargement/lexage/parsage. */
    void ajoute_unité_pour_chargement_fichier(UniteCompilation *unité);

    /* Quand un chargement ou un lexage est fini, déplace l'unité dans la file suivante. */
    void déplace_unité_pour_chargement_fichier(UniteCompilation *unité);

    /* Quand un parsage est fini, supprime l'unité de la file de parsage. */
    void supprime_unité_pour_chargement_fichier(UniteCompilation *unité);

    bool tous_les_fichiers_à_parser_le_sont() const;

    void imprime_état() const;

  private:
    void enfile(UniteCompilation *unité);
    void défile(UniteCompilation *unité);
};

/** \} */

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
    /* Toutes les unités de compilation créées pour tous les espaces. */
    kuri::tableau_page<UniteCompilation> unités{};

    /* Les unités qui attendent sur quelque chose. */
    kuri::tableau<UniteCompilation *> unités_en_attente{};
    kuri::tableau<UniteCompilation *> métaprogrammes_en_attente_de_crée_contexte{};
    bool metaprogrammes_en_attente_de_crée_contexte_est_ouvert = true;

    Compilatrice *m_compilatrice = nullptr;

    /* Les programmes en cours de compilation, ils sont ajoutés et retirés de la liste en fonction
     * de leurs états de compilation. Les programmes ne sont retirés que si leur compilation ou
     * exécution est terminée. */
    kuri::file<Programme *> programmes_en_cours{};

    /* Les dépendances d'une déclaration qui sont rassemblées après la fin du typage, nous ne
     * stockons pas définitivement cette information, ce membre ne sers qu'à réutiliser la mémoire
     * allouée précédemment afin de ne pas trop faire d'allocations dynamiques. */
    DonnéesRésolutionDépendances dépendances{};

    AllocatriceNoeud allocatrice_noeud{};
    AssembleuseArbre *m_assembleuse = nullptr;

    /* Toutes les fonctions parsées et typées lors de la compilation, qui ont traversées
     * typage_terminé. Accessible via les métaprogrammes, via compilatrice_fonctions_parsées(). */
    kuri::tableau<NoeudDéclarationEntêteFonction *> m_fonctions_parsées{};

    mutable StatistiquesGestion stats{};

    ÉtatChargementFichiers m_état_chargement_fichiers{};

    /* Tous les noeuds autres que les noeuds de charge ou importe d'un fichier. Ces noeuds seront
     * ajoutés à la compilation lorsque les fichiers seront tous parsés. */
    struct InfoNoeudÀValider {
        EspaceDeTravail *espace = nullptr;
        NoeudExpression *noeud = nullptr;
    };
    kuri::tableau<InfoNoeudÀValider> m_noeuds_à_valider{};

    /* Toutes les fonctions d'initialisation de type créées avant que tous les fichiers ne soient
     * parsés; elles seront ajoutées à la compilation lorsque les fichiers le seront. */
    struct InfoPourFonctionInit {
        EspaceDeTravail *espace = nullptr;
        Type *type = nullptr;
    };
    kuri::tableau<InfoPourFonctionInit> m_fonctions_init_type_requises{};

    bool m_validation_doit_attendre_sur_lexage = true;

    /* Utilisé afin de récupérer la mémoire dans crée_tâches. */
    kuri::tableau<UniteCompilation *> m_nouvelles_unités{};

  public:
    GestionnaireCode() = default;
    GestionnaireCode(Compilatrice *compilatrice);

    GestionnaireCode(GestionnaireCode const &) = delete;
    GestionnaireCode &operator=(GestionnaireCode const &) = delete;

    ~GestionnaireCode();

    /* Notification qu'un espace fut créé, son programme est ajouté à la liste des programmes en
     * cours de compilation */
    void espace_créé(EspaceDeTravail *espace);

    /* Notification qu'un espace fut créé, son programme est ajouté à la liste des programmes en
     * cours de compilation */
    void métaprogramme_créé(MetaProgramme *metaprogramme);

    /* Création des unités pour le typage, etc. */
    void requiers_chargement(EspaceDeTravail *espace, Fichier *fichier);

    void requiers_lexage(EspaceDeTravail *espace, Fichier *fichier);

    void requiers_parsage(EspaceDeTravail *espace, Fichier *fichier);

    void requiers_typage(EspaceDeTravail *espace, NoeudExpression *noeud);

    void requiers_génération_ri(EspaceDeTravail *espace, NoeudExpression *noeud);

    /* Crée une unité de compilation pour le métaprogramme.
     * Si peut_planifier_compilation est vrai, l'unité est ajoutée à la liste d'unités en attente.
     * Sinon, l'unité est ajoutée à la liste des métaprogrammes en attentes de la disponibilité de
     * la RI pour #crée_contexte. */
    void requiers_génération_ri_principale_métaprogramme(EspaceDeTravail *espace,
                                                         MetaProgramme *metaprogramme,
                                                         bool peut_planifier_compilation);

    void requiers_compilation_métaprogramme(EspaceDeTravail *espace, MetaProgramme *metaprogramme);

    UniteCompilation *requiers_génération_code_machine(EspaceDeTravail *espace,
                                                       Programme *programme);

    void requiers_liaison_executable(EspaceDeTravail *espace, Programme *programme);

    void requiers_initialisation_type(EspaceDeTravail *espace, Type *type);

    UniteCompilation *crée_unité(EspaceDeTravail *espace, RaisonDÊtre raison, bool met_en_attente);
    UniteCompilation *crée_unité_pour_fichier(EspaceDeTravail *espace,
                                              Fichier *fichier,
                                              RaisonDÊtre raison);
    UniteCompilation *crée_unité_pour_noeud(EspaceDeTravail *espace,
                                            NoeudExpression *noeud,
                                            RaisonDÊtre raison,
                                            bool met_en_attente);

    /* Attente sur quelque chose. */
    void mets_en_attente(UniteCompilation *unite_attendante, Attente attente);
    void mets_en_attente(UniteCompilation *unite_attendante,
                         kuri::tableau_statique<Attente> attentes);

    /* Fin d'une tâche. */
    void tâche_unité_terminée(UniteCompilation *unité);

    void message_reçu(Message const *message);

    void rassemble_statistiques(Statistiques &stats) const;

  private:
    void chargement_fichier_terminé(UniteCompilation *unite);

    void lexage_fichier_terminé(UniteCompilation *unite);

    void parsage_fichier_terminé(UniteCompilation *unite);

    void typage_terminé(UniteCompilation *unite);

    void generation_ri_terminée(UniteCompilation *unite);

    void envoi_message_terminé(UniteCompilation *unité);

    void execution_terminée(UniteCompilation *unite);

    void generation_code_machine_terminée(UniteCompilation *unite);

    void liaison_programme_terminée(UniteCompilation *unite);

    void conversion_noeud_code_terminée(UniteCompilation *unite);

    void fonction_initialisation_type_créée(UniteCompilation *unite);

    void optimisation_terminée(UniteCompilation *unite);

  public:
    /* Remplis les tâches. */
    void crée_tâches(OrdonnanceuseTache &ordonnanceuse);

    const kuri::tableau<NoeudDéclarationEntêteFonction *> &fonctions_parsées() const
    {
        return m_fonctions_parsées;
    }

    /* Appelé par la MachineVirtuelle quand l'interception de messages est terminée. Ceci notifie à
     * son tour la Messagère.
     * Toutes les unités d'envoie de messages sont annulées, et toutes les unités attendant sur un
     * message sont marquées comme prêtes. */
    void interception_message_terminée(EspaceDeTravail *espace);

    void ajourne_espace_pour_nouvelles_options(EspaceDeTravail *espace);

    void imprime_stats() const;

  private:
    UniteCompilation *crée_unité_pour_message(EspaceDeTravail *espace, Message *message);

    UniteCompilation *requiers_noeud_code(EspaceDeTravail *espace, NoeudExpression *noeud);

    /* Ajoute l'unité à la liste d'attente, et change son état vers EN_ATTENTE. */
    void ajoute_unité_à_liste_attente(UniteCompilation *unité);

    void requiers_exécution(EspaceDeTravail *espace, MetaProgramme *metaprogramme);

    void ajoute_programme(Programme *programme);

    void enleve_programme(Programme *programme);

    void détermine_dépendances(NoeudExpression *noeud,
                               EspaceDeTravail *espace,
                               GrapheDépendance &graphe);

    bool plus_rien_n_est_à_faire();
    bool tente_de_garantir_présence_création_contexte(EspaceDeTravail *espace,
                                                      Programme *programme,
                                                      GrapheDépendance &graphe);

    void tente_de_garantir_fonction_point_d_entrée(EspaceDeTravail *espace);

    void finalise_programme_avant_génération_code_machine(EspaceDeTravail *espace,
                                                          Programme *programme);

    void flush_métaprogrammes_en_attente_de_crée_contexte();

    void ajoute_requêtes_pour_attente(EspaceDeTravail *espace, Attente attente);

    void imprime_état_parsage() const;

    bool tous_les_fichiers_à_parser_le_sont() const;

    void flush_noeuds_à_typer();
};
