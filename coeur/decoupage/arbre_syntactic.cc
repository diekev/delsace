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

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>

#include <sstream>

#include "contexte_generation_code.h"
#include "donnees_type.h"
#include "erreur.h"
#include "morceaux.h"
#include "nombres.h"
#include "tampon_source.h"

/* ************************************************************************** */

static auto cree_bloc(ContexteGenerationCode &contexte, const char *nom)
{
	return llvm::BasicBlock::Create(contexte.contexte, nom, contexte.fonction);
}

static llvm::Type *converti_type(
		ContexteGenerationCode &contexte,
		const DonneesType &donnees_type)
{
	llvm::Type *type = nullptr;

	for (id_morceau identifiant : donnees_type) {
		switch (identifiant & 0xff) {
			case id_morceau::BOOL:
				type = llvm::Type::getInt1Ty(contexte.contexte);
				break;
			case id_morceau::N8:
			case id_morceau::Z8:
				/* À FAIRE : LLVM supporte les entiers non-signés ? */
				type = llvm::Type::getInt8Ty(contexte.contexte);
				break;
			case id_morceau::N16:
			case id_morceau::Z16:
				type = llvm::Type::getInt16Ty(contexte.contexte);
				break;
			case id_morceau::N32:
			case id_morceau::Z32:
				type = llvm::Type::getInt32Ty(contexte.contexte);
				break;
			case id_morceau::N64:
			case id_morceau::Z64:
				type = llvm::Type::getInt64Ty(contexte.contexte);
				break;
			case id_morceau::R16:
				type = llvm::Type::getHalfTy(contexte.contexte);
				break;
			case id_morceau::R32:
				type = llvm::Type::getFloatTy(contexte.contexte);
				break;
			case id_morceau::R64:
				type = llvm::Type::getDoubleTy(contexte.contexte);
				break;
			case id_morceau::RIEN:
				type = llvm::Type::getVoidTy(contexte.contexte);
				break;
			case id_morceau::POINTEUR:
				type = llvm::PointerType::get(type, 0);
				break;
			case id_morceau::CHAINE_CARACTERE:
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
			case id_morceau::TABLEAU:
			{
				const auto taille = static_cast<uint64_t>(identifiant) & 0xffffff00;
				type = llvm::ArrayType::get(type, taille >> 8);
				break;
			}
			default:
				assert(false);
		}
	}

	return type;
}

static unsigned alignement(
		ContexteGenerationCode &contexte,
		const DonneesType &donnees_type)
{
	id_morceau identifiant = donnees_type.type_base();

	switch (identifiant & 0xff) {
		case id_morceau::BOOL:
		case id_morceau::N8:
		case id_morceau::Z8:
			return 1;
		case id_morceau::R16:
		case id_morceau::N16:
		case id_morceau::Z16:
			return 2;
		case id_morceau::R32:
		case id_morceau::N32:
		case id_morceau::Z32:
			return 4;
		case id_morceau::TABLEAU:
		case id_morceau::POINTEUR:
		case id_morceau::R64:
		case id_morceau::N64:
		case id_morceau::Z64:
			return 8;
		case id_morceau::CHAINE_CARACTERE:
		{
			const auto &id_structure = (static_cast<uint64_t>(identifiant) & 0xffffff00) >> 8;
			auto &donnees_structure = contexte.donnees_structure(id_structure);

			std::vector<llvm::Type *> types_membres;
			types_membres.resize(donnees_structure.donnees_types.size());

			auto a = 0u;

			for (const auto &donnees : donnees_structure.donnees_types) {
				a = std::max(a, alignement(contexte, donnees));
			}

			return a;
		}
		default:
			assert(false);
	}

	return 0;
}

/**
 * Retourne vrai si le premier type est un pointeur, le deuxième un tableau, et
 * leurs types déréférencés sont égaux.
 */
static bool sont_pointeur_tableau_compatibles(
		const DonneesType &type1,
		const DonneesType &type2)
{
	if (type1.type_base() != id_morceau::POINTEUR) {
		return false;
	}

	if ((type2.type_base() & 0xff) != id_morceau::TABLEAU) {
		return false;
	}

	return type1.derefence() == type2.derefence();
}

/**
 * Retourne vrai si les deux types peuvent être convertis silencieusement par le
 * compileur.
 */
static bool sont_compatibles(
		const DonneesType &type1,
		const DonneesType &type2)
{
	if (type1 == type2) {
		return true;
	}

	if (sont_pointeur_tableau_compatibles(type1, type2)) {
		return true;
	}

	if (sont_pointeur_tableau_compatibles(type2, type1)) {
		return true;
	}

	return false;
}

static bool est_type_entier(id_morceau type)
{
	switch (type) {
		case id_morceau::BOOL:
		case id_morceau::N8:
		case id_morceau::N16:
		case id_morceau::N32:
		case id_morceau::N64:
		case id_morceau::Z8:
		case id_morceau::Z16:
		case id_morceau::Z32:
		case id_morceau::Z64:
		case id_morceau::POINTEUR:  /* À FAIRE : sépare ça. */
			return true;
		default:
			return false;
	}
}

static bool est_type_entier_naturel(id_morceau type)
{
	switch (type) {
		case id_morceau::N8:
		case id_morceau::N16:
		case id_morceau::N32:
		case id_morceau::N64:
		case id_morceau::POINTEUR:  /* À FAIRE : sépare ça. */
			return true;
		default:
			return false;
	}
}

static bool est_type_entier_relatif(id_morceau type)
{
	switch (type) {
		case id_morceau::Z8:
		case id_morceau::Z16:
		case id_morceau::Z32:
		case id_morceau::Z64:
			return true;
		default:
			return false;
	}
}

static size_t taille_de(id_morceau type)
{
	switch (type) {
		case id_morceau::BOOL:
			return 1;
		case id_morceau::N8:
		case id_morceau::Z8:
			return 8;
		case id_morceau::N16:
		case id_morceau::R16:
		case id_morceau::Z16:
			return 16;
		case id_morceau::N32:
		case id_morceau::R32:
		case id_morceau::Z32:
			return 32;
		case id_morceau::N64:
		case id_morceau::R64:
		case id_morceau::Z64:
		case id_morceau::POINTEUR:
			return 64;
		default:
			return 0ul;
	}
}

static bool est_plus_petit(id_morceau type1, id_morceau type2)
{
	return taille_de(type1) < taille_de(type2);
}

static bool est_type_reel(id_morceau type)
{
	switch (type) {
		case id_morceau::R16:
		case id_morceau::R32:
		case id_morceau::R64:
			return true;
		default:
			return false;
	}
}

/**
 * Retourne vrai si le type à droite peut-être assigné au type à gauche. Si les
 * types ne correspondent pas directement, on vérifie s'il est possible de
 * convertir silencieusement les types littéraux.
 */
static bool peut_assigner(
		const DonneesType &gauche,
		const DonneesType &droite,
		type_noeud type_droite)
{
	if (gauche == droite) {
		return true;
	}

	if (type_droite == type_noeud::NOMBRE_ENTIER && est_type_entier(gauche.type_base())) {
		return true;
	}

	if (type_droite == type_noeud::NOMBRE_REEL && est_type_reel(gauche.type_base())) {
		return true;
	}

	return false;
}

/* ************************************************************************** */

static void imprime_tab(std::ostream &os, int tab)
{
	for (int i = 0; i < tab; ++i) {
		os << ' ' << ' ';
	}
}

static bool est_branche_ou_retour(llvm::Value *valeur)
{
	return (valeur != nullptr) && (llvm::isa<llvm::BranchInst>(*valeur) || llvm::isa<llvm::ReturnInst>(*valeur));
}

char caractere_echape(const char *sequence)
{
	switch (sequence[0]) {
		case '\\':
			switch (sequence[1]) {
				case '\\':
					return '\\';
				case '\'':
					return '\'';
				case 'a':
					return '\a';
				case 'b':
					return '\b';
				case 'f':
					return '\f';
				case 'n':
					return '\n';
				case 'r':
					return '\r';
				case 't':
					return '\t';
				case 'v':
					return '\v';
				case '0':
					return '\0';
				default:
					return sequence[1];
			}
		default:
			return sequence[0];
	}
}

/* ************************************************************************** */

