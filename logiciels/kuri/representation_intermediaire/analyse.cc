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
 * valides dans la fonction afin de pouvoir proprement détecter qu'une fonction retourne. Se baser
 * uniquement sur la dernière instruction de la fonction est fragile car la génération de RI peut
 * ajouter des labels inutilisés à la fin des fonctions (pour les discriminations, boucles, ou
 * encores les instructions si), mais nous pouvons aussi avoir une branche vers un bloc définis
 * avant celle-ci et étant le bloc de retour effectif de la fonction.
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

        /* À FAIRE : la fonction peut être déclarée par la compilatrice (p.e. les initialisations
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

enum {
    EST_PARAMETRE_FONCTION = (1 << 1),
};

static Atome *dereference_instruction(Instruction *inst)
{
    if (inst->est_acces_index()) {
        auto acces = inst->comme_acces_index();
        return acces->accede;
    }

    if (inst->est_acces_membre()) {
        auto acces = inst->comme_acces_membre();
        return acces->accede;
    }

    // pour les déréférencements de pointeurs
    if (inst->est_charge()) {
        auto charge = inst->comme_charge();
        return charge->chargee;
    }

    if (inst->est_transtype()) {
        auto transtype = inst->comme_transtype();
        return transtype->valeur;
    }

    return inst;
}

static bool est_locale_ou_globale(Atome *atome)
{
    if (atome->est_globale()) {
        return true;
    }

    if (atome->est_instruction()) {
        auto inst = atome->comme_instruction();
        return inst->est_alloc();
    }

    return false;
}

static Atome *cible_finale_stockage(Atome *ou)
{
    auto ancien_ou = ou;

    while (!est_locale_ou_globale(ou)) {
        if (!ou->est_instruction()) {
            break;
        }

        auto inst = ou->comme_instruction();
        ou = dereference_instruction(inst);

        if (ou == ancien_ou) {
            std::cerr << "Boucle infinie !!!!!!\n";
            imprime_atome(ou, std::cerr);

            if (ou->est_instruction()) {
                imprime_instruction(ou->comme_instruction(), std::cerr);
            }

            std::cerr << "\n";
        }

        ancien_ou = ou;
    }

    return ou;
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
                auto cible = cible_finale_stockage(stocke->ou);

                if ((cible->etat & EST_PARAMETRE_FONCTION) || cible->nombre_utilisations != 0 ||
                    cible->est_globale()) {
                    incremente_nombre_utilisations_recursif(stocke);
                }
                else {
                    /* Vérifie si l'instruction de stockage prend la valeur d'une globale ou d'un
                     * paramètre. */
#if 0
                    auto valeur_stocke = cible_finale_stockage(stocke->valeur);

                    if ((valeur_stocke->etat & EST_PARAMETRE_FONCTION) ||
                        valeur_stocke->est_globale()) {
                        incremente_nombre_utilisations_recursif(stocke);
                    }
#endif
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

/**
 * Trouve les paramètres, variables locales, ou les retours d'appels de fonctions non-utilisés.
 *
 * À FAIRE(analyse_ri) :
 * - fonctions nichées inutilisées
 * - retours appels inutilisées
 */
static bool detecte_declarations_inutilisees(EspaceDeTravail &espace, AtomeFonction *atome)
{
    /* Ignore les fonctions d'initalisation des types car les paramètres peuvent ne pas être
     * utilisés, par exemple pour la fonction d'initialisation du type « rien ». */
    if (atome->decl && atome->decl->est_initialisation_type) {
        return true;
    }

    POUR (atome->params_entrees) {
        it->etat = EST_PARAMETRE_FONCTION;

        /* Ignore le contexte implicite qui peut ne pas être utilisé. */
        if (it->ident == ID::contexte) {
            it->nombre_utilisations += 1;
        }
    }

    atome->param_sortie->etat = EST_PARAMETRE_FONCTION;

    POUR (atome->instructions) {
        if (!it->est_alloc()) {
            continue;
        }

        /* Les variables d'indexion des boucles pour peuvent ne pas être utilisées. */
        if (it->ident == ID::it || it->ident == ID::index_it) {
            it->nombre_utilisations += 1;
            continue;
        }

        /* '_' est un peu spécial, il sers à définir une variable qui ne sera pas
         * utilisée, bien que ceci ne soit pas en score formalisé dans le langage. */
        if (it->ident && it->ident->nom == "_") {
            it->nombre_utilisations += 1;
            continue;
        }
    }

    /* Deux passes pour prendre en compte les variables d'itérations des boucles. */
    marque_instructions_utilisees(atome->instructions);
    marque_instructions_utilisees(atome->instructions);

    kuri::tableau<InstructionAllocation *> allocs_inutilisees;

    POUR (atome->params_entrees) {
        if (it->nombre_utilisations != 0) {
            continue;
        }

        auto alloc = it->comme_instruction()->comme_alloc();
        auto decl_alloc = alloc->site;

        /* Si le site n'est pas une déclaration de variable (le contexte implicite n'a pas de
         * site propre, celui de la fonction est utilisé), ajoutons-la à la liste des
         * allocations non-utilisées pour avoir un avertissement. */
        if (!decl_alloc || !decl_alloc->est_declaration_variable()) {
            allocs_inutilisees.ajoute(alloc);
            continue;
        }

        auto decl_var = decl_alloc->comme_declaration_variable();
        if (!possede_annotation(decl_var, "inutilisée")) {
            allocs_inutilisees.ajoute(alloc);
        }
    }

    POUR (atome->instructions) {
        if (!it->est_alloc() || it->nombre_utilisations != 0) {
            continue;
        }

        auto alloc = it->comme_alloc();
        if (alloc->ident == nullptr) {
            continue;
        }

        allocs_inutilisees.ajoute(alloc);
    }

#if 0
    if (allocs_inutilisees.taille() != 0) {
        imprime_fonction(atome, std::cerr, false, true);
    }
#endif

    POUR (allocs_inutilisees) {
        espace.rapporte_avertissement(it->site, "Allocation inutilisée");
    }

    return true;
}

/* ******************************************************************************************** */

/* Performe différentes analyses de la RI. Ces analyses nous servent à valider un peu plus la
 * structures du programme. Nous pourrions les faire lors de la validation sémantique, mais ce
 * serait un peu plus complexe car l'arbre syntaxique, contrairement à la RI, a plus de cas
 * spéciaux.
 *
 * À FAIRE(analyse_ri) :
 * - membre actifs des unions
 */
void analyse_ri(EspaceDeTravail &espace, AtomeFonction *atome)
{
    if (!detecte_declarations_inutilisees(espace, atome)) {
        return;
    }

    if (!detecte_retour_manquant(espace, atome)) {
        return;
    }
}
