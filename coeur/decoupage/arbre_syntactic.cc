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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "arbre_syntactic.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

#include <sstream>

#include "contexte_generation_code.h"
#include "erreur.h"
#include "morceaux.h"
#include "nombres.h"
#include "tampon_source.h"

/* ************************************************************************** */

static llvm::Type *type_argument(llvm::LLVMContext &contexte, int identifiant)
{
	switch (identifiant) {
		case ID_BOOL:
			return llvm::Type::getInt1Ty(contexte);
		case ID_E8:
			return llvm::Type::getInt8Ty(contexte);
		case ID_E16:
			return llvm::Type::getInt16Ty(contexte);
		case ID_E32:
			return llvm::Type::getInt32Ty(contexte);
		case ID_E64:
			return llvm::Type::getInt64Ty(contexte);
		case ID_R16:
			return llvm::Type::getHalfTy(contexte);
		case ID_R32:
			return llvm::Type::getFloatTy(contexte);
		case ID_R64:
			return llvm::Type::getDoubleTy(contexte);
		default:
		case ID_RIEN:
			return llvm::Type::getVoidTy(contexte);
	}
}

static bool est_type_entier(int type)
{
	switch (type) {
		case ID_BOOL:
		case ID_E8:
		case ID_E16:
		case ID_E32:
		case ID_E64:
			return true;
		default:
			return false;
	}
}

#if 0
static bool est_type_reel(int type)
{
	switch (type) {
		case ID_R16:
		case ID_R32:
		case ID_R64:
			return true;
		default:
			return false;
	}
}
#endif

/* ************************************************************************** */

static void imprime_tab(std::ostream &os, int tab)
{
	for (int i = 0; i < tab; ++i) {
		os << ' ' << ' ';
	}
}

/* ************************************************************************** */

Noeud::Noeud(const DonneesMorceaux &morceau)
	: m_donnees_morceaux(morceau)
{}

void Noeud::ajoute_noeud(Noeud *noeud)
{
	m_enfants.push_back(noeud);
}

int Noeud::calcul_type(ContexteGenerationCode &/*contexte*/)
{
	return this->type;
}

int Noeud::identifiant() const
{
	return m_donnees_morceaux.identifiant;
}

/* ************************************************************************** */

