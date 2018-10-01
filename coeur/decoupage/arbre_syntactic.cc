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

#include "morceaux.h"
#include "nombres.h"

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

/* ************************************************************************** */

static void imprime_tab(std::ostream &os, int tab)
{
	for (int i = 0; i < tab; ++i) {
		os << ' ' << ' ';
	}
}

/* ************************************************************************** */

void ContexteGenerationCode::pousse_block(llvm::BasicBlock *block)
{
	Block b;
	b.block = block;
	pile_block.push(b);
}

void ContexteGenerationCode::jete_block()
{
	pile_block.pop();
}

llvm::BasicBlock *ContexteGenerationCode::block_courant() const
{
	if (pile_block.empty()) {
		return nullptr;
	}

	return pile_block.top().block;
}

void ContexteGenerationCode::pousse_locale(const std::string &nom, llvm::Value *valeur)
{
	pile_block.top().locals.insert({nom, valeur});
}

llvm::Value *ContexteGenerationCode::locale(const std::string &nom)
{
	auto iter = pile_block.top().locals.find(nom);

	if (iter == pile_block.top().locals.end()) {
		return nullptr;
	}

	return iter->second;
}

/* ************************************************************************** */

Noeud::Noeud(const std::string &chaine, int id)
	: m_chaine(chaine)
	, identifiant(id)
{}

void Noeud::ajoute_noeud(Noeud *noeud)
{
	m_enfants.push_back(noeud);
}

int Noeud::calcul_type()
{
	return this->type;
}

/* ************************************************************************** */

NoeudRacine::NoeudRacine(const std::string &chaine, int id)
	: Noeud(chaine, id)
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

NoeudAppelFonction::NoeudAppelFonction(const std::string &chaine, int id)
	: Noeud(chaine, id)
{}

void NoeudAppelFonction::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudAppelFonction : " << m_chaine << '\n';
	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudAppelFonction::genere_code_llvm(ContexteGenerationCode &contexte)
{
	auto fonction = contexte.module->getFunction(m_chaine);

	if (fonction == nullptr) {
		std::cerr << "Impossible de trouver la fonction '" << m_chaine << "' !\n";
		return nullptr;
	}

	/* Cherche la liste d'arguments */
	std::vector<llvm::Value *> parametres;

	for (auto noeud : m_enfants) {
		auto valeur = noeud->genere_code_llvm(contexte);
		parametres.push_back(valeur);
	}

	llvm::ArrayRef<llvm::Value *> args(parametres);

	return llvm::CallInst::Create(fonction, args, "", contexte.block_courant());
}

int NoeudAppelFonction::calcul_type()
{
	return this->type;
}

/* ************************************************************************** */

NoeudDeclarationFonction::NoeudDeclarationFonction(const std::string &chaine, int id)
	: Noeud(chaine, id)
{}

void NoeudDeclarationFonction::ajoute_argument(const ArgumentFonction &argument)
{
	m_arguments.push_back(argument);
}

