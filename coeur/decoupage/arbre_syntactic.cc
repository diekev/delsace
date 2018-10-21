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
		ContexteGenerationCode &contexte,
		const DonneesType &donnees_type)
{
	llvm::Type *type = nullptr;

	for (int identifiant : donnees_type) {
		switch (identifiant & 0xff) {
			case ID_BOOL:
				type = llvm::Type::getInt1Ty(contexte.contexte);
				break;
			case ID_N8:
			case ID_Z8:
				/* À FAIRE : LLVM supporte les entiers non-signés ? */
				type = llvm::Type::getInt8Ty(contexte.contexte);
				break;
			case ID_N16:
			case ID_Z16:
				type = llvm::Type::getInt16Ty(contexte.contexte);
				break;
			case ID_N32:
			case ID_Z32:
				type = llvm::Type::getInt32Ty(contexte.contexte);
				break;
			case ID_N64:
			case ID_Z64:
				type = llvm::Type::getInt64Ty(contexte.contexte);
				break;
			case ID_R16:
				type = llvm::Type::getHalfTy(contexte.contexte);
				break;
			case ID_R32:
				type = llvm::Type::getFloatTy(contexte.contexte);
				break;
			case ID_R64:
				type = llvm::Type::getDoubleTy(contexte.contexte);
				break;
			case ID_RIEN:
				type = llvm::Type::getVoidTy(contexte.contexte);
				break;
			case ID_POINTEUR:
				type = llvm::PointerType::get(type, 0);
				break;
			case ID_CHAINE_CARACTERE:
			{
				const auto &id_structure = (static_cast<uint64_t>(identifiant) & 0xffffff00) >> 8;
				auto &donnees_structure = contexte.donnees_structure(id_structure);

				if (donnees_structure.type_llvm == nullptr) {
					std::vector<llvm::Type *> types_membres;
					types_membres.resize(donnees_structure.donnees_types.size());

					std::transform(donnees_structure.donnees_types.begin(),
								   donnees_structure.donnees_types.end(),
								   types_membres.begin(),
								   [&](const DonneesType &donnees)
					{
						return converti_type(contexte, donnees);
					});

					donnees_structure.type_llvm = llvm::StructType::get(
													  contexte.contexte,
													  types_membres,
													  false);
				}

				type = donnees_structure.type_llvm;
				break;
			}
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
		case ID_N8:
		case ID_N16:
		case ID_N32:
		case ID_N64:
		case ID_Z8:
		case ID_Z16:
		case ID_Z32:
		case ID_Z64:
		case ID_POINTEUR:  /* À FAIRE : sépare ça. */
			return true;
		default:
			return false;
	}
}

static bool est_type_entier_naturel(int type)
{
	switch (type) {
		case ID_N8:
		case ID_N16:
		case ID_N32:
		case ID_N64:
		case ID_POINTEUR:  /* À FAIRE : sépare ça. */
			return true;
		default:
			return false;
	}
}

static bool est_type_entier_relatif(int type)
{
	switch (type) {
		case ID_Z8:
		case ID_Z16:
		case ID_Z32:
		case ID_Z64:
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

void Noeud::reserve_enfants(size_t /*n*/)
{
	//m_enfants.reserve(n);
}

bool Noeud::est_constant() const
{
	return false;
}

const std::string_view &Noeud::chaine() const
{
	return m_donnees_morceaux.chaine;
}

bool Noeud::peut_etre_assigne(ContexteGenerationCode &/*contexte*/) const
{
	return false;
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
	return static_cast<int>(m_donnees_morceaux.identifiant);
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

int NoeudRacine::type_noeud() const
{
	return NOEUD_RACINE;
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

	const auto &donnees_fonction = contexte.donnees_fonction(m_donnees_morceaux.chaine);

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
		std::vector<Noeud *> enfants(m_noms_arguments.size());

		auto enfant = m_enfants.begin();

		for (const auto &nom : m_noms_arguments) {
			auto iter = donnees_fonction.args.find(nom);

			if (iter == donnees_fonction.args.end()) {
				erreur::lance_erreur_argument_inconnu(
							nom,
							contexte.tampon,
							m_donnees_morceaux);
			}

			const auto index_arg = iter->second.index;
			const auto type_arg = iter->second.donnees_type;
			const auto type_enf = (*enfant)->calcul_type(contexte);

			if (type_arg != type_enf) {
				erreur::lance_erreur_type_arguments(
							type_arg.type_base(),
							type_enf.type_base(),
							nom,
							contexte.tampon,
							m_donnees_morceaux);
			}

			enfants[index_arg] = *enfant;

			++enfant;
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
		const auto &donnees_fonction = contexte.donnees_fonction(m_donnees_morceaux.chaine);
		this->donnees_type = donnees_fonction.donnees_type;
	}

	return this->donnees_type;
}

void NoeudAppelFonction::ajoute_nom_argument(const std::string_view &nom)
{
	m_noms_arguments.push_back(nom);
}

int NoeudAppelFonction::type_noeud() const
{
	return NOEUD_APPEL_FONCTION;
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
		return converti_type(contexte, donnees.donnees_type);
	});

	llvm::ArrayRef<llvm::Type*> args(parametres);

	/* À FAIRE : calcule type retour, considération fonction récursive. */
	if (this->donnees_type.est_invalide()) {
		this->donnees_type.pousse(ID_RIEN);
	}

	/* Crée fonction */
	auto type_fonction = llvm::FunctionType::get(
							 converti_type(contexte, this->donnees_type),
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
						 converti_type(contexte, argument.donnees_type),
						 argument.chaine,
						 contexte.block_courant());

		contexte.pousse_locale(argument.chaine, alloc, argument.donnees_type, argument.est_variable);

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

int NoeudDeclarationFonction::type_noeud() const
{
	return NOEUD_DECLARATION_FONCTION;
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

int NoeudExpression::type_noeud() const
{
	return NOEUD_EXPRESSION;
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
	assert(m_enfants.size() == 2);

	if (!m_enfants.front()->peut_etre_assigne(contexte)) {
		erreur::lance_erreur(
					"Impossible d'assigner l'expression à la variable !",
					contexte.tampon,
					m_donnees_morceaux,
					erreur::ASSIGNATION_INVALIDE);
	}

	this->donnees_type = m_enfants.back()->calcul_type(contexte);

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

	/* Ajourne les données du premier enfant si elles sont invalides, dans le
	 * cas d'une déclaration de variable. */
	const auto type_gauche = m_enfants.front()->calcul_type(contexte);

	if (type_gauche.est_invalide()) {
		m_enfants.front()->donnees_type = this->donnees_type;
	}
	else {
		if (type_gauche != this->donnees_type) {
			erreur::lance_erreur_assignation_type_differents(
						type_gauche,
						this->donnees_type,
						contexte.tampon,
						m_donnees_morceaux);
		}
	}

	/* Génère d'abord le code de l'enfant afin que l'instruction d'allocation de
	 * la variable sur la pile et celle de stockage de la valeur soit côte à
	 * côte. */
	auto valeur = m_enfants.back()->genere_code_llvm(contexte);

	auto alloc = m_enfants.front()->genere_code_llvm(contexte);
	return new llvm::StoreInst(valeur, alloc, false, contexte.block_courant());
}

int NoeudAssignationVariable::type_noeud() const
{
	return NOEUD_ASSIGNATION_VARIABLE;
}

/* ************************************************************************** */

NoeudDeclarationVariable::NoeudDeclarationVariable(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudDeclarationVariable::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);
	os << "NoeudDeclarationVariable : " << m_donnees_morceaux.chaine << '\n';
}

llvm::Value *NoeudDeclarationVariable::genere_code_llvm(ContexteGenerationCode &contexte)
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

	auto type_llvm = converti_type(contexte, this->donnees_type);

	auto alloc = new llvm::AllocaInst(
					 type_llvm,
					 std::string(m_donnees_morceaux.chaine),
					 contexte.block_courant());

	contexte.pousse_locale(m_donnees_morceaux.chaine, alloc, this->donnees_type, this->est_variable);

	return alloc;
}

int NoeudDeclarationVariable::type_noeud() const
{
	return NOEUD_DECLARATION_VARIABLE;
}

bool NoeudDeclarationVariable::peut_etre_assigne(ContexteGenerationCode &/*contexte*/) const
{
	return true;
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
		this->donnees_type = m_enfants.front()->calcul_type(contexte);

		if (this->donnees_type.est_invalide()) {
			erreur::lance_erreur(
						"Impossible de définir le type de la variable globale !",
						contexte.tampon,
						m_donnees_morceaux,
						erreur::TYPE_INCONNU);
		}
	}

	valeur = m_enfants.front()->genere_code_llvm(contexte);

	contexte.pousse_globale(m_donnees_morceaux.chaine, valeur, this->donnees_type);

	return valeur;
}

const DonneesType &NoeudConstante::calcul_type(ContexteGenerationCode &contexte)
{
	return contexte.type_globale(m_donnees_morceaux.chaine);
}

int NoeudConstante::type_noeud() const
{
	return NOEUD_CONSTANTE;
}

/* ************************************************************************** */

NoeudNombreEntier::NoeudNombreEntier(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{
	this->donnees_type.pousse(ID_Z32);
}

void NoeudNombreEntier::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudNombreEntier : ";

	if (this->calcule) {
		os << this->valeur_entiere << '\n';
	}
	else {
		os << m_donnees_morceaux.chaine << '\n';
	}
}

llvm::Value *NoeudNombreEntier::genere_code_llvm(ContexteGenerationCode &contexte)
{
	const auto valeur = this->calcule ? this->valeur_entiere :
										converti_chaine_nombre_entier(
											m_donnees_morceaux.chaine,
											static_cast<int>(m_donnees_morceaux.identifiant));

	return llvm::ConstantInt::get(
				llvm::Type::getInt32Ty(contexte.contexte),
				static_cast<uint64_t>(valeur),
				false);
}

const DonneesType &NoeudNombreEntier::calcul_type(ContexteGenerationCode &/*contexte*/)
{
	return this->donnees_type;
}

bool NoeudNombreEntier::est_constant() const
{
	return true;
}

int NoeudNombreEntier::type_noeud() const
{
	return NOEUD_NOMBRE_ENTIER;
}

/* ************************************************************************** */

NoeudBooleen::NoeudBooleen(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{
	this->donnees_type.pousse(ID_BOOL);
}

void NoeudBooleen::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudBooleen : ";

	if (this->calcule) {
		os << ((this->valeur_boolenne) ? "vrai" : "faux") << '\n';
	}
	else {
		os << m_donnees_morceaux.chaine << '\n';
	}
}

llvm::Value *NoeudBooleen::genere_code_llvm(ContexteGenerationCode &contexte)
{
	const auto valeur = this->calcule ? this->valeur_boolenne
									  : (this->chaine() == "vrai");
	return llvm::ConstantInt::get(
				llvm::Type::getInt1Ty(contexte.contexte),
				static_cast<uint64_t>(valeur),
				false);
}

const DonneesType &NoeudBooleen::calcul_type(ContexteGenerationCode &/*contexte*/)
{
	return this->donnees_type;
}

bool NoeudBooleen::est_constant() const
{
	return true;
}

int NoeudBooleen::type_noeud() const
{
	return NOEUD_BOOLEEN;
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

	os << "NoeudNombreReel : ";

	if (this->calcule) {
		os << this->valeur_reelle << '\n';
	}
	else {
		os << m_donnees_morceaux.chaine << '\n';
	}
}

llvm::Value *NoeudNombreReel::genere_code_llvm(ContexteGenerationCode &contexte)
{
	const auto valeur = this->calcule ? this->valeur_reelle :
										converti_chaine_nombre_reel(
											m_donnees_morceaux.chaine,
											static_cast<int>(m_donnees_morceaux.identifiant));

	return llvm::ConstantFP::get(
				llvm::Type::getDoubleTy(contexte.contexte),
				valeur);
}

const DonneesType &NoeudNombreReel::calcul_type(ContexteGenerationCode &/*contexte*/)
{
	return this->donnees_type;
}

bool NoeudNombreReel::est_constant() const
{
	return true;
}

int NoeudNombreReel::type_noeud() const
{
	return NOEUD_NOMBRE_REEL;
}

/* ************************************************************************** */

NoeudChaineLitterale::NoeudChaineLitterale(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{
	this->donnees_type.pousse(ID_TABLEAU | static_cast<int>(m_donnees_morceaux.chaine.size() << 8));
	this->donnees_type.pousse(ID_N8);
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

	auto type = converti_type(contexte, this->donnees_type);

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

int NoeudChaineLitterale::type_noeud() const
{
	return NOEUD_CHAINE_LITTERALE;
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

int NoeudVariable::type_noeud() const
{
	return NOEUD_VARIABLE;
}

bool NoeudVariable::peut_etre_assigne(ContexteGenerationCode &contexte) const
{
	return contexte.peut_etre_assigne(m_donnees_morceaux.chaine);
}

/* ************************************************************************** */

NoeudAccesMembre::NoeudAccesMembre(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudAccesMembre::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudAccesVariable : " << m_donnees_morceaux.chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudAccesMembre::genere_code_llvm(ContexteGenerationCode &contexte)
{
	const auto &type_structure = m_enfants.back()->calcul_type(contexte);
	const auto &nom_membre = m_enfants.front()->chaine();

	auto &donnees_structure = contexte.donnees_structure(size_t(type_structure.type_base() >> 8));

	const auto iter = donnees_structure.index_membres.find(nom_membre);

	if (iter == donnees_structure.index_membres.end()) {
		/* À FAIRE : proposer des candidats possibles ou imprimer la structure. */
		erreur::lance_erreur(
					"Membre inconnu",
					contexte.tampon,
					m_donnees_morceaux,
					erreur::MEMBRE_INCONNU);
	}

	const auto index_membre = iter->second;

	auto valeur = m_enfants.back()->genere_code_llvm(contexte);

	return llvm::ExtractValueInst::Create(
				valeur, {static_cast<unsigned>(index_membre)}, "", contexte.block_courant());
}

const DonneesType &NoeudAccesMembre::calcul_type(ContexteGenerationCode &contexte)
{
	const auto &type_structure = m_enfants.back()->calcul_type(contexte);
	auto &donnees_structure = contexte.donnees_structure(size_t(type_structure.type_base() >> 8));
	const auto &nom_membre = m_enfants.front()->chaine();

	const auto iter = donnees_structure.index_membres.find(nom_membre);

	if (iter == donnees_structure.index_membres.end()) {
		/* À FAIRE : proposer des candidats possibles ou imprimer la structure. */
		erreur::lance_erreur(
					"Membre inconnu",
					contexte.tampon,
					m_donnees_morceaux,
					erreur::MEMBRE_INCONNU);
	}

	const auto index_membre = iter->second;

	return donnees_structure.donnees_types[index_membre];
}

int NoeudAccesMembre::type_noeud() const
{
	return NOEUD_ACCES_MEMBRE;
}

bool NoeudAccesMembre::peut_etre_assigne(ContexteGenerationCode &contexte) const
{
	return m_enfants.back()->peut_etre_assigne(contexte);
}

/* ************************************************************************** */

NoeudOperation::NoeudOperation(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudOperation::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudOperation : " << m_donnees_morceaux.chaine << " : " << this->donnees_type << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudOperation::genere_code_llvm(ContexteGenerationCode &contexte)
{
	if (m_enfants.size() == 1) {
		llvm::Instruction::BinaryOps instr;
		auto valeur1 = m_enfants.front()->genere_code_llvm(contexte);
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
		auto instr = llvm::Instruction::Add;
		auto predicat = llvm::CmpInst::Predicate::FCMP_FALSE;
		auto est_comp_entier = false;
		auto est_comp_reel = false;

		const auto type1 = m_enfants.front()->calcul_type(contexte);
		const auto type2 = m_enfants.back()->calcul_type(contexte);

		if (this->m_donnees_morceaux.identifiant != ID_CROCHET_OUVRANT && type1 != type2) {
			erreur::lance_erreur(
						"Les types de l'opération sont différents !",
						contexte.tampon,
						m_donnees_morceaux,
						erreur::TYPE_DIFFERENTS);
		}

		/* À FAIRE : typage */

		auto valeur1 = m_enfants.front()->genere_code_llvm(contexte);
		auto valeur2 = m_enfants.back()->genere_code_llvm(contexte);

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
				if (est_type_entier_naturel(type1.type_base())) {
					instr = llvm::Instruction::UDiv;
				}
				else if (est_type_entier_relatif(type1.type_base())) {
					instr = llvm::Instruction::SDiv;
				}
				else {
					instr = llvm::Instruction::FDiv;
				}

				break;
			case ID_POURCENT:
				if (est_type_entier_naturel(type1.type_base())) {
					instr = llvm::Instruction::URem;
				}
				else if (est_type_entier_relatif(type1.type_base())) {
					instr = llvm::Instruction::SRem;
				}
				else {
					instr = llvm::Instruction::FRem;
				}

				break;
			case ID_DECALAGE_DROITE:
				if (est_type_entier_naturel(type1.type_base())) {
					instr = llvm::Instruction::LShr;
				}
				else if (est_type_entier_relatif(type1.type_base())) {
					instr = llvm::Instruction::AShr;
				}
				else {
					erreur::lance_erreur(
								"Besoin d'un type entier pour le décalage !",
								contexte.tampon,
								m_donnees_morceaux,
								erreur::TYPE_DIFFERENTS);
				}
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
				if (est_type_entier_naturel(type1.type_base())) {
					est_comp_entier = true;
					predicat = llvm::CmpInst::Predicate::ICMP_ULT;
				}
				else if (est_type_entier_relatif(type1.type_base())) {
					est_comp_entier = true;
					predicat = llvm::CmpInst::Predicate::ICMP_SLT;
				}
				else {
					est_comp_reel = true;
					predicat = llvm::CmpInst::Predicate::FCMP_OLT;
				}

				break;
			case ID_INFERIEUR_EGAL:
				if (est_type_entier_naturel(type1.type_base())) {
					est_comp_entier = true;
					predicat = llvm::CmpInst::Predicate::ICMP_ULE;
				}
				else if (est_type_entier_relatif(type1.type_base())) {
					est_comp_entier = true;
					predicat = llvm::CmpInst::Predicate::ICMP_SLE;
				}
				else {
					est_comp_reel = true;
					predicat = llvm::CmpInst::Predicate::FCMP_OLE;
				}

				break;
			case ID_SUPERIEUR:
				if (est_type_entier_naturel(type1.type_base())) {
					est_comp_entier = true;
					predicat = llvm::CmpInst::Predicate::ICMP_UGT;
				}
				else if (est_type_entier_relatif(type1.type_base())) {
					est_comp_entier = true;
					predicat = llvm::CmpInst::Predicate::ICMP_SGT;
				}
				else {
					est_comp_reel = true;
					predicat = llvm::CmpInst::Predicate::FCMP_OGT;
				}

				break;
			case ID_SUPERIEUR_EGAL:
				if (est_type_entier_naturel(type1.type_base())) {
					est_comp_entier = true;
					predicat = llvm::CmpInst::Predicate::ICMP_UGE;
				}
				else if (est_type_entier_relatif(type1.type_base())) {
					est_comp_entier = true;
					predicat = llvm::CmpInst::Predicate::ICMP_SGE;
				}
				else {
					est_comp_reel = true;
					predicat = llvm::CmpInst::Predicate::FCMP_OGE;
				}

				break;
			case ID_EGALITE:
				if (est_type_entier(type1.type_base())) {
					est_comp_entier = true;
					predicat = llvm::CmpInst::Predicate::ICMP_EQ;
				}
				else {
					est_comp_reel = true;
					predicat = llvm::CmpInst::Predicate::FCMP_OEQ;
				}

				break;
			case ID_DIFFERENCE:
				if (est_type_entier(type1.type_base())) {
					est_comp_entier = true;
					predicat = llvm::CmpInst::Predicate::ICMP_NE;
				}
				else {
					est_comp_reel = true;
					predicat = llvm::CmpInst::Predicate::FCMP_ONE;
				}

				break;
			case ID_CROCHET_OUVRANT:
				if (type2.type_base() != ID_POINTEUR && (type2.type_base() & 0xff) != ID_TABLEAU) {
					erreur::lance_erreur(
								"Le type ne peut être déréférencé !",
								contexte.tampon,
								m_donnees_morceaux,
								erreur::TYPE_DIFFERENTS);
				}

				return llvm::GetElementPtrInst::Create(
							converti_type(contexte, this->donnees_type),
							valeur2,
							{ valeur1 },
							"",
							contexte.block_courant());
			default:
				return nullptr;
		}

		if (est_comp_entier) {
			return llvm::ICmpInst::Create(llvm::Instruction::ICmp, predicat, valeur1, valeur2, "", contexte.block_courant());
		}

		if (est_comp_reel) {
			return llvm::FCmpInst::Create(llvm::Instruction::FCmp, predicat, valeur1, valeur2, "", contexte.block_courant());
		}

		return llvm::BinaryOperator::Create(instr, valeur1, valeur2, "", contexte.block_courant());
	}

	return nullptr;
}

const DonneesType &NoeudOperation::calcul_type(ContexteGenerationCode &contexte)
{
	if (this->donnees_type.est_invalide()) {
		if (m_donnees_morceaux.identifiant == ID_AROBASE) {
			this->donnees_type.pousse(ID_POINTEUR);
			this->donnees_type.pousse(m_enfants.front()->calcul_type(contexte));
		}
		else if (m_donnees_morceaux.identifiant == ID_CROCHET_OUVRANT) {
			auto donnees_enfant = m_enfants.back()->calcul_type(contexte);
			this->donnees_type = donnees_enfant.derefence();
		}
		else {
			this->donnees_type = m_enfants.front()->calcul_type(contexte);
		}
	}

	return this->donnees_type;
}

int NoeudOperation::type_noeud() const
{
	return NOEUD_OPERATION;
}

bool NoeudOperation::peut_etre_assigne(ContexteGenerationCode &contexte) const
{
	if (m_donnees_morceaux.identifiant == ID_AROBASE) {
		/* ne peut assigné dans une prise d'addresse */
		return false;
	}

	if (m_donnees_morceaux.identifiant == ID_CROCHET_OUVRANT) {
		return m_enfants.back()->peut_etre_assigne(contexte);
	}

	return false;
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

	this->calcul_type(contexte);

	if (m_enfants.size() > 0) {
		assert(m_enfants.size() == 1);
		valeur = m_enfants.front()->genere_code_llvm(contexte);
	}

	return llvm::ReturnInst::Create(contexte.contexte, valeur, contexte.block_courant());
}

const DonneesType &NoeudRetour::calcul_type(ContexteGenerationCode &contexte)
{
	if (m_enfants.size() == 0) {
		return this->donnees_type;
	}

	this->donnees_type = m_enfants.front()->calcul_type(contexte);
	return this->donnees_type;
}

int NoeudRetour::type_noeud() const
{
	return NOEUD_RETOUR;
}
