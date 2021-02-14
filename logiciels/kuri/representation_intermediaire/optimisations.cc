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
#include "compilation/identifiant.hh"

#include "constructrice_ri.hh"
#include "impression.hh"
#include "instructions.hh"

/*
  À FAIRE(optimisations) :
  - crée toujours des blocs pour la RI, l'enlignage sera plus simple
  - bug dans la fusion des blocs, qui nous laissent avec des labels inconnus
  - déplace les instruction dans les blocs au plus près de leurs utilisations

  À FAIRE(enlignage) :
  - détecte les fonctions récursives, empêche leurs enlignages
  - enlignage ascendant (considère la fonction enlignée d'abord) ou descendant (considère la fonction où enligner d'abord)
  - problème avec l'enlignage : il semblerait que les pointeurs ne soit pas correctement « enlignés » pour les accès de membres
  - change la métriques pour être sur le nombre de lignes, et non le nombre d'instructions
 */

static bool log_actif = false;

static void active_log()
{
	log_actif = true;
}

static void desactive_log()
{
	log_actif = false;
}

template <typename... Ts>
void log(std::ostream &os, Ts... ts)
{
	if (!log_actif) {
		return;
	}

	((os << ts), ...);
	os << '\n';
}

enum {
	PROPRE                          = 0,

	/* des fonctions sont à enlignées */
	REQUIERS_ENLIGNAGE              = (1 << 0),

	/* des blocs sont vides ou furent insérés ou supprimés */
	REQUIERS_CORRECTION_BLOCS       = (1 << 1),

	/* des constantes ont été détectées
	 * - soit des nombres littéraux
	 * - soit l'accès à des globales (taille de chaines...) */
	REQUIERS_PROPAGATION_CONSTANTES = (1 << 2),

	/* des boucles peuvent être « débouclées » */
	REQUIERS_DEBOUCLAGE             = (1 << 3),

	/* du code peut être supprimé */
	REQUIERS_SUPPRESSION_CODE_MORT  = (1 << 4),
};

struct Bloc;
static void imprime_bloc(Bloc *bloc, int decalage_instruction, std::ostream &os, bool surligne_inutilisees = false);
static void imprime_blocs(const dls::tableau<Bloc *> &blocs, std::ostream &os);

struct Bloc {
	InstructionLabel *label = nullptr;

	kuri::tableau<Instruction *> instructions{};

	dls::tableau<Bloc *> parents{};
	dls::tableau<Bloc *> enfants{};

	/* les variables déclarées dans ce bloc */
	dls::tableau<InstructionAllocation *> variables_declarees{};

	/* les variables utilisées dans ce bloc */
	dls::tableau<InstructionAllocation *> variables_utilisees{};

	void ajoute_enfant(Bloc *enfant)
	{
		enfant->ajoute_parent(this);

		POUR (enfants) {
			if (it == enfant) {
				return;
			}
		}

		enfants.ajoute(enfant);
	}

	void remplace_enfant(Bloc *enfant, Bloc *par)
	{
		enleve_du_tableau(enfants, enfant);
		ajoute_enfant(par);
		enfant->enleve_parent(this);
		par->ajoute_parent(this);

		auto inst = instructions.derniere();

		if (inst->est_branche()) {
			auto branche = inst->comme_branche();
			branche->label = par->label;
			return;
		}

		if (inst->est_branche_cond()) {
			auto branche_cond = inst->comme_branche_cond();
			auto label_si_vrai = branche_cond->label_si_vrai;
			auto label_si_faux = branche_cond->label_si_faux;

			if (label_si_vrai == enfant->label) {
				branche_cond->label_si_vrai = par->label;
			}

			if (label_si_faux == enfant->label) {
				branche_cond->label_si_faux = par->label;
			}

			return;
		}
	}

	void enleve_parent(Bloc *parent)
	{
		enleve_du_tableau(parents, parent);
	}

	void enleve_enfant(Bloc *enfant)
	{
		enleve_du_tableau(enfants, enfant);

		/* quand nous enlevons un enfant, il faut modifier la cible des branches potentielles */

		if (instructions.est_vide()) {
			return;
		}

		/* création_contexte n'a pas d'instruction de retour à la fin de son bloc, et après l'enlignage, nous nous retrouvons avec un bloc vide à la fin de la fonction */
		if (enfant->instructions.est_vide()) {
			return;
		}

		auto inst = instructions.derniere();

		if (log_actif) {
			std::cerr << "-- dernière inststruction : ";
			imprime_instruction(inst, std::cerr);
		}

		if (inst->est_branche()) {
			// À FAIRE
			return;
		}

		if (inst->est_branche_cond()) {
			auto branche_cond = inst->comme_branche_cond();
			auto label_si_vrai = branche_cond->label_si_vrai;
			auto label_si_faux = branche_cond->label_si_faux;

			if (label_si_vrai == enfant->label) {
				branche_cond->label_si_vrai = label_si_faux;
			}
			else if (label_si_faux == enfant->label) {
				branche_cond->label_si_faux = label_si_vrai;
			}
			else {
				//assert(0);
			}

			return;
		}

		if (inst->est_retour()) {
			return;
		}

		log(std::cerr, "bloc ", label->id);
		assert(0);
	}

	bool peut_fusionner_enfant()
	{
		if (enfants.taille() == 0) {
			log(std::cerr, "enfant == 0");
			return false;
		}

		if (enfants.taille() > 1) {
			log(std::cerr, "enfants.taille() > 1");
			return false;
		}

		auto enfant = enfants[0];

		if (enfant->parents.taille() > 1) {
			log(std::cerr, "enfants.parents.taille() > 1");
			return false;
		}

		return true;
	}

	void utilise_variable(InstructionAllocation *variable)
	{
		if (!variable) {
			return;
		}

		for (auto var : this->variables_utilisees) {
			if (var == variable) {
				return;
			}
		}

		this->variables_utilisees.ajoute(variable);
		variable->blocs_utilisants += 1;
	}