static llvm::FunctionType *obtiens_type_fonction(
		ContexteGenerationCode &contexte,
		std::list<ArgumentFonction> donnees_args,
		const DonneesType &donnees_retour,
		bool est_variadique)
{
	std::vector<llvm::Type *> parametres(donnees_args.size());

	std::transform(donnees_args.begin(), donnees_args.end(), parametres.begin(),
				   [&](const ArgumentFonction &donnees)
	{
		return converti_type(contexte, donnees.donnees_type);
	});

	llvm::ArrayRef<llvm::Type *> args(parametres);

	return llvm::FunctionType::get(
				converti_type(contexte, donnees_retour),
				args,
				est_variadique);
}

static void ajoute_printf(ContexteGenerationCode &contexte)
{
	std::list<ArgumentFonction> donnees_args;
	auto donnees_arg = ArgumentFonction{};
	donnees_arg.donnees_type.pousse(id_morceau::POINTEUR);
	donnees_arg.donnees_type.pousse(id_morceau::Z8);
	donnees_args.push_back(donnees_arg);

	auto donnees_retour = DonneesType{};
	donnees_retour.pousse(id_morceau::Z32);

	auto type_printf = obtiens_type_fonction(
						   contexte,
						   donnees_args,
						   donnees_retour,
						   true);

	llvm::Function::Create(
				type_printf,
				llvm::Function::ExternalLinkage,
				"printf",
				contexte.module);
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

const DonneesMorceaux &Noeud::donnees_morceau() const
{
	return m_donnees_morceaux;
}

Noeud *Noeud::dernier_enfant() const
{
	if (m_enfants.empty()) {
		return nullptr;
	}

	return m_enfants.back();
}

void Noeud::ajoute_noeud(Noeud *noeud)
{
	m_enfants.push_back(noeud);
}

const DonneesType &Noeud::calcul_type(ContexteGenerationCode &/*contexte*/)
{
	return this->donnees_type;
}

id_morceau Noeud::identifiant() const
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

llvm::Value *NoeudRacine::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	ajoute_printf(contexte);

	for (auto noeud : m_enfants) {
		noeud->genere_code_llvm(contexte);
	}

	return nullptr;
}

type_noeud NoeudRacine::type() const
{
	return type_noeud::RACINE;
}

/* ************************************************************************** */

