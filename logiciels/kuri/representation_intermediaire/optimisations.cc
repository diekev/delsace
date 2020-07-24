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

#include "impression.hh"
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

static auto decremente_nombre_utilisations_recursif(Atome *racine) -> void
{
	// déjà à zéro, ça ne sers à rien de continuer à récurser
	if (racine->nombre_utilisations == 0) {
		return;
	}

	racine->nombre_utilisations -= 1;

	switch (racine->genre_atome) {
		case Atome::Genre::GLOBALE:
		case Atome::Genre::FONCTION:
		case Atome::Genre::CONSTANTE:
		{
			break;
		}
		case Atome::Genre::INSTRUCTION:
		{
			auto inst = static_cast<Instruction *>(racine);

			switch (inst->genre) {
				case Instruction::Genre::APPEL:
				{
					auto appel = static_cast<InstructionAppel *>(inst);

					POUR (appel->args) {
						decremente_nombre_utilisations_recursif(it);
					}

					break;
				}
				case Instruction::Genre::CHARGE_MEMOIRE:
				{
					auto charge = static_cast<InstructionChargeMem *>(inst);
					decremente_nombre_utilisations_recursif(charge->chargee);
					break;
				}
				case Instruction::Genre::STOCKE_MEMOIRE:
				{
					auto stocke = static_cast<InstructionStockeMem *>(inst);
					decremente_nombre_utilisations_recursif(stocke->valeur);
					decremente_nombre_utilisations_recursif(stocke->ou);
					break;
				}
				case Instruction::Genre::OPERATION_UNAIRE:
				{
					auto op = static_cast<InstructionOpUnaire *>(inst);
					decremente_nombre_utilisations_recursif(op->valeur);
					break;
				}
				case Instruction::Genre::OPERATION_BINAIRE:
				{
					auto op = static_cast<InstructionOpBinaire *>(inst);
					decremente_nombre_utilisations_recursif(op->valeur_droite);
					decremente_nombre_utilisations_recursif(op->valeur_gauche);
					break;
				}
				case Instruction::Genre::ACCEDE_INDEX:
				{
					auto acces = static_cast<InstructionAccedeIndex *>(inst);
					decremente_nombre_utilisations_recursif(acces->index);
					decremente_nombre_utilisations_recursif(acces->accede);
					break;
				}
				case Instruction::Genre::ACCEDE_MEMBRE:
				{
					auto acces = static_cast<InstructionAccedeMembre *>(inst);
					decremente_nombre_utilisations_recursif(acces->index);
					decremente_nombre_utilisations_recursif(acces->accede);
					break;
				}
				case Instruction::Genre::TRANSTYPE:
				{
					auto transtype = static_cast<InstructionTranstype *>(inst);
					decremente_nombre_utilisations_recursif(transtype->valeur);
					break;
				}
				case Instruction::Genre::ENREGISTRE_LOCALES:
				case Instruction::Genre::RESTAURE_LOCALES:
				case Instruction::Genre::ALLOCATION:
				case Instruction::Genre::INVALIDE:
				case Instruction::Genre::BRANCHE:
				case Instruction::Genre::BRANCHE_CONDITION:
				case Instruction::Genre::RETOUR:
				case Instruction::Genre::LABEL:
				{
					break;
				}
			}

			break;
		}
	}
}

/* Petit algorithme de suppression de code mort.
 *
 * Le code est pour le moment défini comme étant une allocation n'étant pas utilisée. Nous vérifions son état lors des stockages de mémoire.
 *
 * Il faudra gérer les cas suivants :
 * - inutilisation du retour d'une fonction, mais dont la fonction a des effets secondaires : supprime la temporaire, mais garde la fonction
 * - modification, via un déréférencement, d'un paramètre d'une fonction, sans utiliser celui-ci dans la fonction
 * - modification, via un déréférenecement, d'un pointeur venant d'une fonction sans retourner le pointeur d'une fonction
 * - détecter quand nous avons une variable qui est réassignée
 *
 * une fonction possède des effets secondaires si :
 * -- elle modifie l'un de ses paramètres
 * -- elle possède une boucle ou un controle de flux non constant
 * -- elle est une fonction externe
 * -- elle appel une fonction ayant des effets secondaires
 *
 * une variable peut être supprimée si :
 * -- son compte d'utilisation est de deux (stocke et charge)
 * -- elle n'a aucun identifiant
 * -- cas pour les temporaires de conversions ?
 *
 * erreur non-utilisation d'une variable
 * -- si la variable fût définie par l'utilisateur
 * -- variable définie par le compilateur : les temporaires dans la RI, le contexte implicite, les it et index_it des boucles pour
 *
 */