	void fusionne_enfant(Bloc *enfant)
	{
		log(std::cerr, "S'apprête à fusionner le bloc ", enfant->label->id, " dans le bloc ", this->label->id);

		this->instructions.supprime_dernier();
		this->instructions.reserve_delta(enfant->instructions.taille);

		POUR (enfant->instructions) {
			this->instructions.ajoute(it);
		}

		this->variables_declarees.reserve(enfant->variables_declarees.taille() + this->variables_declarees.taille());
		POUR (enfant->variables_declarees) {
			this->variables_declarees.ajoute(it);
		}

		POUR (enfant->variables_utilisees) {
			it->blocs_utilisants -= 1;
			this->utilise_variable(it);
		}

		this->enleve_enfant(enfant);

		POUR (enfant->enfants) {
			this->ajoute_enfant(it);
		}

		POUR (this->enfants) {
			it->enleve_parent(enfant);
		}

//		std::cerr << "-- enfants après fusion : ";
//		POUR (this->enfants) {
//			std::cerr << it->label->id << " ";
//		}
//		std::cerr << "\n";

		if (log_actif) {
			std::cerr << "-- bloc après fusion :\n";
			imprime_bloc(this, 0, std::cerr);
		}

		enfant->instructions.taille = 0;
	}

private:
	void enleve_du_tableau(dls::tableau<Bloc *> &tableau, Bloc *bloc)
	{
		for (auto i = 0; i < tableau.taille(); ++i) {
			if (tableau[i] == bloc) {
				std::swap(tableau[i], tableau[tableau.taille() - 1]);
				tableau.redimensionne(tableau.taille() - 1);
				break;
			}
		}
	}

	void ajoute_parent(Bloc *parent)
	{
		POUR (parents) {
			if (it == parent) {
				return;
			}
		}

		parents.ajoute(parent);
	}
};

static void imprime_bloc(Bloc *bloc, int decalage_instruction, std::ostream &os, bool surligne_inutilisees)
{
	os << "Bloc " << bloc->label->id << ' ';

	auto virgule = " [";
	if (bloc->parents.est_vide()) {
		os << virgule;
	}
	for (auto parent : bloc->parents) {
		os << virgule << parent->label->id;
		virgule = ", ";
	}
	os << "]";

	virgule = " [";
	if (bloc->enfants.est_vide()) {
		os << virgule;
	}
	for (auto enfant : bloc->enfants) {
		os << virgule << enfant->label->id;
		virgule = ", ";
	}
	os << "]\n";

	imprime_instructions(bloc->instructions, decalage_instruction, os, false, surligne_inutilisees);
}

static void imprime_blocs(const dls::tableau<Bloc *> &blocs, std::ostream &os)
{
	os << "=================== Blocs ===================\n";

	int decalage_instruction = 0;
	POUR (blocs) {
		imprime_bloc(it, decalage_instruction, os);
		decalage_instruction += static_cast<int>(it->instructions.taille);
	}
}

static InstructionAllocation *alloc_ou_nul(Atome *atome)
{
	if (!atome->est_instruction()) {
		return nullptr;
	}

	auto inst = atome->comme_instruction();

	if (inst->est_alloc()) {
		return inst->comme_alloc();
	}

	if (inst->est_acces_membre()) {
		return alloc_ou_nul(inst->comme_acces_membre()->accede);
	}

	return nullptr;
}

static void construit_liste_variables_utilisees(Bloc *bloc)
{
	POUR (bloc->instructions) {
		if (it->est_alloc()) {
			auto alloc = it->comme_alloc();
			bloc->variables_declarees.ajoute(alloc);
			continue;
		}

		if (it->est_stocke_mem()) {
			auto stocke = it->comme_stocke_mem();
			bloc->utilise_variable(alloc_ou_nul(stocke->ou));
		}
		else if (it->est_acces_membre()) {
			auto membre = it->comme_acces_membre();
			bloc->utilise_variable(alloc_ou_nul(membre->accede));
		}
		else if (it->est_op_binaire()) {
			auto op = it->comme_op_binaire();
			bloc->utilise_variable(alloc_ou_nul(op->valeur_gauche));
			bloc->utilise_variable(alloc_ou_nul(op->valeur_droite));
		}
	}
}

static Bloc *bloc_pour_label(dls::tableau<Bloc *> &blocs, InstructionLabel *label)
{
	POUR (blocs) {
		if (it->label == label) {
			return it;
		}
	}

	auto bloc = memoire::loge<Bloc>("Bloc");
	bloc->label = label;
	blocs.ajoute(bloc);
	return bloc;
}

/* À FAIRE(optimisations) : non-urgent
 * - Substitutrice, pour généraliser les substitions d'instructions
 * - copie des instructions (requiers de séparer les allocations des instructions de la ConstructriceRI)
 */

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