NoeudAppelFonction::NoeudAppelFonction(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{
	/* réutilisation du membre std::any pour économiser un peu de mémoire */
	valeur_calculee = std::list<std::string_view>{};
}

void NoeudAppelFonction::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudAppelFonction : " << m_donnees_morceaux.chaine << '\n';
	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudAppelFonction::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	auto fonction = contexte.module->getFunction(std::string(m_donnees_morceaux.chaine));

	if (fonction == nullptr) {
		erreur::lance_erreur(
					"Fonction inconnue",
					contexte.tampon,
					m_donnees_morceaux,
					erreur::type_erreur::FONCTION_INCONNUE);
	}

	const auto &donnees_fonction = contexte.donnees_fonction(m_donnees_morceaux.chaine);

	if (!fonction->isVarArg() && m_enfants.size() != donnees_fonction.args.size()) {
		erreur::lance_erreur_nombre_arguments(
					donnees_fonction.args.size(),
					m_enfants.size(),
					contexte.tampon,
					m_donnees_morceaux);
	}

	/* Cherche la liste d'arguments */
	std::vector<llvm::Value *> parametres;

	auto noms_arguments = std::any_cast<std::list<std::string_view>>(&valeur_calculee);

	if (noms_arguments->empty()) {
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

				if (!sont_compatibles(type_arg, type_enf)) {
					erreur::lance_erreur_type_arguments(
								type_arg,
								type_enf,
								contexte.tampon,
								enfant->donnees_morceau(),
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

		if (noms_arguments->size() != donnees_fonction.args.size()) {
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
		std::vector<Noeud *> enfants(noms_arguments->size());

		auto enfant = m_enfants.begin();

		for (const auto &nom : *noms_arguments) {
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

			if (!sont_compatibles(type_arg, type_enf)) {
				erreur::lance_erreur_type_arguments(
							type_arg,
							type_enf,
							contexte.tampon,
							(*enfant)->donnees_morceau(),
							m_donnees_morceaux);
			}

			enfants[index_arg] = *enfant;

			++enfant;
		}

		parametres.resize(noms_arguments->size());

		std::transform(enfants.begin(), enfants.end(), parametres.begin(),
					   [&](Noeud *enfant)
		{
			return enfant->genere_code_llvm(contexte);
		});
	}

	llvm::ArrayRef<llvm::Value *> args(parametres);

	return llvm::CallInst::Create(fonction, args, "", contexte.bloc_courant());
}

const DonneesType &NoeudAppelFonction::calcul_type(ContexteGenerationCode &contexte)
{
	auto fonction = contexte.module->getFunction(std::string(m_donnees_morceaux.chaine));

	if (fonction == nullptr) {
		erreur::lance_erreur(
					"Fonction inconnue",
					contexte.tampon,
					m_donnees_morceaux,
					erreur::type_erreur::FONCTION_INCONNUE);
	}

	if (this->donnees_type.est_invalide()) {
		const auto &donnees_fonction = contexte.donnees_fonction(m_donnees_morceaux.chaine);
		this->donnees_type = donnees_fonction.donnees_type;
	}

	return this->donnees_type;
}

void NoeudAppelFonction::ajoute_nom_argument(const std::string_view &nom)
{
	auto noms_arguments = std::any_cast<std::list<std::string_view>>(&valeur_calculee);
	noms_arguments->push_back(nom);
}

type_noeud NoeudAppelFonction::type() const
{
	return type_noeud::APPEL_FONCTION;
}

/* ************************************************************************** */

NoeudDeclarationFonction::NoeudDeclarationFonction(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{
	/* réutilisation du membre std::any pour économiser un peu de mémoire */
	valeur_calculee = std::list<ArgumentFonction>{};
}

void NoeudDeclarationFonction::ajoute_argument(const ArgumentFonction &argument)
{
	auto arguments = std::any_cast<std::list<ArgumentFonction>>(&valeur_calculee);
	arguments->push_back(argument);
}

void NoeudDeclarationFonction::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudDeclarationFonction : " << m_donnees_morceaux.chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudDeclarationFonction::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	/* À FAIRE : inférence de type
	 * - considération du type de retour des fonctions récursive
	 * - il est possible que le retour dépende des variables locales de la
	 *   fonction, donc il faut d'abord générer le code ou faire une prépasse
	 *   pour générer les données nécessaires.
	 */

	auto arguments = std::any_cast<std::list<ArgumentFonction>>(&valeur_calculee);

	/* Crée le type de la fonction */
	auto type_fonction = obtiens_type_fonction(
							 contexte,
							 *arguments,
							 this->donnees_type,
							 false);

	/* Crée fonction */
	auto fonction = llvm::Function::Create(
						type_fonction,
						llvm::Function::ExternalLinkage,
						std::string(m_donnees_morceaux.chaine),
						contexte.module);

	contexte.commence_fonction(fonction);

	auto block = cree_bloc(contexte, "entree");

	contexte.bloc_courant(block);

	/* Crée code pour les arguments */
	auto valeurs_args = fonction->arg_begin();

	for (const auto &argument : *arguments) {
		auto alloc = new llvm::AllocaInst(
						 converti_type(contexte, argument.donnees_type),
						 argument.chaine,
						 contexte.bloc_courant());

		alloc->setAlignment(alignement(contexte, argument.donnees_type));

		contexte.pousse_locale(argument.chaine, alloc, argument.donnees_type, argument.est_variable);

		llvm::Value *valeur = &*valeurs_args++;
		valeur->setName(argument.chaine.c_str());

		new llvm::StoreInst(valeur, alloc, false, contexte.bloc_courant());
	}

	/* Crée code pour le bloc. */
	auto bloc = m_enfants.front();

	auto ret = bloc->genere_code_llvm(contexte);

	/* Ajoute une instruction de retour si la dernière n'en est pas une. */
	if (!est_branche_ou_retour(ret)) {
		llvm::ReturnInst::Create(
					contexte.contexte,
					nullptr,
					contexte.bloc_courant());
	}

	/* vérifie le type du bloc */
	auto type_bloc = bloc->calcul_type(contexte);
	auto dernier = bloc->dernier_enfant();

	/* si le bloc est vide -> vérifie qu'aucun type n'a été spécifié */
	if (dernier == nullptr) {
		if (this->donnees_type.type_base() != id_morceau::RIEN) {
			erreur::lance_erreur(
						"Instruction de retour manquante",
						contexte.tampon,
						m_donnees_morceaux,
						erreur::type_erreur::TYPE_DIFFERENTS);
		}
	}
	/* si le bloc n'est pas vide */
	else {
		/* si le dernier noeud n'est pas un noeud de retour -> vérifie qu'aucun type n'a été spécifié */
		if (dernier->type() != type_noeud::RETOUR) {
			if (this->donnees_type.type_base() != id_morceau::RIEN) {
				erreur::lance_erreur(
							"Instruction de retour manquante",
							contexte.tampon,
							m_donnees_morceaux,
							erreur::type_erreur::TYPE_DIFFERENTS);
			}
		}
		/* vérifie que le type du bloc correspond au type de la fonction */
		else {
			if (this->donnees_type != type_bloc) {
				erreur::lance_erreur(
							"Le type de retour est invalide",
							contexte.tampon,
							m_donnees_morceaux,
							erreur::type_erreur::TYPE_DIFFERENTS);
			}
		}
	}

	contexte.termine_fonction();

	/* optimise la fonction */
	if (contexte.menageur_pass_fonction != nullptr) {
		contexte.menageur_pass_fonction->run(*fonction);
	}

	return nullptr;
}

type_noeud NoeudDeclarationFonction::type() const
{
	return type_noeud::DECLARATION_FONCTION;
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

llvm::Value *NoeudAssignationVariable::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	assert(m_enfants.size() == 2);

	auto variable = m_enfants.front();

	if (!variable->peut_etre_assigne(contexte)) {
		erreur::lance_erreur(
					"Impossible d'assigner l'expression à la variable !",
					contexte.tampon,
					m_donnees_morceaux,
					erreur::type_erreur::ASSIGNATION_INVALIDE);
	}

	this->donnees_type = m_enfants.back()->calcul_type(contexte);

	if (this->donnees_type.est_invalide()) {
		erreur::lance_erreur(
					"Impossible de définir le type de la variable !",
					contexte.tampon,
					m_donnees_morceaux,
					erreur::type_erreur::TYPE_INCONNU);
	}

	if (this->donnees_type.type_base() == id_morceau::RIEN) {
		erreur::lance_erreur(
					"Impossible d'assigner une expression de type 'rien' à une variable !",
					contexte.tampon,
					m_donnees_morceaux,
					erreur::type_erreur::ASSIGNATION_RIEN);
	}

	/* Ajourne les données du premier enfant si elles sont invalides, dans le
	 * cas d'une déclaration de variable. */
	const auto type_gauche = variable->calcul_type(contexte);

	auto expression = m_enfants.back();

	if (type_gauche.est_invalide()) {
		variable->donnees_type = this->donnees_type;
	}
	else {
		if (!peut_assigner(type_gauche, this->donnees_type, expression->type())) {
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
	auto valeur = expression->genere_code_llvm(contexte);

	auto alloc = variable->genere_code_llvm(contexte, true);
	return new llvm::StoreInst(valeur, alloc, false, contexte.bloc_courant());
}

const DonneesType &NoeudAssignationVariable::calcul_type(ContexteGenerationCode &/*contexte*/)
{
	if (this->donnees_type.est_invalide()) {
		this->donnees_type.pousse(id_morceau::RIEN);
	}

	return this->donnees_type;
}

type_noeud NoeudAssignationVariable::type() const
{
	return type_noeud::ASSIGNATION_VARIABLE;
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

llvm::Value *NoeudDeclarationVariable::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	auto valeur = contexte.valeur_locale(m_donnees_morceaux.chaine);

	if (valeur != nullptr) {
		erreur::lance_erreur(
					"Redéfinition de la variable !",
					contexte.tampon,
					m_donnees_morceaux,
					erreur::type_erreur::VARIABLE_REDEFINIE);
	}
	else {
		valeur = contexte.valeur_globale(m_donnees_morceaux.chaine);

		if (valeur != nullptr) {
			erreur::lance_erreur(
						"Redéfinition de la variable !",
						contexte.tampon,
						m_donnees_morceaux,
						erreur::type_erreur::VARIABLE_REDEFINIE);
		}
	}

	auto type_llvm = converti_type(contexte, this->donnees_type);

	auto alloc = new llvm::AllocaInst(
					 type_llvm,
					 std::string(m_donnees_morceaux.chaine),
					 contexte.bloc_courant());

	alloc->setAlignment(alignement(contexte, this->donnees_type));

	contexte.pousse_locale(m_donnees_morceaux.chaine, alloc, this->donnees_type, this->est_variable);

	return alloc;
}

type_noeud NoeudDeclarationVariable::type() const
{
	return type_noeud::DECLARATION_VARIABLE;
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

llvm::Value *NoeudConstante::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	auto valeur = contexte.valeur_globale(m_donnees_morceaux.chaine);

	if (valeur != nullptr) {
		erreur::lance_erreur(
					"Redéfinition de la variable globale !",
					contexte.tampon,
					m_donnees_morceaux,
					erreur::type_erreur::VARIABLE_REDEFINIE);
	}

	if (this->donnees_type.est_invalide()) {
		this->donnees_type = m_enfants.front()->calcul_type(contexte);

		if (this->donnees_type.est_invalide()) {
			erreur::lance_erreur(
						"Impossible de définir le type de la variable globale !",
						contexte.tampon,
						m_donnees_morceaux,
						erreur::type_erreur::TYPE_INCONNU);
		}
	}

	/* À FAIRE : énumération avec des expressions contenant d'autres énums.
	 * différents types (réel, bool, etc..)
	 */

	auto n = converti_chaine_nombre_entier(
				 m_enfants.front()->chaine(),
				 m_enfants.front()->identifiant());

	auto constante = llvm::ConstantInt::get(
						 converti_type(contexte, this->donnees_type),
						 static_cast<uint64_t>(n));

	valeur = new llvm::GlobalVariable(
				 *contexte.module,
				 converti_type(contexte, this->donnees_type),
				 true,
				 llvm::GlobalValue::InternalLinkage,
				 constante);

	contexte.pousse_globale(m_donnees_morceaux.chaine, valeur, this->donnees_type);

	return valeur;
}

const DonneesType &NoeudConstante::calcul_type(ContexteGenerationCode &contexte)
{
	return contexte.type_globale(m_donnees_morceaux.chaine);
}

type_noeud NoeudConstante::type() const
{
	return type_noeud::CONSTANTE;
}

/* ************************************************************************** */

NoeudNombreEntier::NoeudNombreEntier(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{
	this->donnees_type.pousse(id_morceau::Z32);
}

void NoeudNombreEntier::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudNombreEntier : ";

	if (this->calcule) {
		os << std::any_cast<long>(this->valeur_calculee) << '\n';
	}
	else {
		os << m_donnees_morceaux.chaine << '\n';
	}
}

llvm::Value *NoeudNombreEntier::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	const auto valeur = this->calcule ? std::any_cast<long>(this->valeur_calculee) :
										converti_chaine_nombre_entier(
											m_donnees_morceaux.chaine,
											m_donnees_morceaux.identifiant);

	return llvm::ConstantInt::get(
				llvm::Type::getInt32Ty(contexte.contexte),
				static_cast<uint64_t>(valeur),
				false);
}

bool NoeudNombreEntier::est_constant() const
{
	return true;
}

type_noeud NoeudNombreEntier::type() const
{
	return type_noeud::NOMBRE_ENTIER;
}

/* ************************************************************************** */

NoeudBooleen::NoeudBooleen(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{
	this->donnees_type.pousse(id_morceau::BOOL);
}

void NoeudBooleen::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudBooleen : ";

	if (this->calcule) {
		os << ((std::any_cast<bool>(this->valeur_calculee)) ? "vrai" : "faux") << '\n';
	}
	else {
		os << m_donnees_morceaux.chaine << '\n';
	}
}

llvm::Value *NoeudBooleen::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	const auto valeur = this->calcule ? std::any_cast<bool>(this->valeur_calculee)
									  : (this->chaine() == "vrai");
	return llvm::ConstantInt::get(
				llvm::Type::getInt1Ty(contexte.contexte),
				static_cast<uint64_t>(valeur),
				false);
}

bool NoeudBooleen::est_constant() const
{
	return true;
}

type_noeud NoeudBooleen::type() const
{
	return type_noeud::BOOLEEN;
}

/* ************************************************************************** */

NoeudCaractere::NoeudCaractere(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{
	this->donnees_type.pousse(id_morceau::Z8);
}

void NoeudCaractere::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);
	os << "NoeudCaractere : " << m_donnees_morceaux.chaine << '\n';
}

llvm::Value *NoeudCaractere::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	auto valeur = caractere_echape(&m_donnees_morceaux.chaine[0]);

	return llvm::ConstantInt::get(
				llvm::Type::getInt8Ty(contexte.contexte),
				static_cast<uint64_t>(valeur),
				false);
}

bool NoeudCaractere::est_constant() const
{
	return true;
}

type_noeud NoeudCaractere::type() const
{
	return type_noeud::CARACTERE;
}

/* ************************************************************************** */

NoeudNombreReel::NoeudNombreReel(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{
	this->donnees_type.pousse(id_morceau::R64);
}

void NoeudNombreReel::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudNombreReel : ";

	if (this->calcule) {
		os << std::any_cast<double>(this->valeur_calculee) << '\n';
	}
	else {
		os << m_donnees_morceaux.chaine << '\n';
	}
}

llvm::Value *NoeudNombreReel::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	const auto valeur = this->calcule ? std::any_cast<double>(this->valeur_calculee) :
										converti_chaine_nombre_reel(
											m_donnees_morceaux.chaine,
											m_donnees_morceaux.identifiant);

	return llvm::ConstantFP::get(
				llvm::Type::getDoubleTy(contexte.contexte),
				valeur);
}

