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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "analyse.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "compilation/espace_de_travail.hh"

#include "impression.hh"
#include "instructions.hh"

/* Détecte le manque de retour. Toutes les fonctions, y compris celles ne retournant rien doivent
 * avoir une porte de sortie.
 *
 * À FAIRE(analyse_ri) : Il nous faudrait une structure en graphe afin de suivre tous les chemins
 * valides dans la fonction afin de pouvoir proprement détecter qu'un fonction retourne. Se baser
 * uniquement sur la dernière instruction de la fonction est fragile car la génération de RI peut
 * ajouter des labels inutilisés à la fin des fonctions (pour les discriminations, boucles, ou
 * encores les instructions si), mais nous pouvons aussi avoir une branche vers un bloc définis
 * afin celle-ci et étant le bloc de retour effectif de la fonction.
 */
static bool detecte_retour_manquant(EspaceDeTravail &espace, AtomeFonction *atome)
{
    auto di = atome->derniere_instruction();

    if (!di || !di->est_retour()) {
        if (di) {
            std::cerr << "La dernière instruction est ";
            imprime_instruction(di, std::cerr);
            imprime_fonction(atome, std::cerr);
        }
        else {
            std::cerr << "La dernière instruction est nulle !\n";
        }

        /* À FAIRE : la fonction peut être déclarer par la compilatrice (p.e. les initialisations
         * des types) et donc peut ne pas avoir de déclaration. */
        espace.rapporte_erreur(atome->decl, "Instruction de retour manquante");
        return false;
    }

    return true;
}

/* Performes différentes analyses de la RI. Ces analyses nous servent à valider
 * un peu plus la structures du programme. Nous pourrions les faire dans la
 * validation sémantique, mais ce serait un peu plus complexe, la RI nous
 * simplifie la vie.
 */
void analyse_ri(EspaceDeTravail &espace, AtomeFonction *atome)
{
    if (!detecte_retour_manquant(espace, atome)) {
        return;
    }
}