/* Petit algorithme de suppression de code mort.
 *
 * Le code mort est pour le moment défini comme étant toute instruction ne participant pas au résultat final, ou à une branche conditionnelle.
 * Les fonctions ne retournant rien sont considérées comme utile pour le moment, il faudra avoir un système pour détecter les effets secondaire.
 * Les fonctions dont la valeur de retour est ignorée sont supprimées malheureusement, il faudra changer cela avant de tenter d'activer ce code.
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

struct CopieuseInstruction {
private:
	std::map<Atome *, Atome *> copies{};
	ConstructriceRI &constructrice;

public:
	CopieuseInstruction(ConstructriceRI &constructrice_)
		: constructrice(constructrice_)
	{}

	void ajoute_substitution(Atome *a, Atome *b)
	{
		copies.insert({ a, b });
	}

	kuri::tableau<Instruction *> copie_instructions(AtomeFonction *atome_fonction)
	{
		kuri::tableau<Instruction *> resultat;
		resultat.reserve(atome_fonction->instructions.taille);

		POUR (atome_fonction->instructions) {
			// s'il existe une substition pour cette instruction, ignore-là
			if (!it->est_label() && copies.find(it) != copies.end()) {
				continue;
			}

			resultat.ajoute(static_cast<Instruction *>(copie_atome(it)));
		}

		return resultat;
	}

	Atome *copie_atome(Atome *atome)
	{
		if (atome == nullptr) {
			return nullptr;
		}

		// les constantes et les globales peuvent être partagées
		if (!atome->est_instruction()) {
			return atome;
		}

		auto inst = atome->comme_instruction();

		auto iter = copies.find(inst);
		if (iter != copies.end()) {
			return iter->second;
		}

		auto nouvelle_inst = static_cast<Instruction *>(nullptr);

		switch (inst->genre) {
			case Instruction::Genre::APPEL:
			{
				auto appel = inst->comme_appel();
				auto nouvelle_appel = constructrice.insts_appel.ajoute_element(inst->site);
				nouvelle_appel->drapeaux = appel->drapeaux;
				nouvelle_appel->appele = copie_atome(appel->appele);
				nouvelle_appel->lexeme = appel->lexeme;
				nouvelle_appel->adresse_retour = static_cast<InstructionAllocation *>(copie_atome(appel->adresse_retour));

				nouvelle_appel->args.reserve(appel->args.taille);

				POUR (appel->args) {
					nouvelle_appel->args.ajoute(copie_atome(it));
				}

				nouvelle_inst = nouvelle_appel;
				break;
			}
			case Instruction::Genre::CHARGE_MEMOIRE:
			{
				auto charge = inst->comme_charge();
				auto n_charge = constructrice.insts_charge_memoire.ajoute_element(inst->site);
				n_charge->chargee = copie_atome(charge->chargee);
				nouvelle_inst = n_charge;
				break;
			}
			case Instruction::Genre::STOCKE_MEMOIRE:
			{
				auto stocke = inst->comme_stocke_mem();
				auto n_stocke = constructrice.insts_stocke_memoire.ajoute_element(inst->site);
				n_stocke->ou = copie_atome(stocke->ou);
				n_stocke->valeur = copie_atome(stocke->valeur);
				nouvelle_inst = n_stocke;
				break;
			}
			case Instruction::Genre::OPERATION_UNAIRE:
			{
				auto op = inst->comme_op_unaire();
				auto n_op = constructrice.insts_opunaire.ajoute_element(inst->site);
				n_op->op = op->op;
				n_op->valeur = copie_atome(op->valeur);
				nouvelle_inst = n_op;
				break;
			}
			case Instruction::Genre::OPERATION_BINAIRE:
			{
				auto op = inst->comme_op_binaire();
				auto n_op = constructrice.insts_opbinaire.ajoute_element(inst->site);
				n_op->op = op->op;
				n_op->valeur_gauche = copie_atome(op->valeur_gauche);
				n_op->valeur_droite = copie_atome(op->valeur_droite);
				nouvelle_inst = n_op;
				break;
			}
			case Instruction::Genre::ACCEDE_INDEX:
			{
				auto acces = inst->comme_acces_index();
				auto n_acces = constructrice.insts_accede_index.ajoute_element(inst->site);
				n_acces->index = copie_atome(acces->index);
				n_acces->accede = copie_atome(acces->accede);
				nouvelle_inst = n_acces;
				break;
			}
			case Instruction::Genre::ACCEDE_MEMBRE:
			{
				auto acces = inst->comme_acces_membre();
				auto n_acces = constructrice.insts_accede_membre.ajoute_element(inst->site);
				n_acces->index = copie_atome(acces->index);
				n_acces->accede = copie_atome(acces->accede);
				nouvelle_inst = n_acces;
				break;
			}
			case Instruction::Genre::TRANSTYPE:
			{
				auto transtype = inst->comme_transtype();
				auto n_transtype = constructrice.insts_transtype.ajoute_element(inst->site);
				n_transtype->op = transtype->op;
				n_transtype->valeur = copie_atome(transtype->valeur);
				nouvelle_inst = n_transtype;
				break;
			}
			case Instruction::Genre::BRANCHE_CONDITION:
			{
				auto branche = inst->comme_branche_cond();
				auto n_branche = constructrice.insts_branche_condition.ajoute_element(inst->site);
				n_branche->condition = copie_atome(branche->condition);
				n_branche->label_si_faux = copie_atome(branche->label_si_faux)->comme_instruction()->comme_label();
				n_branche->label_si_vrai = copie_atome(branche->label_si_vrai)->comme_instruction()->comme_label();
				nouvelle_inst = n_branche;
				break;
			}
			case Instruction::Genre::BRANCHE:
			{
				auto branche = inst->comme_branche();
				auto n_branche = constructrice.insts_branche.ajoute_element(inst->site);
				n_branche->label = copie_atome(branche->label)->comme_instruction()->comme_label();
				nouvelle_inst = n_branche;
				break;
			}
			case Instruction::Genre::RETOUR:
			{
				auto retour = inst->comme_retour();
				auto n_retour = constructrice.insts_retour.ajoute_element(inst->site);
				n_retour->valeur = copie_atome(retour->valeur);
				nouvelle_inst = n_retour;
				break;
			}
			case Instruction::Genre::ALLOCATION:
			{
				auto alloc = inst->comme_alloc();
				auto n_alloc = constructrice.insts_allocation.ajoute_element(inst->site);
				n_alloc->ident = alloc->ident;
				nouvelle_inst = n_alloc;
				break;
			}
			case Instruction::Genre::LABEL:
			{
				auto label = inst->comme_label();
				auto n_label = constructrice.insts_label.ajoute_element(inst->site);
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
			copies.insert({ inst, nouvelle_inst });
		}

		return nouvelle_inst;
	}
};

void performe_enlignage(
		ConstructriceRI &constructrice,
		kuri::tableau<Instruction *> &nouvelles_instructions,
		AtomeFonction *fonction_appelee,
		kuri::tableau<Atome *> const &arguments,
		int &nombre_labels,
		InstructionLabel *label_post,
		InstructionAllocation *adresse_retour)
{

	auto copieuse = CopieuseInstruction(constructrice);

	for (auto i = 0; i < fonction_appelee->params_entrees.taille; ++i) {
		auto parametre = fonction_appelee->params_entrees[i]->comme_instruction();
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
				auto type_pointe = inst->type->comme_pointeur()->type_pointe;
				if (type_pointe != atome->type) {
					// remplace l'instruction de déréférence par l'atome
					POUR (fonction_appelee->instructions) {
						if (it->est_charge()) {
							auto charge = it->comme_charge();

							if (charge->chargee == parametre) {
								copieuse.ajoute_substitution(charge, atome);
							}
						}
					}
				}
			}
		}
		else if (atome->genre_atome == Atome::Genre::CONSTANTE) {
			POUR (fonction_appelee->instructions) {
				if (it->est_charge()) {
					auto charge = it->comme_charge();

					if (charge->chargee == parametre) {
						copieuse.ajoute_substitution(charge, atome);
					}
				}
			}
		}

		copieuse.ajoute_substitution(parametre, atome);
	}

	auto instructions_copiees = copieuse.copie_instructions(fonction_appelee);
	nouvelles_instructions.reserve_delta(instructions_copiees.taille);

	POUR (instructions_copiees) {
		if (it->genre == Instruction::Genre::LABEL) {
			auto label = it->comme_label();

			// saute le label d'entrée de la fonction
			if (label->id == 0) {
				continue;
			}

			label->id = nombre_labels++;
		}
		else if (it->genre == Instruction::Genre::RETOUR) {
			auto retour = it->comme_retour();

			if (retour->valeur) {
				auto stockage = constructrice.cree_stocke_mem(nullptr, adresse_retour, retour->valeur, true);
				nouvelles_instructions.ajoute(stockage);
			}

			auto branche = constructrice.cree_branche(nullptr, label_post, true);
			nouvelles_instructions.ajoute(branche);
			continue;
		}

		nouvelles_instructions.ajoute(it);
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

	dls::tablet<DonneesSubstitution, 16> substitutions{};

public:
	void ajoute_substitution(Atome *original, Atome *substitut, SubstitutDans substitut_dans)
	{
		assert(original);
		assert(substitut);

		if (log_actif) {
			std::cerr << "Subtitut : ";
			imprime_atome(original, std::cerr);
			std::cerr << " avec ";
			imprime_atome(substitut, std::cerr);
			std::cerr << '\n';
		}

		POUR (substitutions) {
			if (it.original == original) {
				it.substitut = substitut;
				it.substitut_dans = substitut_dans;
				return;
			}
		}

		substitutions.ajoute({ original, substitut, substitut_dans });
	}

	void reinitialise()
	{
		substitutions.efface();
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

					if (it.original == charge) {
						return static_cast<Instruction *>(it.substitut);
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

				op->valeur_gauche = valeur_substituee(op->valeur_gauche);
				op->valeur_droite = valeur_substituee(op->valeur_droite);

//				POUR (substitutions) {
//					if (it.original == op->valeur_gauche) {
//						op->valeur_gauche = it.substitut;
//					}
//					else if (it.original == op->valeur_droite) {
//						op->valeur_droite = it.substitut;
//					}
//				}

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
			case Instruction::Genre::APPEL:
			{
				auto appel = instruction->comme_appel();
				appel->appele = valeur_substituee(appel->appele);

				POUR (appel->args) {
					it = valeur_substituee(it);
				}

				return appel;
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

// À FAIRE : définis de bonnes heuristiques pour l'enlignage
static bool est_candidate_pour_enlignage(AtomeFonction *fonction)
{
	log(std::cerr, "candidate pour enlignage : ", fonction->nom);

	/* appel d'une fonction externe */
	if (fonction->instructions.taille == 0) {
		log(std::cerr, "-- ignore la candidate car il n'y a pas d'instructions...");
		return false;
	}

	if (fonction->instructions.taille < 32) {
		return true;
	}

	if (fonction->decl) {
		if (fonction->decl->possede_drapeau(FORCE_ENLIGNE)) {
			return true;
		}

		if (fonction->decl->possede_drapeau(FORCE_HORSLIGNE)) {
			log(std::cerr, "-- ignore la candidate car nous forçons un horslignage...");
			return false;
		}
	}

	log(std::cerr, "-- ignore la candidate car il y a trop d'instructions...");
	return false;
}

