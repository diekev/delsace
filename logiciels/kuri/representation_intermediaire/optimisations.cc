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

#include "compilation/arbre_syntaxique.hh"

#include "constructrice_ri.hh"
#include "impression.hh"
#include "instructions.hh"

/* À FAIRE(optimisations) : non-urgent
 * - structures de blocs basiques
 * - Substitutrice, pour généraliser les substitions d'instructions
 * - copie des instructions (requiers de séparer les allocations des instructions de la ConstructriceRI)
 * - supprime les caches d'accès et de charges de la ConstructionRI, ils ne prenent pas en compte les blocs
 */

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
			auto inst = static_cast<Instruction *>(racine);

			switch (inst->genre) {
				case Instruction::Genre::APPEL:
				{
					auto appel = static_cast<InstructionAppel *>(inst);

					POUR (appel->args) {
						incremente_nombre_utilisations_recursif(it);
					}

					break;
				}
				case Instruction::Genre::CHARGE_MEMOIRE:
				{
					auto charge = static_cast<InstructionChargeMem *>(inst);
					incremente_nombre_utilisations_recursif(charge->chargee);
					break;
				}
				case Instruction::Genre::STOCKE_MEMOIRE:
				{
					auto stocke = static_cast<InstructionStockeMem *>(inst);
					incremente_nombre_utilisations_recursif(stocke->valeur);
					incremente_nombre_utilisations_recursif(stocke->ou);
					break;
				}
				case Instruction::Genre::OPERATION_UNAIRE:
				{
					auto op = static_cast<InstructionOpUnaire *>(inst);
					incremente_nombre_utilisations_recursif(op->valeur);
					break;
				}
				case Instruction::Genre::OPERATION_BINAIRE:
				{
					auto op = static_cast<InstructionOpBinaire *>(inst);
					incremente_nombre_utilisations_recursif(op->valeur_droite);
					incremente_nombre_utilisations_recursif(op->valeur_gauche);
					break;
				}
				case Instruction::Genre::ACCEDE_INDEX:
				{
					auto acces = static_cast<InstructionAccedeIndex *>(inst);
					incremente_nombre_utilisations_recursif(acces->index);
					incremente_nombre_utilisations_recursif(acces->accede);
					break;
				}
				case Instruction::Genre::ACCEDE_MEMBRE:
				{
					auto acces = static_cast<InstructionAccedeMembre *>(inst);
					incremente_nombre_utilisations_recursif(acces->index);
					incremente_nombre_utilisations_recursif(acces->accede);
					break;
				}
				case Instruction::Genre::TRANSTYPE:
				{
					auto transtype = static_cast<InstructionTranstype *>(inst);
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
				case Instruction::Genre::ENREGISTRE_LOCALES:
				case Instruction::Genre::RESTAURE_LOCALES:
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

		if (inst->genre == Instruction::Genre::ALLOCATION) {
			return inst->nombre_utilisations != 0;
		}

		if (inst->genre == Instruction::Genre::ACCEDE_INDEX) {
			auto acces = inst->comme_acces_index();
			return est_utilise(acces->accede);
		}

		if (inst->genre == Instruction::Genre::ACCEDE_MEMBRE) {
			auto acces = inst->comme_acces_membre();
			return est_utilise(acces->accede);
		}

		// pour les déréférencements de pointeurs
		if (inst->genre == Instruction::Genre::CHARGE_MEMOIRE) {
			auto charge = inst->comme_charge();
			return est_utilise(charge->chargee);
		}
	}

	return atome->nombre_utilisations != 0;
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
 * erreur non-utilisation d'une variable
 * -- si la variable fût définie par l'utilisateur
 * -- variable définie par le compilateur : les temporaires dans la RI, le contexte implicite, les it et index_it des boucles pour
 *
 */
static void marque_instructions_utilisees(kuri::tableau<Instruction *> &instructions)
{
	for (auto i = instructions.taille - 1; i >= 0; --i) {
		auto it = instructions[i];

		if (it->nombre_utilisations != 0) {
			continue;
		}

		switch (it->genre) {
			case Instruction::Genre::BRANCHE:
			case Instruction::Genre::BRANCHE_CONDITION:
			case Instruction::Genre::ENREGISTRE_LOCALES:
			case Instruction::Genre::RESTAURE_LOCALES:
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

#undef DEBOGUE_SUPPRESSION_CODE_MORT

void supprime_code_mort(AtomeFonction *atome_fonc)
{
	POUR (atome_fonc->instructions) {
		it->nombre_utilisations = 0;
	}

	/* performe deux passes, car les boucles « pour » verraient les incrémentations de
	 * leurs variables supprimées puisque nous ne marquons la variable comme utilisée
	 * que lors de la visite de la condition du bloc après les incrémentations (nous
	 * traversons les intructions en arrière pour que seules les dépendances du retour
	 * soient considérées) */
	marque_instructions_utilisees(atome_fonc->instructions);
	marque_instructions_utilisees(atome_fonc->instructions);

	/* élimine les branches superflues, et compte les instructions restantes */
	auto instructions_restantes = 0;
	auto index = 0;
	auto derniere_instruction = atome_fonc->instructions[atome_fonc->instructions.taille - 1];

	POUR (atome_fonc->instructions) {
		if (it->est_branche() && it != derniere_instruction) {
			auto branche = it->comme_branche();

			if (branche->label == atome_fonc->instructions[index + 1]) {
				branche->label->nombre_utilisations -= 1;
				branche->nombre_utilisations = 0;
				instructions_restantes -= 1;
			}
		}

		instructions_restantes += 1;

		index += 1;
	}

#ifdef DEBOGUE_SUPPRESSION_CODE_MORT
	std::cerr << "======== avant suppression code mort ========\n"
	imprime_fonction(atome_fonc, std::cerr, true, true);
#endif

	auto nouvelles_instructions = kuri::tableau<Instruction *>();
	nouvelles_instructions.reserve(instructions_restantes);

	POUR (atome_fonc->instructions) {
		if (it->nombre_utilisations == 0) {
			index += 1;
			continue;
		}

		nouvelles_instructions.pousse(it);
	}

	atome_fonc->instructions = nouvelles_instructions;

#ifdef DEBOGUE_SUPPRESSION_CODE_MORT
	std::cerr << "======== après suppression code mort ========\n"
	imprime_fonction(atome_fonc, std::cerr);
#endif

#if 0
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
#endif
}

Atome *copie_atome(ConstructriceRI &constructrice, Atome *atome)
{
	if (atome == nullptr) {
		return nullptr;
	}

	// les constantes et les globales peuvent être partagées
	if (!atome->est_instruction()) {
		return atome;
	}

	auto inst = atome->comme_instruction();
	auto nouvelle_inst = static_cast<Instruction *>(nullptr);

	switch (inst->genre) {
		case Instruction::Genre::APPEL:
		{
			auto appel = inst->comme_appel();
			auto nouvelle_appel = constructrice.insts_appel.ajoute_element();
			nouvelle_appel->drapeaux = appel->drapeaux;
			nouvelle_appel->appele = copie_atome(constructrice, appel->appele);
			nouvelle_appel->lexeme = appel->lexeme;
			//nouvelle_appel->adresse_retour // XXX - À FAIRE

			nouvelle_appel->args.reserve(appel->args.taille);

			POUR (appel->args) {
				nouvelle_appel->args.pousse(copie_atome(constructrice, it));
			}

			break;
		}
		case Instruction::Genre::CHARGE_MEMOIRE:
		{
			auto charge = inst->comme_charge();
			auto n_charge = constructrice.insts_charge_memoire.ajoute_element();
			n_charge->chargee = copie_atome(constructrice, charge->chargee);
			nouvelle_inst = n_charge;
			break;
		}
		case Instruction::Genre::STOCKE_MEMOIRE:
		{
			auto stocke = inst->comme_stocke_mem();
			auto n_stocke = constructrice.insts_stocke_memoire.ajoute_element();
			n_stocke->ou = copie_atome(constructrice, stocke->ou);
			n_stocke->valeur = copie_atome(constructrice, stocke->valeur);
			nouvelle_inst = n_stocke;
			break;
		}
		case Instruction::Genre::OPERATION_UNAIRE:
		{
			auto op = inst->comme_op_unaire();
			auto n_op = constructrice.insts_opunaire.ajoute_element();
			n_op->op = op->op;
			n_op->valeur = copie_atome(constructrice, op->valeur);
			nouvelle_inst = n_op;
			break;
		}
		case Instruction::Genre::OPERATION_BINAIRE:
		{
			auto op = inst->comme_op_binaire();
			auto n_op = constructrice.insts_opbinaire.ajoute_element();
			n_op->op = op->op;
			n_op->valeur_gauche = copie_atome(constructrice, op->valeur_gauche);
			n_op->valeur_droite = copie_atome(constructrice, op->valeur_droite);
			nouvelle_inst = n_op;
			break;
		}
		case Instruction::Genre::ACCEDE_INDEX:
		{
			auto acces = inst->comme_acces_index();
			auto n_acces = constructrice.insts_accede_index.ajoute_element();
			n_acces->index = copie_atome(constructrice, acces->index);
			n_acces->accede = copie_atome(constructrice, acces->accede);
			nouvelle_inst = n_acces;
			break;
		}
		case Instruction::Genre::ACCEDE_MEMBRE:
		{
			auto acces = inst->comme_acces_membre();
			auto n_acces = constructrice.insts_accede_membre.ajoute_element();
			n_acces->index = copie_atome(constructrice, acces->index);
			n_acces->accede = copie_atome(constructrice, acces->accede);
			nouvelle_inst = n_acces;
			break;
		}
		case Instruction::Genre::TRANSTYPE:
		{
			auto transtype = inst->comme_transtype();
			auto n_transtype = constructrice.insts_transtype.ajoute_element();
			n_transtype->op = transtype->op;
			n_transtype->valeur = copie_atome(constructrice, transtype->valeur);
			nouvelle_inst = n_transtype;
			break;
		}
		case Instruction::Genre::BRANCHE_CONDITION:
		{
			auto branche = inst->comme_branche_cond();
			auto n_branche = constructrice.insts_branche_condition.ajoute_element();
			n_branche->condition = copie_atome(constructrice, branche->condition);
			n_branche->label_si_faux = static_cast<InstructionLabel *>(copie_atome(constructrice, branche->label_si_faux));
			n_branche->label_si_vrai = static_cast<InstructionLabel *>(copie_atome(constructrice, branche->label_si_vrai));
			nouvelle_inst = n_branche;
			break;
		}
		case Instruction::Genre::BRANCHE:
		{
			auto branche = inst->comme_branche();
			auto n_branche = constructrice.insts_branche.ajoute_element();
			n_branche->label = static_cast<InstructionLabel *>(copie_atome(constructrice, branche->label));
			nouvelle_inst = n_branche;
			break;
		}
		case Instruction::Genre::RETOUR:
		{
			auto retour = inst->comme_retour();
			auto n_retour = constructrice.insts_retour.ajoute_element();
			n_retour->valeur = copie_atome(constructrice, retour->valeur);
			nouvelle_inst = n_retour;
			break;
		}
		case Instruction::Genre::ENREGISTRE_LOCALES:
		case Instruction::Genre::RESTAURE_LOCALES:
		{
			nouvelle_inst = inst;
			break;
		}
		case Instruction::Genre::ALLOCATION:
		{
			auto alloc = inst->comme_alloc();
			auto n_alloc = constructrice.insts_allocation.ajoute_element();
			n_alloc->ident = alloc->ident;
			nouvelle_inst = n_alloc;
			break;
		}
		case Instruction::Genre::LABEL:
		{
			auto label = inst->comme_label();
			auto n_label = constructrice.insts_label.ajoute_element();
			n_label->id = label->id;
			nouvelle_inst = n_label;
			break;
		}
		case Instruction::Genre::INVALIDE:
		{
			break;
		}
	}

	if (nouvelle_inst) {
		nouvelle_inst->type = inst->type;
	}

	return nouvelle_inst;
}

void performe_enlignage(
		ConstructriceRI &constructrice,
		kuri::tableau<Instruction *> &nouvelles_instructions,
		kuri::tableau<Instruction *> const &instructions,
		AtomeFonction *fonction_appelee,
		kuri::tableau<Atome *> const &arguments,
		int &nombre_labels,
		InstructionLabel *label_post,
		InstructionAllocation *adresse_retour)
{
	using TypePaireSubstitution = std::pair<Atome *, Atome *>;
	dls::tablet<TypePaireSubstitution, 16> substitution;

	for (auto i = 0; i < fonction_appelee->params_entrees.taille; ++i) {
		auto atome = arguments[i];

		// À FAIRE : il faudrait que tous les arguments des fonctions soient des instructions (-> utilisation de temporaire)
		if (atome->genre_atome == Atome::Genre::INSTRUCTION) {
			auto inst = atome->comme_instruction();

			if (inst->genre == Instruction::Genre::CHARGE_MEMOIRE) {
				atome = inst->comme_charge()->chargee;
			}
			// À FAIRE : détection des pointeurs locaux plus robuste
			// détecte les cas où nous avons une référence à une variable
			else if (inst->est_alloc()) {
				auto type_pointe = static_cast<TypePointeur *>(inst->type)->type_pointe;
				if (type_pointe != atome->type) {
					// remplace l'instruction de déréférence par l'atome
					POUR (fonction_appelee->instructions) {
						if (it->est_charge()) {
							auto charge = it->comme_charge();

							if (charge->chargee == fonction_appelee->params_entrees[i]) {
								substitution.pousse({ charge, atome });
							}
						}
					}
				}
			}
		}

		substitution.pousse({ fonction_appelee->params_entrees[i], atome });
	}

	// À FAIRE : pour les paramètres il nous faudrait plutôt les adresses des variables chargées...
	POUR (instructions) {
		// À FAIRE : auto n_it = copie_atome(constructrice, it)->comme_instruction();
		auto n_it = it;

		if (n_it->genre == Instruction::Genre::LABEL) {
			auto label = n_it->comme_label();

			// saute le label d'entrée de la fonction
			if (label->id == 0) {
				continue;
			}

			label->id = nombre_labels++;
		}
		else if (n_it->genre == Instruction::Genre::RETOUR) {
			auto retour = n_it->comme_retour();

			if (retour->valeur) {
				auto stockage = constructrice.cree_stocke_mem(adresse_retour, retour->valeur, true);
				nouvelles_instructions.pousse(stockage);
			}

			auto branche = constructrice.cree_branche(label_post, true);
			nouvelles_instructions.pousse(branche);
			continue;
		}
		else if (n_it->genre == Instruction::Genre::CHARGE_MEMOIRE) {
			auto charge_loc = n_it->comme_charge();

			for (auto &p : substitution) {
				if (p.first == charge_loc->chargee) {
					charge_loc->chargee = p.second;
					break;
				}
			}
		}
		else if (n_it->genre == Instruction::Genre::STOCKE_MEMOIRE) {
			auto charge_loc = n_it->comme_stocke_mem();

			for (auto &p : substitution) {
				if (p.first == charge_loc->ou) {
					charge_loc->ou = p.second;
					break;
				}
			}
		}
		else if (n_it->genre == Instruction::Genre::ACCEDE_MEMBRE) {
			auto acces = n_it->comme_acces_membre();

			for (auto &p : substitution) {
				if (p.first == acces->accede) {
					acces->accede = p.second;
					break;
				}
			}
		}

		nouvelles_instructions.pousse(n_it);
	}
}

enum class SubstitutDans : int {
	ZERO = 0,
	CHARGE = (1 << 0),
	VALEUR_STOCKEE = (1 << 1),
	ADRESSE_STOCKEE = (1 << 2),

	TOUT   = (CHARGE | VALEUR_STOCKEE | ADRESSE_STOCKEE),
};

DEFINIE_OPERATEURS_DRAPEAU(SubstitutDans, int)

struct Substitutrice {
private:
	struct DonneesSubstitution {
		Atome *original = nullptr;
		Atome *substitut = nullptr;
		SubstitutDans substitut_dans = SubstitutDans::TOUT;
	};

	dls::tableau<DonneesSubstitution> substitutions{};

public:
	void ajoute_substitution(Atome *original, Atome *substitut, SubstitutDans substitut_dans)
	{
		POUR (substitutions) {
			if (it.original == original) {
				it.substitut = substitut;
				it.substitut_dans = substitut_dans;
				return;
			}
		}

		substitutions.pousse({ original, substitut, substitut_dans });
	}

	Instruction *instruction_substituee(Instruction *instruction)
	{
		switch (instruction->genre) {
			case Instruction::Genre::CHARGE_MEMOIRE:
			{
				auto charge = instruction->comme_charge();

				POUR (substitutions) {
					if (it.original == charge->chargee && (it.substitut_dans & SubstitutDans::CHARGE) != SubstitutDans::ZERO) {
						charge->chargee = it.substitut;
						break;
					}
				}

				return charge;
			}
			case Instruction::Genre::STOCKE_MEMOIRE:
			{
				auto stocke = instruction->comme_stocke_mem();

				POUR (substitutions) {
					if (it.original == stocke->ou && (it.substitut_dans & SubstitutDans::ADRESSE_STOCKEE) != SubstitutDans::ZERO) {
						stocke->ou = it.substitut;
					}
					else if (it.original == stocke->valeur && (it.substitut_dans & SubstitutDans::VALEUR_STOCKEE) != SubstitutDans::ZERO) {
						stocke->valeur = it.substitut;
					}
				}

				return stocke;
			}
			case Instruction::Genre::OPERATION_BINAIRE:
			{
				auto op = instruction->comme_op_binaire();

				POUR (substitutions) {
					if (it.original == op->valeur_gauche) {
						op->valeur_gauche = it.substitut;
					}
					else if (it.original == op->valeur_droite) {
						op->valeur_droite = it.substitut;
					}
				}

				return op;
			}
			case Instruction::Genre::RETOUR:
			{
				auto retour = instruction->comme_retour();

				if (retour->valeur) {
					retour->valeur = valeur_substituee(retour->valeur);
				}

				return retour;
			}
			case Instruction::Genre::ACCEDE_MEMBRE:
			{
				auto acces = instruction->comme_acces_membre();
				acces->accede = valeur_substituee(acces->accede);
				return acces;
			}
			default:
			{
				return instruction;
			}
		}
	}

	Atome *valeur_substituee(Atome *original)
	{
		POUR (substitutions) {
			if (it.original == original) {
				return it.substitut;
			}
		}

		return original;
	}
};

#undef DEBOGUE_ENLIGNAGE

void enligne_fonctions(ConstructriceRI &constructrice, AtomeFonction *atome_fonc)
{
	auto nouvelle_instructions = kuri::tableau<Instruction *>();
	nouvelle_instructions.reserve(atome_fonc->instructions.taille);

	auto substitutrice = Substitutrice();

	auto nombre_labels = 0;

	POUR (atome_fonc->instructions) {
		nombre_labels += it->genre == Instruction::Genre::LABEL;
	}

	POUR (atome_fonc->instructions) {
		if (it->genre != Instruction::Genre::APPEL) {
			nouvelle_instructions.pousse(substitutrice.instruction_substituee(it));
			continue;
		}

		auto appel = static_cast<InstructionAppel *>(it);
		auto appele = appel->appele;

		if (appele->genre_atome != Atome::Genre::FONCTION) {
			nouvelle_instructions.pousse(substitutrice.instruction_substituee(it));
			continue;
		}

		auto atome_fonc_appelee = static_cast<AtomeFonction *>(appele);

		auto &instructions = atome_fonc_appelee->instructions;

		if (atome_fonc_appelee->decl) {
			if (atome_fonc_appelee->decl->est_externe) {
				nouvelle_instructions.pousse(substitutrice.instruction_substituee(it));
				continue;
			}

			if ((atome_fonc_appelee->decl->drapeaux & FORCE_ENLIGNE) == 0) {
				nouvelle_instructions.pousse(substitutrice.instruction_substituee(it));
				continue;
			}
			else if ((atome_fonc_appelee->decl->drapeaux & FORCE_HORSLIGNE) != 0) {
				nouvelle_instructions.pousse(substitutrice.instruction_substituee(it));
				continue;
			}
		}
		else {
			// À FAIRE : définis de bonnes heuristiques pour l'enlignage
			// À FAIRE : pour les enlignages des fonctions d'initialisations, ou de n'importe
			// quelle fonction prenant un pointeur, il faudra convertir les types pour enlever le pointeur
			// si le pointeur est l'adresse de la variable
			if (instructions.taille >= 32) {
				nouvelle_instructions.pousse(substitutrice.instruction_substituee(it));
				continue;
			}
//			nouvelle_instructions.pousse(substitutrice.instruction_substituee(it));
//			continue;
		}

		// À FAIRE : attend que les fonctions soient disponibles
		// À FAIRE : les instructions ne pourraient être composées que de retour « rien » et de labels
		if (instructions.est_vide()) {
			nouvelle_instructions.pousse(substitutrice.instruction_substituee(it));
			continue;
		}

		nouvelle_instructions.reserve_delta(instructions.taille + 1);

		// crée une nouvelle adresse retour pour faciliter la suppression de l'instruction de stockage de la valeur de retour dans l'ancienne adresse
		auto adresse_retour = static_cast<InstructionAllocation *>(nullptr);

		if (appel->type->genre != GenreType::RIEN) {
			adresse_retour = constructrice.cree_allocation(appel->type, nullptr, true);
			nouvelle_instructions.pousse(adresse_retour);
		}

		auto label_post = constructrice.reserve_label();
		label_post->id = nombre_labels++;

		performe_enlignage(constructrice, nouvelle_instructions, instructions, atome_fonc_appelee, appel->args, nombre_labels, label_post, adresse_retour);

		atome_fonc_appelee->nombre_utilisations -= 1;

		nouvelle_instructions.pousse(label_post);

		if (adresse_retour) {
			// nous ne substituons l'adresse que pour le chargement de sa valeur, ainsi lors du stockage de la valeur
			// l'ancienne adresse aura un compte d'utilisation de zéro et l'instruction de stockage sera supprimée avec
			// l'ancienne adresse dans la passe de suppression de code mort
			substitutrice.ajoute_substitution(appel->adresse_retour, adresse_retour, SubstitutDans::CHARGE);
		}
	}

#ifdef DEBOGUE_ENLIGNAGE
	std::cerr << "===== avant enlignage =====\n";
	imprime_fonction(atome_fonc, std::cerr);
#endif

	atome_fonc->instructions = nouvelle_instructions;

#ifdef DEBOGUE_ENLIGNAGE
	std::cerr << "===== après enlignage =====\n";
	imprime_fonction(atome_fonc, std::cerr);
#endif
}

#undef DEBOGUE_PROPAGATION

void propage_constantes_et_temporaires(AtomeFonction *atome_fonc)
{
	auto nouvelle_instructions = kuri::tableau<Instruction *>();
	nouvelle_instructions.reserve(atome_fonc->instructions.taille);

	dls::tableau<std::pair<Atome *, Atome *>> dernieres_valeurs;

	// À FAIRE : renseigne la dernière valeur des membres
	auto renseigne_derniere_valeur = [&](Atome *ptr, Atome *valeur)
	{
		POUR (dernieres_valeurs) {
			if (it.first == ptr) {
				it.second = valeur;
				return;
			}
		}

		dernieres_valeurs.pousse({ ptr, valeur });
	};

	auto substitutrice = Substitutrice();

	POUR (atome_fonc->instructions) {
		if (it->genre == Instruction::Genre::STOCKE_MEMOIRE) {
			auto stocke = it->comme_stocke_mem();

			stocke->valeur = substitutrice.valeur_substituee(stocke->valeur);
			renseigne_derniere_valeur(stocke->ou, stocke->valeur);
			nouvelle_instructions.pousse(it);
		}
		else if (it->genre == Instruction::Genre::CHARGE_MEMOIRE) {
			auto charge = it->comme_charge();

			for (auto dv : dernieres_valeurs) {
				if (dv.first == charge->chargee) {
					substitutrice.ajoute_substitution(it, dv.second, SubstitutDans::TOUT);
					break;
				}
			}

			nouvelle_instructions.pousse(it);
		}
		else {
			nouvelle_instructions.pousse(substitutrice.instruction_substituee(it));
		}
	}

#ifdef DEBOGUE_PROPAGATION
	std::cerr << "===== avant propagation =====\n";
	imprime_fonction(atome_fonc, std::cerr);
#endif

	atome_fonc->instructions = nouvelle_instructions;

#ifdef DEBOGUE_PROPAGATION
	std::cerr << "===== après propagation =====\n";
	imprime_fonction(atome_fonc, std::cerr);
#endif

	supprime_code_mort(atome_fonc);
}
