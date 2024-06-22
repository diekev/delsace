/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#else
typedef unsigned char bool;
#endif

struct ContexteKuri;

struct FilExecution;

uint32_t EXETRON_nombre_fils_materiel();

struct FilExecution *EXETRON_cree_fil(struct ContexteKuri *ctx_kuri,
                                      void (*rappel)(void *),
                                      void *argument);

/* À FAIRE : ajout d'une durée minimum à attendre avant d'annuler si le fil ne fut pas joint. */
bool EXETRON_joins_fil(struct FilExecution *ptr_fil);

void EXETRON_detache_fil(struct FilExecution *ptr_fil);

void EXETRON_detruit_fil(struct ContexteKuri *ctx_kuri, struct FilExecution *ptr_fil);

struct Mutex;

struct Mutex *EXETRON_cree_mutex(struct ContexteKuri *ctx_kuri);

void EXETRON_verrouille_mutex(struct Mutex *mutex);

void EXETRON_deverrouille_mutex(struct Mutex *mutex);

void EXETRON_detruit_mutex(struct ContexteKuri *ctx_kuri, struct Mutex *mutex);

struct VariableCondition;

struct VariableCondition *EXETRON_cree_variable_condition(struct ContexteKuri *ctx_kuri);

void EXETRON_variable_condition_notifie_tous(struct VariableCondition *condition_variable);

void EXETRON_variable_condition_notifie_un(struct VariableCondition *condition_variable);

void EXETRON_variable_condition_attend(struct VariableCondition *condition_variable,
                                       struct Mutex *mutex);

void EXETRON_detruit_variable_condition(struct ContexteKuri *ctx_kuri,
                                        struct VariableCondition *mutex);

/* Exécutions en boucle parallèle. */

/* Plage pour une exécution en parallèle. */
struct PlageExecution {
    int64_t debut;
    int64_t fin;
};

/* Données des tâches à exécuter dans une boucle parallèle. */
struct DonneesTacheParallele {
    void (*fonction)(struct DonneesTacheParallele *donnees, struct PlageExecution const *plage);
};

/* Exécution de la tâche en série. */
void EXETRON_boucle_serie(struct PlageExecution const *plage,
                          struct DonneesTacheParallele *donnees);

/* Exécution de la tâche en parallèle. Si la taille de la plage est inférieure ou égale à la
 * granularite, la tâche sera exécuté dans le fil d'exécution que la fonction appelante. */
void EXETRON_boucle_parallele(struct PlageExecution const *plage,
                              struct DonneesTacheParallele *donnees,
                              int granularite);

/* Exécution de la tâche en parallèle pour des calculs léger. */
void EXETRON_boucle_parallele_legere(struct PlageExecution const *plage,
                                     struct DonneesTacheParallele *donnees);

/* Exécution de la tâche en parallèle pour des calculs lourds. */
void EXETRON_boucle_parallele_lourde(struct PlageExecution const *plage,
                                     struct DonneesTacheParallele *donnees);

struct poignee_graphe_execution;
typedef void (*fonction_noeud_execution)(void *);

struct poignee_graphe_execution *EXETRON_cree_graphe_execution();
void EXETRON_detruit_graphe_execution(struct poignee_graphe_execution *graphe);

void EXETRON_graphe_execute(struct poignee_graphe_execution *graphe, uint64_t racine);

void EXETRON_graphe_attend_sur_tous(struct poignee_graphe_execution *graphe);

void EXETRON_graphe_execute_et_attend_sur_tous(struct poignee_graphe_execution *graphe,
                                               uint64_t racine);

uint64_t EXETRON_graphe_cree_racine(struct poignee_graphe_execution *graphe);

uint64_t EXETRON_graphe_cree_noeud(struct poignee_graphe_execution *graphe,
                                   fonction_noeud_execution fonction,
                                   void *donnees);

void EXETRON_graphe_connecte_noeud_noeud(struct poignee_graphe_execution *graphe,
                                         uint64_t sortie,
                                         uint64_t entree);

#ifdef __cplusplus
}
#endif