bool enligne_fonctions(ConstructriceRI &constructrice, AtomeFonction *atome_fonc)
{
	auto nouvelle_instructions = kuri::tableau<Instruction *>();
	nouvelle_instructions.reserve(atome_fonc->instructions.taille);

	auto substitutrice = Substitutrice();
	auto nombre_labels = 0;
	auto nombre_fonctions_enlignees = 0;

	POUR (atome_fonc->instructions) {
		nombre_labels += it->genre == Instruction::Genre::LABEL;
	}

	POUR (atome_fonc->instructions) {
		if (it->genre != Instruction::Genre::APPEL) {
			nouvelle_instructions.ajoute(substitutrice.instruction_substituee(it));
			continue;
		}

		auto appel = it->comme_appel();
		auto appele = appel->appele;

		if (appele->genre_atome != Atome::Genre::FONCTION) {
			nouvelle_instructions.ajoute(substitutrice.instruction_substituee(it));
			continue;
		}

		auto atome_fonc_appelee = static_cast<AtomeFonction *>(appele);

		if (!est_candidate_pour_enlignage(atome_fonc_appelee)) {
			nouvelle_instructions.ajoute(substitutrice.instruction_substituee(it));
			continue;
		}

		nouvelle_instructions.reserve_delta(atome_fonc_appelee->instructions.taille + 1);

		// crée une nouvelle adresse retour pour faciliter la suppression de l'instruction de stockage de la valeur de retour dans l'ancienne adresse
		auto adresse_retour = static_cast<InstructionAllocation *>(nullptr);

		if (appel->type->genre != GenreType::RIEN) {
			adresse_retour = constructrice.cree_allocation(nullptr, appel->type, nullptr, true);
			nouvelle_instructions.ajoute(adresse_retour);
		}

		auto label_post = constructrice.reserve_label(nullptr);
		label_post->id = nombre_labels++;

		performe_enlignage(constructrice, nouvelle_instructions, atome_fonc_appelee, appel->args, nombre_labels, label_post, adresse_retour);
		nombre_fonctions_enlignees += 1;

		atome_fonc_appelee->nombre_utilisations -= 1;

		nouvelle_instructions.ajoute(label_post);

		if (adresse_retour) {
			// nous ne substituons l'adresse que pour le chargement de sa valeur, ainsi lors du stockage de la valeur
			// l'ancienne adresse aura un compte d'utilisation de zéro et l'instruction de stockage sera supprimée avec
			// l'ancienne adresse dans la passe de suppression de code mort
			auto charge = constructrice.cree_charge_mem(appel->site, adresse_retour, true);
			nouvelle_instructions.ajoute(charge);
			substitutrice.ajoute_substitution(appel, charge, SubstitutDans::VALEUR_STOCKEE);
		}
	}

#ifdef DEBOGUE_ENLIGNAGE
	std::cerr << "===== avant enlignage =====\n";
	imprime_fonction(atome_fonc, std::cerr);
#endif

	atome_fonc->instructions = std::move(nouvelle_instructions);

#ifdef DEBOGUE_ENLIGNAGE
	std::cerr << "===== après enlignage =====\n";
	imprime_fonction(atome_fonc, std::cerr);
#endif

	return nombre_fonctions_enlignees != 0;
}

#undef DEBOGUE_PROPAGATION

/* principalement pour détecter des accès à des membres */
static bool sont_equivalents(Atome *a, Atome *b)
{
	if (a == b) {
		return true;
	}

	if (a->genre_atome != b->genre_atome) {
		return false;
	}

	if (a->est_constante()) {
		auto const_a = static_cast<AtomeConstante *>(a);
		auto const_b = static_cast<AtomeConstante *>(b);

		if (const_a->genre != const_b->genre) {
			return false;
		}

		if (const_a->genre == AtomeConstante::Genre::VALEUR) {
			auto val_a = static_cast<AtomeValeurConstante *>(a);
			auto val_b = static_cast<AtomeValeurConstante *>(b);

			if (val_a->valeur.genre != val_b->valeur.genre) {
				return false;
			}

			if (val_a->valeur.valeur_entiere != val_b->valeur.valeur_entiere) {
				return false;
			}

			return true;
		}
	}

	if (a->est_instruction()) {
		auto inst_a = a->comme_instruction();
		auto inst_b = b->comme_instruction();

		if (inst_a->genre != inst_b->genre) {
			return false;
		}

		if (inst_a->est_acces_membre()) {
			auto ma = inst_a->comme_acces_membre();
			auto mb = inst_b->comme_acces_membre();

			if (!sont_equivalents(ma->accede, mb->accede)) {
				return false;
			}

			if (!sont_equivalents(ma->index, mb->index)) {
				return false;
			}

			return true;
		}
	}

	return false;
}

