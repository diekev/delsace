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

#include "constructrice_ri.hh"

#include "arbre_syntactic.h"
#include "contexte_generation_code.h"
#include "erreur.h"
#include "outils_lexemes.hh"

ConstructriceRI::ConstructriceRI(ContexteGenerationCode &contexte)
	: m_contexte(contexte)
{
	ident_contexte = m_contexte.table_identifiants.identifiant_pour_chaine("contexte");
}

ConstructriceRI::~ConstructriceRI()
{
#define DELOGE_ATOMES(Type, Tableau) \
	for (auto ptr : Tableau) {\
	memoire::deloge(#Type, ptr); \
}

	DELOGE_ATOMES(AtomeFonction, atomes_fonction);
	DELOGE_ATOMES(AtomeConstante, atomes_constante);
	DELOGE_ATOMES(AtomeGlobale, atomes_globale);
	DELOGE_ATOMES(InstructionAllocation, insts_allocation);
	DELOGE_ATOMES(InstructionAppel, insts_appel);
	DELOGE_ATOMES(InstructionBranche, insts_branche);
	DELOGE_ATOMES(InstructionBrancheCondition, insts_branche_condition);
	DELOGE_ATOMES(InstructionChargeMem, insts_charge_memoire);
	DELOGE_ATOMES(InstructionLabel, insts_label);
	DELOGE_ATOMES(InstructionOpBinaire, insts_opbinaire);
	DELOGE_ATOMES(InstructionOpUnaire, insts_opunaire);
	DELOGE_ATOMES(InstructionStockeMem, insts_stocke_memoire);
	DELOGE_ATOMES(InstructionRetour, insts_retour);
	DELOGE_ATOMES(InstructionAccedeIndex, insts_accede_index);
	DELOGE_ATOMES(InstructionAccedeMembre, insts_accede_membre);
	DELOGE_ATOMES(InstructionTranstype, insts_transtype);

#undef DELOGE_ATOMES
}

void ConstructriceRI::genere_ri()
{
	auto temps_generation = 0.0;

	auto &graphe_dependance = m_contexte.graphe_dependance;
	auto noeud_fonction_principale = graphe_dependance.cherche_noeud_fonction("principale");

	if (noeud_fonction_principale == nullptr) {
		erreur::fonction_principale_manquante();
	}

	traverse_graphe(noeud_fonction_principale);

	m_contexte.temps_generation = temps_generation;
}

void ConstructriceRI::traverse_graphe(NoeudDependance *racine)
{
	racine->fut_visite = true;

	for (auto const &relation : racine->relations) {
		auto accepte = relation.type == TypeRelation::UTILISE_TYPE;
		accepte |= relation.type == TypeRelation::UTILISE_FONCTION;
		accepte |= relation.type == TypeRelation::UTILISE_GLOBALE;

		if (!accepte) {
			continue;
		}

		if (relation.noeud_fin->fut_visite) {
			continue;
		}

		traverse_graphe(relation.noeud_fin);
	}

	if (racine->type == TypeNoeudDependance::TYPE) {
		if (racine->noeud_syntactique != nullptr) {
			//genere_code_C(noeud->noeud_syntactique, constructrice, contexte, false);
		}
	}
	else {
		imprime_arbre(racine->noeud_syntactique, std::cerr, 0);
		genere_ri_pour_noeud(racine->noeud_syntactique);
	}
}

void ConstructriceRI::imprime_programme() const
{
	auto &os = std::cerr;

	POUR (programme) {
		switch (it->genre_atome) {
			case Atome::Genre::CONSTANTE:
			{
				break;
			}
			case Atome::Genre::FONCTION:
			{
				auto atome_fonc = static_cast<AtomeFonction const *>(it);
				os << "fonction " << atome_fonc->nom;

				auto virgule = '(';

				for (auto param : atome_fonc->params_entrees) {
					os << virgule;
					os << chaine_type(param->type) << ' ';
					os << param->ident->nom;
					virgule = ',';
				}

				if (atome_fonc->params_entrees.taille == 0) {
					os << virgule;
				}

				os << ")\n";

				for (auto inst : atome_fonc->instructions) {
					imprime_instruction(static_cast<Instruction const *>(inst), os);
				}

				break;
			}
			case Atome::Genre::INSTRUCTION:
			{
				auto inst = static_cast<Instruction const *>(it);
				imprime_instruction(inst, os);
				break;
			}
			case Atome::Genre::GLOBALE:
			{
				break;
			}
		}

		os << '\n';
	}
}

static void imprime_atome(Atome const *atome, std::ostream &os)
{
	if (atome->genre_atome == Atome::Genre::CONSTANTE) {
		auto atome_constante = static_cast<AtomeConstante const *>(atome);

		switch (atome_constante->valeur.genre) {
			case AtomeConstante::Valeur::Genre::BOOLEENNE:
			{
				os << atome_constante->valeur.valeur_booleenne;
				break;
			}
			case AtomeConstante::Valeur::Genre::ENTIERE:
			{
				os << atome_constante->valeur.valeur_entiere;
				break;
			}
			case AtomeConstante::Valeur::Genre::REELLE:
			{
				os << atome_constante->valeur.valeur_reelle;
				break;
			}
			case AtomeConstante::Valeur::Genre::NULLE:
			{
				os << "nul";
				break;
			}
			case AtomeConstante::Valeur::Genre::CARACTERE:
			{
				os << atome_constante->valeur.valeur_reelle;
				break;
			}
			case AtomeConstante::Valeur::Genre::CHAINE:
			{
				os << "\"";

				for (auto i = 0; i < atome_constante->valeur.valeur_chaine.taille; ++i) {
					os << atome_constante->valeur.valeur_chaine.pointeur[i];
				}

				os << '"';

				break;
			}
			case AtomeConstante::Valeur::Genre::INDEFINIE:
			{
				os << "indéfinie";
				break;
			}
			case AtomeConstante::Valeur::Genre::STRUCTURE:
			{
				os << "à faire : structure";
				break;
			}
		}
	}
	else if (atome->genre_atome == Atome::Genre::FONCTION) {
		auto atome_fonction = static_cast<AtomeFonction const *>(atome);
		os << atome_fonction->nom;
	}
	else {
		auto inst_valeur = static_cast<Instruction const *>(atome);
		os << "%" << inst_valeur->numero;
	}
}

void ConstructriceRI::imprime_instruction(Instruction const *inst, std::ostream &os) const
{
	auto nombre_chiffres = [](long nombre)
	{
		if (nombre == 0) {
			return 1;
		}

		auto compte = 0;

		while (nombre > 0) {
			nombre /= 10;
			compte += 1;
		}

		return compte;
	};

	auto nombre_zero_avant_numero = nombre_chiffres(nombre_instructions) - nombre_chiffres(inst->numero);

	for (auto i = 0; i < nombre_zero_avant_numero; ++i) {
		os << ' ';
	}

	os << "%" << inst->numero << ' ';

	switch (inst->genre) {
		case Instruction::Genre::INVALIDE:
		{
			os << "  invalide\n";
			break;
		}
		case Instruction::Genre::ALLOCATION:
		{
			os << "  alloue " << chaine_type(inst->type) << ' ' << inst->ident->nom << '\n';
			break;
		}
		case Instruction::Genre::APPEL:
		{
			auto inst_appel = static_cast<InstructionAppel const *>(inst);
			os << "  appel " << chaine_type(inst_appel->type) << ' ';
			imprime_atome(inst_appel->appele, os);

			auto virgule = '(';

			POUR (inst_appel->args) {
				os << virgule;
				imprime_atome(it, os);
				virgule = ',';
			}

			if (inst_appel->args.taille == 0) {
				os << virgule;
			}

			os << ")\n";

			break;
		}
		case Instruction::Genre::BRANCHE:
		{
			auto inst_branche = static_cast<InstructionBranche const *>(inst);
			os << "  branche %" << inst_branche->label->numero << "\n";
			break;
		}
		case Instruction::Genre::BRANCHE_CONDITION:
		{
			auto inst_branche = static_cast<InstructionBrancheCondition const *>(inst);
			os << "  si ";
			imprime_atome(inst_branche->condition, os);
			os  << " alors %" << inst_branche->label_si_vrai->numero
			   << " sinon %" << inst_branche->label_si_faux->numero
			   << '\n';
			break;
		}
		case Instruction::Genre::CHARGE_MEMOIRE:
		{
			os << "  charge\n";
			break;
		}
		case Instruction::Genre::STOCKE_MEMOIRE:
		{
			auto inst_stocke = static_cast<InstructionStockeMem const *>(inst);
			os << "  stocke " << chaine_type(inst->type) << " %" << inst_stocke->ou->numero << ", ";
			imprime_atome(inst_stocke->valeur, os);
			os << '\n';
			break;
		}
		case Instruction::Genre::LABEL:
		{
			auto inst_label = static_cast<InstructionLabel const *>(inst);
			os << "label " << inst_label->id << '\n';
			break;
		}
		case Instruction::Genre::OPERATION_UNAIRE:
		{
			auto inst_un = static_cast<InstructionOpUnaire const *>(inst);
			os << "  " << chaine_pour_genre_op(inst_un->op)
			   << ' ' << chaine_type(inst_un->type);
			imprime_atome(inst_un->valeur, os);
			os << '\n';
			break;
		}
		case Instruction::Genre::OPERATION_BINAIRE:
		{
			auto inst_bin = static_cast<InstructionOpBinaire const *>(inst);
			os << "  " << chaine_pour_genre_op(inst_bin->op)
			   << ' ' << chaine_type(inst_bin->type) << ' ';
			imprime_atome(inst_bin->valeur_gauche, os);
			os << ", ";
			imprime_atome(inst_bin->valeur_droite, os);
			os << '\n';
			break;
		}
		case Instruction::Genre::RETOUR:
		{
			auto inst_retour = static_cast<InstructionRetour const *>(inst);
			os << "  retourne ";
			if (inst_retour->valeur != nullptr) {
				auto atome = inst_retour->valeur;
				os << chaine_type(atome->type);
				os << ' ';

				imprime_atome(atome, os);
			}
			os << '\n';
			break;
		}
		case Instruction::Genre::ACCEDE_INDEX:
		{
			auto inst_acces = static_cast<InstructionAccedeIndex const *>(inst);
			os << "  index " << chaine_type(inst_acces->type) << ' ';
			imprime_atome(inst_acces->accede, os);
			os << " ,";
			imprime_atome(inst_acces->index, os);
			os << '\n';
			break;
		}
		case Instruction::Genre::ACCEDE_MEMBRE:
		{
			auto inst_acces = static_cast<InstructionAccedeMembre const *>(inst);
			os << "  membre " << chaine_type(inst_acces->type) << ' ';
			imprime_atome(inst_acces->accede, os);
			os << " ,";
			imprime_atome(inst_acces->index, os);
			os << '\n';
			break;
		}
		case Instruction::Genre::TRANSTYPE:
		{
			auto inst_transtype = static_cast<InstructionTranstype const *>(inst);
			os << "  transtype ";
			imprime_atome(inst_transtype->valeur, os);
			os << " vers " << chaine_type(inst_transtype->type) << '\n';
			break;
		}
	}
}

size_t ConstructriceRI::memoire_utilisee() const
{
	auto memoire = 0ul;

#define COMPTE_MEMOIRE(Type, Tableau) \
	memoire += static_cast<size_t>(Tableau.taille) * (sizeof(Type *) + sizeof(Type))

	COMPTE_MEMOIRE(AtomeFonction, atomes_fonction);
	COMPTE_MEMOIRE(AtomeConstante, atomes_constante);
	COMPTE_MEMOIRE(AtomeGlobale, atomes_globale);
	COMPTE_MEMOIRE(InstructionAllocation, insts_allocation);
	COMPTE_MEMOIRE(InstructionAppel, insts_appel);
	COMPTE_MEMOIRE(InstructionBranche, insts_branche);
	COMPTE_MEMOIRE(InstructionBrancheCondition, insts_branche_condition);
	COMPTE_MEMOIRE(InstructionChargeMem, insts_charge_memoire);
	COMPTE_MEMOIRE(InstructionLabel, insts_label);
	COMPTE_MEMOIRE(InstructionOpBinaire, insts_opbinaire);
	COMPTE_MEMOIRE(InstructionOpUnaire, insts_opunaire);
	COMPTE_MEMOIRE(InstructionStockeMem, insts_stocke_memoire);
	COMPTE_MEMOIRE(InstructionRetour, insts_retour);
	COMPTE_MEMOIRE(InstructionAccedeIndex, insts_accede_index);
	COMPTE_MEMOIRE(InstructionAccedeMembre, insts_accede_membre);
	COMPTE_MEMOIRE(InstructionTranstype, insts_transtype);

#undef COMPTE_MEMOIRE

	return memoire;
}

AtomeFonction *ConstructriceRI::cree_fonction(dls::chaine const &nom, kuri::tableau<Atome *> &&params)
{
	auto atome = AtomeFonction::cree(nom, std::move(params));
	atomes_fonction.pousse(atome);
	programme.pousse(atome);
	return atome;
}

InstructionAllocation *ConstructriceRI::cree_allocation(Type *type, IdentifiantCode *ident)
{
	auto inst = InstructionAllocation::cree(type, ident);
	inst->numero = nombre_instructions++;

	// nous utilisons pour l'instant cree_allocation pour les paramètres des fonctions
	if (fonction_courante) {
		fonction_courante->instructions.pousse(inst);
	}

	insts_allocation.pousse(inst);

	return inst;
}

AtomeConstante *ConstructriceRI::cree_constante_entiere(Type *type, unsigned long long valeur)
{
	auto atome = AtomeConstante::cree(type, valeur);
	atomes_constante.pousse(atome);
	return atome;
}

AtomeConstante *ConstructriceRI::cree_constante_reelle(Type *type, double valeur)
{
	auto atome = AtomeConstante::cree(type, valeur);
	atomes_constante.pousse(atome);
	return atome;
}

AtomeConstante *ConstructriceRI::cree_constante_structure(Type *type, kuri::tableau<AtomeConstante *> &&valeurs)
{
	auto atome = AtomeConstante::cree(type, std::move(valeurs));
	atomes_constante.pousse(atome);
	return atome;
}

AtomeGlobale *ConstructriceRI::cree_globale(Type *type, AtomeConstante *initialisateur)
{
	auto atome = AtomeGlobale::cree(type, initialisateur);
	atomes_globale.pousse(atome);
	return atome;
}

AtomeConstante *ConstructriceRI::cree_constante_booleenne(Type *type, bool valeur)
{
	auto atome = AtomeConstante::cree(type, valeur);
	atomes_constante.pousse(atome);
	return atome;
}

AtomeConstante *ConstructriceRI::cree_constante_caractere(Type *type, unsigned long long valeur)
{
	auto atome = AtomeConstante::cree(type, valeur);
	atome->valeur.genre = AtomeConstante::Valeur::Genre::CARACTERE;
	atomes_constante.pousse(atome);
	return atome;
}

AtomeConstante *ConstructriceRI::cree_constante_nulle(Type *type)
{
	auto atome = AtomeConstante::cree(type);
	atomes_constante.pousse(atome);
	return atome;
}

AtomeConstante *ConstructriceRI::cree_constante_chaine(Type *type, kuri::chaine const &chaine)
{
	auto atome = AtomeConstante::cree(type, chaine);
	atomes_constante.pousse(atome);
	return atome;
}

InstructionBranche *ConstructriceRI::cree_branche(InstructionLabel *label)
{
	auto inst = InstructionBranche::cree(label);
	inst->numero = nombre_instructions++;
	insts_branche.pousse(inst);
	fonction_courante->instructions.pousse(inst);
	return inst;
}

InstructionBrancheCondition *ConstructriceRI::cree_branche_condition(Atome *valeur, InstructionLabel *label_si_vrai, InstructionLabel *label_si_faux)
{
	auto inst = InstructionBrancheCondition::cree(valeur, label_si_vrai, label_si_faux);
	inst->numero = nombre_instructions++;
	insts_branche_condition.pousse(inst);
	fonction_courante->instructions.pousse(inst);
	return inst;
}

InstructionLabel *ConstructriceRI::cree_label()
{
	auto inst = InstructionLabel::cree(nombre_labels++);
	inst->numero = nombre_instructions++;
	insts_label.pousse(inst);
	fonction_courante->instructions.pousse(inst);
	return inst;
}

InstructionLabel *ConstructriceRI::reserve_label()
{
	auto inst = InstructionLabel::cree(nombre_labels++);
	insts_label.pousse(inst);
	return inst;
}

void ConstructriceRI::insere_label(InstructionLabel *label)
{
	label->numero = nombre_instructions++;
	fonction_courante->instructions.pousse(label);
}

InstructionRetour *ConstructriceRI::cree_retour(Atome *valeur)
{
	auto inst = InstructionRetour::cree(valeur);
	inst->numero = nombre_instructions++;
	insts_retour.pousse(inst);
	fonction_courante->instructions.pousse(inst);
	return inst;
}

InstructionStockeMem *ConstructriceRI::cree_stocke_mem(Type *type, Instruction *ou, Atome *valeur)
{
	auto inst = InstructionStockeMem::cree(type, ou, valeur);
	inst->numero = nombre_instructions++;
	fonction_courante->instructions.pousse(inst);
	insts_stocke_memoire.pousse(inst);
	return inst;
}

InstructionChargeMem *ConstructriceRI::cree_charge_mem(Type *type, Atome *ou)
{
	assert(ou->genre_atome == Atome::Genre::INSTRUCTION);
	auto inst_chargee = static_cast<Instruction *>(ou);
	assert(inst_chargee->genre == Instruction::Genre::ALLOCATION);

	auto inst = InstructionChargeMem::cree(type, inst_chargee);
	inst->numero = nombre_instructions++;
	fonction_courante->instructions.pousse(inst);
	insts_charge_memoire.pousse(inst);
	return inst;
}

InstructionAppel *ConstructriceRI::cree_appel(Type *type, Atome *appele, kuri::tableau<Atome *> &&args)
{
	auto inst = InstructionAppel::cree(type, appele, std::move(args));
	inst->numero = nombre_instructions++;
	fonction_courante->instructions.pousse(inst);
	insts_appel.pousse(inst);
	return inst;
}

InstructionOpUnaire *ConstructriceRI::cree_op_unaire(Type *type, OperateurUnaire::Genre op, Atome *valeur)
{
	auto inst = InstructionOpUnaire::cree(type, op, valeur);
	inst->numero = nombre_instructions++;
	fonction_courante->instructions.pousse(inst);
	insts_opunaire.pousse(inst);
	return inst;
}

InstructionOpBinaire *ConstructriceRI::cree_op_binaire(Type *type, OperateurBinaire::Genre op, Atome *valeur_gauche, Atome *valeur_droite)
{
	auto inst = InstructionOpBinaire::cree(type, op, valeur_gauche, valeur_droite);
	inst->numero = nombre_instructions++;
	fonction_courante->instructions.pousse(inst);
	insts_opbinaire.pousse(inst);
	return inst;
}

InstructionAccedeIndex *ConstructriceRI::cree_acces_index(Type *type, Atome *accede, Atome *index)
{
	auto inst = InstructionAccedeIndex::cree(type, accede, index);
	inst->numero = nombre_instructions++;
	fonction_courante->instructions.pousse(inst);
	insts_accede_index.pousse(inst);
	return inst;
}

InstructionAccedeMembre *ConstructriceRI::cree_acces_membre(Type *type, Atome *accede, Atome *index)
{
	auto inst = InstructionAccedeMembre::cree(type, accede, index);
	inst->numero = nombre_instructions++;
	fonction_courante->instructions.pousse(inst);
	insts_accede_membre.pousse(inst);
	return inst;
}

InstructionTranstype *ConstructriceRI::cree_transtype(Type *type, Atome *valeur)
{
	auto inst = InstructionTranstype::cree(type, valeur);
	inst->numero = nombre_instructions++;
	fonction_courante->instructions.pousse(inst);
	insts_transtype.pousse(inst);
	return inst;
}

void ConstructriceRI::empile_controle_boucle(IdentifiantCode *ident, InstructionLabel *label_continue, InstructionLabel *label_arrete)
{
	insts_continue_arrete.pousse({ ident, label_continue, label_arrete });
}

void ConstructriceRI::depile_controle_boucle()
{
	insts_continue_arrete.pop_back();
}

// case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES
// case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE
// case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU
// case GenreNoeud::EXPRESSION_INFO_DE
Atome *ConstructriceRI::genere_ri_pour_noeud(NoeudExpression *noeud)
{
	switch (noeud->genre) {
		case GenreNoeud::DECLARATION_ENUM:
		case GenreNoeud::DIRECTIVE_EXECUTION:
		case GenreNoeud::EXPRESSION_PLAGE:
		case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
		case GenreNoeud::INSTRUCTION_SINON:
		{
			return nullptr;
		}
		case GenreNoeud::DECLARATION_OPERATEUR:
		case GenreNoeud::DECLARATION_FONCTION:
		{
			auto decl = static_cast<NoeudDeclarationFonction *>(noeud);

			auto params = kuri::tableau<Atome *>();
			params.reserve(decl->params.taille);

			POUR (decl->params) {
				auto atome = cree_allocation(it->type, it->ident);
				table_locales.insere({ it->ident, atome });
				params.pousse(atome);
			}

			auto atome_fonc = cree_fonction(decl->nom_broye, std::move(params));

			table_fonctions.insere({ decl->nom_broye, atome_fonc });

			if (decl->est_externe) {
				return atome_fonc;
			}

			fonction_courante = atome_fonc;

			cree_label();

			genere_ri_pour_noeud(decl->bloc);

			fonction_courante = nullptr;

			return atome_fonc;
		}
		case GenreNoeud::DECLARATION_COROUTINE:
		{
			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			auto noeud_bloc = static_cast<NoeudBloc *>(noeud);

			// À FAIRE : blocs différés

			POUR (noeud_bloc->expressions) {
				genere_ri_pour_noeud(it);
			}

			return nullptr;
		}
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			auto expr_appel = static_cast<NoeudExpressionAppel *>(noeud);

			auto args = kuri::tableau<Atome *>();
			args.reserve(expr_appel->exprs.taille);

			POUR (expr_appel->exprs) {
				auto atome = genere_ri_transformee_pour_noeud(it);
				args.pousse(atome);
			}

			auto atome_fonc = static_cast<Atome *>(nullptr);

			if (expr_appel->aide_generation_code == APPEL_POINTEUR_FONCTION) {
				atome_fonc = genere_ri_pour_expression_droite(expr_appel->appelee);
			}
			else {
				auto decl = static_cast<NoeudDeclarationFonction const *>(expr_appel->noeud_fonction_appelee);
				atome_fonc = table_fonctions[decl->nom_broye];
			}

			return cree_appel(expr_appel->type, atome_fonc, std::move(args));
		}
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		{
			// À FAIRE : globales, pointeurs fonctions
			return table_locales[noeud->ident];
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		{
			auto noeud_bin = static_cast<NoeudExpressionMembre *>(noeud);
			return genere_ri_pour_acces_membre(noeud_bin);
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			auto noeud_bin = static_cast<NoeudExpressionMembre *>(noeud);
			return genere_ri_pour_acces_membre_union(noeud_bin);
		}
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		{
			auto expr_ass = static_cast<NoeudExpressionBinaire *>(noeud);

			auto pointeur = genere_ri_pour_noeud(expr_ass->expr1);
			auto valeur = genere_ri_transformee_pour_noeud(expr_ass->expr2);
			cree_stocke_mem(expr_ass->expr1->type, static_cast<Instruction *>(pointeur), valeur);
			return nullptr;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			auto decl = static_cast<NoeudDeclarationVariable *>(noeud);

			if (fonction_courante == nullptr) {
				auto valeur = static_cast<AtomeConstante *>(nullptr);

				if (decl->expression) {
					auto valeur_expr = genere_ri_pour_noeud(decl->expression);
					assert(valeur_expr->genre_atome == Atome::Genre::GLOBALE || valeur_expr->genre_atome == Atome::Genre::CONSTANTE);
					valeur = static_cast<AtomeConstante *>(valeur_expr);
				}
				else {
					valeur = genere_initialisation_defaut_pour_type(noeud->type);
				}

				auto atome = cree_globale(noeud->type, valeur);

				table_globales.insere({ noeud->ident, atome });

				return atome;
			}

			auto pointeur = cree_allocation(noeud->type, noeud->ident);

			if (decl->expression) {
				auto valeur = genere_ri_transformee_pour_noeud(decl->expression);
				cree_stocke_mem(pointeur->type, pointeur, valeur);
			}
			else {
				if (noeud->type->genre == GenreType::TABLEAU_FIXE) {
					// À FAIRE : valeur défaut pour tableau fixe
				}
				else if (noeud->type->genre == GenreType::STRUCTURE) {
					// À FAIRE : appel fonction initialisation
				}
				else {
					auto valeur = genere_initialisation_defaut_pour_type(noeud->type);
					cree_stocke_mem(pointeur->type, pointeur, valeur);
				}
			}

			table_locales.insere({ noeud->ident, pointeur });

			return pointeur;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		{
			return cree_constante_reelle(noeud->type, noeud->lexeme->valeur_reelle);
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		{
			return cree_constante_entiere(noeud->type, noeud->lexeme->valeur_entiere);
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		{
			auto chaine = kuri::chaine();
			chaine.pointeur = noeud->lexeme->pointeur;
			chaine.taille = noeud->lexeme->taille;
			return cree_constante_chaine(noeud->type, chaine);
		}
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		{
			return cree_constante_booleenne(noeud->type, noeud->lexeme->chaine == "vrai");
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		{
			return cree_constante_caractere(noeud->type, 0);
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		{
			return cree_constante_nulle(noeud->type);
		}
		case GenreNoeud::OPERATEUR_BINAIRE:
		{
			auto expr_bin = static_cast<NoeudExpressionBinaire *>(noeud);

			auto traduit_operation_binaire = [&](OperateurBinaire const *op, Atome *valeur_gauche, Atome *valeur_droite) -> Atome*
			{
				// À FAIRE : arithmétique de pointeur, opérateurs logiques
				if (op->est_basique) {
					return cree_op_binaire(noeud->type, op->genre, valeur_gauche, valeur_droite);
				}

				// À FAIRE : contexte
				auto atome_fonction = table_fonctions[op->decl->nom_broye];
				auto args = kuri::tableau<Atome *>(2);
				args.pousse(valeur_gauche);
				args.pousse(valeur_droite);

				return cree_appel(expr_bin->type, atome_fonction, std::move(args));
			};

			if ((expr_bin->drapeaux & EST_ASSIGNATION_COMPOSEE) != 0) {
				auto pointeur = genere_ri_pour_noeud(expr_bin->expr1);
				auto valeur_gauche = cree_charge_mem(expr_bin->expr1->type, pointeur);
				auto valeur_droite = genere_ri_transformee_pour_noeud(expr_bin->expr2);

				auto valeur = traduit_operation_binaire(expr_bin->op, valeur_gauche, valeur_droite);
				return cree_stocke_mem(expr_bin->expr1->type, static_cast<Instruction *>(pointeur), valeur);
			}

			auto valeur_gauche = genere_ri_transformee_pour_noeud(expr_bin->expr1);
			auto valeur_droite = genere_ri_transformee_pour_noeud(expr_bin->expr2);
			return traduit_operation_binaire(expr_bin->op, valeur_gauche, valeur_droite);
		}
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		{
			return genere_ri_pour_comparaison_chainee(noeud);
		}
		case GenreNoeud::EXPRESSION_INDEXAGE:
		{
			auto expr_bin = static_cast<NoeudExpressionBinaire *>(noeud);
			auto type_gauche = expr_bin->expr1->type;
			auto pointeur = genere_ri_pour_noeud(expr_bin->expr1);
			auto valeur = genere_ri_transformee_pour_noeud(expr_bin->expr2);

			if (type_gauche->genre == GenreType::POINTEUR) {
				pointeur = cree_charge_mem(type_gauche, pointeur);
				return cree_acces_index(expr_bin->type, pointeur, valeur);
			}

			// À FAIRE : protection accès hors limite

			if (type_gauche->genre == GenreType::TABLEAU_FIXE) {
				return cree_acces_index(expr_bin->type, pointeur, valeur);
			}

			if (type_gauche->genre == GenreType::TABLEAU_DYNAMIQUE) {
				auto type_tableau = static_cast<TypeTableauDynamique *>(type_gauche);
				pointeur = cree_acces_membre(m_contexte.typeuse.type_pointeur_pour(type_tableau->type_pointe), pointeur, cree_constante_entiere(m_contexte.typeuse[TypeBase::Z64], 0));
				return cree_acces_index(expr_bin->type, pointeur, valeur);
			}

			if (type_gauche->genre == GenreType::CHAINE) {
				pointeur = cree_acces_membre(m_contexte.typeuse[TypeBase::PTR_Z8], pointeur, cree_constante_entiere(m_contexte.typeuse[TypeBase::Z64], 0));
				return cree_acces_index(expr_bin->type, pointeur, valeur);
			}

			return nullptr;
		}
		case GenreNoeud::OPERATEUR_UNAIRE:
		{
			auto expr_un = static_cast<NoeudExpressionUnaire *>(noeud);

			if (expr_un->op->est_basique) {
				auto valeur = genere_ri_transformee_pour_noeud(expr_un->expr);
				return cree_op_unaire(expr_un->type, expr_un->op->genre, valeur);
			}

			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_RETIENS:
		{
			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_RETOUR:
		{
			return cree_retour(nullptr);
		}
		case GenreNoeud::INSTRUCTION_RETOUR_SIMPLE:
		{
			auto inst_retour = static_cast<NoeudExpressionUnaire *>(noeud);
			auto valeur = genere_ri_transformee_pour_noeud(inst_retour->expr);
			return cree_retour(valeur);
		}
		case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:
		{
			return cree_retour(nullptr);
		}
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		{
			auto inst_si = static_cast<NoeudSi *>(noeud);

			auto valeur_condition = genere_ri_pour_noeud(inst_si->condition);
			auto label_si_vrai = reserve_label();
			auto label_si_faux = reserve_label();

			if (noeud->genre == GenreNoeud::INSTRUCTION_SAUFSI) {
				std::swap(label_si_faux, label_si_vrai);
			}

			cree_branche_condition(valeur_condition, label_si_vrai, label_si_faux);

			insere_label(label_si_vrai);
			genere_ri_pour_noeud(inst_si->bloc_si_vrai);
			// bug

			insere_label(label_si_faux);
			if (inst_si->bloc_si_faux) {
				genere_ri_pour_noeud(inst_si->bloc_si_faux);
			}

			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_POUR:
		{
			auto noeud_pour = static_cast<NoeudPour *>(noeud);
			return genere_ri_pour_boucle_pour(noeud_pour);
		}
		case GenreNoeud::INSTRUCTION_BOUCLE:
		{
			auto inst_boucle = static_cast<NoeudBoucle *>(noeud);
			auto label_boucle = cree_label();
			auto label_apres_boucle = reserve_label();

			empile_controle_boucle(nullptr, label_boucle, label_apres_boucle);

			genere_ri_pour_noeud(inst_boucle->bloc);
			cree_branche(label_boucle);
			insere_label(label_apres_boucle);

			depile_controle_boucle();

			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_REPETE:
		{
			auto inst_boucle = static_cast<NoeudBoucle *>(noeud);
			auto label_boucle = cree_label();
			auto label_apres_boucle = reserve_label();

			empile_controle_boucle(nullptr, label_boucle, label_apres_boucle);

			genere_ri_pour_noeud(inst_boucle->bloc);
			auto condition = genere_ri_pour_noeud(inst_boucle->condition);
			cree_branche_condition(condition, label_boucle, label_apres_boucle);
			insere_label(label_apres_boucle);

			depile_controle_boucle();

			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_TANTQUE:
		{
			auto inst_boucle = static_cast<NoeudBoucle *>(noeud);
			auto label_boucle = reserve_label();
			auto label_condition = reserve_label();
			auto label_apres_boucle = reserve_label();

			empile_controle_boucle(nullptr, label_condition, label_apres_boucle);

			insere_label(label_condition);
			auto condition = genere_ri_pour_noeud(inst_boucle->condition);
			cree_branche_condition(condition, label_boucle, label_apres_boucle);

			insere_label(label_boucle);
			genere_ri_pour_noeud(inst_boucle->bloc);
			cree_branche(label_condition);
			insere_label(label_apres_boucle);

			depile_controle_boucle();

			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(noeud);
			auto label = static_cast<InstructionLabel *>(nullptr);

			if (inst->expr == nullptr) {
				if (inst->lexeme->genre == GenreLexeme::CONTINUE) {
					label = insts_continue_arrete.back().t1;
				}
				else {
					label = insts_continue_arrete.back().t2;
				}
			}
			else {
				auto ident = inst->expr->ident;

				POUR (insts_continue_arrete) {
					if (it.t0 != ident) {
						continue;
					}

					if (inst->lexeme->genre == GenreLexeme::CONTINUE) {
						label = it.t1;
					}
					else {
						label = it.t2;
					}

					break;
				}
			}

			return cree_branche(label);
		}
		case GenreNoeud::EXPRESSION_COMME:
		{
			auto inst = static_cast<NoeudExpressionBinaire *>(noeud);
			auto enfant = inst->expr1;
			auto const &type_de = enfant->type;

			auto valeur_enfant = genere_ri_pour_noeud(enfant);

			if (type_de == inst->type) {
				return valeur_enfant;
			}

			return cree_transtype(inst->type, valeur_enfant);
		}
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		{
			auto type = std::any_cast<Type *>(noeud->valeur_calculee);
			return cree_constante_entiere(noeud->type, type->taille_octet);
		}
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		{
			return nullptr;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		{
			return nullptr;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		{
			return nullptr;
		}
		case GenreNoeud::EXPRESSION_INFO_DE:
		{
			// À FAIRE : globales
			return nullptr;
		}
		case GenreNoeud::EXPRESSION_MEMOIRE:
		{
			auto inst_mem = static_cast<NoeudExpressionUnaire *>(noeud);
			auto valeur = genere_ri_pour_noeud(inst_mem->expr);
			return cree_charge_mem(inst_mem->type, valeur);
		}
		case GenreNoeud::EXPRESSION_LOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(noeud);
			return genere_ri_pour_logement(expr->type, 0, expr, expr->expr, expr->expr_chaine, expr->bloc);
		}
		case GenreNoeud::EXPRESSION_DELOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(noeud);
			return genere_ri_pour_logement(expr->expr->type, 2, expr->expr, expr->expr, expr->expr_chaine, expr->bloc);
		}
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(noeud);
			return genere_ri_pour_logement(expr->expr->type, 1, expr, expr->expr, expr->expr_chaine, expr->bloc);
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			auto noeud_struct = static_cast<NoeudStruct *>(noeud);
			return genere_ri_pour_declaration_structure(noeud_struct);
		}
		case GenreNoeud::INSTRUCTION_DISCR:
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		{
			auto noeud_discr = static_cast<NoeudDiscr *>(noeud);
			return genere_ri_pour_discr(noeud_discr);
		}
		case GenreNoeud::EXPRESSION_PARENTHESE:
		{
			return genere_ri_transformee_pour_noeud(static_cast<NoeudExpressionParenthese *>(noeud)->expr);
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			auto noeud_pc = static_cast<NoeudPousseContexte *>(noeud);
			auto atome_nouveau_contexte = genere_ri_pour_noeud(noeud_pc->expr);
			auto atome_ancien_contexte = table_locales[ident_contexte];

			table_locales[ident_contexte] = atome_nouveau_contexte;
			genere_ri_pour_noeud(noeud_pc->bloc);
			table_locales[ident_contexte] = atome_ancien_contexte;

			return nullptr;
		}
		case GenreNoeud::EXPANSION_VARIADIQUE:
		{
			return genere_ri_pour_noeud(static_cast<NoeudExpressionUnaire *>(noeud)->expr);
		}
		case GenreNoeud::INSTRUCTION_TENTE:
		{
			auto noeud_tente = static_cast<NoeudTente *>(noeud);
			return genere_ri_pour_tente(noeud_tente);
		}
	}

	return nullptr;
}

Atome *ConstructriceRI::genere_ri_pour_expression_droite(NoeudExpression *noeud)
{
	auto atome = genere_ri_pour_noeud(noeud);
	return cree_charge_mem(noeud->type, atome);
}

Atome *ConstructriceRI::genere_ri_transformee_pour_noeud(NoeudExpression *noeud)
{
	auto &transformation = noeud->transformation;
	auto valeur = genere_ri_pour_noeud(noeud);

	switch (transformation.type) {
		case TypeTransformation::INUTILE:
		case TypeTransformation::IMPOSSIBLE:
		{
			break;
		}
		case TypeTransformation::CONVERTI_ENTIER_CONSTANT:
		{
			// valeur est déjà une constante, change simplement le type
			valeur->type = transformation.type_cible;
			break;
		}
		case TypeTransformation::CONSTRUIT_UNION:
		{
			auto alloc = cree_allocation(transformation.type_cible, nullptr);
			auto index = cree_constante_entiere(m_contexte.typeuse[TypeBase::Z64], 0);
			auto acces_membre = cree_acces_membre(noeud->type, alloc, index);
			cree_stocke_mem(noeud->type, acces_membre, valeur);
			index = cree_constante_entiere(m_contexte.typeuse[TypeBase::Z64], 1);
			acces_membre = cree_acces_membre(m_contexte.typeuse[TypeBase::Z32], alloc, index);
			index = cree_constante_entiere(m_contexte.typeuse[TypeBase::Z32], static_cast<unsigned long>(transformation.index_membre));
			cree_stocke_mem(m_contexte.typeuse[TypeBase::Z32], acces_membre, index);
			break;
		}
		case TypeTransformation::EXTRAIT_UNION:
		{
			break;
		}
		case TypeTransformation::CONVERTI_VERS_PTR_RIEN:
		{
			valeur = cree_transtype(m_contexte.typeuse[TypeBase::PTR_RIEN], valeur);
			break;
		}
		case TypeTransformation::CONVERTI_VERS_TYPE_CIBLE:
		{
			valeur = cree_transtype(transformation.type_cible, valeur);
			break;
		}
		case TypeTransformation::AUGMENTE_TAILLE_TYPE:
		{
			// À FAIRE : granuralise les expressions de transtypage
			valeur = cree_transtype(transformation.type_cible, valeur);
			break;
		}
		case TypeTransformation::CONSTRUIT_EINI:
		{
			break;
		}
		case TypeTransformation::EXTRAIT_EINI:
		{
			break;
		}
		case TypeTransformation::CONSTRUIT_TABL_OCTET:
		{
			break;
		}
		case TypeTransformation::CONVERTI_TABLEAU:
		{
			break;
		}
		case TypeTransformation::FONCTION:
		{
			auto iter_atome_fonction = table_fonctions.trouve(transformation.nom_fonction);
			auto atome_fonction = iter_atome_fonction->second;

			// À FAIRE : contexte
			auto args = kuri::tableau<Atome *>();
			args.pousse(valeur);

			valeur = cree_appel(atome_fonction->type, atome_fonction, std::move(args));
			break;
		}
		case TypeTransformation::PREND_REFERENCE:
		{
			break;
		}
		case TypeTransformation::DEREFERENCE:
		{
			valeur = cree_charge_mem(m_contexte.typeuse.type_dereference_pour(noeud->type), valeur);
			break;
		}
		case TypeTransformation::CONVERTI_VERS_BASE:
		{
			// À FAIRE : décalage dans la structure
			valeur = cree_transtype(transformation.type_cible, valeur);
			break;
		}
	}

	return valeur;
}

Atome *ConstructriceRI::genere_ri_pour_discr(NoeudDiscr *noeud)
{
//	auto expression = noeud->expr;
	//auto op = inst->op;
//	auto decl_enum = static_cast<NoeudEnum *>(nullptr);
//	auto decl_struct = static_cast<NoeudStruct *>(nullptr);

//	if (noeud->genre == GenreNoeud::INSTRUCTION_DISCR_ENUM) {
//		auto type_enum = static_cast<TypeEnum *>(expression->type);
//		decl_enum = type_enum->decl;
//	}
//	else if (noeud->genre == GenreNoeud::INSTRUCTION_DISCR_UNION) {
//		auto type_struct = static_cast<TypeStructure *>(expression->type);
//		decl_struct = type_struct->decl;
//	}

	struct DonneesPaireDiscr {
		NoeudExpression *expr = nullptr;
		NoeudBloc *bloc = nullptr;
		InstructionLabel *bloc_de_la_condition = nullptr;
		InstructionLabel *bloc_si_vrai = nullptr;
		InstructionLabel *bloc_si_faux = nullptr;
	};

	auto bloc_post_discr = reserve_label();

	dls::tableau<DonneesPaireDiscr> donnees_paires;
	donnees_paires.reserve(noeud->paires_discr.taille + noeud->bloc_sinon != nullptr);

	POUR (noeud->paires_discr) {
		auto donnees = DonneesPaireDiscr();
		donnees.expr = it.first;
		donnees.bloc = it.second;
		donnees.bloc_de_la_condition = reserve_label();
		donnees.bloc_si_vrai = reserve_label();

		if (!donnees_paires.est_vide()) {
			donnees_paires.back().bloc_si_faux = donnees.bloc_de_la_condition;
		}

		donnees_paires.pousse(donnees);
	}

	if (noeud->bloc_sinon) {
		auto donnees = DonneesPaireDiscr();
		donnees.bloc = noeud->bloc_sinon;
		donnees.bloc_si_vrai = reserve_label();

		if (!donnees_paires.est_vide()) {
			donnees_paires.back().bloc_si_faux = donnees.bloc_si_vrai;
		}

		donnees_paires.pousse(donnees);
	}

	donnees_paires.back().bloc_si_faux = bloc_post_discr;

//	auto valeur_expression = genere_code_llvm(expression, contexte, b->genre == GenreNoeud::INSTRUCTION_DISCR_UNION);
//	auto ptr_structure = valeur_expression;

//	if (noeud->genre == GenreNoeud::INSTRUCTION_DISCR_UNION) {
//		valeur_expression = accede_membre_structure(contexte, valeur_expression, 1, true);
//	}

	cree_branche(donnees_paires.front().bloc_de_la_condition);

	for (auto &donnees : donnees_paires) {
		auto enf0 = donnees.expr;
		auto enf1 = donnees.bloc;

		insere_label(donnees.bloc_de_la_condition);

		if (enf0 != nullptr) {
			auto feuilles = dls::tablet<NoeudExpression *, 10>();
			rassemble_feuilles(enf0, feuilles);

			// les différentes feuilles sont évaluées dans des blocs
			// séparés afin de pouvoir éviter de tester trop de conditions
			// dès qu'une condition est vraie, nous allons dans le bloc_si_vrai
			// sinon nous allons dans le bloc pour la feuille suivante
			for (auto f : feuilles) {
				auto bloc_si_faux = donnees.bloc_si_faux;

#if 0
				if (f != feuilles.back()) {
					bloc_si_faux = reserve_label();
				}

				auto valeur_f = static_cast<Atome *>(nullptr);

				if (noeud->genre == GenreNoeud::INSTRUCTION_DISCR_ENUM) {
					valeur_f = valeur_enum(decl_enum, f->ident->nom, builder);
				}
				else if (noeud->genre == GenreNoeud::INSTRUCTION_DISCR_UNION) {
					auto idx_membre = trouve_index_membre(decl_struct, f->ident->nom);

					valeur_f = builder.getInt32(static_cast<unsigned>(idx_membre + 1));

					auto valeur = accede_membre_union(contexte, ptr_structure, decl_struct, f->ident->nom);

					contexte.ajoute_locale(f->ident, valeur);
				}
				else {
					valeur_f = genere_ri_pour_expression_droite(f);
				}

				// op est nul pour les énums
				if (!op || op->est_basique) {
					auto condition = llvm::ICmpInst::Create(
								llvm::Instruction::ICmp,
								llvm::CmpInst::Predicate::ICMP_EQ,
								valeur_expression,
								valeur_f,
								"",
								contexte.bloc_courant());

					cree_branche_condition(condition, donnees.bloc_si_vrai, bloc_si_faux);
				}
				else {
					auto condition = constructrice.appel_operateur(op, valeur_expression, valeur_f);
					cree_branche_condition(condition, donnees.bloc_si_vrai, bloc_si_faux);
				}

#endif
				if (f != feuilles.back()) {
					insere_label(bloc_si_faux);
				}
			}
		}

		insere_label(donnees.bloc_si_vrai);

		genere_ri_pour_noeud(enf1);
	}

	insere_label(bloc_post_discr);

	return nullptr;
}

Atome *ConstructriceRI::genere_ri_pour_tente(NoeudTente *noeud)
{
	return nullptr;
}

Atome *ConstructriceRI::genere_ri_pour_boucle_pour(NoeudPour *inst)
{
#if 0
	/* on génère d'abord le type de la variable */
	auto enfant1 = inst->variable;
	auto enfant2 = inst->expression;
	auto enfant3 = inst->bloc;
	auto enfant_sans_arret = inst->bloc_sansarret;
	auto enfant_sinon = inst->bloc_sinon;

	auto type = enfant2->type;
	enfant1->type = type;

	/* création des blocs */
	auto bloc_boucle = reserve_label();
	auto bloc_corps = reserve_label();
	auto bloc_inc = reserve_label();

	auto bloc_sansarret = static_cast<InstructionLabel *>(nullptr);
	auto bloc_sinon = static_cast<InstructionLabel *>(nullptr);

	if (enfant_sans_arret) {
		bloc_sansarret = reserve_label();
	}

	if (enfant_sinon) {
		bloc_sinon = reserve_label();
	}

	auto bloc_apres = reserve_label();

	auto var = enfant1;
	auto idx = static_cast<NoeudExpression *>(nullptr);

	if (enfant1->lexeme->genre == GenreLexeme::VIRGULE) {
		auto expr_bin = static_cast<NoeudExpressionBinaire *>(var);
		var = expr_bin->expr1;
		idx = expr_bin->expr2;
	}

	empile_controle_boucle(var->ident, bloc_inc, (bloc_sinon != nullptr) ? bloc_sinon : bloc_apres);

	auto type_de_la_variable = static_cast<Type *>(nullptr);
	auto type_de_l_index = static_cast<Type *>(nullptr);

	if (inst->aide_generation_code == GENERE_BOUCLE_PLAGE || inst->aide_generation_code == GENERE_BOUCLE_PLAGE_INDEX) {
		type_de_la_variable = enfant2->type;
		type_de_l_index = enfant2->type;
	}
	else {
		type_de_la_variable = m_contexte.typeuse[TypeBase::Z64];
		type_de_l_index = m_contexte.typeuse[TypeBase::Z64];
	}

	auto valeur_debut = cree_allocation(type_de_la_variable, var->ident);
	auto valeur_index = static_cast<Atome *>(nullptr);

	if (idx != nullptr) {
		valeur_index = cree_allocation(type_de_l_index, idx->ident);
	}

	if (inst->aide_generation_code == GENERE_BOUCLE_PLAGE || inst->aide_generation_code == GENERE_BOUCLE_PLAGE_INDEX) {
		auto expr_plage = static_cast<NoeudExpressionBinaire *>(enfant2);
		auto init_debut = genere_ri_pour_expression_droite(expr_plage->expr1);
		cree_stocke_mem(type_de_la_variable, valeur_debut, init_debut);
	}

	/* bloc_boucle */
	/* on crée une branche explicite dans le bloc */
	insere_label(bloc_boucle);

	auto pointeur_tableau = static_cast<llvm::Value *>(nullptr);

	switch (inst->aide_generation_code) {
		case GENERE_BOUCLE_PLAGE:
		case GENERE_BOUCLE_PLAGE_INDEX:
		{
			/* création du bloc de condition */

			auto expr_plage = static_cast<NoeudExpressionBinaire *>(enfant2);
			auto valeur_fin = genere_ri_pour_expression_droite(expr_plage->expr2);

			auto condition = llvm::ICmpInst::Create(
								 llvm::Instruction::ICmp,
								 llvm::CmpInst::Predicate::ICMP_SLE,
								 builder.CreateLoad(valeur_debut),
								 valeur_fin,
								 "",
								 contexte.bloc_courant());

			cree_branche_condition(condition, bloc_corps, (bloc_sansarret != nullptr) ? bloc_sansarret : bloc_apres);

			table_locales[var->ident] = valeur_debut;
			if (inst->aide_generation_code == GENERE_BOUCLE_PLAGE_INDEX) {
				table_locales[idx->ident] = valeur_index;
			}

			/* création du bloc de corps */

			insere_label(bloc_corps);

			genere_ri_pour_noeud(enfant3);

			/* bloc_inc */
			insere_label(bloc_inc);

			constructrice.incremente(valeur_debut, type_de_la_variable);

			if (valeur_index) {
				constructrice.incremente(valeur_index, type_de_l_index);
			}

			cree_branche(bloc_boucle);

			break;
		}
		case GENERE_BOUCLE_TABLEAU:
		case GENERE_BOUCLE_TABLEAU_INDEX:
		{
			auto taille_tableau = 0l;

			if (type->genre == GenreType::TABLEAU_FIXE) {
				taille_tableau = static_cast<TypeTableauFixe *>(type)->taille;
			}

			auto valeur_fin = static_cast<llvm::Value *>(nullptr);

			if (taille_tableau != 0) {
				valeur_fin = llvm::ConstantInt::get(
								 llvm::Type::getInt64Ty(contexte.contexte),
								 static_cast<unsigned long>(taille_tableau),
								 false);
			}
			else {
				pointeur_tableau = genere_code_llvm(enfant2, contexte, true);
				valeur_fin = accede_membre_structure(contexte, pointeur_tableau, TAILLE_TABLEAU, true);
			}

			auto condition = llvm::ICmpInst::Create(
								 llvm::Instruction::ICmp,
								 llvm::CmpInst::Predicate::ICMP_SLT,
								 builder.CreateLoad(valeur_debut),
								 valeur_fin,
								 "",
								 contexte.bloc_courant());

			cree_branche_condition(
						condition,
						bloc_corps,
						(bloc_sansarret != nullptr) ? bloc_sansarret : bloc_apres);

			insere_label(bloc_corps);

			auto valeur_arg = static_cast<Atome *>(nullptr);

			if (taille_tableau != 0) {
				auto valeur_tableau = genere_ri_pour_noeud(enfant2);

				valeur_arg = accede_element_tableau(
							 contexte,
							 valeur_tableau,
							 converti_type_llvm(contexte, type),
							 builder.CreateLoad(valeur_debut));
			}
			else {
				auto pointeur = accede_membre_structure(contexte, pointeur_tableau, POINTEUR_TABLEAU);

				pointeur = new llvm::LoadInst(pointeur, "", contexte.bloc_courant());

				valeur_arg = llvm::GetElementPtrInst::CreateInBounds(
							 pointeur,
							 builder.CreateLoad(valeur_debut),
							 "",
							 contexte.bloc_courant());
			}

			table_locales[var->ident] = valeur_arg;
			if (inst->aide_generation_code == GENERE_BOUCLE_TABLEAU_INDEX) {
				table_locales[idx->ident] = valeur_index;
			}

			/* création du bloc de corps */
			genere_ri_pour_noeud(enfant3);

			/* bloc_inc */
			insere_label(bloc_inc);

			constructrice.incremente(valeur_debut, type_de_la_variable);

			if (valeur_index) {
				constructrice.incremente(valeur_index, type_de_l_index);
			}

			cree_branche(bloc_boucle);

			break;
		}
		case GENERE_BOUCLE_COROUTINE:
		case GENERE_BOUCLE_COROUTINE_INDEX:
		{
			/* À FAIRE(coroutine) */
			break;
		}
	}

	/* 'continue'/'arrête' dans les blocs 'sinon'/'sansarrêt' n'a aucun sens */
	depile_controle_boucle();

	if (enfant_sans_arret) {
		insere_label(bloc_sansarret);
		genere_ri_pour_noeud(enfant_sans_arret);
	}

	if (enfant_sinon) {
		insere_label(bloc_sinon);
		genere_ri_pour_noeud(enfant_sinon);
	}

	insere_label(bloc_apres);
#endif
	return nullptr;
}

Atome *ConstructriceRI::genere_ri_pour_logement(Type *type, int mode, NoeudExpression *b, NoeudExpression *variable, NoeudExpression *expression, NoeudExpression *bloc_sinon)
{
	return nullptr;
}

struct DonneesComparaisonChainee {
	NoeudExpression *operande_gauche = nullptr;
	NoeudExpression *operande_droite = nullptr;
	OperateurBinaire const *op = nullptr;

	COPIE_CONSTRUCT(DonneesComparaisonChainee);
};

static void rassemble_operations_chainees(
		NoeudExpression *racine,
		kuri::tableau<DonneesComparaisonChainee> &comparaisons)
{
	auto expr_bin = static_cast<NoeudExpressionBinaire *>(racine);

	if (est_operateur_comp(expr_bin->expr1->lexeme->genre)) {
		rassemble_operations_chainees(expr_bin->expr1, comparaisons);

		auto expr_operande = static_cast<NoeudExpressionBinaire *>(expr_bin->expr1);

		auto comparaison = DonneesComparaisonChainee{};
		comparaison.operande_gauche = expr_operande->expr2;
		comparaison.operande_droite = expr_bin->expr2;
		comparaison.op = expr_bin->op;

		comparaisons.pousse(comparaison);
	}
	else {
		auto comparaison = DonneesComparaisonChainee{};
		comparaison.operande_gauche = expr_bin->expr1;
		comparaison.operande_droite = expr_bin->expr2;
		comparaison.op = expr_bin->op;

		comparaisons.pousse(comparaison);
	}
}

Atome *ConstructriceRI::genere_ri_pour_comparaison_chainee(NoeudExpression *noeud)
{
	auto comparaisons = kuri::tableau<DonneesComparaisonChainee>();
	rassemble_operations_chainees(noeud, comparaisons);

	// À FAIRE : bonne gestion des labels, le label si toutes les conditions
	// sont vraies devrait venir de la génération de code des boucles ou des
	// controles de flux.
	auto label_si_vrai = reserve_label();
	auto label_si_faux = reserve_label();

	POUR (comparaisons) {
		auto atome_gauche = genere_ri_transformee_pour_noeud(it.operande_gauche);
		auto atome_droite = genere_ri_transformee_pour_noeud(it.operande_droite);
		auto atome_op_bin = cree_op_binaire(noeud->type, it.op->genre, atome_gauche, atome_droite);

		cree_branche_condition(atome_op_bin, label_si_vrai, label_si_faux);
	}

	return nullptr;
}

Atome *ConstructriceRI::genere_ri_pour_declaration_structure(NoeudStruct *noeud)
{
	// génére la fonction d'initialisation
	return nullptr;
}

Atome *ConstructriceRI::genere_ri_pour_acces_membre(NoeudExpressionMembre *noeud)
{
	// À FAIRE : ceci ignore les espaces de noms.
	auto accede = noeud->accede;
	auto type_accede = accede->type;

	auto est_pointeur = type_accede->genre == GenreType::POINTEUR || type_accede->genre == GenreType::REFERENCE;

	while (type_accede->genre == GenreType::POINTEUR || type_accede->genre == GenreType::REFERENCE) {
		type_accede = m_contexte.typeuse.type_dereference_pour(type_accede);
	}

	if (type_accede->genre == GenreType::TABLEAU_FIXE) {
		auto taille = static_cast<TypeTableauFixe *>(type_accede)->taille;
		return cree_constante_entiere(noeud->type, static_cast<unsigned long>(taille));
	}

	if (type_accede->genre == GenreType::ENUM || type_accede->genre == GenreType::ERREUR) {
		auto type_enum = static_cast<TypeEnum *>(type_accede);
		auto valeur_enum = type_enum->membres[noeud->index_membre].valeur;
		return cree_constante_entiere(type_enum->type_donnees, static_cast<unsigned>(valeur_enum));
	}

	auto pointeur_accede = genere_ri_pour_noeud(accede);

	if (est_pointeur) {
		pointeur_accede = cree_charge_mem(type_accede, pointeur_accede);
	}

	// À FAIRE : gestion des constantes globales (chaines)
	// À FAIRE : les unions nonsûres ont toujours un index à 0

	auto atome_index = cree_constante_entiere(m_contexte.typeuse[TypeBase::Z64], static_cast<unsigned>(noeud->index_membre));
	return cree_acces_membre(noeud->type, pointeur_accede, atome_index);
}

Atome *ConstructriceRI::genere_ri_pour_acces_membre_union(NoeudExpressionMembre *noeud)
{
	// À FAIRE : nous devons savoir si nous avons une expression gauche ou non
	return nullptr;
}

AtomeConstante *ConstructriceRI::genere_initialisation_defaut_pour_type(Type *type)
{
	switch (type->genre) {
		case GenreType::INVALIDE:
		case GenreType::RIEN:
		{
			return nullptr;
		}
		case GenreType::BOOL:
		{
			return cree_constante_booleenne(type, false);
		}
		case GenreType::REFERENCE: /* À FAIRE : une référence ne peut être nulle ! */
		case GenreType::POINTEUR:
		case GenreType::FONCTION:
		{
			return cree_constante_nulle(type);
		}
		case GenreType::OCTET:
		case GenreType::ENTIER_NATUREL:
		case GenreType::ENTIER_RELATIF:
		case GenreType::ENTIER_CONSTANT:
		{
			return cree_constante_entiere(type, 0);
		}
		case GenreType::REEL:
		{
			return cree_constante_reelle(type, 0.0);
		}
		case GenreType::TABLEAU_FIXE:
		{
			// À FAIRE : initialisation défaut pour tableau fixe
			return nullptr;
		}
		case GenreType::UNION:
		{
			auto type_union = static_cast<TypeUnion *>(type);

			if (type_union->est_nonsure) {
				return genere_initialisation_defaut_pour_type(type_union->type_le_plus_grand);
			}

			auto valeurs = kuri::tableau<AtomeConstante *>();
			valeurs.reserve(2);

			valeurs.pousse(genere_initialisation_defaut_pour_type(type_union->type_le_plus_grand));
			valeurs.pousse(genere_initialisation_defaut_pour_type(m_contexte.typeuse[TypeBase::Z32]));

			return cree_constante_structure(type, std::move(valeurs));
		}
		case GenreType::CHAINE:
		case GenreType::EINI:
		case GenreType::STRUCTURE:
		case GenreType::TABLEAU_DYNAMIQUE:
		case GenreType::VARIADIQUE:
		{
			auto type_compose = static_cast<TypeCompose *>(type);
			auto valeurs = kuri::tableau<AtomeConstante *>();
			valeurs.reserve(type_compose->membres.taille);

			POUR (type_compose->membres) {
				auto valeur = genere_initialisation_defaut_pour_type(it.type);
				valeurs.pousse(valeur);
			}

			return cree_constante_structure(type, std::move(valeurs));
		}
		case GenreType::ENUM:
		case GenreType::ERREUR:
		{
			auto type_enum = static_cast<TypeEnum *>(type);
			return cree_constante_entiere(type_enum->type_donnees, 0);
		}
	}

	return nullptr;
}
