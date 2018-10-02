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

#include "erreur.h"
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

void ContexteGenerationCode::pousse_locale(const std::string &nom, llvm::Value *valeur, int type)
{
	pile_block.top().locals.insert({nom, {valeur, type, 0}});
}

llvm::Value *ContexteGenerationCode::valeur_locale(const std::string &nom)
{
	auto iter = pile_block.top().locals.find(nom);

	if (iter == pile_block.top().locals.end()) {
		return nullptr;
	}

	return iter->second.valeur;
}

int ContexteGenerationCode::type_locale(const std::string &nom)
{
	auto iter = pile_block.top().locals.find(nom);

	if (iter == pile_block.top().locals.end()) {
		return -1;
	}

	return iter->second.type;
}

void ContexteGenerationCode::ajoute_donnees_fonctions(const std::string &nom, const DonneesFonction &donnees)
{
	fonctions.insert({nom, donnees});
}

DonneesFonction ContexteGenerationCode::donnees_fonction(const std::string &nom)
{
	return fonctions[nom];
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

int Noeud::calcul_type(ContexteGenerationCode &/*contexte*/)
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
		throw "Fonction inconnue !\n";
	}

	auto donnees_fonction = contexte.donnees_fonction(m_chaine);

	if (m_enfants.size() != donnees_fonction.args.size()) {
		throw "Le nombre d'arguments de la fonction est incorrect.";
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

				if (pair.second.type != enfant->calcul_type(contexte)) {
					std::stringstream ss;
					ss << "Fonction : '" << m_chaine << "', argument " << index << '\n';
					ss << "Les types d'arguments ne correspondent pas !\n";
					ss << "Requiers " << chaine_identifiant(type_arg) << '\n';
					ss << "Obtenu " << chaine_identifiant(type_enf) << '\n';
					throw erreur::frappe(ss.str().c_str());
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
			throw "Le nombre d'arguments de la fonction est incorrect.";
		}

		/* Réordonne les enfants selon l'apparition des arguments car LLVM est
		 * tatillon : ce n'est pas l'ordre dans lequel les valeurs apparaissent
		 * dans le vecteur de paramètres qui compte, mais l'ordre dans lequel le
		 * code est généré. */
		std::vector<Noeud *> enfants;
		enfants.reserve(m_noms_arguments.size());

		for (const auto &nom : m_noms_arguments) {
			auto iter = donnees_fonction.args.find(nom);

			if (iter == donnees_fonction.args.end()) {
				throw "Argument inconnu !\n";
			}

			const auto index = iter->second.index;
			const auto type_arg = iter->second.type;
			const auto type_enf = m_enfants[index]->calcul_type(contexte);

			if (type_arg != type_enf) {
				std::stringstream ss;
				ss << "Fonction : '" << m_chaine << "', argument " << index << '\n';
				ss << "Les types d'arguments ne correspondent pas !\n";
				ss << "Requiers " << chaine_identifiant(type_arg) << '\n';
				ss << "Obtenu " << chaine_identifiant(type_enf) << '\n';
				throw erreur::frappe(ss.str().c_str());
			}

			enfants.push_back(m_enfants[index]);
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
		auto donnees_fonction = contexte.donnees_fonction(m_chaine);
		this->type = donnees_fonction.type_retour;
	}

	return this->type;
}

void NoeudAppelFonction::ajoute_nom_argument(const std::string &nom)
{
	m_noms_arguments.push_back(nom);
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
						m_chaine,
						contexte.module);

	auto block = llvm::BasicBlock::Create(
					 contexte.contexte,
					 "entrypoint",
					 fonction);

	contexte.pousse_block(block);

	/* Crée code pour les arguments */
	auto valeurs_args = fonction->arg_begin();
	auto index = 0ul;

	auto donnees_fonctions = DonneesFonction();
	donnees_fonctions.type_retour = this->type_retour;

	for (const auto &argument : m_arguments) {
		if (contexte.valeur_locale(argument.chaine) != nullptr) {
			throw "Redéclaration de l'argument !";
		}

		auto alloc = new llvm::AllocaInst(
						 type_argument(contexte.contexte, argument.id_type),
						 argument.chaine,
						 contexte.block_courant());

		donnees_fonctions.args.insert({argument.chaine, {index++, argument.id_type, 0}});

		contexte.pousse_locale(argument.chaine, alloc, argument.id_type);

		llvm::Value *valeur = &*valeurs_args++;
		valeur->setName(argument.chaine.c_str());

		new llvm::StoreInst(valeur, alloc, false, contexte.block_courant());
	}

	contexte.ajoute_donnees_fonctions(m_chaine, donnees_fonctions);

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

llvm::Value *NoeudExpression::genere_code_llvm(ContexteGenerationCode &/*contexte*/)
{
	return nullptr;
}

int NoeudExpression::calcul_type(ContexteGenerationCode &/*contexte*/)
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
	auto valeur = contexte.valeur_locale(m_chaine);

	if (valeur != nullptr) {
		throw "Variable redéfinie !";
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
	auto alloc = new llvm::AllocaInst(type_llvm, m_chaine, contexte.block_courant());
	new llvm::StoreInst(valeur, alloc, false, contexte.block_courant());

	contexte.pousse_locale(m_chaine, alloc, this->type);

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
	llvm::Value *valeur = contexte.valeur_locale(m_chaine);

	if (valeur == nullptr) {
		throw "Variable inconnue";
	}

	return new llvm::LoadInst(valeur, "", false, contexte.block_courant());
}

int NoeudVariable::calcul_type(ContexteGenerationCode &contexte)
{
	return contexte.type_locale(m_chaine);
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

	if (m_enfants[0]->calcul_type(contexte) != m_enfants[1]->calcul_type(contexte)) {
		throw "Les types de l'opération sont différents !";
	}

	auto valeur1 = m_enfants[0]->genere_code_llvm(contexte);
	auto valeur2 = m_enfants[1]->genere_code_llvm(contexte);

	return llvm::BinaryOperator::Create(instr, valeur1, valeur2, "", contexte.block_courant());
}

int NoeudOperation::calcul_type(ContexteGenerationCode &contexte)
{
	return m_enfants[0]->calcul_type(contexte);
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

int NoeudRetour::calcul_type(ContexteGenerationCode &contexte)
{
	if (m_enfants.size() == 0) {
		return ID_RIEN;
	}

	return m_enfants[0]->calcul_type(contexte);
}