bool NoeudNombreReel::est_constant() const
{
	return true;
}

type_noeud NoeudNombreReel::type() const
{
	return type_noeud::NOMBRE_REEL;
}

/* ************************************************************************** */

NoeudChaineLitterale::NoeudChaineLitterale(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{
	/* fais en sorte que les caractères échappés ne soient pas comptés comme
	 * deux caractères distincts, ce qui ne peut se faire avec la
	 * std::string_view */
	std::string corrigee;
	corrigee.reserve(m_donnees_morceaux.chaine.size());

	for (size_t i = 0; i < m_donnees_morceaux.chaine.size(); ++i) {
		auto c = m_donnees_morceaux.chaine[i];

		if (c == '\\') {
			c = caractere_echape(&m_donnees_morceaux.chaine[i]);
			++i;
		}

		corrigee.push_back(c);
	}

	this->valeur_calculee = corrigee;

	this->donnees_type.pousse(id_morceau::TABLEAU | static_cast<int>(corrigee.size() << 8));
	this->donnees_type.pousse(id_morceau::Z8);
}

void NoeudChaineLitterale::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudChaineLitterale : ";
	if (this->calcule) {
		os << std::any_cast<std::string>(this->valeur_calculee) << '\n';
	}
	else {
		os << m_donnees_morceaux.chaine << '\n';
	}
}

llvm::Value *NoeudChaineLitterale::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	auto chaine = std::any_cast<std::string>(this->valeur_calculee);

	auto constante = llvm::ConstantDataArray::getString(
						 contexte.contexte,
						 chaine);

	auto type = converti_type(contexte, this->donnees_type);

	return new llvm::GlobalVariable(
				*contexte.module,
				type,
				true,
				llvm::GlobalValue::InternalLinkage,
				constante);
}

bool NoeudChaineLitterale::est_constant() const
{
	return true;
}

type_noeud NoeudChaineLitterale::type() const
{
	return type_noeud::CHAINE_LITTERALE;
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

llvm::Value *NoeudVariable::genere_code_llvm(ContexteGenerationCode &contexte, const bool expr_gauche)
{
	auto valeur = contexte.valeur_locale(m_donnees_morceaux.chaine);

	if (valeur == nullptr) {
		valeur = contexte.valeur_globale(m_donnees_morceaux.chaine);

		if (valeur == nullptr) {
			erreur::lance_erreur(
						"Variable inconnue !",
						contexte.tampon,
						m_donnees_morceaux,
						erreur::type_erreur::VARIABLE_INCONNUE);
		}
	}

	if (expr_gauche || dynamic_cast<llvm::PHINode *>(valeur)) {
		return valeur;
	}

	return new llvm::LoadInst(valeur, "", false, contexte.bloc_courant());
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
				erreur::type_erreur::VARIABLE_INCONNUE);
}

