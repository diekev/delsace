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

#include "instructions.hh"

#include "biblinternes/structures/ensemble.hh"

Instruction *AtomeFonction::derniere_instruction() const
{
    return instructions[instructions.taille() - 1];
}

void VisiteuseAtome::reinitialise()
{
    visites.efface();
}

void VisiteuseAtome::visite_atome(Atome *racine, std::function<void(Atome *)> rappel)
{
    if (!racine) {
        return;
    }

    if (visites.possede(racine)) {
        return;
    }

    visites.insere(racine);

    rappel(racine);

    switch (racine->genre_atome) {
        case Atome::Genre::CONSTANTE:
        {
            auto constante = static_cast<AtomeConstante *>(racine);

            switch (constante->genre) {
                case AtomeConstante::Genre::GLOBALE:
                {
                    /* Déjà gérée, genre_atome étant GLOBALE. */
                    break;
                }
                case AtomeConstante::Genre::TRANSTYPE_CONSTANT:
                {
                    auto transtype_const = static_cast<TranstypeConstant const *>(constante);
                    visite_atome(transtype_const->valeur, rappel);
                    break;
                }
                case AtomeConstante::Genre::OP_UNAIRE_CONSTANTE:
                {
                    auto op_unaire_const = static_cast<OpUnaireConstant const *>(constante);
                    visite_atome(op_unaire_const->operande, rappel);
                    break;
                }
                case AtomeConstante::Genre::OP_BINAIRE_CONSTANTE:
                {
                    auto op_unaire_const = static_cast<OpBinaireConstant const *>(constante);
                    visite_atome(op_unaire_const->operande_gauche, rappel);
                    visite_atome(op_unaire_const->operande_droite, rappel);
                    break;
                }
                case AtomeConstante::Genre::ACCES_INDEX_CONSTANT:
                {
                    auto inst_acces = static_cast<AccedeIndexConstant const *>(constante);
                    visite_atome(inst_acces->accede, rappel);
                    visite_atome(inst_acces->index, rappel);
                    break;
                }
                case AtomeConstante::Genre::VALEUR:
                {
                    auto valeur_const = static_cast<AtomeValeurConstante const *>(constante);

                    switch (valeur_const->valeur.genre) {
                        case AtomeValeurConstante::Valeur::Genre::NULLE:
                        case AtomeValeurConstante::Valeur::Genre::TYPE:
                        case AtomeValeurConstante::Valeur::Genre::REELLE:
                        case AtomeValeurConstante::Valeur::Genre::ENTIERE:
                        case AtomeValeurConstante::Valeur::Genre::BOOLEENNE:
                        case AtomeValeurConstante::Valeur::Genre::CARACTERE:
                        case AtomeValeurConstante::Valeur::Genre::TABLEAU_DONNEES_CONSTANTES:
                        case AtomeValeurConstante::Valeur::Genre::INDEFINIE:
                        {
                            /* Pas de sous-atome. */
                            break;
                        }
                        case AtomeValeurConstante::Valeur::Genre::STRUCTURE:
                        {
                            auto pointeur_tableau = valeur_const->valeur.valeur_structure.pointeur;
                            auto taille_tableau = valeur_const->valeur.valeur_structure.taille;
                            for (auto i = 0; i < taille_tableau; ++i) {
                                visite_atome(pointeur_tableau[i], rappel);
                            }
                            break;
                        }
                        case AtomeValeurConstante::Valeur::Genre::TABLEAU_FIXE:
                        {
                            auto pointeur_tableau = valeur_const->valeur.valeur_tableau.pointeur;
                            auto taille_tableau = valeur_const->valeur.valeur_tableau.taille;
                            for (auto i = 0; i < taille_tableau; ++i) {
                                visite_atome(pointeur_tableau[i], rappel);
                            }
                            break;
                        }
                    }
                }
            }

            break;
        }
        case Atome::Genre::GLOBALE:
        {
            auto globale = static_cast<AtomeGlobale *>(racine);
            visite_atome(globale->initialisateur, rappel);
            break;
        }
        case Atome::Genre::FONCTION:
        {
            /* Pour l'instant nous faisons la visite depuis les fonctions, inutile de les
             * traverser. */
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
                    visite_atome(appel->appele, rappel);

                    POUR (appel->args) {
                        visite_atome(it, rappel);
                    }

                    break;
                }
                case Instruction::Genre::CHARGE_MEMOIRE:
                {
                    auto charge = inst->comme_charge();
                    visite_atome(charge->chargee, rappel);
                    break;
                }
                case Instruction::Genre::STOCKE_MEMOIRE:
                {
                    auto stocke = inst->comme_stocke_mem();
                    visite_atome(stocke->valeur, rappel);
                    visite_atome(stocke->ou, rappel);
                    break;
                }
                case Instruction::Genre::OPERATION_UNAIRE:
                {
                    auto op = inst->comme_op_unaire();
                    visite_atome(op->valeur, rappel);
                    break;
                }
                case Instruction::Genre::OPERATION_BINAIRE:
                {
                    auto op = inst->comme_op_binaire();
                    visite_atome(op->valeur_droite, rappel);
                    visite_atome(op->valeur_gauche, rappel);
                    break;
                }
                case Instruction::Genre::ACCEDE_INDEX:
                {
                    auto acces = inst->comme_acces_index();
                    visite_atome(acces->index, rappel);
                    visite_atome(acces->accede, rappel);
                    break;
                }
                case Instruction::Genre::ACCEDE_MEMBRE:
                {
                    auto acces = inst->comme_acces_membre();
                    visite_atome(acces->index, rappel);
                    visite_atome(acces->accede, rappel);
                    break;
                }
                case Instruction::Genre::TRANSTYPE:
                {
                    auto transtype = inst->comme_transtype();
                    visite_atome(transtype->valeur, rappel);
                    break;
                }
                case Instruction::Genre::BRANCHE_CONDITION:
                {
                    auto branche = inst->comme_branche_cond();
                    visite_atome(branche->condition, rappel);
                    break;
                }
                case Instruction::Genre::RETOUR:
                {
                    auto retour = inst->comme_retour();
                    visite_atome(retour->valeur, rappel);
                    break;
                }
                case Instruction::Genre::ALLOCATION:
                case Instruction::Genre::INVALIDE:
                case Instruction::Genre::BRANCHE:
                case Instruction::Genre::LABEL:
                {
                    /* Pas de sous-atome. */
                    break;
                }
            }

            break;
        }
    }
}

void visite_atome(Atome *racine, std::function<void(Atome *)> rappel)
{
    VisiteuseAtome visiteuse{};
    visiteuse.visite_atome(racine, rappel);
}
