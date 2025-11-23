/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#pragma once

#include "instructions.hh"

/* Visite uniquement les opérandes de l'instruction, s'il y en a. */
template <typename Rappel>
void visite_opérandes_instruction(Instruction *inst, Rappel &&rappel)
{
    switch (inst->genre) {
        case GenreInstruction::APPEL:
        {
            auto appel = inst->comme_appel();

            /* appele peut être un pointeur de fonction */
            rappel(appel->appelé);

            POUR (appel->args) {
                rappel(it);
            }

            break;
        }
        case GenreInstruction::CHARGE_MÉMOIRE:
        {
            auto charge = inst->comme_charge();
            rappel(charge->chargée);
            break;
        }
        case GenreInstruction::STOCKE_MÉMOIRE:
        {
            auto stocke = inst->comme_stocke_mem();
            rappel(stocke->source);
            rappel(stocke->destination);
            break;
        }
        case GenreInstruction::OPÉRATION_UNAIRE:
        {
            auto op = inst->comme_op_unaire();
            rappel(op->valeur);
            break;
        }
        case GenreInstruction::OPÉRATION_BINAIRE:
        {
            auto op = inst->comme_op_binaire();
            rappel(op->valeur_droite);
            rappel(op->valeur_gauche);
            break;
        }
        case GenreInstruction::ACCÈS_INDICE:
        {
            auto accès = inst->comme_accès_indice();
            rappel(accès->indice);
            rappel(accès->accédé);
            break;
        }
        case GenreInstruction::ACCÈS_RUBRIQUE:
        {
            auto accès = inst->comme_accès_rubrique();
            rappel(accès->accédé);
            break;
        }
        case GenreInstruction::TRANSTYPE:
        {
            auto transtype = inst->comme_transtype();
            rappel(transtype->valeur);
            break;
        }
        case GenreInstruction::BRANCHE_CONDITION:
        {
            auto branche = inst->comme_branche_cond();
            rappel(branche->condition);
            break;
        }
        case GenreInstruction::RETOUR:
        {
            auto retour = inst->comme_retour();
            if (retour->valeur) {
                rappel(retour->valeur);
            }
            break;
        }
        case GenreInstruction::SÉLECTION:
        {
            auto sélection = inst->comme_sélection();
            rappel(sélection->condition);
            rappel(sélection->si_vrai);
            rappel(sélection->si_faux);
            break;
        }
        case GenreInstruction::COPIE_MÉMOIRE:
        {
            auto copie_mémoire = inst->comme_copie_mémoire();
            rappel(copie_mémoire->destination);
            rappel(copie_mémoire->source);
            break;
        }
        case GenreInstruction::ALLOCATION:
        case GenreInstruction::BRANCHE:
        case GenreInstruction::LABEL:
        case GenreInstruction::INATTEIGNABLE:
        case GenreInstruction::ARRÊT_DÉBUG:
        {
            /* Pas de sous-atome. */
            break;
        }
    }
}