type_noeud NoeudVariable::type() const
{
	return type_noeud::VARIABLE;
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

llvm::Value *NoeudAccesMembre::genere_code_llvm(ContexteGenerationCode &contexte, const bool expr_gauche)
{
	auto structure = m_enfants.back();
	auto membre = m_enfants.front();

	const auto &type_structure = structure->calcul_type(contexte);

	auto index_structure = 0ul;
	auto est_pointeur = false;

	if ((type_structure.type_base() & 0xff) != id_morceau::CHAINE_CARACTERE) {
		if (type_structure.type_base() == id_morceau::POINTEUR) {
			auto deref = type_structure.derefence();

			if ((deref.type_base() & 0xff) == id_morceau::CHAINE_CARACTERE) {
				index_structure = size_t(deref.type_base() >> 8);
				est_pointeur = true;
			}
			else {
				erreur::lance_erreur(
							"Impossible d'accéder au membre d'un objet n'étant pas une structure",
							contexte.tampon,
							structure->donnees_morceau(),
							erreur::type_erreur::TYPE_DIFFERENTS);
			}
		}
		else {
			erreur::lance_erreur(
						"Impossible d'accéder au membre d'un objet n'étant pas une structure",
						contexte.tampon,
						structure->donnees_morceau(),
						erreur::type_erreur::TYPE_DIFFERENTS);
		}
	}
	else {
		index_structure = size_t(type_structure.type_base() >> 8);
	}

	const auto &nom_membre = membre->chaine();

	auto &donnees_structure = contexte.donnees_structure(index_structure);

	const auto iter = donnees_structure.index_membres.find(nom_membre);

	if (iter == donnees_structure.index_membres.end()) {
		/* À FAIRE : proposer des candidats possibles ou imprimer la structure. */
		erreur::lance_erreur(
					"Membre inconnu",
					contexte.tampon,
					m_donnees_morceaux,
					erreur::type_erreur::MEMBRE_INCONNU);
	}

	const auto index_membre = iter->second;

	auto valeur = structure->genere_code_llvm(contexte, true);

	llvm::Value *ret;

	if (est_pointeur) {
		/* déréférence le pointeur en le chargeant */
		valeur = new llvm::LoadInst(valeur, "", contexte.bloc_courant());
	}

	ret = llvm::GetElementPtrInst::CreateInBounds(
			  valeur, {
				  llvm::ConstantInt::get(llvm::Type::getInt64Ty(contexte.contexte), 0),
				  llvm::ConstantInt::get(llvm::Type::getInt32Ty(contexte.contexte), index_membre)
			  },
			  "",
			  contexte.bloc_courant());


	if (!expr_gauche) {
		ret = new llvm::LoadInst(ret, "", contexte.bloc_courant());
	}

	return ret;
}

const DonneesType &NoeudAccesMembre::calcul_type(ContexteGenerationCode &contexte)
{
	auto structure = m_enfants.back();
	auto membre = m_enfants.front();

	const auto &type_structure = structure->calcul_type(contexte);

	auto index_structure = 0ul;

	if ((type_structure.type_base() & 0xff) != id_morceau::CHAINE_CARACTERE) {
		if (type_structure.type_base() == id_morceau::POINTEUR) {
			auto deref = type_structure.derefence();

			if ((deref.type_base() & 0xff) == id_morceau::CHAINE_CARACTERE) {
				index_structure = size_t(deref.type_base() >> 8);
			}
			else {
				erreur::lance_erreur(
							"Impossible d'accéder au membre d'un objet n'étant pas une structure",
							contexte.tampon,
							structure->donnees_morceau(),
							erreur::type_erreur::TYPE_DIFFERENTS);
			}
		}
		else {
			erreur::lance_erreur(
						"Impossible d'accéder au membre d'un objet n'étant pas une structure",
						contexte.tampon,
						structure->donnees_morceau(),
						erreur::type_erreur::TYPE_DIFFERENTS);
		}
	}
	else {
		index_structure = size_t(type_structure.type_base() >> 8);
	}

	const auto &nom_membre = membre->chaine();

	auto &donnees_structure = contexte.donnees_structure(index_structure);

	const auto iter = donnees_structure.index_membres.find(nom_membre);

	if (iter == donnees_structure.index_membres.end()) {
		/* À FAIRE : proposer des candidats possibles ou imprimer la structure. */
		erreur::lance_erreur(
					"Membre inconnu",
					contexte.tampon,
					m_donnees_morceaux,
					erreur::type_erreur::MEMBRE_INCONNU);
	}

	const auto index_membre = iter->second;

	return donnees_structure.donnees_types[index_membre];
}

type_noeud NoeudAccesMembre::type() const
{
	return type_noeud::ACCES_MEMBRE;
}

bool NoeudAccesMembre::peut_etre_assigne(ContexteGenerationCode &contexte) const
{
	return m_enfants.back()->peut_etre_assigne(contexte);
}

/* ************************************************************************** */

static bool peut_operer(
		const DonneesType &type1,
		const DonneesType &type2,
		type_noeud type_gauche,
		type_noeud type_droite)
{
	if (est_type_entier(type1.type_base())) {
		if (est_type_entier(type2.type_base())) {
			return true;
		}

		if (type_droite == type_noeud::NOMBRE_ENTIER) {
			return true;
		}

		return false;
	}

	if (est_type_entier(type2.type_base())) {
		if (est_type_entier(type1.type_base())) {
			return true;
		}

		if (type_gauche == type_noeud::NOMBRE_ENTIER) {
			return true;
		}

		return false;
	}

	if (est_type_reel(type1.type_base())) {
		if (est_type_reel(type2.type_base())) {
			return true;
		}

		if (type_droite == type_noeud::NOMBRE_REEL) {
			return true;
		}

		return false;
	}

	if (est_type_reel(type2.type_base())) {
		if (est_type_reel(type1.type_base())) {
			return true;
		}

		if (type_gauche == type_noeud::NOMBRE_REEL) {
			return true;
		}

		return false;
	}

	return false;
}

NoeudOperationBinaire::NoeudOperationBinaire(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudOperationBinaire::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudOperationBinaire : " << m_donnees_morceaux.chaine << " : " << this->donnees_type << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudOperationBinaire::genere_code_llvm(ContexteGenerationCode &contexte, const bool expr_gauche)
{
	auto instr = llvm::Instruction::Add;
	auto predicat = llvm::CmpInst::Predicate::FCMP_FALSE;
	auto est_comp_entier = false;
	auto est_comp_reel = false;

	auto enfant1 = m_enfants.front();
	auto enfant2 = m_enfants.back();

	const auto type1 = enfant1->calcul_type(contexte);
	const auto type2 = enfant2->calcul_type(contexte);

	if ((this->m_donnees_morceaux.identifiant != id_morceau::CROCHET_OUVRANT)) {
		if (!peut_operer(type1, type2, enfant1->type(), enfant2->type())) {
			erreur::lance_erreur_type_operation(
						type1,
						type2,
						contexte.tampon,
						m_donnees_morceaux);
		}
	}

	/* À FAIRE : typage */

	/* Ne crée pas d'instruction de chargement si nous avons un tableau. */
	const auto valeur2_brut = ((type2.type_base() & 0xff) == id_morceau::TABLEAU);

	auto valeur1 = enfant1->genere_code_llvm(contexte);
	auto valeur2 = enfant2->genere_code_llvm(contexte, valeur2_brut);

	switch (this->m_donnees_morceaux.identifiant) {
		case id_morceau::PLUS:
			if (est_type_entier(type1.type_base())) {
				instr = llvm::Instruction::Add;
			}
			else {
				instr = llvm::Instruction::FAdd;
			}

			break;
		case id_morceau::MOINS:
			if (est_type_entier(type1.type_base())) {
				instr = llvm::Instruction::Sub;
			}
			else {
				instr = llvm::Instruction::FSub;
			}

			break;
		case id_morceau::FOIS:
			if (est_type_entier(type1.type_base())) {
				instr = llvm::Instruction::Mul;
			}
			else {
				instr = llvm::Instruction::FMul;
			}

			break;
		case id_morceau::DIVISE:
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
		case id_morceau::POURCENT:
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
		case id_morceau::DECALAGE_DROITE:
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
							erreur::type_erreur::TYPE_DIFFERENTS);
			}
			break;
		case id_morceau::DECALAGE_GAUCHE:
			if (!est_type_entier(type1.type_base())) {
				erreur::lance_erreur(
							"Besoin d'un type entier pour le décalage !",
							contexte.tampon,
							m_donnees_morceaux,
							erreur::type_erreur::TYPE_DIFFERENTS);
			}

			instr = llvm::Instruction::Shl;
			break;
		case id_morceau::ESPERLUETTE:
		case id_morceau::ESP_ESP:
			if (!est_type_entier(type1.type_base())) {
				erreur::lance_erreur(
							"Besoin d'un type entier pour l'opération binaire !",
							contexte.tampon,
							m_donnees_morceaux,
							erreur::type_erreur::TYPE_DIFFERENTS);
			}
			instr = llvm::Instruction::And;
			break;
		case id_morceau::BARRE:
		case id_morceau::BARRE_BARRE:
			if (!est_type_entier(type1.type_base())) {
				erreur::lance_erreur(
							"Besoin d'un type entier pour l'opération binaire !",
							contexte.tampon,
							m_donnees_morceaux,
							erreur::type_erreur::TYPE_DIFFERENTS);
			}
			instr = llvm::Instruction::Or;
			break;
		case id_morceau::CHAPEAU:
			if (!est_type_entier(type1.type_base())) {
				erreur::lance_erreur(
							"Besoin d'un type entier pour l'opération binaire !",
							contexte.tampon,
							m_donnees_morceaux,
							erreur::type_erreur::TYPE_DIFFERENTS);
			}
			instr = llvm::Instruction::Xor;
			break;
			/* À FAIRE. */
		case id_morceau::INFERIEUR:
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
		case id_morceau::INFERIEUR_EGAL:
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
		case id_morceau::SUPERIEUR:
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
		case id_morceau::SUPERIEUR_EGAL:
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
		case id_morceau::EGALITE:
			if (est_type_entier(type1.type_base())) {
				est_comp_entier = true;
				predicat = llvm::CmpInst::Predicate::ICMP_EQ;
			}
			else {
				est_comp_reel = true;
				predicat = llvm::CmpInst::Predicate::FCMP_OEQ;
			}

			break;
		case id_morceau::DIFFERENCE:
			if (est_type_entier(type1.type_base())) {
				est_comp_entier = true;
				predicat = llvm::CmpInst::Predicate::ICMP_NE;
			}
			else {
				est_comp_reel = true;
				predicat = llvm::CmpInst::Predicate::FCMP_ONE;
			}

			break;
		case id_morceau::CROCHET_OUVRANT:
		{
			if (type2.type_base() != id_morceau::POINTEUR && (type2.type_base() & 0xff) != id_morceau::TABLEAU) {
				erreur::lance_erreur(
							"Le type ne peut être déréférencé !",
							contexte.tampon,
							m_donnees_morceaux,
							erreur::type_erreur::TYPE_DIFFERENTS);
			}

			llvm::Value *valeur;

			if (type2.type_base() == id_morceau::POINTEUR) {
				valeur = llvm::GetElementPtrInst::Create(
							 converti_type(contexte, this->donnees_type),
							 valeur2,
				{ valeur1 },
							 "",
							 contexte.bloc_courant());
			}
			else {
				valeur = llvm::GetElementPtrInst::CreateInBounds(
							 converti_type(contexte, type2),
							 valeur2,
				{ llvm::ConstantInt::get(valeur1->getType(), 0), valeur1 },
							 "",
							 contexte.bloc_courant());
			}

			/* Dans le cas d'une assignation, on n'a pas besoin de charger
				 * la valeur dans un registre. */
			if (expr_gauche) {
				return valeur;
			}

			/* Ajout d'un niveau d'indirection pour pouvoir proprement
			 * générer un code pour les expressions de type x[0][0]. Sans ça
			 * LLVM n'arrive pas à déterminer correctement la valeur
			 * déréférencée : on se retrouve avec type(x[0][0]) == (type[0])
			 * ce qui n'est pas forcément le cas. */
			return new llvm::LoadInst(valeur, "", contexte.bloc_courant());
		}
		default:
			return nullptr;
	}

	if (est_comp_entier) {
		return llvm::ICmpInst::Create(llvm::Instruction::ICmp, predicat, valeur1, valeur2, "", contexte.bloc_courant());
	}

	if (est_comp_reel) {
		return llvm::FCmpInst::Create(llvm::Instruction::FCmp, predicat, valeur1, valeur2, "", contexte.bloc_courant());
	}

	return llvm::BinaryOperator::Create(instr, valeur1, valeur2, "", contexte.bloc_courant());
}

