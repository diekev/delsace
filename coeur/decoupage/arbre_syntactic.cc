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
#include "donnees_type.h"
#include "erreur.h"
#include "morceaux.h"
#include "nombres.h"
#include "tampon_source.h"

/* ************************************************************************** */

static llvm::Type *converti_type(
		llvm::LLVMContext &contexte,
		const DonneesType &donnees_type)
{
	llvm::Type *type = nullptr;

	for (int identifiant : donnees_type) {
		switch (identifiant & 0xff) {
			case ID_BOOL:
				type = llvm::Type::getInt1Ty(contexte);
				break;
			case ID_E8:
				type = llvm::Type::getInt8Ty(contexte);
				break;
			case ID_E16:
				type = llvm::Type::getInt16Ty(contexte);
				break;
			case ID_E32:
				type = llvm::Type::getInt32Ty(contexte);
				break;
			case ID_E64:
				type = llvm::Type::getInt64Ty(contexte);
				break;
			case ID_R16:
				type = llvm::Type::getHalfTy(contexte);
				break;
			case ID_R32:
				type = llvm::Type::getFloatTy(contexte);
				break;
			case ID_R64:
				type = llvm::Type::getDoubleTy(contexte);
				break;
			case ID_RIEN:
				type = llvm::Type::getVoidTy(contexte);
				break;
			case ID_POINTEUR:
				type = llvm::PointerType::get(type, 0);
				break;
			case ID_TABLEAU:
			{
				const auto taille = static_cast<uint64_t>(identifiant) & 0xffffff00;
				type = llvm::ArrayType::get(type, taille >> 8);
				break;
			}
		}
	}

	return type;
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

void Noeud::reserve_enfants(size_t n)
{
	m_enfants.reserve(n);
}

void Noeud::ajoute_noeud(Noeud *noeud)
{
	m_enfants.push_back(noeud);
}

const DonneesType &Noeud::calcul_type(ContexteGenerationCode &/*contexte*/)
{
	return this->donnees_type;
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
					m_donnees_morceaux,
					erreur::FONCTION_INCONNUE);
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

				const auto type_arg = pair.second.donnees_type;
				const auto type_enf = enfant->calcul_type(contexte);

				if (type_arg != type_enf) {
					erreur::lance_erreur_type_arguments(
								type_arg.type_base(),
								type_enf.type_base(),
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

			const auto type_arg = iter->second.donnees_type;
			const auto type_enf = m_enfants[index]->calcul_type(contexte);

			if (type_arg != type_enf) {
				erreur::lance_erreur_type_arguments(
							type_arg.type_base(),
							type_enf.type_base(),
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

const DonneesType &NoeudAppelFonction::calcul_type(ContexteGenerationCode &contexte)
{
	if (this->donnees_type.est_invalide()) {
		auto donnees_fonction = contexte.donnees_fonction(m_donnees_morceaux.chaine);
		this->donnees_type = donnees_fonction.donnees_type;
	}

	return this->donnees_type;
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
		return converti_type(contexte.contexte, donnees.donnees_type);
	});

	llvm::ArrayRef<llvm::Type*> args(parametres);

	/* À FAIRE : calcule type retour, considération fonction récursive. */
	if (this->donnees_type.est_invalide()) {
		this->donnees_type.pousse(ID_RIEN);
	}

	/* Crée fonction */
	auto type_fonction = llvm::FunctionType::get(
							 converti_type(contexte.contexte, this->donnees_type),
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
						 converti_type(contexte.contexte, argument.donnees_type),
						 argument.chaine,
						 contexte.block_courant());

		contexte.pousse_locale(argument.chaine, alloc, argument.donnees_type);

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

const DonneesType &NoeudExpression::calcul_type(ContexteGenerationCode &/*contexte*/)
{
	return this->donnees_type;
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
		erreur::lance_erreur(
					"Redéfinition de la variable !",
					contexte.tampon,
					m_donnees_morceaux,
					erreur::VARIABLE_REDEFINIE);
	}
	else {
		valeur = contexte.valeur_globale(m_donnees_morceaux.chaine);

		if (valeur != nullptr) {
			erreur::lance_erreur(
						"Redéfinition de la variable !",
						contexte.tampon,
						m_donnees_morceaux,
						erreur::VARIABLE_REDEFINIE);
		}
	}

	assert(m_enfants.size() == 1);

	if (this->donnees_type.est_invalide()) {
		this->donnees_type = m_enfants[0]->calcul_type(contexte);

		if (this->donnees_type.est_invalide()) {
			erreur::lance_erreur(
						"Impossible de définir le type de la variable !",
						contexte.tampon,
						m_donnees_morceaux,
						erreur::TYPE_INCONNU);
		}

		if (this->donnees_type.type_base() == ID_RIEN) {
			erreur::lance_erreur(
						"Impossible d'assigner une expression de type 'rien' à une variable !",
						contexte.tampon,
						m_donnees_morceaux,
						erreur::ASSIGNATION_RIEN);
		}
	}

	/* Génère d'abord le code de l'enfant afin que l'instruction d'allocation de
	 * la variable sur la pile et celle de stockage de la valeur soit côte à
	 * côte. */
	valeur = m_enfants[0]->genere_code_llvm(contexte);

	auto type_llvm = converti_type(contexte.contexte, this->donnees_type);
	auto alloc = new llvm::AllocaInst(type_llvm, std::string(m_donnees_morceaux.chaine), contexte.block_courant());
	new llvm::StoreInst(valeur, alloc, false, contexte.block_courant());

	contexte.pousse_locale(m_donnees_morceaux.chaine, alloc, this->donnees_type);

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
		erreur::lance_erreur(
					"Redéfinition de la variable globale !",
					contexte.tampon,
					m_donnees_morceaux,
					erreur::VARIABLE_REDEFINIE);
	}

	if (this->donnees_type.est_invalide()) {
		this->donnees_type = m_enfants[0]->calcul_type(contexte);

		if (this->donnees_type.est_invalide()) {
			erreur::lance_erreur(
						"Impossible de définir le type de la variable globale !",
						contexte.tampon,
						m_donnees_morceaux,
						erreur::TYPE_INCONNU);
		}
	}

	valeur = m_enfants[0]->genere_code_llvm(contexte);

	contexte.pousse_globale(m_donnees_morceaux.chaine, valeur, this->donnees_type);

	return valeur;
}

const DonneesType &NoeudConstante::calcul_type(ContexteGenerationCode &contexte)
{
	return contexte.type_globale(m_donnees_morceaux.chaine);
}

/* ************************************************************************** */

NoeudNombreEntier::NoeudNombreEntier(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{
	this->donnees_type.pousse(ID_E32);
}

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

const DonneesType &NoeudNombreEntier::calcul_type(ContexteGenerationCode &/*contexte*/)
{
	return this->donnees_type;
}

/* ************************************************************************** */

NoeudNombreReel::NoeudNombreReel(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{
	this->donnees_type.pousse(ID_R64);
}

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

const DonneesType &NoeudNombreReel::calcul_type(ContexteGenerationCode &/*contexte*/)
{
	return this->donnees_type;
}

/* ************************************************************************** */

NoeudChaineLitterale::NoeudChaineLitterale(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{
	this->donnees_type.pousse(ID_TABLEAU | static_cast<int>(m_donnees_morceaux.chaine.size() << 8));
	this->donnees_type.pousse(ID_E8);
}

void NoeudChaineLitterale::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudChaineLitterale : " << m_donnees_morceaux.chaine << '\n';
}

llvm::Value *NoeudChaineLitterale::genere_code_llvm(ContexteGenerationCode &contexte)
{
	auto constante = llvm::ConstantDataArray::getString(
						 contexte.contexte,
						 std::string(m_donnees_morceaux.chaine));

	auto type = converti_type(contexte.contexte, this->donnees_type);

	return new llvm::GlobalVariable(
				*contexte.module,
				type,
				true,
				llvm::GlobalValue::InternalLinkage,
				constante);
}

const DonneesType &NoeudChaineLitterale::calcul_type(ContexteGenerationCode &/*contexte*/)
{
	return this->donnees_type;
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
			erreur::lance_erreur(
						"Variable inconnue !",
						contexte.tampon,
						m_donnees_morceaux,
						erreur::VARIABLE_INCONNUE);
		}
	}

	return new llvm::LoadInst(valeur, "", false, contexte.block_courant());
}

const DonneesType &NoeudVariable::calcul_type(ContexteGenerationCode &contexte)
{
	if (contexte.valeur_locale(m_donnees_morceaux.chaine) != nullptr) {
		return contexte.type_locale(m_donnees_morceaux.chaine);
	}

	if (contexte.valeur_globale(m_donnees_morceaux.chaine) != nullptr) {
		return contexte.type_globale(m_donnees_morceaux.chaine);
	}

	erreur::lance_erreur(
				"NoeudVariable::calcul_type : variable inconnue !",
				contexte.tampon,
				m_donnees_morceaux,
				erreur::VARIABLE_INCONNUE);
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
			{
				auto inst_load = dynamic_cast<llvm::LoadInst *>(valeur1);

				if (inst_load == nullptr) {
					/* Ne devrais pas arriver. */
					return nullptr;
				}

				return inst_load->getPointerOperand();
			}
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
			erreur::lance_erreur(
						"Les types de l'opération sont différents !",
						contexte.tampon,
						m_donnees_morceaux,
						erreur::TYPE_DIFFERENTS);
		}

		/* À FAIRE : typage */

		switch (this->m_donnees_morceaux.identifiant) {
			case ID_PLUS:
				if (est_type_entier(type1.type_base())) {
					instr = llvm::Instruction::Add;
				}
				else {
					instr = llvm::Instruction::FAdd;
				}

				break;
			case ID_MOINS:
				if (est_type_entier(type1.type_base())) {
					instr = llvm::Instruction::Sub;
				}
				else {
					instr = llvm::Instruction::FSub;
				}

				break;
			case ID_FOIS:
				if (est_type_entier(type1.type_base())) {
					instr = llvm::Instruction::Mul;
				}
				else {
					instr = llvm::Instruction::FMul;
				}

				break;
			case ID_DIVISE:
				if (est_type_entier(type1.type_base())) {
					instr = llvm::Instruction::SDiv;
				}
				else {
					instr = llvm::Instruction::FDiv;
				}

				break;
			case ID_POURCENT:
				if (est_type_entier(type1.type_base())) {
					instr = llvm::Instruction::SRem;
				}
				else {
					instr = llvm::Instruction::FRem;
				}

				break;
			case ID_DECALAGE_DROITE:
				if (!est_type_entier(type1.type_base())) {
					erreur::lance_erreur(
								"Besoin d'un type entier pour le décalage !",
								contexte.tampon,
								m_donnees_morceaux,
								erreur::TYPE_DIFFERENTS);
				}
				instr = llvm::Instruction::LShr;
				break;
			case ID_DECALAGE_GAUCHE:
				if (!est_type_entier(type1.type_base())) {
					erreur::lance_erreur(
								"Besoin d'un type entier pour le décalage !",
								contexte.tampon,
								m_donnees_morceaux,
								erreur::TYPE_DIFFERENTS);
				}
				instr = llvm::Instruction::Shl;
				break;
			case ID_ESPERLUETTE:
			case ID_ESP_ESP:
				if (!est_type_entier(type1.type_base())) {
					erreur::lance_erreur(
								"Besoin d'un type entier pour l'opération binaire !",
								contexte.tampon,
								m_donnees_morceaux,
								erreur::TYPE_DIFFERENTS);
				}
				instr = llvm::Instruction::And;
				break;
			case ID_BARRE:
			case ID_BARRE_BARRE:
				if (!est_type_entier(type1.type_base())) {
					erreur::lance_erreur(
								"Besoin d'un type entier pour l'opération binaire !",
								contexte.tampon,
								m_donnees_morceaux,
								erreur::TYPE_DIFFERENTS);
				}
				instr = llvm::Instruction::Or;
				break;
			case ID_CHAPEAU:
				if (!est_type_entier(type1.type_base())) {
					erreur::lance_erreur(
								"Besoin d'un type entier pour l'opération binaire !",
								contexte.tampon,
								m_donnees_morceaux,
								erreur::TYPE_DIFFERENTS);
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

const DonneesType &NoeudOperation::calcul_type(ContexteGenerationCode &contexte)
{
	if (m_donnees_morceaux.identifiant == ID_AROBASE) {
		this->donnees_type.pousse(ID_POINTEUR);
		this->donnees_type.pousse(m_enfants[0]->calcul_type(contexte));
		return this->donnees_type;
	}

	return m_enfants[0]->calcul_type(contexte);
}

/* ************************************************************************** */

NoeudRetour::NoeudRetour(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{
	this->donnees_type.pousse(ID_RIEN);
}

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

const DonneesType &NoeudRetour::calcul_type(ContexteGenerationCode &contexte)
{
	if (m_enfants.size() == 0) {
		return this->donnees_type;
	}

	return m_enfants[0]->calcul_type(contexte);
}
