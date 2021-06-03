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

#include "biblinternes/structures/tableau_page.hh"

#include "structures/tableau.hh"

#include "unite_compilation.hh"

class GestionnaireCode {
    tableau_page<UniteCompilation> unites{};

    kuri::tableau<UniteCompilation *> unites_en_attente{};

public:
    void requiers_chargement(EspaceDeTravail *espace, Fichier *fichier)
    {
        espace->tache_chargement_ajoutee(m_compilatrice->messagere);

        auto unite = unites.ajoute_element(espace);
        unite->fichier = fichier;

        unites_en_attente.ajoute(unite);
    }

    void requiers_lexage(EspaceDeTravail *espace, Fichier *fichier)
    {
        assert(fichier->donnees_constantes->fut_charge);
        espace->tache_lexage_ajoutee(m_compilatrice->messagere);

        auto unite = unites.ajoute_element(espace);
        unite->fichier = fichier;

        unites_en_attente.ajoute(unite);
    }

    void requiers_parsage(EspaceDeTravail *espace, Fichier *fichier)
    {
        assert(fichier->donnees_constantes->fut_lexe);
        espace->tache_parsage_ajoutee(m_compilatrice->messagere);

        auto unite = unites.ajoute_element(espace);
        unite->fichier = fichier;

        unites_en_attente.ajoute(unite);
    }

    /* Création des unités pour le typage, etc. */
    void requiers_typage(EspaceDeTravail *espace, NoeudExpression *noeud)
    {
        espace->tache_typage_ajoutee(m_compilatrice->messagere);

        auto unite = unites.ajoute_element(espace);
        unite->noeud = noeud;

        noeud->unite = unite;

        unites_en_attente.ajoute(unite);
    }

    void requiers_generation_ri(EspaceDeTravail *espace, NoeudExpression *noeud)
    {
        espace->tache_ri_ajoutee(m_compilatrice->messagere);

        auto unite = unites.ajoute_element(espace);
        unite->noeud = noeud;

        noeud->unite = unite;

        unites_en_attente.ajoute(unite);
    }

    void requiers_execution(EspaceDeTravail *espace, MetaProgramme *metaprogramme)
    {
        espace->tache_execution_ajoutee(m_compilatrice->messagere);

        auto unite = unites.ajoute_element(espace);
        unite->metaprogramme = metaprogramme;

        metaprogramme->unite = unite;

        unites_en_attente.ajoute(unite);
    }

    /* Attente sur quelque chose. */
    void mets_en_attente(UniteCompilation *unite_attendante, Attente attente)
    {
        unites_en_attente.ajoute(unite_attendante);
    }

    /* Fin d'une tâche. */
    void symbole_defini(IdentifiantCode *ident)
    {
        POUR (unites_en_attente) {
            if (it.attend_sur_symbole() == ident) {
                it.marque_prete();
            }
        }
    }

    void chargement_fichier_termine()
    {
        // si tous les fichiers sont chargés, envoie un message


    }

    void parsage_fichier_termine()
    {

    }

    void typage_termine()
    {
        // si toutes les unités requérant un typage dans l'espace sont typées, envoie un message
        // envoie un message
    }

    void generation_ri_terminee()
    {
        // si toutes les unités requérant un typage dans l'espace ont eu leurs RI générées, envoie un message
    }

    /* Remplis les tâches. */
    void cree_taches(OrdonnanceuseTache &ordonnanceuse)
    {
        POUR (unites_en_attente) {
            if (!it.prete()) {
                continue;
            }

            if () {
                ordonnanceuse.cree_tache_generation_ri(it);
            }
            else if () {
                ordonnanceuse.cree_tache_pour_chargement(it);
            }
            else if () {
                ordonnanceuse.cree_tache_pour_lexage(it);
            }
            else if () {
                ordonnanceuse.cree_tache_pour_parsage(it);
            }
            else if () {
                ordonnanceuse.cree_tache_pour_typage(it);
            }
            else if () {
                ordonnanceuse.cree_tache_pour_generation_ri(it);
            }
            else if () {
                ordonnanceuse.cree_tache_pour_execution(it);
            }
            else {
                it.espace->rapporte_erreur();
            }
        }
    }
};

/*


class GestionnaireCode {
private:

public:
    void requiers_typage(UniteCompilation *unite_attendante, NoeudDeclaration *)
    {
        unite->attend_sur(...);
        unites_en_attente.insere(unite_attendante);
    }

    void requiers_monomorphisation(UniteCompilation *unite_attendante, NoeudDeclaration *polymorphe, Params ...)
    {
        unite->attend_sur(...);
        unites_en_attente.insere(unite_attendante);
    }

    void requiers_symbole(UniteCompilation *unite_attendante, ...)
    {
        unite->attend_sur(...);
        unites_en_attente.insere(unite_attendante);
    }

    void requiers_interface_kuri(UniteCompilation *unite_attendante, ...)

    void requiers_(UniteCompilation *unite_attendante, )

    // -------------------

    void typage_termine(...)
    {
        if (entete || declaration_type)
            symbole_resolu(...);
    }

    void ri_generee(...)

    void metaprogramme_execute(...)
    {
        POUR (unites_en_attente) {

        }
    }

    void symbole_resolu(IdentifiantCode *symbole)
    {
        unites_pretes;

        POUR (unites_en_attente) {
            if (it->attend_sur_symbole() == symbole) {
                unites_pretes.insere(it);
            }
        }
    }
};

*/