const DonneesType &NoeudOperationBinaire::calcul_type(ContexteGenerationCode &contexte)
{
	if (this->donnees_type.est_invalide()) {
		switch (this->identifiant()) {
			default:
			{
				this->donnees_type = m_enfants.front()->calcul_type(contexte);
				break;
			}
			case id_morceau::CROCHET_OUVRANT:
			{
				auto donnees_enfant = m_enfants.back()->calcul_type(contexte);
				this->donnees_type = donnees_enfant.derefence();
				break;
			}
			case id_morceau::EGALITE:
			case id_morceau::DIFFERENCE:
			case id_morceau::INFERIEUR:
			case id_morceau::INFERIEUR_EGAL:
			case id_morceau::SUPERIEUR:
			case id_morceau::SUPERIEUR_EGAL:
			case id_morceau::ESP_ESP:
			case id_morceau::BARRE_BARRE:
			{
				this->donnees_type.pousse(id_morceau::BOOL);
				break;
			}
		}
	}

	return this->donnees_type;
}

type_noeud NoeudOperationBinaire::type() const
{
	return type_noeud::OPERATION_BINAIRE;
}

bool NoeudOperationBinaire::peut_etre_assigne(ContexteGenerationCode &contexte) const
{
	if (this->m_donnees_morceaux.identifiant == id_morceau::CROCHET_OUVRANT) {
		return m_enfants.back()->peut_etre_assigne(contexte);
	}

	return false;
}

/* ************************************************************************** */

NoeudOperationUnaire::NoeudOperationUnaire(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudOperationUnaire::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudOperationUnaire : " << m_donnees_morceaux.chaine
	   << " : " << this->donnees_type << '\n';

	m_enfants.front()->imprime_code(os, tab + 1);
}

llvm::Value *NoeudOperationUnaire::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	llvm::Instruction::BinaryOps instr;
	auto enfant = m_enfants.front();
	auto type1 = enfant->calcul_type(contexte);
	auto valeur1 = enfant->genere_code_llvm(contexte);
	auto valeur2 = static_cast<llvm::Value *>(nullptr);

	switch (this->m_donnees_morceaux.identifiant) {
		case id_morceau::EXCLAMATION:
		{
			if (type1.type_base() != id_morceau::BOOL) {
				erreur::lance_erreur(
							"L'opérateur '!' doit recevoir une expression de type 'bool'",
							contexte.tampon,
							enfant->donnees_morceau(),
							erreur::type_erreur::TYPE_DIFFERENTS);
			}

			instr = llvm::Instruction::Xor;
			valeur2 = valeur1;
			break;
		}
		case id_morceau::TILDE:
		{
			instr = llvm::Instruction::Xor;
			valeur2 = llvm::ConstantInt::get(
						  llvm::Type::getInt32Ty(contexte.contexte),
						  static_cast<uint64_t>(0),
						  false);
			break;
		}
		case id_morceau::AROBASE:
		{
			auto inst_load = dynamic_cast<llvm::LoadInst *>(valeur1);

			if (inst_load == nullptr) {
				/* Ne devrais pas arriver. */
				return nullptr;
			}

			return inst_load->getPointerOperand();
		}
		case id_morceau::PLUS_UNAIRE:
		{
			return valeur1;
		}
		case id_morceau::MOINS_UNAIRE:
		{
			valeur2 = valeur1;

			if (est_type_entier(type1.type_base())) {
				valeur1 = llvm::ConstantInt::get(
							  valeur2->getType(),
							  static_cast<uint64_t>(0),
							  false);
				instr = llvm::Instruction::Sub;
			}
			else {
				valeur1 = llvm::ConstantFP::get(valeur2->getType(), 0);
				instr = llvm::Instruction::FSub;
			}

			break;
		}
		default:
		{
			return nullptr;
		}
	}

	return llvm::BinaryOperator::Create(instr, valeur1, valeur2, "", contexte.bloc_courant());
}

const DonneesType &NoeudOperationUnaire::calcul_type(ContexteGenerationCode &contexte)
{
	if (this->donnees_type.est_invalide()) {
		switch (this->identifiant()) {
			default:
			{
				this->donnees_type = m_enfants.front()->calcul_type(contexte);
				break;
			}
			case id_morceau::AROBASE:
			{
				this->donnees_type.pousse(id_morceau::POINTEUR);
				this->donnees_type.pousse(m_enfants.front()->calcul_type(contexte));
				break;
			}
			case id_morceau::CROCHET_OUVRANT:
			{
				auto donnees_enfant = m_enfants.back()->calcul_type(contexte);
				this->donnees_type = donnees_enfant.derefence();
				break;
			}
			case id_morceau::EXCLAMATION:
			{
				this->donnees_type.pousse(id_morceau::BOOL);
				break;
			}
		}
	}

	return this->donnees_type;
}

type_noeud NoeudOperationUnaire::type() const
{
	return type_noeud::OPERATION_UNAIRE;
}

/* ************************************************************************** */

NoeudRetour::NoeudRetour(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{
	this->donnees_type.pousse(id_morceau::RIEN);
}

void NoeudRetour::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudRetour : " << m_donnees_morceaux.chaine << '\n';
	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudRetour::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	llvm::Value *valeur = nullptr;

	this->calcul_type(contexte);

	if (!m_enfants.empty()) {
		assert(m_enfants.size() == 1);
		valeur = m_enfants.front()->genere_code_llvm(contexte);
	}

	return llvm::ReturnInst::Create(contexte.contexte, valeur, contexte.bloc_courant());
}

const DonneesType &NoeudRetour::calcul_type(ContexteGenerationCode &contexte)
{
	if (m_enfants.empty()) {
		this->donnees_type.pousse(id_morceau::RIEN);
		return this->donnees_type;
	}

	this->donnees_type = m_enfants.front()->calcul_type(contexte);
	return this->donnees_type;
}

type_noeud NoeudRetour::type() const
{
	return type_noeud::RETOUR;
}

/* ************************************************************************** */

NoeudSi::NoeudSi(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudSi::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudSi : " << m_donnees_morceaux.chaine << '\n';
	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudSi::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	const auto nombre_enfants = m_enfants.size();
	auto iter_enfant = m_enfants.begin();

	/* noeud 1 : condition */
	auto enfant1 = *iter_enfant++;

	auto type_condition = enfant1->calcul_type(contexte);

	if (type_condition.type_base() != id_morceau::BOOL) {
		erreur::lance_erreur("Attendu un type booléen pour l'expression 'si'",
							 contexte.tampon,
							 enfant1->donnees_morceau(),
							 erreur::type_erreur::TYPE_DIFFERENTS);
	}

	auto condition = enfant1->genere_code_llvm(contexte);

	auto bloc_alors = cree_bloc(contexte, "alors");

	auto bloc_sinon = (nombre_enfants == 3)
					  ? cree_bloc(contexte, "sinon")
					  : nullptr;

	auto bloc_fusion = cree_bloc(contexte, "cont_si");

	llvm::BranchInst::Create(
				bloc_alors,
				(bloc_sinon != nullptr) ? bloc_sinon : bloc_fusion,
				condition,
				contexte.bloc_courant());

	contexte.bloc_courant(bloc_alors);

	contexte.empile_nombre_locales();

	/* noeud 2 : bloc */
	auto enfant2 = *iter_enfant++;
	auto ret = enfant2->genere_code_llvm(contexte);

	contexte.depile_nombre_locales();

	/* Il est possible d'avoir des contrôles récursif, donc on fait une branche
	 * dans le bloc courant du contexte qui peut être différent de bloc_alors. */
	if (!est_branche_ou_retour(ret) || (contexte.bloc_courant() != bloc_alors)) {
		ret = llvm::BranchInst::Create(bloc_fusion, contexte.bloc_courant());
	}

	/* noeud 3 : sinon (optionel) */
	if (nombre_enfants == 3) {
		contexte.bloc_courant(bloc_sinon);

		contexte.empile_nombre_locales();

		auto enfant3 = *iter_enfant++;
		ret = enfant3->genere_code_llvm(contexte);

		contexte.depile_nombre_locales();

		/* Il est possible d'avoir des contrôles récursif, donc on fait une
		 * branche dans le bloc courant du contexte qui peut être différent de
		 * bloc_sinon. */
		if (!est_branche_ou_retour(ret)) {
			ret = llvm::BranchInst::Create(bloc_fusion, contexte.bloc_courant());
		}
	}

	contexte.bloc_courant(bloc_fusion);

	return ret;
}