void NoeudDeclarationFonction::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudDeclarationFonction : " << m_chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudDeclarationFonction::genere_code_llvm(ContexteGenerationCode &contexte)
{
	/* Crée la liste de paramètres */
	std::vector<llvm::Type *> parametres;

	for (const auto &argument : m_arguments) {
		parametres.push_back(type_argument(contexte.contexte, argument.id_type));
	}

	llvm::ArrayRef<llvm::Type*> args(parametres);

	/* Crée fonction */
	auto type_fonction = llvm::FunctionType::get(
							 type_argument(contexte.contexte, this->type_retour),
							 args,
							 false);

	auto fonction = llvm::Function::Create(
						type_fonction,
						llvm::Function::ExternalLinkage,
						m_chaine,
						contexte.module);

	auto block = llvm::BasicBlock::Create(
					 contexte.contexte,
					 "entrypoint",
					 fonction);

	contexte.pousse_block(block);

	/* Crée code pour les arguments */
	auto valeurs_args = fonction->arg_begin();
	llvm::Value *valeur;

	for (const auto &argument : m_arguments) {
		auto alloc = new llvm::AllocaInst(
						 type_argument(contexte.contexte, argument.id_type),
						 argument.chaine,
						 contexte.block_courant());

		contexte.pousse_locale(argument.chaine, alloc);

		valeur = &*valeurs_args++;
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

NoeudExpression::NoeudExpression(const std::string &chaine, int id)
	: Noeud(chaine, id)
{}

void NoeudExpression::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudExpression : " << m_chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudExpression::genere_code_llvm(ContexteGenerationCode &contexte)
{
	return nullptr;
}

int NoeudExpression::calcul_type()
{
	return this->type;
}

/* ************************************************************************** */

NoeudAssignationVariable::NoeudAssignationVariable(const std::string &chaine, int id)
	: Noeud(chaine, id)
{}

void NoeudAssignationVariable::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudAssignationVariable : " << m_chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudAssignationVariable::genere_code_llvm(ContexteGenerationCode &contexte)
{
	assert(m_enfants.size() == 1);

	if (this->type == -1) {
		this->type = m_enfants[0]->calcul_type();
	}

	/* Génère d'abord le code de l'enfant afin que l'instruction d'allocation de
	 * la variable sur la pile et celle de stockage de la valeur soit côte à
	 * côte. */
	auto valeur = m_enfants[0]->genere_code_llvm(contexte);

	auto type_llvm = type_argument(contexte.contexte, this->type);
	auto alloc = new llvm::AllocaInst(type_llvm, m_chaine, contexte.block_courant());
	new llvm::StoreInst(valeur, alloc, false, contexte.block_courant());

	contexte.pousse_locale(m_chaine, alloc);

	return alloc;
}

/* ************************************************************************** */

NoeudNombreEntier::NoeudNombreEntier(const std::string &chaine, int id)
	: Noeud(chaine, id)
{}

void NoeudNombreEntier::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudNombreEntier : " << m_chaine << '\n';
}

llvm::Value *NoeudNombreEntier::genere_code_llvm(ContexteGenerationCode &contexte)
{
	const auto valeur = converti_chaine_nombre_entier(m_chaine, identifiant);
	return llvm::ConstantInt::get(llvm::Type::getInt32Ty(contexte.contexte), valeur, false);
}

int NoeudNombreEntier::calcul_type()
{
	this->type = ID_E32;
	return this->type;
}

/* ************************************************************************** */

NoeudNombreReel::NoeudNombreReel(const std::string &chaine, int id)
	: Noeud(chaine, id)
{}

void NoeudNombreReel::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudNombreReel : " << m_chaine << '\n';
}

llvm::Value *NoeudNombreReel::genere_code_llvm(ContexteGenerationCode &contexte)
{
	const auto valeur = converti_chaine_nombre_reel(m_chaine, identifiant);
	return llvm::ConstantFP::get(llvm::Type::getDoubleTy(contexte.contexte), valeur);
}

int NoeudNombreReel::calcul_type()
{
	this->type = ID_R64;
	return this->type;
}

/* ************************************************************************** */

NoeudVariable::NoeudVariable(const std::string &chaine, int id)
	: Noeud(chaine, id)
{}

void NoeudVariable::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudVariable : " << m_chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudVariable::genere_code_llvm(ContexteGenerationCode &contexte)
{
	llvm::Value *valeur = contexte.locale(m_chaine);

	if (valeur == nullptr) {
		std::cerr << "NoeudVariable::genere_code_llvm : Variable '" << m_chaine << "' inconnue !\n";
		return nullptr;
	}

	return new llvm::LoadInst(valeur, "", false, contexte.block_courant());
}

int NoeudVariable::calcul_type()
{
	/* À FAIRE : stocke le type de la variable. */
	return this->type;
}

/* ************************************************************************** */

NoeudOperation::NoeudOperation(const std::string &chaine, int id)
	: Noeud(chaine, id)
{}

void NoeudOperation::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudOperation : " << m_chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudOperation::genere_code_llvm(ContexteGenerationCode &contexte)
{
	llvm::Instruction::BinaryOps instr;

	switch (this->identifiant) {
		case ID_PLUS:
			instr = llvm::Instruction::Add;
			break;
		case ID_MOINS:
			instr = llvm::Instruction::Sub;
			break;
		case ID_FOIS:
			instr = llvm::Instruction::Mul;
			break;
		case ID_DIVISE:
			instr = llvm::Instruction::SDiv;
			break;
		default:
			return nullptr;
	}

	assert(m_enfants.size() == 2);

	auto valeur1 = m_enfants[0]->genere_code_llvm(contexte);
	auto valeur2 = m_enfants[1]->genere_code_llvm(contexte);

	return llvm::BinaryOperator::Create(instr, valeur1, valeur2, "", contexte.block_courant());
}

int NoeudOperation::calcul_type()
{
	return this->type;
}

/* ************************************************************************** */

NoeudRetour::NoeudRetour(const std::string &chaine, int id)
	: Noeud(chaine, id)
{}

void NoeudRetour::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudRetour : " << m_chaine << '\n';
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

int NoeudRetour::calcul_type()
{
	return this->type;
}
