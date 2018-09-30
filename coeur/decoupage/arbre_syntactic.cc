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

/* ************************************************************************** */

void test_llvm()
{
	auto context = llvm::LLVMContext();
	auto module = new llvm::Module("top", context);

	auto constructeur = llvm::IRBuilder<>(context);

	/* Crée fonction main */
	auto type_fonction = llvm::FunctionType::get(constructeur.getInt32Ty(), false);
	auto fonction_main = llvm::Function::Create(type_fonction, llvm::Function::ExternalLinkage, "main", module);

	auto entree = llvm::BasicBlock::Create(context, "entrypoint", fonction_main);
	constructeur.SetInsertPoint(entree);

	/* Crée appel vers fonction puts */
	std::vector<llvm::Type *> putsArgs;
	putsArgs.push_back(constructeur.getInt8Ty()->getPointerTo());
	llvm::ArrayRef<llvm::Type*>  argsRef(putsArgs);

	auto *putsType =
	  llvm::FunctionType::get(constructeur.getInt32Ty(), argsRef, false);
	llvm::Constant *putsFunc = module->getOrInsertFunction("puts", putsType);

	auto valeur = constructeur.CreateGlobalStringPtr("hello world!\n");

	constructeur.CreateCall(putsFunc, valeur);

	constructeur.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0, false));

	module->dump();
}

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

Noeud::Noeud(const std::string &chaine, int id)
	: m_chaine(chaine)
	, identifiant(id)
{}

void Noeud::ajoute_noeud(Noeud *noeud)
{
	m_enfants.push_back(noeud);
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

void NoeudRacine::genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module)
{
	for (auto noeud : m_enfants) {
		noeud->genere_code_llvm(contexte, module);
	}
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

void NoeudAppelFonction::genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module)
{
	auto fonction = module->getFunction(m_chaine);

	if (fonction == nullptr) {
		std::cerr << "Impossible de trouver la fonction '" << m_chaine << "' !\n";
		return;
	}

	/* Cherche la liste d'arguments */
	std::vector<llvm::Value *> parametres;

	for (auto noeud : m_enfants) {
		noeud->genere_code_llvm(contexte, module);
	}

	llvm::ArrayRef<llvm::Value*> args(parametres);

	llvm::CallInst::Create(fonction, args, "");
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

void NoeudDeclarationFonction::genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module)
{
	auto constructeur = llvm::IRBuilder<>(contexte);

	/* Crée la liste de paramètres */
	std::vector<llvm::Type *> parametres;

	for (const auto &argument : m_arguments) {
		parametres.push_back(type_argument(contexte, argument.id_type));
	}

	llvm::ArrayRef<llvm::Type*> args(parametres);

	/* Crée fonction */
	auto type_fonction = llvm::FunctionType::get(type_argument(contexte, this->type_retour), args, false);
	auto fonction_main = llvm::Function::Create(type_fonction, llvm::Function::ExternalLinkage, m_chaine, module);

	auto entree = llvm::BasicBlock::Create(contexte, "entrypoint", fonction_main);
	constructeur.SetInsertPoint(entree);

	for (auto noeud : m_enfants) {
		noeud->genere_code_llvm(contexte, module);
	}

	constructeur.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(contexte), 0, false));
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

void NoeudExpression::genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module)
{

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

void NoeudAssignationVariable::genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module)
{

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

void NoeudNombreEntier::genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module)
{

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

void NoeudNombreReel::genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module)
{

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

void NoeudVariable::genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module)
{

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

void NoeudOperation::genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module)
{

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

void NoeudRetour::genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module)
{

}
