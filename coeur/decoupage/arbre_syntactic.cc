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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#pragma GCC diagnostic pop

#include <sstream>

#include "contexte_generation_code.h"
#include "donnees_type.h"
#include "erreur.h"
#include "modules.hh"
#include "morceaux.h"
#include "nombres.h"
#include "tampon_source.h"

#undef NOMME_IR

/* ************************************************************************** */

static auto cree_bloc(ContexteGenerationCode &contexte, const char *nom)
{
#ifdef NOMME_IR
	return llvm::BasicBlock::Create(contexte.contexte, nom, contexte.fonction);
#else
	static_cast<void>(nom);
	return llvm::BasicBlock::Create(contexte.contexte, "", contexte.fonction);
#endif
}

static llvm::Type *converti_type(
		ContexteGenerationCode &contexte,
		const DonneesType &donnees_type);

static llvm::Type *converti_type_simple(
		ContexteGenerationCode &contexte,
		const id_morceau &identifiant,
		llvm::Type *type_entree)
{
	llvm::Type *type = nullptr;

	switch (identifiant & 0xff) {
		case id_morceau::BOOL:
			type = llvm::Type::getInt1Ty(contexte.contexte);
			break;
		case id_morceau::N8:
		case id_morceau::Z8:
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
			/* À FAIRE : type R16 */
			//type = llvm::Type::getHalfTy(contexte.contexte);
			type = llvm::Type::getFloatTy(contexte.contexte);
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
			type = llvm::PointerType::get(type_entree, 0);
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

				auto nom = "struct." + contexte.nom_struct(donnees_structure.id);

				donnees_structure.type_llvm = llvm::StructType::create(
												  contexte.contexte,
												  types_membres,
												  nom,
												  false);
			}

			type = donnees_structure.type_llvm;
			break;
		}
		case id_morceau::TABLEAU:
		{
			const auto taille = static_cast<uint64_t>(identifiant) & 0xffffff00;
			type = llvm::ArrayType::get(type_entree, taille >> 8);
			break;
		}
		default:
			assert(false);
	}

	return type;
}

