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

#include "biblinternes/structures/file.hh"
#include "biblinternes/structures/tableau_page.hh"

#include "structures/tableau.hh"

#include "graphe_dependance.hh"
#include "unite_compilation.hh"

class Compilatrice;
class GrapheDependance;
class OrdonnanceuseTache;
struct Programme;

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
    tableau_page<UniteCompilation> unites{};

    /* Les unités qui attendent sur quelque chose. */
    kuri::tableau<UniteCompilation *> unites_en_attente{};

    Compilatrice *m_compilatrice = nullptr;

    /* Les programmes en cours de compilation, ils sont ajoutés et retirés de la liste en fonction
     * de leurs états de compilation. Les programmes ne sont retirés que si leur compilation ou
     * exécution est terminée. */
    dls::file<Programme *> programmes_en_cours{};

    /* Les dépendances d'une déclaration qui sont rassemblées après la fin du typage, nous ne
     * stockons pas définitivement cette information, ce membre ne sers qu'à réutiliser la mémoire
     * allouée précédemment afin de ne pas trop faire d'allocations dynamiques. */
    DonneesDependance dependances{};

  public:
    GestionnaireCode() = default;
    GestionnaireCode(Compilatrice *compilatrice) : m_compilatrice(compilatrice)
    {
    }

    GestionnaireCode(GestionnaireCode const &) = delete;
    GestionnaireCode &operator=(GestionnaireCode const &) = delete;

    /* Notification qu'un espace fut créé, son programme est ajouté à la liste des programmes en
     * cours de compilation */
    void espace_cree(EspaceDeTravail *espace);

    /* Notification qu'un espace fut créé, son programme est ajouté à la liste des programmes en
     * cours de compilation */
    void metaprogramme_cree(MetaProgramme *metaprogramme);

    /* Création des unités pour le typage, etc. */
    void requiers_chargement(EspaceDeTravail *espace, Fichier *fichier);

    void requiers_lexage(EspaceDeTravail *espace, Fichier *fichier);

    void requiers_parsage(EspaceDeTravail *espace, Fichier *fichier);

    void requiers_typage(EspaceDeTravail *espace, NoeudExpression *noeud);

    void requiers_generation_ri(EspaceDeTravail *espace, NoeudExpression *noeud);

    void requiers_generation_ri_principale_metaprogramme(EspaceDeTravail *espace,
                                                         MetaProgramme *metaprogramme);

    void requiers_compilation_metaprogramme(EspaceDeTravail *espace, MetaProgramme *metaprogramme);

    void requiers_generation_code_machine(EspaceDeTravail *espace, Programme *programme);

    void requiers_liaison_executable(EspaceDeTravail *espace, Programme *programme);

    /* Attente sur quelque chose. */
    void mets_en_attente(UniteCompilation *unite_attendante, Attente attente);

    /* Fin d'une tâche. */
    void chargement_fichier_termine(UniteCompilation *unite);

    void lexage_fichier_termine(UniteCompilation *unite);

    void parsage_fichier_termine(UniteCompilation *unite);

    void typage_termine(UniteCompilation *unite);

    void generation_ri_terminee(UniteCompilation *unite);

    void message_recu(Message const *message);

    void execution_terminee(UniteCompilation *unite);

    void generation_code_machine_terminee(UniteCompilation *unite);

    void liaison_programme_terminee(UniteCompilation *unite);

    void optimisation_terminee(UniteCompilation *unite);

    /* Remplis les tâches. */
    void cree_taches(OrdonnanceuseTache &ordonnanceuse);

  private:
    void requiers_execution(EspaceDeTravail *espace, MetaProgramme *metaprogramme);

    void ajoute_programme(Programme *programme);

    void enleve_programme(Programme *programme);

    void determine_dependances(NoeudExpression *noeud,
                               EspaceDeTravail *espace,
                               GrapheDependance &graphe);

    bool plus_rien_n_est_a_faire();
    std::optional<Attente> tente_de_garantir_presence_creation_contexte(EspaceDeTravail *espace,
                                                                        Programme *programme,
                                                                        GrapheDependance &graphe);
};
