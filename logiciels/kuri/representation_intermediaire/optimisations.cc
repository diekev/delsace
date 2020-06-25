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

#include "optimisations.hh"

#include "biblinternes/structures/tablet.hh"

#include "instructions.hh"

/* Supprime les labels ne contenant aucune instruction, et remplace ceux-ci dans
 * les branches les utilisant.
 * Supprime les labels immédiatement après une instruction de retour (nous pourrions
 * corriger la génération de RI pour ne pas les insérer en premier lieu).
 *
 * Idées :
 * - supprime les blocs n'ayant aucun ancêtre, sauf le premier
 * - remplace les branches simples par des instructions de retour si le bloc pointé n'a que ça
 * - si un bloc n'a qu'une branche et aucun ancêtre dépendant de la branche, supprime le, et remplace les pointeurs dans les ancêtres
 */
void corrige_labels(AtomeFonction *atome_fonc)
{
	dls::tableau<std::pair<InstructionLabel *, InstructionLabel *>> paires_labels;
	dls::tableau<std::pair<InstructionLabel *, long>> nombre_instructions;
	long instructions_a_supprimer = 0;

	InstructionLabel *label_courant = nullptr;
	auto branche_ou_retour_rencontre = true;
	auto nombre_instructions_label = 0;
	Instruction::Genre derniere_instruction = Instruction::Genre::INVALIDE;

	POUR (atome_fonc->instructions) {
		if (it->genre == Instruction::Genre::LABEL) {
			auto label = static_cast<InstructionLabel *>(it);

			if (!branche_ou_retour_rencontre) {
				paires_labels.pousse({ label_courant, label });
			}

			if (label_courant) {
				nombre_instructions.pousse({ label_courant, nombre_instructions_label });
			}

			label_courant = label;
			branche_ou_retour_rencontre = false;
			nombre_instructions_label = 0;
			derniere_instruction = Instruction::Genre::LABEL;
			continue;
		}

		if (it->genre == Instruction::Genre::BRANCHE) {
			if (derniere_instruction == Instruction::Genre::RETOUR) {
				it->drapeaux |= Instruction::SUPPRIME_INSTRUCTION;
				instructions_a_supprimer += 1;
			}
			else {
				auto branche = static_cast<InstructionBranche *>(it);
				paires_labels.pousse({ label_courant, branche->label });
				branche_ou_retour_rencontre = true;
			}
		}
		else if (it->genre == Instruction::Genre::BRANCHE_CONDITION) {
			auto branche = static_cast<InstructionBrancheCondition *>(it);
			paires_labels.pousse({ label_courant, branche->label_si_vrai });
			paires_labels.pousse({ label_courant, branche->label_si_faux });
			branche_ou_retour_rencontre = true;
		}
		else if (it->genre == Instruction::Genre::RETOUR) {
			paires_labels.pousse({ label_courant, nullptr });
			branche_ou_retour_rencontre = true;
		}

		nombre_instructions_label += 1;
		derniere_instruction = it->genre;
	}

	dls::tablet<std::pair<InstructionLabel *, InstructionLabel *>, 16> paires_remplacement;
	POUR (nombre_instructions) {
		if (it.second != 0) {
			continue;
		}

		for (auto i = 0; i < paires_labels.taille(); ++i) {
			if (paires_labels[i].first == it.first) {
				it.first->drapeaux |= Instruction::SUPPRIME_INSTRUCTION;
				instructions_a_supprimer += 1;
				paires_remplacement.pousse({ it.first, paires_labels[i].second });
			}
		}
	}

	POUR (paires_remplacement) {
		for (auto &inst : atome_fonc->instructions) {
			if (inst->genre == Instruction::Genre::BRANCHE) {
				auto branche = static_cast<InstructionBranche *>(inst);

				if (branche->label == it.first) {
					branche->label = it.second;
				}
			}
			else if (inst->genre == Instruction::Genre::BRANCHE_CONDITION) {
				auto branche = static_cast<InstructionBrancheCondition *>(inst);

				if (branche->label_si_vrai == it.first) {
					branche->label_si_vrai = it.second;
				}
				else if (branche->label_si_faux == it.first) {
					branche->label_si_faux = it.second;
				}
			}
		}
	}

	if (instructions_a_supprimer != 0) {
		auto nouvelles_instructions = kuri::tableau<Instruction *>();
		nouvelles_instructions.reserve(atome_fonc->instructions.taille - instructions_a_supprimer);

		POUR (atome_fonc->instructions) {
			if ((it->drapeaux & Instruction::SUPPRIME_INSTRUCTION) == 0) {
				nouvelles_instructions.pousse(it);
			}
		}

		atome_fonc->instructions = nouvelles_instructions;
	}
}