const DonneesType &NoeudSi::calcul_type(ContexteGenerationCode &contexte)
{
	/* retourne le type du bloc */
	return m_enfants.back()->calcul_type(contexte);
}

type_noeud NoeudSi::type() const
{
	return type_noeud::SI;
}

/* ************************************************************************** */

NoeudBloc::NoeudBloc(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudBloc::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudBloc : " << m_donnees_morceaux.chaine << '\n';
	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudBloc::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	llvm::Value *valeur = nullptr;

	auto bloc_entree = contexte.bloc_courant();

	for (auto enfant : m_enfants) {
		valeur = enfant->genere_code_llvm(contexte);

		/* nul besoin de continuer à générer du code pour des expressions qui ne
		 * seront jamais executées. À FAIRE : erreur de compilation ? */
		if (est_branche_ou_retour(valeur) && bloc_entree == contexte.bloc_courant()) {
			break;
		}
	}

	return valeur;
}

const DonneesType &NoeudBloc::calcul_type(ContexteGenerationCode &contexte)
{
	if (m_enfants.empty()) {
		this->donnees_type.pousse(id_morceau::RIEN);
		return this->donnees_type;
	}

	return m_enfants.back()->calcul_type(contexte);
}

type_noeud NoeudBloc::type() const
{
	return type_noeud::BLOC;
}

/* ************************************************************************** */

NoeudPour::NoeudPour(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudPour::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudPour : " << m_donnees_morceaux.chaine << '\n';
	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

/* Arbre :
 * NoeudPour
 * - enfant 1 : déclaration variable
 * - enfant 2 : expr début
 * - enfant 3 : expr fin
 * - enfant 4 : bloc
 * - enfant 5 : bloc sinon (optionel)
 *
 * boucle:
 *	phi [entrée] [corps_boucle]
 *	cmp phi, fin
 *	br corps_boucle, apre_boucle
 *
 * corps_boucle:
 *	...
 *	br inc_boucle
 *
 * inc_boucle:
 *	inc phi
 *	br boucle
 *
 * apres_boucle:
 *	...
 */
llvm::Value *NoeudPour::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	auto iter = m_enfants.begin();

	/* on génère d'abord le type de la variable */
	auto enfant1 = *iter++;
	auto enfant2 = *iter++;
	auto enfant3 = *iter++;
	auto enfant4 = *iter++;
	auto enfant5 = (m_enfants.size() == 5) ? *iter++ : nullptr;

	auto type_debut = enfant2->calcul_type(contexte);
	auto type_fin = enfant3->calcul_type(contexte);

	if (type_debut.est_invalide() || type_fin.est_invalide()) {
		erreur::lance_erreur(
					"Les types de l'expression sont invalides !",
					contexte.tampon,
					m_donnees_morceaux,
					erreur::type_erreur::TYPE_INCONNU);
	}

	if (type_debut != type_fin) {
		erreur::lance_erreur_type_operation(
					type_debut,
					type_fin,
					contexte.tampon,
					m_donnees_morceaux);
	}

	if (!est_type_entier(type_debut.type_base())) {
		erreur::lance_erreur(
					"Attendu des types entiers dans la plage de la boucle 'pour'",
					contexte.tampon,
					this->donnees_morceau(),
					erreur::type_erreur::TYPE_DIFFERENTS);
	}

	auto valeur = contexte.valeur_locale(enfant1->chaine());

	if (valeur != nullptr) {
		erreur::lance_erreur(
					"Rédéfinition de la variable",
					contexte.tampon,
					enfant1->donnees_morceau(),
					erreur::type_erreur::VARIABLE_REDEFINIE);
	}
	else {
		valeur = contexte.valeur_globale(enfant1->chaine());

		if (valeur != nullptr) {
			erreur::lance_erreur(
						"Rédéfinition de la variable globale",
						contexte.tampon,
						enfant1->donnees_morceau(),
						erreur::type_erreur::VARIABLE_REDEFINIE);
		}
	}

	enfant1->donnees_type = type_debut;

	/* création des blocs */
	auto bloc_boucle = cree_bloc(contexte, "boucle");
	auto bloc_corps = cree_bloc(contexte, "corps_boucle");
	auto bloc_inc = cree_bloc(contexte, "inc_boucle");
	auto bloc_sinon = (enfant5 != nullptr)
					  ? cree_bloc(contexte, "sinon_boucle")
					  : nullptr;
	auto bloc_apres = cree_bloc(contexte, "apres_boucle");

	contexte.empile_bloc_continue(bloc_inc);
	contexte.empile_bloc_arrete((bloc_sinon != nullptr) ? bloc_sinon : bloc_apres);

	auto bloc_pre = contexte.bloc_courant();
	contexte.empile_nombre_locales();

	auto noeud_phi = static_cast<llvm::PHINode *>(nullptr);

	/* bloc_boucle */
	{
		/* on crée une branche explicite dans le bloc */
		llvm::BranchInst::Create(bloc_boucle, contexte.bloc_courant());

		contexte.bloc_courant(bloc_boucle);

		noeud_phi = llvm::PHINode::Create(
						converti_type(contexte, type_debut),
						2,
						std::string(enfant1->chaine()),
						contexte.bloc_courant());

		contexte.pousse_locale(enfant1->chaine(), noeud_phi, type_debut, false);

		auto valeur_debut = enfant2->genere_code_llvm(contexte);
		noeud_phi->addIncoming(valeur_debut, bloc_pre);

		auto valeur_fin = enfant3->genere_code_llvm(contexte);

		auto condition = llvm::ICmpInst::Create(
							 llvm::Instruction::ICmp,
							 llvm::CmpInst::Predicate::ICMP_SLT,
							 noeud_phi,
							 valeur_fin,
							 "",
							 contexte.bloc_courant());

		llvm::BranchInst::Create(
					bloc_corps,
					bloc_apres,
					condition,
					contexte.bloc_courant());
	}

	/* bloc_corps */
	llvm::Value *ret;
	{
		contexte.bloc_courant(bloc_corps);

		/* génère le code du bloc */
		ret = enfant4->genere_code_llvm(contexte);

		/* incrémente la variable (noeud_phi) */
		if (!est_branche_ou_retour(ret) || (contexte.bloc_courant() != bloc_corps)) {
			ret = llvm::BranchInst::Create(bloc_inc, contexte.bloc_courant());
		}
	}

	/* inc_boucle */
	{
		contexte.bloc_courant(bloc_inc);

		/* incrémente la variable (noeud_phi) */
		auto val_inc = llvm::ConstantInt::get(
						   llvm::Type::getInt32Ty(contexte.contexte),
						   static_cast<uint64_t>(1),
						   false);

		auto inc = llvm::BinaryOperator::Create(
					   llvm::Instruction::Add,
					   noeud_phi,
					   val_inc,
					   "",
					   contexte.bloc_courant());

		noeud_phi->addIncoming(inc, contexte.bloc_courant());

		ret = llvm::BranchInst::Create(bloc_boucle, contexte.bloc_courant());
	}

	contexte.depile_nombre_locales();
	contexte.empile_nombre_locales();

	if (bloc_sinon != nullptr) {
		contexte.bloc_courant(bloc_sinon);

		/* génère le code du bloc */
		ret = enfant5->genere_code_llvm(contexte);

		/* incrémente la variable (noeud_phi) */
		if (!est_branche_ou_retour(ret) || (contexte.bloc_courant() != bloc_sinon)) {
			ret = llvm::BranchInst::Create(bloc_apres, contexte.bloc_courant());
		}
	}

	contexte.depile_bloc_continue();
	contexte.depile_bloc_arrete();
	contexte.depile_nombre_locales();
	contexte.bloc_courant(bloc_apres);

	return ret;
}

const DonneesType &NoeudPour::calcul_type(ContexteGenerationCode &contexte)
{
	/* retourne le type du bloc */
	return m_enfants.back()->calcul_type(contexte);
}

type_noeud NoeudPour::type() const
{
	return type_noeud::POUR;
}

/* ************************************************************************** */

NoeudContArr::NoeudContArr(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudContArr::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);
	os << "NoeudContArr : " << m_donnees_morceaux.chaine << '\n';
}