static bool operandes_sont_constantes(InstructionOpBinaire *op)
{
	if (op->valeur_droite->genre_atome != Atome::Genre::CONSTANTE) {
		return false;
	}

	if (op->valeur_gauche->genre_atome != Atome::Genre::CONSTANTE) {
		return false;
	}

	auto const_a = static_cast<AtomeConstante *>(op->valeur_gauche);
	auto const_b = static_cast<AtomeConstante *>(op->valeur_droite);

	if (const_a->genre != const_b->genre) {
		return false;
	}

	if (const_a->genre != AtomeConstante::Genre::VALEUR) {
		return false;
	}

	auto val_a = static_cast<AtomeValeurConstante *>(op->valeur_gauche);
	auto val_b = static_cast<AtomeValeurConstante *>(op->valeur_droite);

	if (val_a->valeur.genre != val_b->valeur.genre) {
		return false;
	}

	if (val_a->valeur.genre != AtomeValeurConstante::Valeur::Genre::ENTIERE) {
		return false;
	}

	auto va = val_a->valeur.valeur_entiere;
	auto vb = val_b->valeur.valeur_entiere;

	if (op->op == OperateurBinaire::Genre::Addition) {
		val_a->valeur.valeur_entiere = va + vb;
		return true;
	}

	if (op->op == OperateurBinaire::Genre::Multiplication) {
		val_a->valeur.valeur_entiere = va * vb;
		return true;
	}

	return false;
}

static bool propage_constantes_et_temporaires(kuri::tableau<Instruction *> &instructions)
{
	dls::tablet<std::pair<Atome *, Atome *>, 16> dernieres_valeurs;
	dls::tablet<InstructionAccedeMembre *, 16> acces_membres;

	auto renseigne_derniere_valeur = [&](Atome *ptr, Atome *valeur)
	{
		if (log_actif) {
			std::cerr << "Dernière valeur pour ";
			imprime_atome(ptr, std::cerr);
			std::cerr << " est ";
			imprime_atome(valeur, std::cerr);
			std::cerr << "\n";
		}

		POUR (dernieres_valeurs) {
			if (it.first == ptr) {
				it.second = valeur;
				return;
			}
		}

		dernieres_valeurs.ajoute({ ptr, valeur });
	};

	auto substitutrice = Substitutrice();
	auto instructions_subtituees = false;

	POUR (instructions) {
		if (it->est_acces_membre()) {
			auto acces = it->comme_acces_membre();

			auto trouve = false;
			for (auto am : acces_membres) {
				if (sont_equivalents(acces, am)) {
					substitutrice.ajoute_substitution(acces, am, SubstitutDans::TOUT);
					trouve = true;
					break;
				}
			}

			if (!trouve) {
				acces_membres.ajoute(acces);
			}
		}
	}

	POUR (instructions) {
		if (it->genre == Instruction::Genre::STOCKE_MEMOIRE) {
			auto stocke = it->comme_stocke_mem();

			stocke->ou = substitutrice.valeur_substituee(stocke->ou);
			stocke->valeur = substitutrice.valeur_substituee(stocke->valeur);
			renseigne_derniere_valeur(stocke->ou, stocke->valeur);

			for (auto dv : dernieres_valeurs) {
				if (sont_equivalents(dv.first, stocke->ou) && dv.first != stocke->ou) {
					renseigne_derniere_valeur(dv.first, stocke->valeur);
					break;
				}
			}
		}
		else if (it->genre == Instruction::Genre::CHARGE_MEMOIRE) {
			auto charge = it->comme_charge();

			for (auto dv : dernieres_valeurs) {
				if (sont_equivalents(dv.first, charge->chargee)) {
					substitutrice.ajoute_substitution(it, dv.second, SubstitutDans::TOUT);
					break;
				}
			}
		}
		else if (it->est_branche_cond()) {
			auto branche = it->comme_branche_cond();
			auto condition = branche->condition;

			if (condition->est_instruction()) {
				auto inst = condition->comme_instruction();
				branche->condition = substitutrice.instruction_substituee(inst);
			}
		}
		else if (it->est_op_binaire() && operandes_sont_constantes(it->comme_op_binaire())) {
			auto op = it->comme_op_binaire();
			substitutrice.ajoute_substitution(it, op->valeur_gauche, SubstitutDans::TOUT);
		}
		else {
			auto nouvelle_inst = substitutrice.instruction_substituee(it);

			if (nouvelle_inst != it) {
				instructions_subtituees = true;
			}

			if (nouvelle_inst->est_op_binaire() && operandes_sont_constantes(nouvelle_inst->comme_op_binaire())) {
				auto op = nouvelle_inst->comme_op_binaire();
				substitutrice.ajoute_substitution(it, op->valeur_gauche, SubstitutDans::TOUT);
			}

			/* si nous avons un appel, il est possible que l'appel modifie l'un
			 * des paramètres, donc réinitialise les substitutions dans le cas
			 * où le programme sauvegarda une valeur avant l'appel */
			if (it->est_appel()) {
				dernieres_valeurs.efface();
				substitutrice.reinitialise();
			}

			it = nouvelle_inst;
		}
	}

	return instructions_subtituees;
}

bool propage_constantes_et_temporaires(dls::tableau<Bloc *> &blocs)
{
	auto constantes_propagees = false;

#ifdef DEBOGUE_PROPAGATION
	std::cerr << "===== avant propagation =====\n";
	imprime_fonction(atome_fonc, std::cerr);
#endif

	POUR (blocs) {
		constantes_propagees |= propage_constantes_et_temporaires(it->instructions);
	}

#ifdef DEBOGUE_PROPAGATION
	std::cerr << "===== après propagation =====\n";
	imprime_fonction(atome_fonc, std::cerr);
#endif

	return constantes_propagees;
}

static void determine_assignations_inutiles(Bloc *bloc)
{
	using paire_atomes = std::pair<Atome *, InstructionStockeMem *>;
	auto anciennes_valeurs = dls::tablet<paire_atomes, 16>();

	auto indique_valeur_chargee = [&](Atome *atome)
	{
		POUR (anciennes_valeurs) {
			if (it.first == atome) {
				it.second = nullptr;
				return;
			}
		}

		anciennes_valeurs.ajoute({ atome, nullptr });
	};

	auto indique_valeur_stockee = [&](Atome *atome, InstructionStockeMem *stocke)
	{
		POUR (anciennes_valeurs) {
			if (it.first == atome) {
				if (it.second == nullptr) {
					it.second = stocke;
				}
				else {
					it.second->nombre_utilisations -= 1;
					it.second = stocke;
				}

				return;
			}
		}

		anciennes_valeurs.ajoute({ atome, stocke });
	};

	POUR (bloc->instructions) {
		if (it->est_stocke_mem()) {
			auto stocke = it->comme_stocke_mem();
			indique_valeur_stockee(stocke->ou, stocke);
		}
		else if (it->est_charge()) {
			auto charge = it->comme_charge();
			indique_valeur_chargee(charge->chargee);
		}
	}
}