static llvm::Type *converti_type(
		ContexteGenerationCode &contexte,
		const DonneesType &donnees_type)
{
	llvm::Type *type = nullptr;

	for (id_morceau identifiant : donnees_type) {
		type = converti_type_simple(contexte, identifiant, type);
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
		{
			return alignement(contexte, donnees_type.derefence());
		}
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
		const DonneesFonction &donnees_fonction,
		const DonneesType &donnees_retour,
		bool est_variadique)
{
	std::vector<llvm::Type *> parametres;
	parametres.reserve(donnees_fonction.nom_args.size());

	for (const auto &nom : donnees_fonction.nom_args) {
		const auto &argument = donnees_fonction.args.find(nom);

		if (argument->second.est_variadic) {
			/* ajout de l'argument implicite du compte d'arguments var_args */
			if (!donnees_fonction.est_externe) {
				parametres.push_back(llvm::Type::getInt32Ty(contexte.contexte));
			}

			break;
		}

		parametres.push_back(converti_type(contexte, argument->second.donnees_type));
	}

	return llvm::FunctionType::get(
				converti_type(contexte, donnees_retour),
				parametres,
				est_variadique);
}

/* ************************************************************************** */

Noeud::Noeud(const DonneesMorceaux &morceau)
	: m_donnees_morceaux{morceau}
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

void Noeud::perfome_validation_semantique(ContexteGenerationCode &contexte)
{
	for (auto enfant : m_enfants) {
		enfant->perfome_validation_semantique(contexte);
	}
}

void Noeud::ajoute_noeud(Noeud *noeud)
{
	m_enfants.push_back(noeud);
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
	for (auto noeud : m_enfants) {
		noeud->perfome_validation_semantique(contexte);
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
	this->module_appel = morceau.module;
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
	/* broyage du nom */
	auto module = contexte.module(static_cast<size_t>(this->module_appel));
	auto nom_module = module->nom;
	auto nom_fonction = std::string(m_donnees_morceaux.chaine);
	auto nom_broye = nom_module.empty() ? nom_fonction : nom_module + '_' + nom_fonction;

	auto fonction = contexte.module_llvm->getFunction(nom_broye);

	const auto &donnees_fonction = contexte.donnees_fonction(m_donnees_morceaux.chaine);

	auto fonction_variadique_interne = fonction->isVarArg() && !donnees_fonction.est_externe;

	/* Cherche la liste d'arguments */
	auto noms_arguments = std::any_cast<std::list<std::string_view>>(&valeur_calculee);

	/* Réordonne les enfants selon l'apparition des arguments car LLVM est
	 * tatillon : ce n'est pas l'ordre dans lequel les valeurs apparaissent
	 * dans le vecteur de paramètres qui compte, mais l'ordre dans lequel le
	 * code est généré. */
	std::vector<Noeud *> enfants(noms_arguments->size() + fonction_variadique_interne);

	auto noeud_nombre_args = static_cast<NoeudNombreEntier *>(nullptr);

	if (fonction_variadique_interne) {
		/* Pour les fonctions variadiques, il nous faut ajouter le nombre
		 * d'arguments à l'appel de la fonction. */
		auto nombre_args = donnees_fonction.args.size();
		auto nombre_args_var = std::max(0ul, noms_arguments->size() - (nombre_args - 1));
		auto index_premier_var_arg = nombre_args - 1;

		noeud_nombre_args = new NoeudNombreEntier({});
		noeud_nombre_args->valeur_calculee = static_cast<long>(nombre_args_var);
		noeud_nombre_args->calcule = true;

		enfants[index_premier_var_arg] = noeud_nombre_args;
	}

	auto enfant = m_enfants.begin();
	auto nombre_arg_variadic = 0ul + fonction_variadique_interne;

	for (const auto &nom : *noms_arguments) {
		/* Pas la peine de vérifier qu'iter n'est pas égal à la fin de la table
		 * car ça a déjà été fait dans l'analyse grammaticale. */
		const auto iter = donnees_fonction.args.find(nom);
		auto index_arg = iter->second.index;
		const auto type_arg = iter->second.donnees_type;
		const auto type_enf = (*enfant)->donnees_type;

		if (iter->second.est_variadic) {
			if (!type_arg.est_invalide()) {
				if (!sont_compatibles(type_arg, type_enf)) {
					erreur::lance_erreur_type_arguments(
								type_arg,
								type_enf,
								contexte,
								(*enfant)->donnees_morceau(),
								m_donnees_morceaux);
				}
			}

			/* Décale l'index selon le nombre d'arguments dans l'argument
			 * variadique, car ici index_arg est l'index dans la déclaration et
			 * la déclaration ne contient qu'un seul argument variadic. */
			index_arg += nombre_arg_variadic;

			++nombre_arg_variadic;
		}
		else {
			if (!sont_compatibles(type_arg, type_enf)) {
				erreur::lance_erreur_type_arguments(
							type_arg,
							type_enf,
							contexte,
							(*enfant)->donnees_morceau(),
							m_donnees_morceaux);
			}
		}

		enfants[index_arg] = *enfant;

		++enfant;
	}

	std::vector<llvm::Value *> parametres;
	parametres.resize(enfants.size());

	std::transform(enfants.begin(), enfants.end(), parametres.begin(),
				   [&](Noeud *noeud_enfant)
	{
		return noeud_enfant->genere_code_llvm(contexte);
	});

	llvm::ArrayRef<llvm::Value *> args(parametres);

	delete noeud_nombre_args;

	return llvm::CallInst::Create(fonction, args, "", contexte.bloc_courant());
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

void NoeudAppelFonction::perfome_validation_semantique(ContexteGenerationCode &contexte)
{
	/* broyage du nom */
	auto module = contexte.module(static_cast<size_t>(this->module_appel));
	auto nom_module = module->nom;
	auto nom_fonction = std::string(m_donnees_morceaux.chaine);
	auto nom_broye = nom_module.empty() ? nom_fonction : nom_module + '_' + nom_fonction;

	if (!module->possede_fonction(m_donnees_morceaux.chaine)) {
		erreur::lance_erreur(
					"Fonction inconnue",
					contexte,
					m_donnees_morceaux,
					erreur::type_erreur::FONCTION_INCONNUE);
	}

	const auto &donnees_fonction = contexte.donnees_fonction(m_donnees_morceaux.chaine);

	if (!donnees_fonction.est_variadique && (m_enfants.size() != donnees_fonction.args.size())) {
		erreur::lance_erreur_nombre_arguments(
					donnees_fonction.args.size(),
					m_enfants.size(),
					contexte,
					m_donnees_morceaux);
	}

	if (this->donnees_type.est_invalide()) {
		this->donnees_type = donnees_fonction.donnees_type;
	}

	Noeud::perfome_validation_semantique(contexte);
}

/* ************************************************************************** */

NoeudDeclarationFonction::NoeudDeclarationFonction(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

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

	auto donnees_fonction = contexte.donnees_fonction(m_donnees_morceaux.chaine);

	/* Pour les fonctions variadiques
	 * - on ajoute manuellement un argument implicit correspondant au nombre
	 *   d'args qui est appelé __compte_args
	 * - lors de l'appel, puisque nous connaissons le nombre d'arguments, on le
	 *   passe à la fonction
	 * - les boucles n'ont plus qu'à utiliser le compte d'arguments comme valeur
	 *   de fin de plage.
	 */

	/* Crée le type de la fonction */
	auto type_fonction = obtiens_type_fonction(
							 contexte,
							 donnees_fonction,
							 this->donnees_type,
							 this->est_variable);

	/* broyage du nom */
	auto nom_module = contexte.module(static_cast<size_t>(m_donnees_morceaux.module))->nom;
	auto nom_fonction = std::string(m_donnees_morceaux.chaine);
	auto nom_broye = (this->est_externe || nom_module.empty()) ? nom_fonction : nom_module + '_' + nom_fonction;

	/* Crée fonction */
	auto fonction = llvm::Function::Create(
						type_fonction,
						llvm::Function::ExternalLinkage,
						nom_broye,
						contexte.module_llvm);

	if (this->est_externe) {
		return fonction;
	}

	contexte.commence_fonction(fonction);

	auto block = cree_bloc(contexte, "entree");

	contexte.bloc_courant(block);

	/* Crée code pour les arguments */
	auto valeurs_args = fonction->arg_begin();

	for (const auto &nom : donnees_fonction.nom_args) {
		const auto &argument = donnees_fonction.args[nom];
		auto align = unsigned{0};
		auto type = static_cast<llvm::Type *>(nullptr);

		if (argument.est_variadic) {
			align = 8;

			auto id = size_t{};

			if (!contexte.structure_existe("va_list")) {
				/* Crée la structure va_list pour Unix x84_64
				 *  %struct.va_list = type { i32, i32, i8*, i8* }
				 */

				auto donnees_structure = DonneesStructure{};

				auto dt = DonneesType{};
				dt.pousse(id_morceau::Z32);

				donnees_structure.index_membres.insert({"", donnees_structure.donnees_types.size()});
				donnees_structure.donnees_types.push_back(dt);
				donnees_structure.index_membres.insert({"", donnees_structure.donnees_types.size()});
				donnees_structure.donnees_types.push_back(dt);

				dt = DonneesType{};
				dt.pousse(id_morceau::POINTEUR);
				dt.pousse(id_morceau::Z8);

				donnees_structure.index_membres.insert({"", donnees_structure.donnees_types.size()});
				donnees_structure.donnees_types.push_back(dt);
				donnees_structure.index_membres.insert({"", donnees_structure.donnees_types.size()});
				donnees_structure.donnees_types.push_back(dt);

				id = contexte.ajoute_donnees_structure("va_list", donnees_structure);
			}
			else {
				id = contexte.donnees_structure("va_list").id;
			}

			auto dt = DonneesType{};
			dt.pousse(id_morceau::CHAINE_CARACTERE | (static_cast<int>(id) << 8));

			type = converti_type(contexte, dt);
		}
		else {
			align = alignement(contexte, argument.donnees_type);
			type = converti_type(contexte, argument.donnees_type);
		}

		if (argument.est_variadic) {
			/* stockage de l'argument implicit de compte d'argument */
			auto valeur = &(*valeurs_args++);
			auto dt = DonneesType{};
			dt.pousse(id_morceau::Z32);

			auto alloc_compte = new llvm::AllocaInst(
									llvm::Type::getInt32Ty(contexte.contexte),
									"",
									contexte.bloc_courant());
			alloc_compte->setAlignment(4);

			auto store = new llvm::StoreInst(valeur, alloc_compte, false, contexte.bloc_courant());
			store->setAlignment(4);

			contexte.pousse_locale("__compte_args", alloc_compte, dt, false, false);

			valeur = &(*valeurs_args++);

			auto alloc = new llvm::AllocaInst(
							 type,
#ifdef NOMME_IR
							 argument.chaine,
#else
							"",
#endif
							 contexte.bloc_courant());

			alloc->setAlignment(align);
			auto cast = new llvm::BitCastInst(alloc,
										  llvm::Type::getInt8PtrTy(contexte.contexte),
										  "",
										  contexte.bloc_courant());

			auto fonc = llvm::Intrinsic::getDeclaration(contexte.module_llvm, llvm::Intrinsic::vastart);
			llvm::CallInst::Create(fonc, cast, "", contexte.bloc_courant());
			contexte.pousse_locale(nom, cast, argument.donnees_type, argument.est_variable, argument.est_variadic);
		}
		else {
			auto valeur = &(*valeurs_args++);
#ifdef NOMME_IR
			valeur->setName(argument.chaine.c_str());
#endif

			auto alloc = new llvm::AllocaInst(
							 type,
	#ifdef NOMME_IR
							 argument.chaine,
	#else
							"",
	#endif
							 contexte.bloc_courant());

			alloc->setAlignment(align);
			auto store = new llvm::StoreInst(valeur, alloc, false, contexte.bloc_courant());
			store->setAlignment(align);
			contexte.pousse_locale(nom, alloc, argument.donnees_type, argument.est_variable, argument.est_variadic);
		}
	}

	/* Crée code pour le bloc. */
	auto bloc = m_enfants.front();
	bloc->valeur_calculee = static_cast<llvm::BasicBlock *>(nullptr);
	auto ret = bloc->genere_code_llvm(contexte);

	/* Ajoute une instruction de retour si la dernière n'en est pas une. */
	if ((ret != nullptr) && !llvm::isa<llvm::ReturnInst>(*ret)) {
		llvm::ReturnInst::Create(
					contexte.contexte,
					nullptr,
					contexte.bloc_courant());
	}

	contexte.termine_fonction();

	/* optimise la fonction */
	if (contexte.menageur_fonctions != nullptr) {
		contexte.menageur_fonctions->run(*fonction);
	}

	return nullptr;
}

type_noeud NoeudDeclarationFonction::type() const
{
	return type_noeud::DECLARATION_FONCTION;
}

void NoeudDeclarationFonction::perfome_validation_semantique(ContexteGenerationCode &contexte)
{
	if (this->est_externe) {
		return;
	}

	contexte.commence_fonction(nullptr);

	auto donnees_fonction = contexte.donnees_fonction(m_donnees_morceaux.chaine);

	/* Pousse les paramètres sur la pile. */
	for (const auto &nom : donnees_fonction.nom_args) {
		const auto &argument = donnees_fonction.args[nom];

		if (argument.est_variadic) {
			auto dt = DonneesType{};
			dt.pousse(id_morceau::Z32);

			contexte.pousse_locale("__compte_args", nullptr, dt, false, false);
			contexte.pousse_locale(nom, nullptr, argument.donnees_type, argument.est_variable, argument.est_variadic);
		}
		else {
			contexte.pousse_locale(nom, nullptr, argument.donnees_type, argument.est_variable, argument.est_variadic);
		}
	}

	/* vérifie le type du bloc */
	auto bloc = m_enfants.front();

	bloc->perfome_validation_semantique(contexte);
	auto type_bloc = bloc->donnees_type;
	auto dernier = bloc->dernier_enfant();

	/* si le bloc est vide -> vérifie qu'aucun type n'a été spécifié */
	if (dernier == nullptr) {
		if (this->donnees_type.type_base() != id_morceau::RIEN) {
			erreur::lance_erreur(
						"Instruction de retour manquante",
						contexte,
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
							contexte,
							m_donnees_morceaux,
							erreur::type_erreur::TYPE_DIFFERENTS);
			}
		}
		/* vérifie que le type du bloc correspond au type de la fonction */
		else {
			if (this->donnees_type != type_bloc) {
				erreur::lance_erreur(
							"Le type de retour est invalide",
							contexte,
							m_donnees_morceaux,
							erreur::type_erreur::TYPE_DIFFERENTS);
			}
		}
	}

	contexte.termine_fonction();
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
	auto expression = m_enfants.back();

	/* Génère d'abord le code de l'enfant afin que l'instruction d'allocation de
	 * la variable sur la pile et celle de stockage de la valeur soit côte à
	 * côte. */
	auto valeur = expression->genere_code_llvm(contexte);

	auto alloc = variable->genere_code_llvm(contexte, true);
	auto store = new llvm::StoreInst(valeur, alloc, false, contexte.bloc_courant());
	store->setAlignment(alignement(contexte, expression->donnees_type));

	return store;
}

type_noeud NoeudAssignationVariable::type() const
{
	return type_noeud::ASSIGNATION_VARIABLE;
}

void NoeudAssignationVariable::perfome_validation_semantique(ContexteGenerationCode &contexte)
{
	auto variable = m_enfants.front();
	auto expression = m_enfants.back();

	if (!variable->peut_etre_assigne(contexte)) {
		erreur::lance_erreur(
					"Impossible d'assigner l'expression à la variable !",
					contexte,
					m_donnees_morceaux,
					erreur::type_erreur::ASSIGNATION_INVALIDE);
	}

	expression->perfome_validation_semantique(contexte);

	this->donnees_type = expression->donnees_type;

	if (this->donnees_type.est_invalide()) {
		erreur::lance_erreur(
					"Impossible de définir le type de la variable !",
					contexte,
					m_donnees_morceaux,
					erreur::type_erreur::TYPE_INCONNU);
	}

	if (this->donnees_type.type_base() == id_morceau::RIEN) {
		erreur::lance_erreur(
					"Impossible d'assigner une expression de type 'rien' à une variable !",
					contexte,
					m_donnees_morceaux,
					erreur::type_erreur::ASSIGNATION_RIEN);
	}

	/* Ajourne les données du premier enfant si elles sont invalides, dans le
	 * cas d'une déclaration de variable. */
	const auto type_gauche = variable->donnees_type;

	if (type_gauche.est_invalide()) {
		variable->donnees_type = this->donnees_type;
	}

	variable->perfome_validation_semantique(contexte);

	if (!peut_assigner(variable->donnees_type, this->donnees_type, expression->type())) {
		erreur::lance_erreur_assignation_type_differents(
					variable->donnees_type,
					this->donnees_type,
					contexte,
					m_donnees_morceaux);
	}
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
	auto type_llvm = converti_type(contexte, this->donnees_type);

	auto alloc = new llvm::AllocaInst(
					 type_llvm,
 #ifdef NOMME_IR
					 std::string(m_donnees_morceaux.chaine),
 #else
					 "",
 #endif
					 contexte.bloc_courant());

	alloc->setAlignment(alignement(contexte, this->donnees_type));

	contexte.pousse_locale(m_donnees_morceaux.chaine, alloc, this->donnees_type, this->est_variable, false);

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

void NoeudDeclarationVariable::perfome_validation_semantique(ContexteGenerationCode &contexte)
{
	auto existe = contexte.locale_existe(m_donnees_morceaux.chaine);

	if (existe) {
		erreur::lance_erreur(
					"Redéfinition de la variable locale",
					contexte,
					m_donnees_morceaux,
					erreur::type_erreur::VARIABLE_REDEFINIE);
	}
	else {
		existe = contexte.globale_existe(m_donnees_morceaux.chaine);

		if (existe) {
			erreur::lance_erreur(
						"Redéfinition de la variable globale",
						contexte,
						m_donnees_morceaux,
						erreur::type_erreur::VARIABLE_REDEFINIE);
		}
	}

	contexte.pousse_locale(m_donnees_morceaux.chaine, nullptr, this->donnees_type, this->est_variable, false);
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
	/* À FAIRE : énumération avec des expressions contenant d'autres énums.
	 * différents types (réel, bool, etc..)
	 */

	auto n = converti_chaine_nombre_entier(
				 m_enfants.front()->chaine(),
				 m_enfants.front()->identifiant());

	auto constante = llvm::ConstantInt::get(
						 converti_type(contexte, this->donnees_type),
						 static_cast<uint64_t>(n));

	auto valeur = new llvm::GlobalVariable(
				 *contexte.module_llvm,
				 converti_type(contexte, this->donnees_type),
				 true,
				 llvm::GlobalValue::InternalLinkage,
				 constante);

	contexte.pousse_globale(m_donnees_morceaux.chaine, valeur, this->donnees_type);

	return valeur;
}

type_noeud NoeudConstante::type() const
{
	return type_noeud::CONSTANTE;
}

void NoeudConstante::perfome_validation_semantique(ContexteGenerationCode &contexte)
{
	auto valeur = contexte.valeur_globale(m_donnees_morceaux.chaine);

	if (valeur != nullptr) {
		erreur::lance_erreur(
					"Redéfinition de la variable globale !",
					contexte,
					m_donnees_morceaux,
					erreur::type_erreur::VARIABLE_REDEFINIE);
	}

	m_enfants.front()->perfome_validation_semantique(contexte);

	if (this->donnees_type.est_invalide()) {
		this->donnees_type = m_enfants.front()->donnees_type;

		if (this->donnees_type.est_invalide()) {
			erreur::lance_erreur(
						"Impossible de définir le type de la variable globale !",
						contexte,
						m_donnees_morceaux,
						erreur::type_erreur::TYPE_INCONNU);
		}
	}
	/* À FAIRE : vérifie typage */

	contexte.pousse_globale(m_donnees_morceaux.chaine, nullptr, this->donnees_type);
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

	this->donnees_type.pousse(id_morceau::TABLEAU | static_cast<int>((corrigee.size() + 1) << 8));
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

	auto globale = new llvm::GlobalVariable(
				*contexte.module_llvm,
				type,
				true,
				llvm::GlobalValue::PrivateLinkage,
				constante,
				".chn");

	globale->setAlignment(1);
	globale->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);

	return llvm::GetElementPtrInst::CreateInBounds(
				globale, {
					llvm::ConstantInt::get(llvm::Type::getInt32Ty(contexte.contexte), 0),
					llvm::ConstantInt::get(llvm::Type::getInt32Ty(contexte.contexte), 0)
				},
				"",
				contexte.bloc_courant());
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
	}

	if (expr_gauche || dynamic_cast<llvm::PHINode *>(valeur) || dynamic_cast<llvm::VAArgInst *>(valeur)) {
		return valeur;
	}

	/* À FAIRE : redondant. */
	auto type = this->donnees_type;

	if (contexte.est_locale_variadique(m_donnees_morceaux.chaine)) {
		auto inst = new llvm::VAArgInst(
						valeur,
						converti_type(contexte, type),
						"",
						contexte.bloc_courant());

		return inst;
	}

	auto charge = new llvm::LoadInst(valeur, "", false, contexte.bloc_courant());
	charge->setAlignment(alignement(contexte, type));

	return charge;
}

type_noeud NoeudVariable::type() const
{
	return type_noeud::VARIABLE;
}

bool NoeudVariable::peut_etre_assigne(ContexteGenerationCode &contexte) const
{
	return contexte.peut_etre_assigne(m_donnees_morceaux.chaine);
}

void NoeudVariable::perfome_validation_semantique(ContexteGenerationCode &contexte)
{
	if (contexte.locale_existe(m_donnees_morceaux.chaine)) {
		this->donnees_type = contexte.type_locale(m_donnees_morceaux.chaine);
		return;
	}

	if (contexte.globale_existe(m_donnees_morceaux.chaine)) {
		this->donnees_type = contexte.type_globale(m_donnees_morceaux.chaine);
		return;
	}

	erreur::lance_erreur(
				"Variable inconnue",
				contexte,
				m_donnees_morceaux,
				erreur::type_erreur::VARIABLE_INCONNUE);
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

	const auto &type_structure = structure->donnees_type;

	auto index_structure = 0ul;
	auto est_pointeur = false;

	if ((type_structure.type_base() & 0xff) != id_morceau::CHAINE_CARACTERE) {
		if (type_structure.type_base() == id_morceau::POINTEUR) {
			auto deref = type_structure.derefence();

			if ((deref.type_base() & 0xff) == id_morceau::CHAINE_CARACTERE) {
				index_structure = size_t(deref.type_base() >> 8);
				est_pointeur = true;
			}
		}
	}
	else {
		index_structure = size_t(type_structure.type_base() >> 8);
	}

	const auto &nom_membre = membre->chaine();

	auto &donnees_structure = contexte.donnees_structure(index_structure);

	const auto iter = donnees_structure.index_membres.find(nom_membre);

	const auto index_membre = iter->second;

	auto valeur = structure->genere_code_llvm(contexte, true);

	llvm::Value *ret;

	if (est_pointeur) {
		/* déréférence le pointeur en le chargeant */
		auto charge = new llvm::LoadInst(valeur, "", contexte.bloc_courant());
		charge->setAlignment(8);
		valeur = charge;
	}

	ret = llvm::GetElementPtrInst::CreateInBounds(
			  valeur, {
				  llvm::ConstantInt::get(llvm::Type::getInt64Ty(contexte.contexte), 0),
				  llvm::ConstantInt::get(llvm::Type::getInt32Ty(contexte.contexte), index_membre)
			  },
			  "",
			  contexte.bloc_courant());


	if (!expr_gauche) {
		auto charge = new llvm::LoadInst(ret, "", contexte.bloc_courant());
		charge->setAlignment(alignement(contexte, donnees_structure.donnees_types[index_membre]));
		ret = charge;
	}

	return ret;
}

type_noeud NoeudAccesMembre::type() const
{
	return type_noeud::ACCES_MEMBRE;
}

bool NoeudAccesMembre::peut_etre_assigne(ContexteGenerationCode &contexte) const
{
	return m_enfants.back()->peut_etre_assigne(contexte);
}

void NoeudAccesMembre::perfome_validation_semantique(ContexteGenerationCode &contexte)
{
	auto structure = m_enfants.back();
	auto membre = m_enfants.front();

	structure->perfome_validation_semantique(contexte);

	const auto &type_structure = structure->donnees_type;

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
							contexte,
							structure->donnees_morceau(),
							erreur::type_erreur::TYPE_DIFFERENTS);
			}
		}
		else {
			erreur::lance_erreur(
						"Impossible d'accéder au membre d'un objet n'étant pas une structure",
						contexte,
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
					contexte,
					m_donnees_morceaux,
					erreur::type_erreur::MEMBRE_INCONNU);
	}

	const auto index_membre = iter->second;

	this->donnees_type = donnees_structure.donnees_types[index_membre];
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

	const auto type1 = enfant1->donnees_type;
	const auto type2 = enfant2->donnees_type;

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
							contexte,
							m_donnees_morceaux,
							erreur::type_erreur::TYPE_DIFFERENTS);
			}
			break;
		case id_morceau::DECALAGE_GAUCHE:
			if (!est_type_entier(type1.type_base())) {
				erreur::lance_erreur(
							"Besoin d'un type entier pour le décalage !",
							contexte,
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
							contexte,
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
							contexte,
							m_donnees_morceaux,
							erreur::type_erreur::TYPE_DIFFERENTS);
			}
			instr = llvm::Instruction::Or;
			break;
		case id_morceau::CHAPEAU:
			if (!est_type_entier(type1.type_base())) {
				erreur::lance_erreur(
							"Besoin d'un type entier pour l'opération binaire !",
							contexte,
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
							contexte,
							m_donnees_morceaux,
							erreur::type_erreur::TYPE_DIFFERENTS);
			}

			llvm::Value *valeur;

			if (type2.type_base() == id_morceau::POINTEUR) {
				valeur = llvm::GetElementPtrInst::CreateInBounds(
							 valeur2,
							 valeur1,
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
			auto charge = new llvm::LoadInst(valeur, "", contexte.bloc_courant());
			charge->setAlignment(alignement(contexte, type2));
			return charge;
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

void NoeudOperationBinaire::perfome_validation_semantique(ContexteGenerationCode &contexte)
{
	auto enfant1 = m_enfants.front();
	auto enfant2 = m_enfants.back();

	enfant1->perfome_validation_semantique(contexte);
	enfant2->perfome_validation_semantique(contexte);

	const auto type1 = enfant1->donnees_type;
	const auto type2 = enfant2->donnees_type;

	if ((this->m_donnees_morceaux.identifiant != id_morceau::CROCHET_OUVRANT)) {
		if (!peut_operer(type1, type2, enfant1->type(), enfant2->type())) {
			erreur::lance_erreur_type_operation(
						type1,
						type2,
						contexte,
						m_donnees_morceaux);
		}
	}

	switch (this->identifiant()) {
		default:
		{
			this->donnees_type = type1;
			break;
		}
		case id_morceau::CROCHET_OUVRANT:
		{
			this->donnees_type = type2.derefence();
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
	auto type1 = enfant->donnees_type;
	auto valeur1 = enfant->genere_code_llvm(contexte);
	auto valeur2 = static_cast<llvm::Value *>(nullptr);

	switch (this->m_donnees_morceaux.identifiant) {
		case id_morceau::EXCLAMATION:
		{
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

type_noeud NoeudOperationUnaire::type() const
{
	return type_noeud::OPERATION_UNAIRE;
}

void NoeudOperationUnaire::perfome_validation_semantique(ContexteGenerationCode &contexte)
{
	auto enfant = m_enfants.front();
	enfant->perfome_validation_semantique(contexte);
	auto type = enfant->donnees_type;

	if (this->donnees_type.est_invalide()) {
		switch (this->identifiant()) {
			default:
			{
				this->donnees_type = enfant->donnees_type;
				break;
			}
			case id_morceau::AROBASE:
			{
				this->donnees_type.pousse(id_morceau::POINTEUR);
				this->donnees_type.pousse(type);
				break;
			}
			case id_morceau::EXCLAMATION:
			{
				if (type.type_base() != id_morceau::BOOL) {
					erreur::lance_erreur(
								"L'opérateur '!' doit recevoir une expression de type 'bool'",
								contexte,
								enfant->donnees_morceau(),
								erreur::type_erreur::TYPE_DIFFERENTS);
				}

				this->donnees_type.pousse(id_morceau::BOOL);
				break;
			}
		}
	}
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

llvm::Value *NoeudRetour::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	llvm::Value *valeur = nullptr;

	/* insère un appel à va_end avant chaque instruction de retour */
	if (contexte.fonction->isVarArg()) {
		const auto &donnees_fonction = contexte.donnees_fonction(std::string(contexte.fonction->getName()));

		for (const auto &arg : donnees_fonction.args) {
			if (arg.second.est_variadic) {
				auto valeur_varg = contexte.valeur_locale(arg.first);

				assert(valeur_varg != nullptr);

				auto fonc = llvm::Intrinsic::getDeclaration(contexte.module_llvm, llvm::Intrinsic::vaend);

				llvm::CallInst::Create(fonc, valeur_varg, "", contexte.bloc_courant());
				break;
			}
		}
	}

	if (!m_enfants.empty()) {
		assert(m_enfants.size() == 1);
		valeur = m_enfants.front()->genere_code_llvm(contexte);
	}

	return llvm::ReturnInst::Create(contexte.contexte, valeur, contexte.bloc_courant());
}

type_noeud NoeudRetour::type() const
{
	return type_noeud::RETOUR;
}

void NoeudRetour::perfome_validation_semantique(ContexteGenerationCode &contexte)
{
	if (m_enfants.empty()) {
		this->donnees_type.pousse(id_morceau::RIEN);
		return;
	}

	m_enfants.front()->perfome_validation_semantique(contexte);
	this->donnees_type = m_enfants.front()->donnees_type;
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

	/* noeud 2 : bloc */
	auto enfant2 = *iter_enfant++;
	enfant2->valeur_calculee = bloc_fusion;
	auto ret = enfant2->genere_code_llvm(contexte);

	/* noeud 3 : sinon (optionel) */
	if (nombre_enfants == 3) {
		contexte.bloc_courant(bloc_sinon);

		auto enfant3 = *iter_enfant++;
		enfant3->valeur_calculee = bloc_fusion;
		ret = enfant3->genere_code_llvm(contexte);
	}

	contexte.bloc_courant(bloc_fusion);

	return ret;
}

type_noeud NoeudSi::type() const
{
	return type_noeud::SI;
}

void NoeudSi::perfome_validation_semantique(ContexteGenerationCode &contexte)
{
	const auto nombre_enfants = m_enfants.size();
	auto iter_enfant = m_enfants.begin();

	auto enfant1 = *iter_enfant++;
	auto enfant2 = *iter_enfant++;

	enfant1->perfome_validation_semantique(contexte);
	auto type_condition = enfant1->donnees_type;

	if (type_condition.type_base() != id_morceau::BOOL) {
		erreur::lance_erreur("Attendu un type booléen pour l'expression 'si'",
							 contexte,
							 enfant1->donnees_morceau(),
							 erreur::type_erreur::TYPE_DIFFERENTS);
	}

	enfant2->perfome_validation_semantique(contexte);

	/* noeud 3 : sinon (optionel) */
	if (nombre_enfants == 3) {
		auto enfant3 = *iter_enfant++;
		enfant3->perfome_validation_semantique(contexte);
	}
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

	contexte.empile_nombre_locales();

	for (auto enfant : m_enfants) {
		valeur = enfant->genere_code_llvm(contexte);

		/* nul besoin de continuer à générer du code pour des expressions qui ne
		 * seront jamais executées. À FAIRE : erreur de compilation ? */
		if (est_branche_ou_retour(valeur) && bloc_entree == contexte.bloc_courant()) {
			break;
		}
	}

	auto bloc_suivant = std::any_cast<llvm::BasicBlock *>(this->valeur_calculee);

	/* Un bloc_suivant nul indique que le bloc est celui d'une fonction, mais
	 * les fonctions une logique différente. */
	if (bloc_suivant != nullptr) {
		/* Il est possible d'avoir des blocs récursifs, donc on fait une
		 * branche dans le bloc courant du contexte qui peut être différent de
		 * bloc_entree. */
		if (!est_branche_ou_retour(valeur) || (bloc_entree != contexte.bloc_courant())) {
			valeur = llvm::BranchInst::Create(bloc_suivant, contexte.bloc_courant());
		}
	}

	contexte.depile_nombre_locales();

	return valeur;
}

type_noeud NoeudBloc::type() const
{
	return type_noeud::BLOC;
}

void NoeudBloc::perfome_validation_semantique(ContexteGenerationCode &contexte)
{
	contexte.empile_nombre_locales();

	Noeud::perfome_validation_semantique(contexte);

	if (m_enfants.empty()) {
		this->donnees_type.pousse(id_morceau::RIEN);
	}
	else {
		this->donnees_type = m_enfants.back()->donnees_type;
	}

	contexte.depile_nombre_locales();
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

static llvm::Value *comparaison_pour_type(
		const id_morceau &type,
		llvm::PHINode *noeud_phi,
		llvm::Value *valeur_fin,
		llvm::BasicBlock *bloc_courant)
{
	if (est_type_entier_naturel(type)) {
		return llvm::ICmpInst::Create(
					llvm::Instruction::ICmp,
					llvm::CmpInst::Predicate::ICMP_ULE,
					noeud_phi,
					valeur_fin,
					"",
					bloc_courant);
	}

	if (est_type_entier_relatif(type)) {
		return llvm::ICmpInst::Create(
					llvm::Instruction::ICmp,
					llvm::CmpInst::Predicate::ICMP_SLE,
					noeud_phi,
					valeur_fin,
					"",
					bloc_courant);
	}

	if (est_type_reel(type)) {
		return llvm::FCmpInst::Create(
					llvm::Instruction::FCmp,
					llvm::CmpInst::Predicate::FCMP_OLE,
					noeud_phi,
					valeur_fin,
					"",
					bloc_courant);
	}

	return nullptr;
}

static llvm::Value *incremente_pour_type(
		const id_morceau &type,
		ContexteGenerationCode &contexte,
		llvm::PHINode *noeud_phi,
		llvm::BasicBlock *bloc_courant)
{
	auto type_llvm = converti_type_simple(contexte, type, nullptr);

	if (est_type_entier(type)) {
		auto val_inc = llvm::ConstantInt::get(
						   type_llvm,
						   static_cast<uint64_t>(1),
						   false);

		return llvm::BinaryOperator::Create(
					   llvm::Instruction::Add,
					   noeud_phi,
					   val_inc,
					   "",
					   bloc_courant);
	}

	if (est_type_reel(type)) {
		auto val_inc = llvm::ConstantFP::get(
						   type_llvm,
						   1.0);

		return llvm::BinaryOperator::Create(
					   llvm::Instruction::FAdd,
					   noeud_phi,
					   val_inc,
					   "",
					   bloc_courant);
	}

	return nullptr;
}

/* Arbre :
 * NoeudPour
 * - enfant 1 : déclaration variable
 * - enfant 2 : expr
 * - enfant 3 : bloc
 * - enfant 4 : bloc sansarrêt ou sinon (optionel)
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
	auto nombre_enfants = m_enfants.size();
	auto iter = m_enfants.begin();

	/* on génère d'abord le type de la variable */
	auto enfant1 = *iter++;
	auto enfant2 = *iter++;
	auto enfant3 = *iter++;
	auto enfant4 = (nombre_enfants >= 4) ? *iter++ : nullptr;
	auto enfant5 = (nombre_enfants == 5) ? *iter++ : nullptr;

	auto enfant_sans_arret = enfant4;
	auto enfant_sinon = (nombre_enfants == 5) ? enfant5 : enfant4;

	auto type_debut = enfant2->donnees_type;

	const auto type = type_debut.type_base();

	enfant1->donnees_type = type_debut;

	/* création des blocs */
	auto bloc_boucle = cree_bloc(contexte, "boucle");
	auto bloc_corps = cree_bloc(contexte, "corps_boucle");
	auto bloc_inc = cree_bloc(contexte, "inc_boucle");

	auto bloc_sansarret = static_cast<llvm::BasicBlock *>(nullptr);
	auto bloc_sinon = static_cast<llvm::BasicBlock *>(nullptr);

	if (nombre_enfants == 4) {
		if (enfant4->identifiant() == id_morceau::SINON) {
			bloc_sinon = cree_bloc(contexte, "sinon_boucle");
		}
		else {
			bloc_sansarret = cree_bloc(contexte, "sansarret_boucle");
		}
	}
	else if (nombre_enfants == 5) {
		bloc_sansarret = cree_bloc(contexte, "sansarret_boucle");
		bloc_sinon = cree_bloc(contexte, "sinon_boucle");
	}

	auto bloc_apres = cree_bloc(contexte, "apres_boucle");

	contexte.empile_bloc_continue(enfant1->chaine(), bloc_inc);
	contexte.empile_bloc_arrete(enfant1->chaine(), (bloc_sinon != nullptr) ? bloc_sinon : bloc_apres);

	auto bloc_pre = contexte.bloc_courant();

	contexte.empile_nombre_locales();

	auto noeud_phi = static_cast<llvm::PHINode *>(nullptr);

	/* bloc_boucle */
	/* on crée une branche explicite dans le bloc */
	llvm::BranchInst::Create(bloc_boucle, contexte.bloc_courant());

	contexte.bloc_courant(bloc_boucle);

	if (enfant2->type() == type_noeud::PLAGE) {
		noeud_phi = llvm::PHINode::Create(
						converti_type(contexte, type_debut),
						2,
						std::string(enfant1->chaine()),
						contexte.bloc_courant());
	}
	else if (enfant2->type() == type_noeud::VARIABLE) {
		noeud_phi = llvm::PHINode::Create(
						llvm::Type::getInt32Ty(contexte.contexte),
						2,
						std::string(enfant1->chaine()),
						contexte.bloc_courant());
	}

	if (enfant2->type() == type_noeud::PLAGE) {
		enfant2->genere_code_llvm(contexte);

		auto valeur_debut = contexte.valeur_locale("__debut");
		auto valeur_fin = contexte.valeur_locale("__fin");

		noeud_phi->addIncoming(valeur_debut, bloc_pre);

		auto condition = comparaison_pour_type(
							 type,
							 noeud_phi,
							 valeur_fin,
							 contexte.bloc_courant());

		llvm::BranchInst::Create(
					bloc_corps,
					(bloc_sansarret != nullptr) ? bloc_sansarret : bloc_apres,
					condition,
					contexte.bloc_courant());

		contexte.pousse_locale(enfant1->chaine(), noeud_phi, type_debut, false, false);
	}
	else if (enfant2->type() == type_noeud::VARIABLE) {
		auto arg = enfant2->genere_code_llvm(contexte);
		contexte.pousse_locale(enfant1->chaine(), arg, type_debut, false, false);

		auto valeur_debut = llvm::ConstantInt::get(
								llvm::Type::getInt32Ty(contexte.contexte),
								static_cast<uint64_t>(0),
								false);

		auto valeur_fin = contexte.valeur_locale("__compte_args");

		/* __compte_args est une AllocaInst (dans NoeudDeclarationFonction)
		 * donc il faut le charger */
		auto charge_valeur_fin = new llvm::LoadInst(
									 valeur_fin, "", contexte.bloc_courant());

		noeud_phi->addIncoming(valeur_debut, bloc_pre);

		auto condition = llvm::ICmpInst::Create(
							 llvm::Instruction::ICmp,
							 llvm::CmpInst::Predicate::ICMP_SLT,
							 noeud_phi,
							 charge_valeur_fin,
							 "",
							 contexte.bloc_courant());

		llvm::BranchInst::Create(
					bloc_corps,
					(bloc_sansarret != nullptr) ? bloc_sansarret : bloc_apres,
					condition,
					contexte.bloc_courant());
	}

	/* bloc_corps */
	contexte.bloc_courant(bloc_corps);
	enfant3->valeur_calculee = bloc_inc;
	auto ret = enfant3->genere_code_llvm(contexte);

	/* bloc_inc */
	contexte.bloc_courant(bloc_inc);

	if (enfant2->type() == type_noeud::PLAGE) {
		auto inc = incremente_pour_type(
					   type,
					   contexte,
					   noeud_phi,
					   contexte.bloc_courant());

		noeud_phi->addIncoming(inc, contexte.bloc_courant());
	}
	else if (enfant2->type() == type_noeud::VARIABLE) {
		auto inc = incremente_pour_type(
					   id_morceau::Z32,
					   contexte,
					   noeud_phi,
					   contexte.bloc_courant());

		noeud_phi->addIncoming(inc, contexte.bloc_courant());
	}

	ret = llvm::BranchInst::Create(bloc_boucle, contexte.bloc_courant());

	/* 'continue'/'arrête' dans les blocs 'sinon'/'sansarrêt' n'a aucun sens */
	contexte.depile_bloc_continue();
	contexte.depile_bloc_arrete();

	if (bloc_sansarret != nullptr) {
		contexte.bloc_courant(bloc_sansarret);
		enfant_sans_arret->valeur_calculee = bloc_apres;
		ret = enfant_sans_arret->genere_code_llvm(contexte);
	}

	if (bloc_sinon != nullptr) {
		contexte.bloc_courant(bloc_sinon);
		enfant_sinon->valeur_calculee = bloc_apres;
		ret = enfant_sinon->genere_code_llvm(contexte);
	}

	contexte.depile_nombre_locales();
	contexte.bloc_courant(bloc_apres);

	return ret;
}

type_noeud NoeudPour::type() const
{
	return type_noeud::POUR;
}

void NoeudPour::perfome_validation_semantique(ContexteGenerationCode &contexte)
{
	const auto nombre_enfants = m_enfants.size();
	auto iter = m_enfants.begin();

	/* on génère d'abord le type de la variable */
	auto enfant1 = *iter++;
	auto enfant2 = *iter++;
	auto enfant3 = *iter++;
	auto enfant4 = (nombre_enfants >= 4) ? *iter++ : nullptr;
	auto enfant5 = (nombre_enfants == 5) ? *iter++ : nullptr;

	if (contexte.locale_existe(enfant1->chaine())) {
		erreur::lance_erreur(
					"Rédéfinition de la variable",
					contexte,
					enfant1->donnees_morceau(),
					erreur::type_erreur::VARIABLE_REDEFINIE);
	}

	if (contexte.globale_existe(enfant1->chaine())) {
		erreur::lance_erreur(
					"Rédéfinition de la variable globale",
					contexte,
					enfant1->donnees_morceau(),
					erreur::type_erreur::VARIABLE_REDEFINIE);
	}


	if (enfant2->type() == type_noeud::PLAGE) {
	}
	else if (enfant2->type() == type_noeud::VARIABLE) {
		auto valeur = contexte.est_locale_variadique(enfant2->chaine());

		if (!valeur) {
			erreur::lance_erreur(
						"La variable n'est pas un argument variadic",
						contexte,
						enfant2->donnees_morceau());
		}
	}
	else {
		erreur::lance_erreur(
					"Expression inattendu dans la boucle 'pour'",
					contexte,
					m_donnees_morceaux);
	}

	enfant2->perfome_validation_semantique(contexte);

	contexte.empile_nombre_locales();

	contexte.pousse_locale(enfant1->chaine(), nullptr, enfant2->donnees_type, false, false);

	enfant3->perfome_validation_semantique(contexte);

	if (enfant4 != nullptr) {
		enfant4->perfome_validation_semantique(contexte);

		if (enfant5 != nullptr) {
			enfant5->perfome_validation_semantique(contexte);
		}
	}

	contexte.depile_nombre_locales();
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
	auto chaine_var = m_enfants.empty() ? std::string_view{""} : m_enfants.front()->chaine();

	auto bloc = (m_donnees_morceaux.identifiant == id_morceau::CONTINUE)
				? contexte.bloc_continue(chaine_var)
				: contexte.bloc_arrete(chaine_var);

	if (bloc == nullptr) {
		if (chaine_var.empty()) {
			erreur::lance_erreur(
						"'continue' ou 'arrête' en dehors d'une boucle",
						contexte,
						m_donnees_morceaux,
						erreur::type_erreur::CONTROLE_INVALIDE);
		}
		else {
			erreur::lance_erreur(
						"Variable inconnue",
						contexte,
						m_enfants.front()->donnees_morceau(),
						erreur::type_erreur::VARIABLE_INCONNUE);
		}
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

	contexte.empile_bloc_continue("", bloc_boucle);
	contexte.empile_bloc_arrete("", (enfant2 != nullptr) ? bloc_sinon : bloc_apres);

	/* on crée une branche explicite dans le bloc */
	llvm::BranchInst::Create(bloc_boucle, contexte.bloc_courant());

	contexte.bloc_courant(bloc_boucle);

	enfant1->valeur_calculee = bloc_boucle;
	auto ret = enfant1->genere_code_llvm(contexte);

	if (bloc_sinon != nullptr) {
		contexte.bloc_courant(bloc_sinon);

		/* génère le code du bloc */
		enfant2->valeur_calculee = bloc_apres;
		ret = enfant2->genere_code_llvm(contexte);
	}

	contexte.depile_bloc_continue();
	contexte.depile_bloc_arrete();
	contexte.bloc_courant(bloc_apres);

	return ret;
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
	auto enfant = m_enfants.front();
	auto valeur = enfant->genere_code_llvm(contexte);
	const auto &donnees_type_de = enfant->donnees_type;

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
				contexte,
				this->donnees_morceau());
}

type_noeud NoeudTranstype::type() const
{
	return type_noeud::TRANSTYPE;
}

void NoeudTranstype::perfome_validation_semantique(ContexteGenerationCode &contexte)
{
	if (this->donnees_type.est_invalide()) {
		erreur::lance_erreur(
					"Ne peut transtyper vers un type invalide",
					contexte,
					this->donnees_morceau(),
					erreur::type_erreur::TYPE_INCONNU);
	}

	auto enfant = m_enfants.front();
	enfant->perfome_validation_semantique(contexte);
	const auto &donnees_type_de = enfant->donnees_type;

	if (donnees_type_de.est_invalide()) {
		erreur::lance_erreur(
					"Ne peut calculer le type d'origine",
					contexte,
					enfant->donnees_morceau(),
					erreur::type_erreur::TYPE_INCONNU);
	}
}

/* ************************************************************************** */

NoeudNul::NoeudNul(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{
	this->donnees_type.pousse(id_morceau::POINTEUR);
	this->donnees_type.pousse(id_morceau::NUL);
}

void NoeudNul::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);
	os << "NoeudNul\n";
}

llvm::Value *NoeudNul::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	return llvm::ConstantInt::get(
				llvm::Type::getInt32Ty(contexte.contexte),
				static_cast<uint64_t>(0),
				false);
}

type_noeud NoeudNul::type() const
{
	return type_noeud::NUL;
}

/* ************************************************************************** */

NoeudTailleDe::NoeudTailleDe(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{
	this->donnees_type.pousse(id_morceau::N32);
}

void NoeudTailleDe::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);
	auto donnees = std::any_cast<DonneesType>(this->valeur_calculee);
	os << "NoeudTailleDe : " << this->donnees_type << '\n';
}

llvm::Value *NoeudTailleDe::genere_code_llvm(
		ContexteGenerationCode &contexte,
		const bool /*expr_gauche*/)
{
	auto dl = llvm::DataLayout(contexte.module_llvm);
	auto donnees = std::any_cast<DonneesType>(this->valeur_calculee);
	auto type = converti_type(contexte, donnees);
	auto taille = dl.getTypeAllocSize(type);

	return llvm::ConstantInt::get(
				llvm::Type::getInt32Ty(contexte.contexte),
				taille,
				false);
}

type_noeud NoeudTailleDe::type() const
{
	return type_noeud::TAILLE_DE;
}

/* ************************************************************************** */

NoeudPlage::NoeudPlage(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudPlage::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);
	os << "NoeudPlage : ...\n";

	for (auto &enfant : m_enfants) {
		enfant->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudPlage::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	auto iter = m_enfants.begin();

	auto enfant1 = *iter++;
	auto enfant2 = *iter++;

	auto valeur_debut = enfant1->genere_code_llvm(contexte);
	auto valeur_fin = enfant2->genere_code_llvm(contexte);

	contexte.pousse_locale("__debut", valeur_debut, this->donnees_type, false, false);
	contexte.pousse_locale("__fin", valeur_fin, this->donnees_type, false, false);

	return valeur_fin;
}

type_noeud NoeudPlage::type() const
{
	return type_noeud::PLAGE;
}

void NoeudPlage::perfome_validation_semantique(ContexteGenerationCode &contexte)
{
	auto iter = m_enfants.begin();

	auto enfant1 = *iter++;
	auto enfant2 = *iter++;

	enfant1->perfome_validation_semantique(contexte);
	enfant2->perfome_validation_semantique(contexte);

	auto type_debut = enfant1->donnees_type;
	auto type_fin   = enfant2->donnees_type;

	if (type_debut.est_invalide() || type_fin.est_invalide()) {
		erreur::lance_erreur(
					"Les types de l'expression sont invalides !",
					contexte,
					m_donnees_morceaux,
					erreur::type_erreur::TYPE_INCONNU);
	}

	if (type_debut != type_fin) {
		erreur::lance_erreur_type_operation(
					type_debut,
					type_fin,
					contexte,
					m_donnees_morceaux);
	}

	const auto type = type_debut.type_base();

	if (!est_type_entier_naturel(type) && !est_type_entier_relatif(type) && !est_type_reel(type)) {
		erreur::lance_erreur(
					"Attendu des types réguliers dans la plage de la boucle 'pour'",
					contexte,
					this->donnees_morceau(),
					erreur::type_erreur::TYPE_DIFFERENTS);
	}

	this->donnees_type = type_debut;

	contexte.pousse_locale("__debut", nullptr, this->donnees_type, false, false);
	contexte.pousse_locale("__fin", nullptr, this->donnees_type, false, false);
}

/* ************************************************************************** */

NoeudAccesMembrePoint::NoeudAccesMembrePoint(const DonneesMorceaux &morceau)
	: Noeud(morceau)
{}

void NoeudAccesMembrePoint::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);
	os << "NoeudAccesMembrePoint : .\n";

	for (auto &enfant : m_enfants) {
		enfant->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudAccesMembrePoint::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	return m_enfants.back()->genere_code_llvm(contexte);
}

type_noeud NoeudAccesMembrePoint::type() const
{
	return type_noeud::ACCES_MEMBRE_POINT;
}

void NoeudAccesMembrePoint::perfome_validation_semantique(ContexteGenerationCode &contexte)
{
	auto enfant1 = m_enfants.front();
	auto enfant2 = m_enfants.back();

	const auto nom_module = enfant1->chaine();

	auto module = contexte.module(static_cast<size_t>(m_donnees_morceaux.module));

	if (!module->importe_module(nom_module)) {
		erreur::lance_erreur(
					"module inconnu",
					contexte,
					enfant1->donnees_morceau(),
					erreur::type_erreur::MODULE_INCONNU);
	}

	auto module_importe = contexte.module(nom_module);

	if (module_importe == nullptr) {
		erreur::lance_erreur(
					"module inconnu",
					contexte,
					enfant1->donnees_morceau(),
					erreur::type_erreur::MODULE_INCONNU);
	}

	const auto nom_fonction = enfant2->chaine();

	if (!module_importe->possede_fonction(nom_fonction)) {
		erreur::lance_erreur(
					"Le module ne possède pas la fonction",
					contexte,
					enfant2->donnees_morceau(),
					erreur::type_erreur::FONCTION_INCONNUE);
	}

	enfant2->module_appel = static_cast<int>(module_importe->id);

	enfant2->perfome_validation_semantique(contexte);

	this->donnees_type = enfant2->donnees_type;
}