llvm::Value *NoeudContArr::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	auto bloc = (m_donnees_morceaux.identifiant == id_morceau::CONTINUE)
				? contexte.bloc_continue()
				: contexte.bloc_arrete();

	if (bloc == nullptr) {
		erreur::lance_erreur(
					"'continue' ou 'arrête' en dehors d'une boucle",
					contexte.tampon,
					m_donnees_morceaux,
					erreur::type_erreur::CONTROLE_INVALIDE);
	}

	return llvm::BranchInst::Create(bloc, contexte.bloc_courant());
}

type_noeud NoeudContArr::type() const
{
	return type_noeud::CONTINUE_ARRETE;
}

/* ************************************************************************** */

NoeudBoucle::NoeudBoucle(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudBoucle::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);
	os << "NoeudContArr : " << m_donnees_morceaux.chaine << '\n';
	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

/* boucle:
 *	corps
 *  br boucle
 *
 * apres_boucle:
 *	...
 */
llvm::Value *NoeudBoucle::genere_code_llvm(
		ContexteGenerationCode &contexte,
		const bool /*expr_gauche*/)
{
	auto iter = m_enfants.begin();
	auto enfant1 = *iter++;
	auto enfant2 = (m_enfants.size() == 2) ? *iter++ : nullptr;

	/* création des blocs */
	auto bloc_boucle = cree_bloc(contexte, "boucle");
	auto bloc_sinon = (enfant2 != nullptr) ? cree_bloc(contexte, "sinon_boucle") : nullptr;
	auto bloc_apres = cree_bloc(contexte, "apres_boucle");

	contexte.empile_bloc_continue(bloc_boucle);
	contexte.empile_bloc_arrete((enfant2 != nullptr) ? bloc_sinon : bloc_apres);

	contexte.empile_nombre_locales();

	/* on crée une branche explicite dans le bloc */
	llvm::BranchInst::Create(bloc_boucle, contexte.bloc_courant());

	contexte.bloc_courant(bloc_boucle);

	auto ret = enfant1->genere_code_llvm(contexte);

	if (!est_branche_ou_retour(ret) || (contexte.bloc_courant() != bloc_boucle)) {
		ret = llvm::BranchInst::Create(bloc_boucle, contexte.bloc_courant());
	}

	contexte.depile_nombre_locales();
	contexte.empile_nombre_locales();

	if (bloc_sinon != nullptr) {
		contexte.bloc_courant(bloc_sinon);

		/* génère le code du bloc */
		ret = enfant2->genere_code_llvm(contexte);

		/* incrémente la variable (noeud_phi) */
		if (!est_branche_ou_retour(ret) || (contexte.bloc_courant() != bloc_sinon)) {
			ret = llvm::BranchInst::Create(bloc_apres, contexte.bloc_courant());
		}
	}

	contexte.depile_bloc_continue();
	contexte.depile_bloc_arrete();
	contexte.depile_nombre_locales();
	contexte.bloc_courant(bloc_apres);

	return ret;
}

const DonneesType &NoeudBoucle::calcul_type(ContexteGenerationCode &contexte)
{
	/* retourne le type du bloc */
	return m_enfants.front()->calcul_type(contexte);
}

type_noeud NoeudBoucle::type() const
{
	return type_noeud::BOUCLE;
}

NoeudTranstype::NoeudTranstype(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudTranstype::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);
	os << "NoeudTranstype : " << this->donnees_type << '\n';
	m_enfants.front()->imprime_code(os, tab + 1);
}

template <llvm::Instruction::CastOps Op>
constexpr llvm::Value *cree_instruction(
		llvm::Value *valeur,
		llvm::Type *type,
		llvm::BasicBlock *bloc)
{
	using CastOps = llvm::Instruction::CastOps;

	static_assert(Op >= CastOps::Trunc && Op <= CastOps::AddrSpaceCast,
				  "OpCode de transtypage inconnu");

	switch (Op) {
		case CastOps::IntToPtr:
			return llvm::IntToPtrInst::Create(Op, valeur, type, "", bloc);
		case CastOps::UIToFP:
			return llvm::UIToFPInst::Create(Op, valeur, type, "", bloc);
		case CastOps::SIToFP:
			return llvm::SIToFPInst::Create(Op, valeur, type, "", bloc);
		case CastOps::Trunc:
			return llvm::TruncInst::Create(Op, valeur, type, "", bloc);
		case CastOps::ZExt:
			return llvm::ZExtInst::Create(Op, valeur, type, "", bloc);
		case CastOps::SExt:
			return llvm::SExtInst::Create(Op, valeur, type, "", bloc);
		case CastOps::FPToUI:
			return llvm::FPToUIInst::Create(Op, valeur, type, "", bloc);
		case CastOps::FPToSI:
			return llvm::FPToSIInst::Create(Op, valeur, type, "", bloc);
		case CastOps::FPTrunc:
			return llvm::FPTruncInst::Create(Op, valeur, type, "", bloc);
		case CastOps::FPExt:
			return llvm::FPExtInst::Create(Op, valeur, type, "", bloc);
		case CastOps::PtrToInt:
			return llvm::PtrToIntInst::Create(Op, valeur, type, "", bloc);
		case CastOps::BitCast:
			return llvm::BitCastInst::Create(Op, valeur, type, "", bloc);
		case CastOps::AddrSpaceCast:
			return llvm::AddrSpaceCastInst::Create(Op, valeur, type, "", bloc);
	}
}

llvm::Value *NoeudTranstype::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	if (this->donnees_type.est_invalide()) {
		erreur::lance_erreur(
					"Ne peut transtyper vers un type invalide",
					contexte.tampon,
					this->donnees_morceau(),
					erreur::type_erreur::TYPE_INCONNU);
	}

	auto enfant = m_enfants.front();
	const auto &donnees_type_de = m_enfants.front()->calcul_type(contexte);

	if (donnees_type_de.est_invalide()) {
		erreur::lance_erreur(
					"Ne peut calculer le type d'origine",
					contexte.tampon,
					enfant->donnees_morceau(),
					erreur::type_erreur::TYPE_INCONNU);
	}

	auto valeur = enfant->genere_code_llvm(contexte);

	if (donnees_type_de == this->donnees_type) {
		return valeur;
	}

	using CastOps = llvm::Instruction::CastOps;

	auto type = converti_type(contexte, this->donnees_type);
	auto bloc = contexte.bloc_courant();
	auto type_de = donnees_type_de.type_base();
	auto type_vers = this->donnees_type.type_base();

	if (est_type_entier(type_de)) {
		/* un nombre entier peut être converti en l'adresse d'un pointeur */
		if (type_vers == id_morceau::POINTEUR) {
			return cree_instruction<CastOps::IntToPtr>(valeur, type, bloc);
		}

		if (est_type_reel(type_vers)) {
			if (est_type_entier_naturel(type_de)) {
				return cree_instruction<CastOps::UIToFP>(valeur, type, bloc);
			}

			return cree_instruction<CastOps::SIToFP>(valeur, type, bloc);
		}

		if (est_type_entier(type_vers)) {
			if (est_plus_petit(type_vers, type_de)) {
				return cree_instruction<CastOps::Trunc>(valeur, type, bloc);
			}

			if (est_type_entier_naturel(type_de)) {
				return cree_instruction<CastOps::ZExt>(valeur, type, bloc);
			}

			return cree_instruction<CastOps::SExt>(valeur, type, bloc);
		}
	}

	if (est_type_reel(type_de)) {
		if (est_type_entier_naturel(type_vers)) {
			return cree_instruction<CastOps::FPToUI>(valeur, type, bloc);
		}

		if (est_type_entier_relatif(type_vers)) {
			return cree_instruction<CastOps::FPToSI>(valeur, type, bloc);
		}

		if (est_type_reel(type_de)) {
			if (est_plus_petit(type_vers, type_de)) {
				return cree_instruction<CastOps::FPTrunc>(valeur, type, bloc);
			}

			return cree_instruction<CastOps::FPExt>(valeur, type, bloc);
		}
	}

	if (type_de == id_morceau::POINTEUR && est_type_entier(type_vers)) {
		return cree_instruction<CastOps::PtrToInt>(valeur, type, bloc);
	}

	/* À FAIRE : BitCast (Type Cast) */
	erreur::lance_erreur_type_operation(
				donnees_type_de,
				this->donnees_type,
				contexte.tampon,
				this->donnees_morceau());
}

type_noeud NoeudTranstype::type() const
{
	return type_noeud::TRANSTYPE;
}