static bool supprime_code_mort(kuri::tableau<Instruction *> &instructions)
{
	if (instructions.est_vide()) {
		return false;
	}

	/* rassemble toutes les instructions utilisées au début du tableau d'instructions */
	auto predicat = [](Instruction *inst) { return inst->nombre_utilisations != 0; };
	auto iter = std::stable_partition(instructions.begin(), instructions.end(), predicat);

	if (iter != instructions.end()) {
		/* ne supprime pas la mémoire, nous pourrions en avoir besoin */
		instructions.taille = std::distance(instructions.begin(), iter);
		return true;
	}

	return false;
}

// À FAIRE : vérifie que ceci ne vide pas les fonctions sans retour
bool supprime_code_mort(dls::tableau<Bloc *> &blocs)
{
	POUR (blocs) {
		for (auto inst : it->instructions) {
			inst->nombre_utilisations = 0;
		}
	}

	/* performe deux passes, car les boucles « pour » verraient les incrémentations de
	 * leurs variables supprimées puisque nous ne marquons la variable comme utilisée
	 * que lors de la visite de la condition du bloc après les incrémentations (nous
	 * traversons les intructions en arrière pour que seules les dépendances du retour
	 * soient considérées) */
	POUR (blocs) {
		marque_instructions_utilisees(it->instructions);
	}

	POUR (blocs) {
		marque_instructions_utilisees(it->instructions);
	}

	// détermine quels stockages sont utilisés
	POUR (blocs) {
		determine_assignations_inutiles(it);
	}

	auto code_mort_supprime = false;

	POUR (blocs) {
		if (log_actif) {
			imprime_bloc(it, 0, std::cerr, true);
		}

		code_mort_supprime |= supprime_code_mort(it->instructions);
	}

	return code_mort_supprime;
}

#if 0
struct ApparieAtome {
	Règle
	Variable

	ApparieAtome *operande1;
	ApparieAtome *operande2;
};


(stocke a (ajt (charge a) 1)) -> (inc a)
(stocke a (sst (charge a) 1)) -> (dec a)
(mul a 2) -> (ajt a a)

/* a ^ b ^ a -> b */
(xor a (xor a b)) -> b

/* (a ^ b) | a */
(or (xor a b) a) -> (or a b)


auto apparie_variable_a = ...;
auto apperie_entier = cree_appariement(EST_ENTIER_CONSTANT, 1);
auto apparie_charge = cree_appariement(EST_CHARGE, apparie_variable_a);
auto apparie_ajoute = cree_appariement(EST_AJOUTE, apparie_charge, apparie_entier);
auto apparie_stocke = cree_appariement(EST_STOCKE, apparie_variable_a);

bool apparie(Appariement *appariement, Atome *atome)
{
	switch(appariement->genre) {
		case EST_STOCKE:
		{
			if (!atome->est_stocke()) {
				return false;
			}

			if (!apparie(appariement->operande1, atome->comme_stocke()->ou)) {
				return false;
			}

			if (!apparie(appariement->operande2, atome->comme_stocke()->valeur)) {
				return false;
			}

			break;
		}
		case EST_AJOUTE:
		{
			if (!atome->est_op_ajoute()) {
				return false;
			}

			if (!apparie(appariement->operande1, atome->comme_op_ajoute()->valeur_gauche)) {
				return false;
			}

			if (!apparie(appariement->operande2, atome->comme_op_ajoute()->valeur_droite)) {
				return false;
			}
		}
	}

	return true;
}


struct Appariement {
	Appariement *inst;
	Appariement *operande1;
	Appariement *operande2;

	enum GenreAppariement {
		EST_INSTRUCTION_MUL,
		EST_INSTRUCTION_AJT,

		EST_VARIABLE,
		EST_NOMBRE,
	};

	GenreAppariement genre;

	Appariement *remplacement;
};

static bool apparie(Appariement *appariement, Atome *atome)
{
	switch (appariement->genre) {
		case Appariement::EST_INSTRUCTION_MUL:
		{
			if (atome->est_instruction()) {
				return false;
			}

			auto inst = atome->comme_instruction();

			if (!inst->est_op_binaire()) {
				return false;
			}

			auto op_binaire = inst->comme_op_binaire();

			if (op_binaire->op != OperateurBinaire::Genre::Multiplication) {
				return false;
			}

			if (!apparie(appariement->operande1, op_binaire->valeur_gauche)) {
				return false;
			}

			if (!apparie(appariement->operande2, op_binaire->valeur_droite)) {
				return false;
			}

			return true;
		}
		default:
		{
			return false;
		}
	}
}

// au lieu d'allouer, nous pourrions le faire en-place si possible
static Atome *cree_remplacement(Appariement *remplacement, ConstructriceRI &constructrice)
{
	switch (remplacement->genre) {
		case Appariement::EST_INSTRUCTION_AJT:
		{
			auto valeur_gauche = cree_remplacement(remplacement->operande1, constructrice);
			auto valeur_droite = cree_remplacement(remplacement->operande2, constructrice);

			auto op = constructrice.cree_op_binaire(type, OperateurBinaire::Genre::Addition, valeur_gauche, valeur_droite);
			break;
		}
		case Appariement::EST_VARIABLE:
		{
			// trouve la variable dans le contexte
			break;
		}
		default:
		{
			return nullptr;
		}
	}

	return nullptr;
}

void lance_optimisations(AtomeFonction *fonction)
{
	// (mul a 2) -> (ajt a a)

	auto appariement_mul = Appariement();
	appariement_mul.genre = Appariement::EST_INSTRUCTION_MUL;

	auto appariement_nombre = Appariement();
	appariement_nombre.genre = Appariement::EST_NOMBRE;

	auto appariement_variable = Appariement();
	appariement_variable.genre = Appariement::EST_VARIABLE;

	appariement_mul.operande1 = &appariement_variable;
	appariement_mul.operande2 = &appariement_nombre;

	POUR (fonction->instructions) {
		if (apparie(&appariement_mul, it)) {
			//auto nouvelle_inst = remplace(appariement_mul.remplacement, it);
		}
	}
}
#endif