void supprime_code_mort(AtomeFonction *atome_fonc)
{
	std::cerr << "Vérifie code pour : " << atome_fonc->nom << '\n';

	POUR (atome_fonc->instructions) {
		if (it->nombre_utilisations == 0) {
			//std::cerr << "-- l'instruction n'est pas utilisée !\n";
			std::cerr << "\033[1m";
		}
		std::cerr << "%" << it->numero << ' ';
		imprime_instruction(it, std::cerr);
		if (it->nombre_utilisations == 0) {
			std::cerr << "\033[0m";
		}
	}

	// détecte les réassignations avant utilisation
	using paire_atomes = std::pair<InstructionAllocation *, InstructionStockeMem *>;
	auto anciennes_valeurs = dls::tablet<paire_atomes, 16>();

	enum {
		VALEUR_NE_FUT_PAS_INITIALISEE,
		VALEUR_FUT_UTILISEE,
		VALEUR_FUT_INITIALISEE,
	};

	POUR (atome_fonc->instructions) {
		if (it->genre == Instruction::Genre::STOCKE_MEMOIRE) {
			auto stocke_mem = static_cast<InstructionStockeMem *>(it);
			auto ou = stocke_mem->ou;

			if (ou->genre_atome != Atome::Genre::INSTRUCTION) {
				continue;
			}

			if (ou->etat == VALEUR_NE_FUT_PAS_INITIALISEE) {
				anciennes_valeurs.pousse({ static_cast<InstructionAllocation *>(ou), stocke_mem });
				ou->etat = VALEUR_FUT_INITIALISEE;
			}
			else if (ou->etat == VALEUR_FUT_INITIALISEE) {
				// la valeur n'a pas encore été utilisée
				for (auto &p : anciennes_valeurs) {
					if (p.first == ou) {
						std::cerr << "La valeur ne fut pas encore utilisée !\n";
						decremente_nombre_utilisations_recursif(p.second);
						p.second = stocke_mem;
						ou->etat = VALEUR_FUT_INITIALISEE;
						break;
					}
				}
			}
		}
		else if (it->genre == Instruction::Genre::CHARGE_MEMOIRE) {
			auto charge_mem = static_cast<InstructionChargeMem *>(it);
			charge_mem->chargee->etat = VALEUR_FUT_UTILISEE;
		}
	}

	// nous partons de la fin de la fonction, pour ne pas avoir à récurser
	for (auto i = atome_fonc->instructions.taille - 1; i >= 0; --i) {
		auto it = atome_fonc->instructions[i];

		if (it->genre == Instruction::Genre::STOCKE_MEMOIRE) {
			auto stocke_mem = static_cast<InstructionStockeMem *>(it);

			if (stocke_mem->ou->nombre_utilisations <= 1) {
				decremente_nombre_utilisations_recursif(stocke_mem);
			}
		}
		else if (it->genre == Instruction::Genre::CHARGE_MEMOIRE && it->nombre_utilisations == 0) {
			auto charge_mem = static_cast<InstructionChargeMem *>(it);
			decremente_nombre_utilisations_recursif(charge_mem->chargee);
		}
	}

//	POUR (atome_fonc->instructions) {
//		if (it->nombre_utilisations <= 0) {
//			//std::cerr << "-- l'instruction n'est pas utilisée !\n";
//			std::cerr << "\033[1m";
//		}
//		std::cerr << '(' << it->nombre_utilisations << ')' << "%" << it->numero << ' ';
//		imprime_instruction(it, std::cerr);
//		if (it->nombre_utilisations <= 0) {
//			std::cerr << "\033[0m";
//		}
//	}

	auto nouvelles_instructions = kuri::tableau<Instruction *>();

	auto numero_instruction = static_cast<int>(atome_fonc->params_entrees.taille);

	POUR (atome_fonc->instructions) {
		if (it->nombre_utilisations == 0) {
			continue;
		}

		it->numero = numero_instruction++;
		nouvelles_instructions.pousse(it);
	}

	atome_fonc->instructions = nouvelles_instructions;

	imprime_fonction(atome_fonc, std::cerr);
}
