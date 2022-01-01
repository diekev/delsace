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

/* ********************************************************************************************* */

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

/* ********************************************************************************************* */

static auto incremente_nombre_utilisations_recursif(Atome *racine) -> void
{
    racine->nombre_utilisations += 1;

    switch (racine->genre_atome) {
        case Atome::Genre::GLOBALE:
        case Atome::Genre::FONCTION:
        case Atome::Genre::CONSTANTE:
        {
            break;
        }
        case Atome::Genre::INSTRUCTION:
        {
            auto inst = racine->comme_instruction();

            switch (inst->genre) {
                case Instruction::Genre::APPEL:
                {
                    auto appel = inst->comme_appel();

                    /* appele peut être un pointeur de fonction */
                    incremente_nombre_utilisations_recursif(appel->appele);

                    POUR (appel->args) {
                        incremente_nombre_utilisations_recursif(it);
                    }

                    break;
                }
                case Instruction::Genre::CHARGE_MEMOIRE:
                {
                    auto charge = inst->comme_charge();
                    incremente_nombre_utilisations_recursif(charge->chargee);
                    break;
                }
                case Instruction::Genre::STOCKE_MEMOIRE:
                {
                    auto stocke = inst->comme_stocke_mem();
                    incremente_nombre_utilisations_recursif(stocke->valeur);
                    incremente_nombre_utilisations_recursif(stocke->ou);
                    break;
                }
                case Instruction::Genre::OPERATION_UNAIRE:
                {
                    auto op = inst->comme_op_unaire();
                    incremente_nombre_utilisations_recursif(op->valeur);
                    break;
                }
                case Instruction::Genre::OPERATION_BINAIRE:
                {
                    auto op = inst->comme_op_binaire();
                    incremente_nombre_utilisations_recursif(op->valeur_droite);
                    incremente_nombre_utilisations_recursif(op->valeur_gauche);
                    break;
                }
                case Instruction::Genre::ACCEDE_INDEX:
                {
                    auto acces = inst->comme_acces_index();
                    incremente_nombre_utilisations_recursif(acces->index);
                    incremente_nombre_utilisations_recursif(acces->accede);
                    break;
                }
                case Instruction::Genre::ACCEDE_MEMBRE:
                {
                    auto acces = inst->comme_acces_membre();
                    incremente_nombre_utilisations_recursif(acces->index);
                    incremente_nombre_utilisations_recursif(acces->accede);
                    break;
                }
                case Instruction::Genre::TRANSTYPE:
                {
                    auto transtype = inst->comme_transtype();
                    incremente_nombre_utilisations_recursif(transtype->valeur);
                    break;
                }
                case Instruction::Genre::BRANCHE_CONDITION:
                {
                    auto branche = inst->comme_branche_cond();
                    incremente_nombre_utilisations_recursif(branche->condition);
                    break;
                }
                case Instruction::Genre::RETOUR:
                {
                    auto retour = inst->comme_retour();

                    if (retour->valeur) {
                        incremente_nombre_utilisations_recursif(retour->valeur);
                    }

                    break;
                }
                case Instruction::Genre::ALLOCATION:
                case Instruction::Genre::INVALIDE:
                case Instruction::Genre::BRANCHE:
                case Instruction::Genre::LABEL:
                {
                    break;
                }
            }

            break;
        }
    }
}

static bool est_utilise(Atome *atome)
{
    if (atome->est_instruction()) {
        auto inst = atome->comme_instruction();

        if (inst->est_alloc()) {
            return inst->nombre_utilisations != 0;
        }

        if (inst->est_acces_index()) {
            auto acces = inst->comme_acces_index();
            return est_utilise(acces->accede);
        }

        if (inst->est_acces_membre()) {
            auto acces = inst->comme_acces_membre();
            return est_utilise(acces->accede);
        }

        // pour les déréférencements de pointeurs
        if (inst->est_charge()) {
            auto charge = inst->comme_charge();
            return est_utilise(charge->chargee);
        }
    }

    return atome->nombre_utilisations != 0;
}

void marque_instructions_utilisees(kuri::tableau<Instruction *, int> &instructions)
{
    for (auto i = instructions.taille() - 1; i >= 0; --i) {
        auto it = instructions[i];

        if (it->nombre_utilisations != 0) {
            continue;
        }

        switch (it->genre) {
            case Instruction::Genre::BRANCHE:
            case Instruction::Genre::BRANCHE_CONDITION:
            case Instruction::Genre::LABEL:
            case Instruction::Genre::RETOUR:
            {
                incremente_nombre_utilisations_recursif(it);
                break;
            }
            case Instruction::Genre::APPEL:
            {
                auto appel = it->comme_appel();

                if (appel->type->genre == GenreType::RIEN) {
                    incremente_nombre_utilisations_recursif(it);
                }

                break;
            }
            case Instruction::Genre::STOCKE_MEMOIRE:
            {
                auto stocke = it->comme_stocke_mem();

                if (est_utilise(stocke->ou)) {
                    incremente_nombre_utilisations_recursif(stocke);
                }

                break;
            }
            default:
            {
                break;
            }
        }
    }
}

/* ********************************************************************************************* */

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