static bool enleve_blocs_vides(dls::tableau<Bloc *> &blocs)
{
	if (blocs.taille() == 1) {
		return false;
	}

	dls::tableau<Bloc *> nouveau_blocs;
	nouveau_blocs.reserve(blocs.taille());

	/* le premier bloc est celui du corps de la fonction, ne le travaille pas */
	nouveau_blocs.ajoute(blocs[0]);

	for (auto i = 1; i < blocs.taille(); ++i) {
		auto bloc = blocs[i];

		/* cas pour les blocs avec un fonction qui a une expression de retour alors que toutes les branches retournes */
		if (bloc->parents.est_vide() && bloc->enfants.est_vide()) {
			continue;
		}

		if (bloc->instructions.est_vide()) {
			POUR (bloc->parents) {
				it->enleve_enfant(bloc);
			}

			POUR (bloc->enfants) {
				it->enleve_parent(bloc);
			}

			POUR (bloc->parents) {
				for (auto enf : bloc->enfants) {
					it->ajoute_enfant(enf);
				}
			}

			continue;
		}

		nouveau_blocs.ajoute(bloc);
	}

	if (nouveau_blocs.taille() != blocs.taille()) {
		blocs = nouveau_blocs;
		return true;
	}

	return false;
}

static bool elimine_branches_inutiles(dls::tableau<Bloc *> &blocs)
{
	auto branche_eliminee = false;

	for (auto i = 0; i < blocs.taille(); ++i) {
		auto bloc = blocs[i];

		if (bloc->instructions.est_vide()) {
			continue;
		}

		auto inst = bloc->instructions.derniere();

		if (bloc->instructions.taille == 1 && inst->est_branche()) {
			auto enfant = bloc->enfants[0];

			POUR (bloc->enfants) {
				it->enleve_parent(bloc);
			}

			// remplace les enfants dans les parents
			POUR (bloc->parents) {
				it->remplace_enfant(bloc, enfant);
			}

			bloc->instructions.taille = 0;
			branche_eliminee = true;
			continue;
		}

		if (inst->est_branche()) {
			log(std::cerr, "Vérifie si l'on peut fusionner l'enfant du bloc : ", bloc->label->id);
			if (bloc->peut_fusionner_enfant()) {
				auto enfant = bloc->enfants[0];
				bloc->fusionne_enfant(enfant);
				branche_eliminee = true;

				/* regère ce bloc */
				--i;
			}
		}
		else if (inst->est_branche_cond()) {
			auto branche = inst->comme_branche_cond();

			if (branche->label_si_faux == branche->label_si_vrai) {
				auto enfant = bloc->enfants[0];
				bloc->fusionne_enfant(enfant);
				branche_eliminee = true;

				/* regère ce bloc */
				--i;
			}
		}
	}

	return branche_eliminee;
}

static int analyse_blocs(dls::tableau<Bloc *> &blocs)
{
	int drapeaux = 0;

	POUR (blocs) {
		if (it->instructions.est_vide()) {
			drapeaux |= REQUIERS_CORRECTION_BLOCS;
			continue;
		}

		if (it->parents.est_vide() && it->enfants.est_vide() && blocs.taille() != 1) {
			drapeaux |= REQUIERS_CORRECTION_BLOCS;
			continue;
		}

		for (auto inst : it->instructions) {
			if (inst->est_branche_cond()) {
				auto branche_cond = inst->comme_branche_cond();

				if (branche_cond->label_si_vrai == branche_cond->label_si_faux) {
					// condition inutile
					// drapeaux |= REQUIERS_SUPPRESSION_CODE_MORT;
				}
			}
			else if (inst->est_op_binaire()) {
				// vérifie si les opérandes sont égales
			}
		}
	}

	return drapeaux;
}

static Bloc *trouve_bloc_utilisant_variable(dls::tableau<Bloc *> const &blocs, Bloc *sauf_lui, InstructionAllocation *var)
{
	POUR (blocs) {
		if (it == sauf_lui) {
			continue;
		}

		for (auto v : it->variables_utilisees) {
			if (v == var) {
				return it;
			}
		}
	}

	return nullptr;
}

static void detecte_utilisations_variables(dls::tableau<Bloc *> const &blocs)
{
	POUR (blocs) {
		construit_liste_variables_utilisees(it);
	}

	POUR (blocs) {
		for (auto decl : it->variables_declarees) {
			auto utilise = false;

//			log(std::cerr, "Le bloc ", it->label->id, " déclare ", it->variables_declarees.taille(), " variables");
//			log(std::cerr, "Le bloc ", it->label->id, " utilise ", it->variables_utilisees.taille(), " variables");

			for (auto var : it->variables_utilisees) {
				if (var == decl) {
					utilise = true;
					break;
				}
			}

			if (!utilise) {
			//	log(std::cerr, "Le bloc ", it->label->id, " n'utilise pas la variable ", decl->numero);

				if (decl->blocs_utilisants == 1) {
					auto autre_bloc = trouve_bloc_utilisant_variable(blocs, it, decl);
				//	log(std::cerr, "La variable peut être déplacée dans le bloc ", autre_bloc->label->id);

					// supprime alloc du bloc
					auto index = 0;

					for (auto inst: it->instructions) {
						if (inst == decl) {
							break;
						}

						index += 1;
					}

					std::rotate(it->instructions.pointeur + index, it->instructions.pointeur + index + 1, it->instructions.pointeur + it->instructions.taille);
					it->instructions.supprime_dernier();

					// ajoute alloc dans bloc
					autre_bloc->instructions.pousse_front(decl);
					autre_bloc->variables_declarees.ajoute(decl);
				}
			}
			else {
				if (decl->blocs_utilisants <= 1) {
					log(std::cerr, "La variable est une temporaire");
				}
			}
		}
	}
}