NoeudRacine::NoeudRacine(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudRacine::imprime_code(std::ostream &os, int tab)
{
	os << "NoeudRacine\n";

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudRacine::genere_code_llvm(ContexteGenerationCode &contexte)
{
	for (auto noeud : m_enfants) {
		noeud->genere_code_llvm(contexte);
	}

	return nullptr;
}

/* ************************************************************************** */

NoeudAppelFonction::NoeudAppelFonction(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudAppelFonction::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudAppelFonction : " << m_donnees_morceaux.chaine << '\n';
	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudAppelFonction::genere_code_llvm(ContexteGenerationCode &contexte)
{
	auto fonction = contexte.module->getFunction(std::string(m_donnees_morceaux.chaine));

	if (fonction == nullptr) {
		erreur::lance_erreur(
					"Fonction inconnue",
					contexte.tampon,
					m_donnees_morceaux);
	}

	auto donnees_fonction = contexte.donnees_fonction(m_donnees_morceaux.chaine);

	if (m_enfants.size() != donnees_fonction.args.size()) {
		erreur::lance_erreur_nombre_arguments(
					donnees_fonction.args.size(),
					m_enfants.size(),
					contexte.tampon,
					m_donnees_morceaux);
	}

	/* Cherche la liste d'arguments */
	std::vector<llvm::Value *> parametres;

	if (m_noms_arguments.empty()) {
		parametres.reserve(m_enfants.size());
		auto index = 0ul;

		/* Les arguments sont dans l'ordre. */
		for (const auto &enfant : m_enfants) {

			/* À FAIRE : trouver mieux que ça. */
			for (const auto &pair : donnees_fonction.args) {
				if (pair.second.index != index) {
					continue;
				}

				const auto type_arg = pair.second.type;
				const auto type_enf = enfant->calcul_type(contexte);

				if (type_arg != type_enf) {
					erreur::lance_erreur_type_arguments(
								type_arg,
								type_enf,
								pair.first,
								contexte.tampon,
								m_donnees_morceaux);
				}
			}

			auto valeur = enfant->genere_code_llvm(contexte);
			parametres.push_back(valeur);

			++index;
		}
	}
	else {
		/* Il faut trouver l'ordre des arguments. À FAIRE : tests. */

		if (m_noms_arguments.size() != donnees_fonction.args.size()) {
			erreur::lance_erreur_nombre_arguments(
						donnees_fonction.args.size(),
						m_enfants.size(),
						contexte.tampon,
						m_donnees_morceaux);
		}

		/* Réordonne les enfants selon l'apparition des arguments car LLVM est
		 * tatillon : ce n'est pas l'ordre dans lequel les valeurs apparaissent
		 * dans le vecteur de paramètres qui compte, mais l'ordre dans lequel le
		 * code est généré. */
		std::vector<Noeud *> enfants;
		enfants.reserve(m_noms_arguments.size());

		auto index = 0ul;

		for (const auto &nom : m_noms_arguments) {
			auto iter = donnees_fonction.args.find(nom);

			if (iter == donnees_fonction.args.end()) {
				erreur::lance_erreur_argument_inconnu(
							nom,
							contexte.tampon,
							m_donnees_morceaux);
			}

			const auto type_arg = iter->second.type;
			const auto type_enf = m_enfants[index]->calcul_type(contexte);

			if (type_arg != type_enf) {
				erreur::lance_erreur_type_arguments(
							type_arg,
							type_enf,
							nom,
							contexte.tampon,
							m_donnees_morceaux);
			}

			enfants.push_back(m_enfants[index]);

			++index;
		}

		parametres.resize(m_noms_arguments.size());

		std::transform(enfants.begin(), enfants.end(), parametres.begin(),
					   [&](Noeud *enfant)
		{
			return enfant->genere_code_llvm(contexte);
		});
	}

	llvm::ArrayRef<llvm::Value *> args(parametres);

	return llvm::CallInst::Create(fonction, args, "", contexte.block_courant());
}

int NoeudAppelFonction::calcul_type(ContexteGenerationCode &contexte)
{
	if (this->type == -1) {
		auto donnees_fonction = contexte.donnees_fonction(m_donnees_morceaux.chaine);
		this->type = donnees_fonction.type_retour;
	}

	return this->type;
}

void NoeudAppelFonction::ajoute_nom_argument(const std::string_view &nom)
{
	m_noms_arguments.push_back(nom);
}

/* ************************************************************************** */

NoeudDeclarationFonction::NoeudDeclarationFonction(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudDeclarationFonction::ajoute_argument(const ArgumentFonction &argument)
{
	m_arguments.push_back(argument);
}

void NoeudDeclarationFonction::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudDeclarationFonction : " << m_donnees_morceaux.chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudDeclarationFonction::genere_code_llvm(ContexteGenerationCode &contexte)
{
	/* Crée la liste de paramètres */
	std::vector<llvm::Type *> parametres(m_arguments.size());

	std::transform(m_arguments.begin(), m_arguments.end(), parametres.begin(),
				   [&](const ArgumentFonction &donnees)
	{
		return type_argument(contexte.contexte, donnees.id_type);
	});

	llvm::ArrayRef<llvm::Type*> args(parametres);

	/* À FAIRE : calcule type retour, considération fonction récursive. */

	/* Crée fonction */
	auto type_fonction = llvm::FunctionType::get(
							 type_argument(contexte.contexte, this->type_retour),
							 args,
							 false);

	auto fonction = llvm::Function::Create(
				   type_fonction,
				   llvm::Function::ExternalLinkage,
				   std::string(m_donnees_morceaux.chaine),
				   contexte.module);

	auto block = llvm::BasicBlock::Create(
					 contexte.contexte,
					 "entrypoint",
					 fonction);

	contexte.pousse_block(block);

	/* Crée code pour les arguments */
	auto valeurs_args = fonction->arg_begin();

	for (const auto &argument : m_arguments) {
		auto alloc = new llvm::AllocaInst(
						 type_argument(contexte.contexte, argument.id_type),
						 argument.chaine,
						 contexte.block_courant());

		contexte.pousse_locale(argument.chaine, alloc, argument.id_type);

		llvm::Value *valeur = &*valeurs_args++;
		valeur->setName(argument.chaine.c_str());

		new llvm::StoreInst(valeur, alloc, false, contexte.block_courant());
	}

	/* Crée code pour les expressions */
	for (auto noeud : m_enfants) {
		noeud->genere_code_llvm(contexte);
	}

	contexte.jete_block();

	return nullptr;
}

/* ************************************************************************** */

NoeudExpression::NoeudExpression(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudExpression::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudExpression : " << m_donnees_morceaux.chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudExpression::genere_code_llvm(ContexteGenerationCode &/*contexte*/)
{
	return nullptr;
}

int NoeudExpression::calcul_type(ContexteGenerationCode &/*contexte*/)
{
	return this->type;
}

/* ************************************************************************** */

NoeudAssignationVariable::NoeudAssignationVariable(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudAssignationVariable::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudAssignationVariable : " << m_donnees_morceaux.chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudAssignationVariable::genere_code_llvm(ContexteGenerationCode &contexte)
{
	auto valeur = contexte.valeur_locale(m_donnees_morceaux.chaine);

	if (valeur != nullptr) {
		throw "Redéfinition de la variable !";
	}
	else {
		valeur = contexte.valeur_globale(m_donnees_morceaux.chaine);

		if (valeur != nullptr) {
			throw "Redéfinition de la variable !";
		}
	}

	assert(m_enfants.size() == 1);

	if (this->type == -1) {
		this->type = m_enfants[0]->calcul_type(contexte);

		if (this->type == -1) {
			throw "Impossible de définir le type de la variable !";
		}

		if (this->type == ID_RIEN) {
			throw "Impossible d'assigner une expression de type 'rien' à une variable !";
		}
	}

	/* Génère d'abord le code de l'enfant afin que l'instruction d'allocation de
	 * la variable sur la pile et celle de stockage de la valeur soit côte à
	 * côte. */
	valeur = m_enfants[0]->genere_code_llvm(contexte);

	auto type_llvm = type_argument(contexte.contexte, this->type);
	auto alloc = new llvm::AllocaInst(type_llvm, std::string(m_donnees_morceaux.chaine), contexte.block_courant());
	new llvm::StoreInst(valeur, alloc, false, contexte.block_courant());

	contexte.pousse_locale(m_donnees_morceaux.chaine, alloc, this->type);

	return alloc;
}

/* ************************************************************************** */

NoeudConstante::NoeudConstante(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudConstante::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudConstante : " << m_donnees_morceaux.chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudConstante::genere_code_llvm(ContexteGenerationCode &contexte)
{
	auto valeur = contexte.valeur_globale(m_donnees_morceaux.chaine);

	if (valeur != nullptr) {
		throw "Redéfinition de la variable globale !";
	}

	if (this->type == -1) {
		this->type = m_enfants[0]->calcul_type(contexte);

		if (this->type == -1) {
			throw "Impossible de définir le type de la variable globale.";
		}
	}

	valeur = m_enfants[0]->genere_code_llvm(contexte);

	contexte.pousse_globale(m_donnees_morceaux.chaine, valeur, this->type);

	return valeur;
}

int NoeudConstante::calcul_type(ContexteGenerationCode &contexte)
{
	return contexte.type_globale(m_donnees_morceaux.chaine);
}

/* ************************************************************************** */

NoeudNombreEntier::NoeudNombreEntier(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudNombreEntier::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudNombreEntier : " << m_donnees_morceaux.chaine << '\n';
}

llvm::Value *NoeudNombreEntier::genere_code_llvm(ContexteGenerationCode &contexte)
{
	const auto valeur = converti_chaine_nombre_entier(
							m_donnees_morceaux.chaine,
							m_donnees_morceaux.identifiant);

	return llvm::ConstantInt::get(
				llvm::Type::getInt32Ty(contexte.contexte),
				static_cast<uint64_t>(valeur),
				false);
}

int NoeudNombreEntier::calcul_type(ContexteGenerationCode &/*contexte*/)
{
	this->type = ID_E32;
	return this->type;
}

/* ************************************************************************** */

NoeudNombreReel::NoeudNombreReel(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudNombreReel::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudNombreReel : " << m_donnees_morceaux.chaine << '\n';
}

llvm::Value *NoeudNombreReel::genere_code_llvm(ContexteGenerationCode &contexte)
{
	const auto valeur = converti_chaine_nombre_reel(
							m_donnees_morceaux.chaine,
							m_donnees_morceaux.identifiant);

	return llvm::ConstantFP::get(
				llvm::Type::getDoubleTy(contexte.contexte),
				valeur);
}

int NoeudNombreReel::calcul_type(ContexteGenerationCode &/*contexte*/)
{
	this->type = ID_R64;
	return this->type;
}

/* ************************************************************************** */

NoeudVariable::NoeudVariable(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudVariable::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudVariable : " << m_donnees_morceaux.chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudVariable::genere_code_llvm(ContexteGenerationCode &contexte)
{
	auto valeur = contexte.valeur_locale(m_donnees_morceaux.chaine);

	if (valeur == nullptr) {
		valeur = contexte.valeur_globale(m_donnees_morceaux.chaine);

		if (valeur == nullptr) {
			throw "Variable inconnue";
		}
	}

	return new llvm::LoadInst(valeur, "", false, contexte.block_courant());
}

int NoeudVariable::calcul_type(ContexteGenerationCode &contexte)
{
	auto valeur = contexte.type_locale(m_donnees_morceaux.chaine);

	if (valeur == -1) {
		valeur = contexte.type_globale(m_donnees_morceaux.chaine);
	}

	return valeur;
}

/* ************************************************************************** */

NoeudOperation::NoeudOperation(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudOperation::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudOperation : " << m_donnees_morceaux.chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudOperation::genere_code_llvm(ContexteGenerationCode &contexte)
{
	if (m_enfants.size() == 1) {
		llvm::Instruction::BinaryOps instr;
		auto valeur1 = m_enfants[0]->genere_code_llvm(contexte);
		auto valeur2 = static_cast<llvm::Value *>(nullptr);

		switch (this->m_donnees_morceaux.identifiant) {
			case ID_EXCLAMATION:
				instr = llvm::Instruction::Xor;
				valeur2 = valeur1;
				break;
			case ID_TILDE:
				instr = llvm::Instruction::Xor;
				valeur2 = llvm::ConstantInt::get(
							  llvm::Type::getInt32Ty(contexte.contexte),
							  static_cast<uint64_t>(0),
							  false);
				break;
			case ID_AROBASE:
				/* À FAIRE : prend addresse. */
				return valeur1;
			default:
				return nullptr;
		}

		return llvm::BinaryOperator::Create(instr, valeur1, valeur2, "", contexte.block_courant());
	}

	if (m_enfants.size() == 2) {
		llvm::Instruction::BinaryOps instr;

		const auto type1 = m_enfants[0]->calcul_type(contexte);
		const auto type2 = m_enfants[1]->calcul_type(contexte);

		if (type1 != type2) {
			throw "Les types de l'opération sont différents !";
		}

		/* À FAIRE : typage */

		switch (this->m_donnees_morceaux.identifiant) {
			case ID_PLUS:
				if (est_type_entier(type1)) {
					instr = llvm::Instruction::Add;
				}
				else {
					instr = llvm::Instruction::FAdd;
				}

				break;
			case ID_MOINS:
				if (est_type_entier(type1)) {
					instr = llvm::Instruction::Sub;
				}
				else {
					instr = llvm::Instruction::FSub;
				}

				break;
			case ID_FOIS:
				if (est_type_entier(type1)) {
					instr = llvm::Instruction::Mul;
				}
				else {
					instr = llvm::Instruction::FMul;
				}

				break;
			case ID_DIVISE:
				if (est_type_entier(type1)) {
					instr = llvm::Instruction::SDiv;
				}
				else {
					instr = llvm::Instruction::FDiv;
				}

				break;
			case ID_POURCENT:
				if (est_type_entier(type1)) {
					instr = llvm::Instruction::SRem;
				}
				else {
					instr = llvm::Instruction::FRem;
				}

				break;
			case ID_DECALAGE_DROITE:
				if (!est_type_entier(type1)) {
					throw "Besoin d'un type entier pour le décalage !";
				}
				instr = llvm::Instruction::LShr;
				break;
			case ID_DECALAGE_GAUCHE:
				if (!est_type_entier(type1)) {
					throw "Besoin d'un type entier pour le décalage !";
				}
				instr = llvm::Instruction::Shl;
				break;
			case ID_ESPERLUETTE:
			case ID_ESP_ESP:
				if (!est_type_entier(type1)) {
					throw "Besoin d'un type entier pour l'opération binaire !";
				}
				instr = llvm::Instruction::And;
				break;
			case ID_BARRE:
			case ID_BARRE_BARRE:
				if (!est_type_entier(type1)) {
					throw "Besoin d'un type entier pour l'opération binaire !";
				}
				instr = llvm::Instruction::Or;
				break;
			case ID_CHAPEAU:
				if (!est_type_entier(type1)) {
					throw "Besoin d'un type entier pour l'opération binaire !";
				}
				instr = llvm::Instruction::Xor;
				break;
			/* À FAIRE. */
			case ID_INFERIEUR:
			case ID_INFERIEUR_EGAL:
			case ID_SUPERIEUR:
			case ID_SUPERIEUR_EGAL:
			case ID_EGALITE:
			case ID_DIFFERENCE:
				instr = llvm::Instruction::Add;
				break;
			default:
				return nullptr;
		}

		auto valeur1 = m_enfants[0]->genere_code_llvm(contexte);
		auto valeur2 = m_enfants[1]->genere_code_llvm(contexte);

		return llvm::BinaryOperator::Create(instr, valeur1, valeur2, "", contexte.block_courant());
	}

	return nullptr;
}

int NoeudOperation::calcul_type(ContexteGenerationCode &contexte)
{
	return m_enfants[0]->calcul_type(contexte);
}

/* ************************************************************************** */

NoeudRetour::NoeudRetour(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudRetour::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudRetour : " << m_donnees_morceaux.chaine << '\n';
	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudRetour::genere_code_llvm(ContexteGenerationCode &contexte)
{
	llvm::Value *valeur = nullptr;

	if (m_enfants.size() > 0) {
		assert(m_enfants.size() == 1);
		valeur = m_enfants[0]->genere_code_llvm(contexte);
	}

	return llvm::ReturnInst::Create(contexte.contexte, valeur, contexte.block_courant());
}

int NoeudRetour::calcul_type(ContexteGenerationCode &contexte)
{
	if (m_enfants.size() == 0) {
		return ID_RIEN;
	}

	return m_enfants[0]->calcul_type(contexte);
}