static dls::tableau<Bloc *> convertis_en_blocs(ConstructriceRI &constructrice, AtomeFonction *atome_fonc, dls::tableau<Bloc *> &blocs___)
{
	dls::tableau<Bloc *> blocs{};

	Bloc *bloc_courant = nullptr;
	auto numero_instruction = static_cast<int>(atome_fonc->params_entrees.taille);

	POUR (atome_fonc->instructions) {
		it->numero = numero_instruction++;
	}

	POUR (atome_fonc->instructions) {
		if (it->est_label()) {
			bloc_courant = bloc_pour_label(blocs___, it->comme_label());
			blocs.ajoute(bloc_courant);
			continue;
		}

		bloc_courant->instructions.ajoute(it);

		if (it->est_branche()) {
			auto bloc_cible = bloc_pour_label(blocs___, it->comme_branche()->label);
			bloc_courant->ajoute_enfant(bloc_cible);
			continue;
		}

		if (it->est_branche_cond()) {
			auto label_si_vrai = it->comme_branche_cond()->label_si_vrai;
			auto label_si_faux = it->comme_branche_cond()->label_si_faux;

			auto bloc_si_vrai = bloc_pour_label(blocs___, label_si_vrai);
			auto bloc_si_faux = bloc_pour_label(blocs___, label_si_faux);

			bloc_courant->ajoute_enfant(bloc_si_vrai);
			bloc_courant->ajoute_enfant(bloc_si_faux);
			continue;
		}
	}

	/* ajoute des branches implicites pour les blocs qui n'en ont pas,
	 * ceci peut arriver pour les NoeudBlocs des conditions ou des boucles, qui
	 * n'ont pas d'expressions */
	for (auto i = 0; i < blocs.taille() - 1; ++i) {
		auto bloc = blocs[i];

		if (bloc->enfants.est_vide() && (bloc->instructions.est_vide() || !bloc->instructions.derniere()->est_retour())) {
			auto enfant = blocs[i + 1];
			bloc->ajoute_enfant(enfant);
			bloc->instructions.ajoute(constructrice.cree_branche(nullptr, enfant->label, true));
		}
	}

	return blocs;
}

static void performe_passes_optimisation(dls::tableau<Bloc *> &blocs)
{
	while (true) {
		if (log_actif) {
			imprime_blocs(blocs, std::cerr);
		}

//		auto drapeaux = analyse_blocs(blocs);

//		if (drapeaux == 0) {
//			break;
//		}

		int drapeaux = 0;

		auto travail_effectue = false;

		if ((drapeaux & REQUIERS_ENLIGNAGE) == REQUIERS_ENLIGNAGE) {
			// performe enlignage
			drapeaux &= REQUIERS_ENLIGNAGE;
		}

		if ((drapeaux & REQUIERS_DEBOUCLAGE) == REQUIERS_DEBOUCLAGE) {
			// performe débouclage
			drapeaux &= REQUIERS_DEBOUCLAGE;
		}

		//if ((drapeaux & REQUIERS_CORRECTION_BLOCS) == REQUIERS_CORRECTION_BLOCS) {
			travail_effectue |= elimine_branches_inutiles(blocs);
			travail_effectue |= enleve_blocs_vides(blocs);
			//drapeaux &= REQUIERS_CORRECTION_BLOCS;
		//}

			//supprime_temporaires(blocs);

		//if ((drapeaux & REQUIERS_PROPAGATION_CONSTANTES) == REQUIERS_PROPAGATION_CONSTANTES) {
			travail_effectue |= propage_constantes_et_temporaires(blocs);
			//drapeaux &= REQUIERS_PROPAGATION_CONSTANTES;
		//}

		//if ((drapeaux & REQUIERS_SUPPRESSION_CODE_MORT) == REQUIERS_SUPPRESSION_CODE_MORT) {
			travail_effectue |= supprime_code_mort(blocs);
			//drapeaux &= REQUIERS_SUPPRESSION_CODE_MORT;
		//}

		if (!travail_effectue) {
			break;
		}
	}
}

static void transfere_instructions_blocs(dls::tableau<Bloc *> const &blocs, AtomeFonction *atome_fonc)
{
	auto nombre_instructions = 0;
	POUR (blocs) {
		nombre_instructions += 1 + static_cast<int>(it->instructions.taille);
	}

	atome_fonc->instructions.taille = 0;
	atome_fonc->instructions.reserve(nombre_instructions);

	POUR (blocs) {
		atome_fonc->instructions.ajoute(it->label);

		for (auto inst : it->instructions) {
			atome_fonc->instructions.ajoute(inst);
		}
	}
}

void optimise_code(ConstructriceRI &constructrice, AtomeFonction *atome_fonc)
{
	//if (atome_fonc->nom == "_KF9Fondation14imprime_chaine_P0__E2_8contexte19KsContexteProgramme6format8Kschaine4args8KtKseini_S1_8Kschaine") {
	//	std::cerr << "========= optimisation pour " << atome_fonc->nom << " =========\n";
	//	active_log();
	//}

	//while (enligne_fonctions(constructrice, atome_fonc)) {}
	enligne_fonctions(constructrice, atome_fonc);

	dls::tableau<Bloc *> blocs___{};
	auto blocs = convertis_en_blocs(constructrice, atome_fonc, blocs___);

	performe_passes_optimisation(blocs);

	transfere_instructions_blocs(blocs, atome_fonc);

	POUR (blocs___) {
		memoire::deloge("Bloc", it);
	}

	desactive_log();
}

/*
	%0 alloue z32 cible
	%1 alloue z32 variable
	%2 charge z32 %1
	%3 alloue z64 temporaire_pour_transtypage
	%4 transtype (1) %2 vers z64
	%5 stocke *z64 %3, z64 %4
	%6 charge z64 %3
	%7 transtype (4) %6 vers z32
	%8 stocke *z32 %0, z32 %7

	doit devenir

	%0 alloue z32 cible
	%1 alloue z32 variable
	%2 charge z32 %1
	%3 stocke *z32 %2

(multi
	(stocke %a (transtype %b))
	(stocke %cible (transtype (charge %a))
	remplace si type(%b) == type(%cible)
		(stocke %cible %b)
)

((op constante constante) évalue)


si (cond) goto label1; else goto label2;

label1:
	val = 1
	goto label3

label2
	val = 0
	goto label3

label3
	si (val) goto label4; else goto label5;

remplace par

	si (cond) goto label4; else goto label5;


  si %116 alors %121 sinon %124
Bloc 9  [8] [11]
  stocke *bool %91, bool 1
  branche %126
Bloc 10  [0, 7, 8] [11]
  stocke *bool %91, bool 0
  branche %126
Bloc 11  [9, 10] [1, 2]
  charge bool %91
  si %127 alors %138 sinon %143
Bloc 1  [11] []
  retourne z32 1
Bloc 2  [11] []
  retourne z32 0

remplace par

  si %116 alors %138 sinon %143

// -------------- simplifie soustraction

// x - 0 => x
(remplace
	(sst x 0)
	(x)
)

// x - x => 0
(remplace
	(sst x x)
	(0)
)

// x + x - x => x
(remplace
	(sst (ajt x x) x)
	(x)
)

// x * 2 - x => x
(remplace
	(sst (mul x 2) x)
	(x)
)

(remplace
	(sst (mul 2 a) a)
	(x)
)

// x * N - x => x * (N - 1)
(remplace
	(sst (mul x @N) x)
	(mul x %(N - 1))
)

// -------------- simplifie autre

// (x + -1) – y -> ~y + x
// (x - 1) – y -> ~y + x
// (( x | y) & c1 ) | (y & c2) -> x & c1 (si c1 est complément binaire de c2)

*/
