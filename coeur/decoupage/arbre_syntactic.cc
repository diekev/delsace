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

#include <iostream>
#include <set>
#include <sstream>

#include <delsace/chrono/chronometrage.hh>

#include "contexte_generation_code.h"
#include "donnees_type.h"
#include "erreur.h"
#include "modules.hh"
#include "morceaux.h"
#include "nombres.h"
#include "tampon_source.h"

#undef NOMME_IR

/* ************************************************************************** */

static auto cree_bloc(ContexteGenerationCode &contexte, char const *nom)
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
		DonneesType &donnees_type);

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
			auto const &id_structure = (static_cast<uint64_t>(identifiant) & 0xffffff00) >> 8;
			auto &donnees_structure = contexte.donnees_structure(id_structure);

			if (donnees_structure.type_llvm == nullptr) {
				std::vector<llvm::Type *> types_membres;
				types_membres.resize(donnees_structure.donnees_types.size());

				std::transform(donnees_structure.donnees_types.begin(),
							   donnees_structure.donnees_types.end(),
							   types_membres.begin(),
							   [&](const size_t index_type)
				{
					auto &dt = contexte.magasin_types.donnees_types[index_type];
					return converti_type(contexte, dt);
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
			auto const taille = (static_cast<uint64_t>(identifiant) & 0xffffff00) >> 8;

			if (taille != 0) {
				type = llvm::ArrayType::get(type_entree, taille);
			}
			else {
				/* type = structure { *type, n64 } */
				std::vector<llvm::Type *> types_membres(2ul);
				types_membres[0] = llvm::PointerType::get(type_entree, 0);
				types_membres[1] = llvm::Type::getInt64Ty(contexte.contexte);

				type = llvm::StructType::create(
						   contexte.contexte,
						   types_membres,
						   "struct.tableau",
						   false);
			}

			break;
		}
		default:
			assert(false);
	}

	return type;
}

/**
 * Retourne un vecteur contenant les DonneesType de chaque paramètre et du type
 * de retour d'un DonneesType d'un pointeur fonction. Si le DonneesType passé en
 * paramètre n'est pas un pointeur fonction, retourne un vecteur vide.
 */
[[nodiscard]] static auto donnees_types_parametres(
		const DonneesType &donnees_type) noexcept(false) -> std::vector<DonneesType>
{
	if (donnees_type.type_base() != id_morceau::FONCTION) {
		return {};
	}

	auto dt = DonneesType{};
	std::vector<DonneesType> donnees_types;

	auto debut = donnees_type.end() - 1;
	auto fin   = donnees_type.begin() - 1;

	--debut; /* fonction */
	--debut; /* ( */

	/* type paramètres */
	while (*debut != id_morceau::PARENTHESE_FERMANTE) {
		while (*debut != id_morceau::PARENTHESE_FERMANTE) {
			dt.pousse(*debut--);

			if (*debut == id_morceau::VIRGULE) {
				--debut;
				break;
			}
		}

		donnees_types.push_back(dt);

		dt = DonneesType{};
	}

	--debut; /* ) */

	/* type retour */
	while (debut != fin) {
		dt.pousse(*debut--);
	}

	donnees_types.push_back(dt);

	return donnees_types;
}

static llvm::Type *converti_type(
		ContexteGenerationCode &contexte,
		DonneesType &donnees_type)
{

	/* Pointeur vers une fonction, seulement valide lors d'assignement, ou en
	 * paramètre de fonction. */
	if (donnees_type.type_base() == id_morceau::FONCTION) {
		if (donnees_type.type_llvm() != nullptr) {
			return llvm::PointerType::get(donnees_type.type_llvm(), 0);
		}

		llvm::Type *type = nullptr;
		auto dt = DonneesType{};
		std::vector<llvm::Type *> parametres;

		auto dt_params = donnees_types_parametres(donnees_type);

		for (size_t i = 0; i < dt_params.size() - 1; ++i) {
			type = converti_type(contexte, dt_params[i]);
			parametres.push_back(type);
		}

		type = converti_type(contexte, dt_params.back());
		type = llvm::FunctionType::get(
					type,
					parametres,
					false);

		donnees_type.type_llvm(type);

		return llvm::PointerType::get(type, 0);
	}

	if (donnees_type.type_llvm() != nullptr) {
		return donnees_type.type_llvm();
	}

	llvm::Type *type = nullptr;

	for (id_morceau identifiant : donnees_type) {
		type = converti_type_simple(contexte, identifiant, type);
	}

	donnees_type.type_llvm(type);

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
			if (size_t(identifiant >> 8) == 0) {
				return 8;
			}

			return alignement(contexte, donnees_type.derefence());
		}
		case id_morceau::FONCTION:
		case id_morceau::POINTEUR:
		case id_morceau::R64:
		case id_morceau::N64:
		case id_morceau::Z64:
			return 8;
		case id_morceau::CHAINE_CARACTERE:
		{
			auto const &id_structure = (static_cast<uint64_t>(identifiant) & 0xffffff00) >> 8;
			auto &donnees_structure = contexte.donnees_structure(id_structure);

			auto a = 0u;

			for (auto const &donnees : donnees_structure.donnees_types) {
				auto const &dt = contexte.magasin_types.donnees_types[donnees];
				a = std::max(a, alignement(contexte, dt));
			}

			return a;
		}
		default:
			assert(false);
	}

	return 0;
}

enum class niveau_compat : char {
	aucune,
	ok,
	converti_tableau,
};

/**
 * Retourne vrai si les deux types peuvent être convertis silencieusement par le
 * compileur.
 */
static niveau_compat sont_compatibles(
		const DonneesType &type1,
		const DonneesType &type2)
{
	if (type1 == type2) {
		return niveau_compat::ok;
	}

	/* Nous savons que les types sont différents, donc si l'un des deux est un
	 * pointeur fonction, nous pouvons retourner faux. */
	if (type1.type_base() == id_morceau::FONCTION) {
		return niveau_compat::aucune;
	}

	if (type1.type_base() == id_morceau::TABLEAU) {
		if ((type2.type_base() & 0xff) != id_morceau::TABLEAU) {
			return niveau_compat::aucune;
		}

		if (type1.derefence() == type2.derefence()) {
			return niveau_compat::converti_tableau;
		}

		return niveau_compat::aucune;
	}

	/* À FAIRE : C-strings */
	if (type1.type_base() == id_morceau::POINTEUR) {
		if (type1.derefence().type_base() != id_morceau::Z8) {
			return niveau_compat::aucune;
		}

		if ((type2.type_base() & 0xff) == id_morceau::TABLEAU) {
			if (size_t(type2.type_base() >> 8) == 0) {
				return niveau_compat::aucune;
			}
		}

		if (type2.derefence().type_base() == id_morceau::Z8) {
			return niveau_compat::ok;
		}
	}

	return niveau_compat::aucune;
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

char caractere_echape(char const *sequence)
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
		DonneesType &donnees_retour,
		bool est_variadique)
{
	std::vector<llvm::Type *> parametres;
	parametres.reserve(donnees_fonction.nom_args.size());

	for (auto const &nom : donnees_fonction.nom_args) {
		auto const &argument = donnees_fonction.args.find(nom);

		if (argument->second.est_variadic) {
			/* ajout de l'argument implicite du compte d'arguments var_args */
			if (!donnees_fonction.est_externe) {
				parametres.push_back(llvm::Type::getInt32Ty(contexte.contexte));
			}

			break;
		}

		auto &dt = contexte.magasin_types.donnees_types[argument->second.donnees_type];
		parametres.push_back(converti_type(contexte, dt));
	}

	return llvm::FunctionType::get(
				converti_type(contexte, donnees_retour),
				parametres,
				est_variadique);
}

/* ************************************************************************** */

static void genere_code_extra_pre_retour(ContexteGenerationCode &contexte, int index_module)
{
	auto module = contexte.module(static_cast<size_t>(index_module));

	/* insère un appel à va_end avant chaque instruction de retour */
	if (contexte.fonction->isVarArg()) {
		auto const &donnees_fonction = module->donnees_fonction(std::string(contexte.fonction->getName()));
		auto const &nom_dernier_arg = donnees_fonction.nom_args.back();
		auto valeur_varg = contexte.valeur_locale(nom_dernier_arg);

		assert(valeur_varg != nullptr);

		auto fonc = llvm::Intrinsic::getDeclaration(contexte.module_llvm, llvm::Intrinsic::vaend);

		llvm::CallInst::Create(fonc, valeur_varg, "", contexte.bloc_courant());
	}

	/* génère le code pour les blocs déférés */
	auto pile_noeud = contexte.noeuds_deferes();

	while (!pile_noeud.empty()) {
		auto noeud = pile_noeud.top();
		noeud->genere_code_llvm(contexte);
		pile_noeud.pop();
	}
}

/* ************************************************************************** */

Noeud::Noeud(ContexteGenerationCode &/*contexte*/, DonneesMorceaux const &morceau)
	: m_donnees_morceaux{morceau}
{}

bool Noeud::est_constant() const
{
	return false;
}

std::string_view const &Noeud::chaine() const
{
	return m_donnees_morceaux.chaine;
}

bool Noeud::peut_etre_assigne(ContexteGenerationCode &/*contexte*/) const
{
	return false;
}

DonneesMorceaux const &Noeud::donnees_morceau() const
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

NoeudRacine::NoeudRacine(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{}

void NoeudRacine::imprime_code(std::ostream &os, int tab)
{
	os << "NoeudRacine\n";

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudRacine::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
{
	auto temps_validation = 0.0;
	auto temps_generation = 0.0;

	for (auto noeud : m_enfants) {
		auto debut_validation = dls::chrono::maintenant();
		noeud->perfome_validation_semantique(contexte);
		temps_validation += dls::chrono::delta(debut_validation);

		auto debut_generation = dls::chrono::maintenant();
		noeud->genere_code_llvm(contexte);
		temps_generation += dls::chrono::delta(debut_generation);
	}

	contexte.temps_generation = temps_generation;
	contexte.temps_validation = temps_validation;

	return nullptr;
}

type_noeud NoeudRacine::type() const
{
	return type_noeud::RACINE;
}

/* ************************************************************************** */

enum {
	/* TABLEAUX */
	POINTEUR_TABLEAU = 0,
	TAILLE_TABLEAU = 1,
};

[[nodiscard]] static llvm::Value *accede_membre_structure(
		ContexteGenerationCode &contexte,
		llvm::Value *structure,
		uint64_t index,
		bool charge = false)
{
	auto ptr = llvm::GetElementPtrInst::CreateInBounds(
			  structure, {
				  llvm::ConstantInt::get(llvm::Type::getInt32Ty(contexte.contexte), 0),
				  llvm::ConstantInt::get(llvm::Type::getInt32Ty(contexte.contexte), index)
			  },
			  "",
			  contexte.bloc_courant());

	if (charge == true) {
		return new llvm::LoadInst(ptr, "", contexte.bloc_courant());
	}

	return ptr;
}

[[nodiscard]] static llvm::Value *accede_element_tableau(
		ContexteGenerationCode &contexte,
		llvm::Value *structure,
		llvm::Type *type,
		llvm::Value *index)
{
	return llvm::GetElementPtrInst::CreateInBounds(
				type,
				structure, {
					llvm::ConstantInt::get(llvm::Type::getInt32Ty(contexte.contexte), 0),
					index
				},
				"",
				contexte.bloc_courant());
}

[[nodiscard]] static llvm::Value *accede_element_tableau(
		ContexteGenerationCode &contexte,
		llvm::Value *structure,
		llvm::Type *type,
		uint64_t index)
{
	return accede_element_tableau(
				contexte,
				structure,
				type,
				llvm::ConstantInt::get(llvm::Type::getInt32Ty(contexte.contexte), index));
}

[[nodiscard]] static auto converti_vers_tableau_dyn(
		ContexteGenerationCode &contexte,
		llvm::Value *tableau,
		DonneesType const &donnees_type)
{
	/* trouve le type de la structure tableau */
	auto deref = donnees_type.derefence();
	auto dt = DonneesType{};
	dt.pousse(id_morceau::TABLEAU);
	dt.pousse(deref);

	auto type_llvm = converti_type(contexte, dt);

	/* alloue de l'espace pour ce type */
	auto alloc = new llvm::AllocaInst(type_llvm, "", contexte.bloc_courant());
	alloc->setAlignment(8);

	/* copie le pointeur du début du tableau */
	auto ptr_valeur = accede_membre_structure(contexte, alloc, POINTEUR_TABLEAU);

	/* charge le premier élément du tableau */
	auto premier_elem = accede_element_tableau(
							contexte,
							tableau,
							donnees_type.type_llvm(),
							0ul);

	auto charge = new llvm::LoadInst(premier_elem, "", contexte.bloc_courant());
	charge->setAlignment(8);
	auto addresse = charge->getPointerOperand();

	auto stocke = new llvm::StoreInst(addresse, ptr_valeur, contexte.bloc_courant());
	stocke->setAlignment(8);

	/* copie la taille du tableau */
	auto ptr_taille = accede_membre_structure(contexte, alloc, TAILLE_TABLEAU);

	auto taille_tableau = donnees_type.type_base() >> 8;
	auto constante = llvm::ConstantInt::get(
						 llvm::Type::getInt64Ty(contexte.contexte),
						 uint64_t(taille_tableau),
						 false);

	stocke = new llvm::StoreInst(constante, ptr_taille, contexte.bloc_courant());
	stocke->setAlignment(8);

	charge = new llvm::LoadInst(alloc, "", contexte.bloc_courant());
	charge->setAlignment(8);
	return charge;
}

NoeudAppelFonction::NoeudAppelFonction(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
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

void NoeudAppelFonction::verifie_compatibilite(
		ContexteGenerationCode &contexte,
		const DonneesType &type_arg,
		const DonneesType &type_enf,
		Noeud *enfant)
{
	auto compat = sont_compatibles(type_arg, type_enf);

	if (compat == niveau_compat::aucune) {
		erreur::lance_erreur_type_arguments(
					type_arg,
					type_enf,
					contexte,
					enfant->donnees_morceau(),
					m_donnees_morceaux);
	}
	else if (compat == niveau_compat::converti_tableau) {
		enfant->drapeaux |= CONVERTI_TABLEAU;
	}
}

template <typename Conteneur>
llvm::Value *cree_appel(
		ContexteGenerationCode &contexte,
		llvm::Value *fonction,
		Conteneur const &conteneur)
{
	std::vector<llvm::Value *> parametres(conteneur.size());

	std::transform(conteneur.begin(), conteneur.end(), parametres.begin(),
				   [&](Noeud *noeud_enfant)
	{
		auto conversion = (noeud_enfant->drapeaux & CONVERTI_TABLEAU) != 0;
		auto valeur_enfant = noeud_enfant->genere_code_llvm(contexte, conversion);

		if (conversion) {
			auto const &dt = contexte.magasin_types.donnees_types[noeud_enfant->donnees_type];
			valeur_enfant = converti_vers_tableau_dyn(contexte, valeur_enfant, dt);
		}

		return valeur_enfant;
	});

	llvm::ArrayRef<llvm::Value *> args(parametres);

	return llvm::CallInst::Create(fonction, args, "", contexte.bloc_courant());
}

llvm::Value *NoeudAppelFonction::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
{
	/* broyage du nom */
	auto module = contexte.module(static_cast<size_t>(this->module_appel));
	auto nom_module = module->nom;
	auto nom_fonction = std::string(m_donnees_morceaux.chaine);
	auto nom_broye = nom_module.empty() ? nom_fonction : nom_module + '_' + nom_fonction;

	auto fonction = contexte.module_llvm->getFunction(nom_broye);
	auto est_pointeur_fonction = (fonction == nullptr && contexte.locale_existe(m_donnees_morceaux.chaine));

	/* Cherche la liste d'arguments */
	if (est_pointeur_fonction) {
		auto index_type = contexte.type_locale(m_donnees_morceaux.chaine);
		auto &dt_fonc = contexte.magasin_types.donnees_types[index_type];
		auto dt_params = donnees_types_parametres(dt_fonc);

		auto enfant = m_enfants.begin();

		/* Validation des types passés en paramètre. */
		for (size_t i = 0; i < dt_params.size() - 1; ++i) {
			auto &type_enf = contexte.magasin_types.donnees_types[(*enfant)->donnees_type];
			verifie_compatibilite(contexte, dt_params[i], type_enf, *enfant);
			++enfant;
		}

		auto valeur = contexte.valeur_locale(m_donnees_morceaux.chaine);

		auto charge = new llvm::LoadInst(valeur, "", false, contexte.bloc_courant());
		/* À FAIRE : alignement pointeur. */
		charge->setAlignment(8);

		return cree_appel(contexte, charge, m_enfants);
	}

	auto const &donnees_fonction = module->donnees_fonction(m_donnees_morceaux.chaine);

	auto fonction_variadique_interne = fonction->isVarArg() && !donnees_fonction.est_externe;

	/* Réordonne les enfants selon l'apparition des arguments car LLVM est
	 * tatillon : ce n'est pas l'ordre dans lequel les valeurs apparaissent
	 * dans le vecteur de paramètres qui compte, mais l'ordre dans lequel le
	 * code est généré. */
	auto noms_arguments = std::any_cast<std::list<std::string_view>>(&valeur_calculee);
	std::vector<Noeud *> enfants(noms_arguments->size() + fonction_variadique_interne);

	auto noeud_nombre_args = static_cast<NoeudNombreEntier *>(nullptr);

	if (fonction_variadique_interne) {
		/* Pour les fonctions variadiques, il nous faut ajouter le nombre
		 * d'arguments à l'appel de la fonction. */
		auto nombre_args = donnees_fonction.args.size();
		auto nombre_args_var = std::max(0ul, noms_arguments->size() - (nombre_args - 1));
		auto index_premier_var_arg = nombre_args - 1;

		noeud_nombre_args = new NoeudNombreEntier(contexte, {});
		noeud_nombre_args->valeur_calculee = static_cast<long>(nombre_args_var);
		noeud_nombre_args->calcule = true;

		enfants[index_premier_var_arg] = noeud_nombre_args;
	}

	auto enfant = m_enfants.begin();
	auto nombre_arg_variadic = 0ul + fonction_variadique_interne;

	for (auto const &nom : *noms_arguments) {
		/* Pas la peine de vérifier qu'iter n'est pas égal à la fin de la table
		 * car ça a déjà été fait dans l'analyse grammaticale. */
		auto const iter = donnees_fonction.args.find(nom);
		auto index_arg = iter->second.index;
		auto const index_type_arg = iter->second.donnees_type;
		auto const index_type_enf = (*enfant)->donnees_type;
		auto const &type_arg = index_type_arg == -1ul ? DonneesType{} : contexte.magasin_types.donnees_types[index_type_arg];
		auto const &type_enf = contexte.magasin_types.donnees_types[index_type_enf];

		if (iter->second.est_variadic) {
			if (!type_arg.est_invalide()) {
				verifie_compatibilite(contexte, type_arg, type_enf, *enfant);
			}

			/* Décale l'index selon le nombre d'arguments dans l'argument
			 * variadique, car ici index_arg est l'index dans la déclaration et
			 * la déclaration ne contient qu'un seul argument variadic. */
			index_arg += nombre_arg_variadic;

			++nombre_arg_variadic;
		}
		else {
			verifie_compatibilite(contexte, type_arg, type_enf, *enfant);
		}

		enfants[index_arg] = *enfant;

		++enfant;
	}

	auto appel = cree_appel(contexte, fonction, enfants);

	delete noeud_nombre_args;

	return appel;
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

	auto noms_arguments = std::any_cast<std::list<std::string_view>>(&valeur_calculee);

	if (!module->possede_fonction(m_donnees_morceaux.chaine)) {
		/* Nous avons un pointeur vers une fonction. */
		if (contexte.locale_existe(m_donnees_morceaux.chaine)) {
			for (auto const &nom : *noms_arguments) {
				if (nom.empty()) {
					continue;
				}

				/* À FAIRE : trouve les données morceaux idoines. */
				erreur::lance_erreur(
							"Les arguments d'un pointeur fonction ne peuvent être nommés",
							contexte,
							this->donnees_morceau(),
							erreur::type_erreur::ARGUMENT_INCONNU);
			}

			/* À FAIRE : bouge ça, trouve le type retour du pointeur de fonction. */

			auto const &dt_pf = contexte.magasin_types.donnees_types[contexte.type_locale(m_donnees_morceaux.chaine)];

			if (dt_pf.type_base() != id_morceau::FONCTION) {
				erreur::lance_erreur(
							"La variable doit être un pointeur vers une fonction",
							contexte,
							this->donnees_morceau(),
							erreur::type_erreur::FONCTION_INCONNUE);
			}

			auto debut = dt_pf.end() - 1;
			auto fin   = dt_pf.begin() - 1;

			while (*debut != id_morceau::PARENTHESE_FERMANTE) {
				--debut;
			}

			--debut;

			auto dt = DonneesType{};

			while (debut != fin) {
				dt.pousse(*debut--);
			}

			this->donnees_type = contexte.magasin_types.ajoute_type(dt);

			Noeud::perfome_validation_semantique(contexte);
			return;
		}

		erreur::lance_erreur(
					"Fonction inconnue",
					contexte,
					m_donnees_morceaux,
					erreur::type_erreur::FONCTION_INCONNUE);
	}

	auto const &donnees_fonction = module->donnees_fonction(m_donnees_morceaux.chaine);

	if (!donnees_fonction.est_variadique && (m_enfants.size() != donnees_fonction.args.size())) {
		erreur::lance_erreur_nombre_arguments(
					donnees_fonction.args.size(),
					m_enfants.size(),
					contexte,
					m_donnees_morceaux);
	}

	if (this->donnees_type == -1ul) {
		this->donnees_type = donnees_fonction.index_type_retour;
	}

	/* vérifie que les arguments soient proprement nommés */
	auto arguments_nommes = false;
	std::set<std::string_view> args;
	auto dernier_arg_variadique = false;
	auto const nombre_args = donnees_fonction.args.size();

	auto index = 0ul;
	auto const index_max = nombre_args - donnees_fonction.est_variadique;

	for (auto &nom_arg : *noms_arguments) {
		if (nom_arg != "") {
			arguments_nommes = true;

			auto iter = donnees_fonction.args.find(nom_arg);

			if (iter == donnees_fonction.args.end()) {
				erreur::lance_erreur_argument_inconnu(
							nom_arg,
							contexte,
							this->donnees_morceau());
			}

			if ((args.find(nom_arg) != args.end()) && !iter->second.est_variadic) {
				/* À FAIRE : trouve le morceau correspondant à l'argument. */
				erreur::lance_erreur("Argument déjà nommé",
									 contexte,
									 this->donnees_morceau(),
									 erreur::type_erreur::ARGUMENT_REDEFINI);
			}

			dernier_arg_variadique = iter->second.est_variadic;

			args.insert(nom_arg);
		}
		else {
			if (arguments_nommes == true && dernier_arg_variadique == false) {
				/* À FAIRE : trouve le morceau correspondant à l'argument. */
				erreur::lance_erreur("Attendu le nom de l'argument",
									 contexte,
									 this->donnees_morceau(),
									 erreur::type_erreur::ARGUMENT_INCONNU);
			}

			if (nombre_args != 0) {
				auto nom_argument = donnees_fonction.nom_args[index];
				args.insert(nom_argument);
				nom_arg = nom_argument;
			}
		}

		index = std::min(index + 1, index_max);
	}

	Noeud::perfome_validation_semantique(contexte);
}

/* ************************************************************************** */

NoeudDeclarationFonction::NoeudDeclarationFonction(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{}

void NoeudDeclarationFonction::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudDeclarationFonction : " << m_donnees_morceaux.chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudDeclarationFonction::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
{
	/* À FAIRE : inférence de type
	 * - considération du type de retour des fonctions récursive
	 * - il est possible que le retour dépende des variables locales de la
	 *   fonction, donc il faut d'abord générer le code ou faire une prépasse
	 *   pour générer les données nécessaires.
	 */

	auto module = contexte.module(static_cast<size_t>(m_donnees_morceaux.module));
	auto &donnees_fonction = module->donnees_fonction(m_donnees_morceaux.chaine);

	/* Pour les fonctions variadiques
	 * - on ajoute manuellement un argument implicit correspondant au nombre
	 *   d'args qui est appelé __compte_args
	 * - lors de l'appel, puisque nous connaissons le nombre d'arguments, on le
	 *   passe à la fonction
	 * - les boucles n'ont plus qu'à utiliser le compte d'arguments comme valeur
	 *   de fin de plage.
	 */

	/* Crée le type de la fonction */
	auto &this_dt = contexte.magasin_types.donnees_types[this->donnees_type];
	auto type_fonction = obtiens_type_fonction(
							 contexte,
							 donnees_fonction,
							 this_dt,
							 (this->drapeaux & VARIADIC) != 0);

	contexte.magasin_types.donnees_types[donnees_fonction.index_type].type_llvm(type_fonction);

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

	for (auto const &nom : donnees_fonction.nom_args) {
		auto const &argument = donnees_fonction.args[nom];
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
				donnees_structure.donnees_types.push_back(contexte.magasin_types.ajoute_type(dt));
				donnees_structure.index_membres.insert({"", donnees_structure.donnees_types.size()});
				donnees_structure.donnees_types.push_back(contexte.magasin_types.ajoute_type(dt));

				dt = DonneesType{};
				dt.pousse(id_morceau::POINTEUR);
				dt.pousse(id_morceau::Z8);

				donnees_structure.index_membres.insert({"", donnees_structure.donnees_types.size()});
				donnees_structure.donnees_types.push_back(contexte.magasin_types.ajoute_type(dt));
				donnees_structure.index_membres.insert({"", donnees_structure.donnees_types.size()});
				donnees_structure.donnees_types.push_back(contexte.magasin_types.ajoute_type(dt));

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
			auto dt = contexte.magasin_types.donnees_types[argument.donnees_type];
			align = alignement(contexte, dt);
			type = converti_type(contexte, dt);
		}

#ifdef NOMME_IR
		auto const &nom_argument = argument.chaine;
#else
		auto const &nom_argument = "";
#endif

		if (argument.est_variadic) {
			/* stockage de l'argument implicit de compte d'argument */
			auto valeur = &(*valeurs_args++);
			auto dt = DonneesType{};
			dt.pousse(id_morceau::Z32);
			auto index_dt = contexte.magasin_types.ajoute_type(dt);

			auto alloc_compte = new llvm::AllocaInst(
									llvm::Type::getInt32Ty(contexte.contexte),
									"",
									contexte.bloc_courant());
			alloc_compte->setAlignment(4);

			auto store = new llvm::StoreInst(valeur, alloc_compte, false, contexte.bloc_courant());
			store->setAlignment(4);

			contexte.pousse_locale("__compte_args", alloc_compte, index_dt, false, false);

			valeur = &(*valeurs_args++);

			auto alloc = new llvm::AllocaInst(
							 type,
							 nom_argument,
							 contexte.bloc_courant());

			alloc->setAlignment(align);
			auto cast = new llvm::BitCastInst(alloc,
											  llvm::Type::getInt8PtrTy(contexte.contexte),
											  "",
											  contexte.bloc_courant());

			auto fonc = llvm::Intrinsic::getDeclaration(contexte.module_llvm, llvm::Intrinsic::vastart);
			llvm::CallInst::Create(fonc, cast, "", contexte.bloc_courant());
			contexte.pousse_locale(nom, cast, argument.donnees_type, argument.est_dynamic, argument.est_variadic);
		}
		else {
			auto valeur = &(*valeurs_args++);
			valeur->setName(nom_argument);

			auto alloc = new llvm::AllocaInst(
							 type,
							 nom_argument,
							 contexte.bloc_courant());

			alloc->setAlignment(align);
			auto store = new llvm::StoreInst(valeur, alloc, false, contexte.bloc_courant());
			store->setAlignment(align);
			contexte.pousse_locale(nom, alloc, argument.donnees_type, argument.est_dynamic, argument.est_variadic);
		}
	}

	/* Crée code pour le bloc. */
	auto bloc = m_enfants.front();
	bloc->valeur_calculee = static_cast<llvm::BasicBlock *>(nullptr);
	auto ret = bloc->genere_code_llvm(contexte);

	/* Ajoute une instruction de retour si la dernière n'en est pas une. */
	if ((ret != nullptr) && !llvm::isa<llvm::ReturnInst>(*ret)) {
		genere_code_extra_pre_retour(contexte, m_donnees_morceaux.module);

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

	auto module = contexte.module(static_cast<size_t>(m_donnees_morceaux.module));
	auto donnees_fonction = module->donnees_fonction(m_donnees_morceaux.chaine);

	/* Pousse les paramètres sur la pile. */
	for (auto const &nom : donnees_fonction.nom_args) {
		auto const &argument = donnees_fonction.args[nom];

		if (argument.est_variadic) {
			auto dt = DonneesType{};
			dt.pousse(id_morceau::Z32);

			auto index_dt = contexte.magasin_types.ajoute_type(dt);

			contexte.pousse_locale("__compte_args", nullptr, index_dt, false, false);
			contexte.pousse_locale(nom, nullptr, argument.donnees_type, argument.est_dynamic, argument.est_variadic);
		}
		else {
			contexte.pousse_locale(nom, nullptr, argument.donnees_type, argument.est_dynamic, argument.est_variadic);
		}
	}

	/* vérifie le type du bloc */
	auto bloc = m_enfants.front();

	bloc->perfome_validation_semantique(contexte);
	auto type_bloc = bloc->donnees_type;
	auto dernier = bloc->dernier_enfant();

	auto dt = contexte.magasin_types.donnees_types[this->donnees_type];

	/* si le bloc est vide -> vérifie qu'aucun type n'a été spécifié */
	if (dernier == nullptr) {
		if (dt.type_base() != id_morceau::RIEN) {
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
			if (dt.type_base() != id_morceau::RIEN) {
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

NoeudAssignationVariable::NoeudAssignationVariable(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{}

void NoeudAssignationVariable::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudAssignationVariable : " << m_donnees_morceaux.chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudAssignationVariable::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
{
	assert(m_enfants.size() == 2);

	auto variable = m_enfants.front();
	auto expression = m_enfants.back();

	/* Génère d'abord le code de l'enfant afin que l'instruction d'allocation de
	 * la variable sur la pile et celle de stockage de la valeur soit côte à
	 * côte. */
	auto valeur = expression->genere_code_llvm(contexte);

	auto alloc = variable->genere_code_llvm(contexte, true);

	if (variable->type() == type_noeud::DECLARATION_VARIABLE && (variable->drapeaux & GLOBAL) != 0) {
		assert(expression->est_constant());
		auto vg = dynamic_cast<llvm::GlobalVariable *>(alloc);
		vg->setInitializer(dynamic_cast<llvm::Constant *>(valeur));
		return vg;
	}

	auto store = new llvm::StoreInst(valeur, alloc, false, contexte.bloc_courant());

	auto const &dt = contexte.magasin_types.donnees_types[expression->donnees_type];
	store->setAlignment(alignement(contexte, dt));

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

	if (this->donnees_type == -1ul) {
		erreur::lance_erreur(
					"Impossible de définir le type de la variable !",
					contexte,
					m_donnees_morceaux,
					erreur::type_erreur::TYPE_INCONNU);
	}

	auto const &dt = contexte.magasin_types.donnees_types[this->donnees_type];

	if (dt.type_base() == id_morceau::RIEN) {
		erreur::lance_erreur(
					"Impossible d'assigner une expression de type 'rien' à une variable !",
					contexte,
					m_donnees_morceaux,
					erreur::type_erreur::ASSIGNATION_RIEN);
	}

	/* Ajourne les données du premier enfant si elles sont invalides, dans le
	 * cas d'une déclaration de variable. */
	if (variable->donnees_type == -1ul) {
		variable->donnees_type = this->donnees_type;
	}

	variable->perfome_validation_semantique(contexte);

	auto const &type_gauche = contexte.magasin_types.donnees_types[variable->donnees_type];

	if (!peut_assigner(type_gauche, dt, expression->type())) {
		erreur::lance_erreur_assignation_type_differents(
					type_gauche,
					dt,
					contexte,
					m_donnees_morceaux);
	}
}

/* ************************************************************************** */

NoeudDeclarationVariable::NoeudDeclarationVariable(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{}

void NoeudDeclarationVariable::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);
	os << "NoeudDeclarationVariable : " << m_donnees_morceaux.chaine << '\n';
}

llvm::Value *NoeudDeclarationVariable::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
{
	auto &type = contexte.magasin_types.donnees_types[this->donnees_type];
	auto type_llvm = converti_type(contexte, type);

	if ((this->drapeaux & GLOBAL) != 0) {
		auto valeur = new llvm::GlobalVariable(
						  *contexte.module_llvm,
						  type_llvm,
						  true,
						  llvm::GlobalValue::InternalLinkage,
						  nullptr);

		valeur->setConstant((this->drapeaux & DYNAMIC) == 0);
		valeur->setAlignment(alignement(contexte, type));

		contexte.pousse_globale(this->chaine(), valeur, this->donnees_type, (this->drapeaux & DYNAMIC) != 0);
		return valeur;
	}

	auto alloc = new llvm::AllocaInst(
					 type_llvm,
#ifdef NOMME_IR
					 std::string(m_donnees_morceaux.chaine),
#else
					 "",
#endif
					 contexte.bloc_courant());

	alloc->setAlignment(alignement(contexte, type));

	/* Mets à zéro les valeurs des tableaux dynamics. */
	if (type.type_base() == id_morceau::TABLEAU) {
		auto pointeur = accede_membre_structure(contexte, alloc, POINTEUR_TABLEAU);

		auto stocke = new llvm::StoreInst(
						  llvm::ConstantInt::get(
							  llvm::Type::getInt64Ty(contexte.contexte),
							  static_cast<uint64_t>(0),
							  false),
						  pointeur,
						  contexte.bloc_courant());
		stocke->setAlignment(8);

		auto taille = accede_membre_structure(contexte, alloc, TAILLE_TABLEAU);
		stocke = new llvm::StoreInst(
					 llvm::ConstantInt::get(
						 llvm::Type::getInt64Ty(contexte.contexte),
						 static_cast<uint64_t>(0),
						 false),
					 taille,
					 contexte.bloc_courant());
		stocke->setAlignment(8);
	}

	contexte.pousse_locale(m_donnees_morceaux.chaine, alloc, this->donnees_type, (this->drapeaux & DYNAMIC) != 0, false);

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

	if ((this->drapeaux & GLOBAL) != 0) {
		contexte.pousse_globale(m_donnees_morceaux.chaine, nullptr, this->donnees_type, (this->drapeaux & DYNAMIC) != 0);
	}
	else {
		contexte.pousse_locale(m_donnees_morceaux.chaine, nullptr, this->donnees_type, (this->drapeaux & DYNAMIC) != 0, false);
	}
}

/* ************************************************************************** */

NoeudConstante::NoeudConstante(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{}

void NoeudConstante::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudConstante : " << m_donnees_morceaux.chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudConstante::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
{
	/* À FAIRE : énumération avec des expressions contenant d'autres énums.
	 * différents types (réel, bool, etc..)
	 */

	auto n = converti_chaine_nombre_entier(
				 m_enfants.front()->chaine(),
				 m_enfants.front()->identifiant());

	auto &type = contexte.magasin_types.donnees_types[this->donnees_type];
	auto type_llvm = converti_type(contexte, type);

	auto constante = llvm::ConstantInt::get(
						 type_llvm,
						 static_cast<uint64_t>(n));

	auto valeur = new llvm::GlobalVariable(
					  *contexte.module_llvm,
					  type_llvm,
					  true,
					  llvm::GlobalValue::InternalLinkage,
					  constante);

	contexte.pousse_globale(m_donnees_morceaux.chaine, valeur, this->donnees_type, false);

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

	if (this->donnees_type == -1ul) {
		this->donnees_type = m_enfants.front()->donnees_type;

		if (this->donnees_type == -1ul) {
			erreur::lance_erreur(
						"Impossible de définir le type de la variable globale !",
						contexte,
						m_donnees_morceaux,
						erreur::type_erreur::TYPE_INCONNU);
		}
	}
	/* À FAIRE : vérifie typage */

	contexte.pousse_globale(m_donnees_morceaux.chaine, nullptr, this->donnees_type, false);
}

/* ************************************************************************** */

NoeudNombreEntier::NoeudNombreEntier(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{
	auto dt = DonneesType{};
	dt.pousse(id_morceau::Z32);

	this->donnees_type = contexte.magasin_types.ajoute_type(dt);
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

llvm::Value *NoeudNombreEntier::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
{
	auto const valeur = this->calcule ? std::any_cast<long>(this->valeur_calculee) :
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

NoeudBooleen::NoeudBooleen(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{
	auto dt = DonneesType{};
	dt.pousse(id_morceau::BOOL);

	this->donnees_type = contexte.magasin_types.ajoute_type(dt);
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

llvm::Value *NoeudBooleen::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
{
	auto const valeur = this->calcule ? std::any_cast<bool>(this->valeur_calculee)
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

NoeudCaractere::NoeudCaractere(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{
	auto dt = DonneesType{};
	dt.pousse(id_morceau::Z8);

	this->donnees_type = contexte.magasin_types.ajoute_type(dt);
}

void NoeudCaractere::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);
	os << "NoeudCaractere : " << m_donnees_morceaux.chaine << '\n';
}

llvm::Value *NoeudCaractere::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
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

NoeudNombreReel::NoeudNombreReel(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{
	auto dt = DonneesType{};
	dt.pousse(id_morceau::R64);

	this->donnees_type = contexte.magasin_types.ajoute_type(dt);
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

llvm::Value *NoeudNombreReel::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
{
	auto const valeur = this->calcule ? std::any_cast<double>(this->valeur_calculee) :
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

NoeudChaineLitterale::NoeudChaineLitterale(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
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

	auto dt = DonneesType{};
	dt.pousse(id_morceau::TABLEAU | static_cast<int>((corrigee.size() + 1) << 8));
	dt.pousse(id_morceau::Z8);

	this->donnees_type = contexte.magasin_types.ajoute_type(dt);
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

llvm::Value *NoeudChaineLitterale::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
{
	auto chaine = std::any_cast<std::string>(this->valeur_calculee);

	auto constante = llvm::ConstantDataArray::getString(
						 contexte.contexte,
						 chaine);

	auto &this_type = contexte.magasin_types.donnees_types[this->donnees_type];
	auto type = converti_type(contexte, this_type);

	auto globale = new llvm::GlobalVariable(
					   *contexte.module_llvm,
					   type,
					   true,
					   llvm::GlobalValue::PrivateLinkage,
					   constante,
					   ".chn");

	globale->setAlignment(1);
	globale->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);

	return accede_membre_structure(contexte, globale, 0);
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

NoeudVariable::NoeudVariable(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{}

void NoeudVariable::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudVariable : " << m_donnees_morceaux.chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudVariable::genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche)
{
	auto valeur = contexte.valeur_locale(m_donnees_morceaux.chaine);

	if (valeur == nullptr) {
		valeur = contexte.valeur_globale(m_donnees_morceaux.chaine);

		if (valeur == nullptr) {
			valeur = contexte.module_llvm->getFunction(std::string(m_donnees_morceaux.chaine));
			return valeur;
		}
	}

	if (expr_gauche || dynamic_cast<llvm::PHINode *>(valeur) || dynamic_cast<llvm::VAArgInst *>(valeur)) {
		return valeur;
	}

	auto const &index_type = this->donnees_type;
	auto &type = contexte.magasin_types.donnees_types[index_type];

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
	auto iter_local = contexte.iter_locale(m_donnees_morceaux.chaine);

	if (iter_local != contexte.fin_locales()) {
		if (!iter_local->second.est_dynamique) {
			erreur::lance_erreur(
						"Ne peut pas assigner une variable locale non-dynamique",
						contexte,
						this->donnees_morceau(),
						erreur::type_erreur::ASSIGNATION_INVALIDE);
		}

		return true;
	}

	auto iter_globale = contexte.iter_globale(m_donnees_morceaux.chaine);

	if (iter_globale != contexte.fin_globales()) {
		if (!contexte.non_sur()) {
			erreur::lance_erreur(
						"Ne peut pas assigner une variable globale en dehors d'un bloc 'nonsûr'",
						contexte,
						this->donnees_morceau(),
						erreur::type_erreur::ASSIGNATION_INVALIDE);
		}

		if (!iter_globale->second.est_dynamique) {
			erreur::lance_erreur(
						"Ne peut pas assigner une variable globale non-dynamique",
						contexte,
						this->donnees_morceau(),
						erreur::type_erreur::ASSIGNATION_INVALIDE);
		}

		return true;
	}

	return false;
}

void NoeudVariable::perfome_validation_semantique(ContexteGenerationCode &contexte)
{
	auto const &iter_locale = contexte.iter_locale(m_donnees_morceaux.chaine);

	if (iter_locale != contexte.fin_locales()) {
		this->donnees_type = iter_locale->second.donnees_type;
		return;
	}

	auto const &iter_globale = contexte.iter_globale(m_donnees_morceaux.chaine);

	if (iter_globale != contexte.fin_globales()) {
		this->donnees_type = iter_globale->second.donnees_type;
		return;
	}

	/* Vérifie si c'est une fonction. */
	auto module = contexte.module(static_cast<size_t>(m_donnees_morceaux.module));

	if (module->fonction_existe(m_donnees_morceaux.chaine)) {
		auto const &donnees_fonction = module->donnees_fonction(m_donnees_morceaux.chaine);
		this->donnees_type = donnees_fonction.index_type;
		return;
	}

	erreur::lance_erreur(
				"Variable inconnue",
				contexte,
				m_donnees_morceaux,
				erreur::type_erreur::VARIABLE_INCONNUE);
}

/* ************************************************************************** */

NoeudAccesMembre::NoeudAccesMembre(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{}

void NoeudAccesMembre::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudAccesVariable : " << m_donnees_morceaux.chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudAccesMembre::genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche)
{
	auto structure = m_enfants.back();
	auto membre = m_enfants.front();

	auto const &index_type = structure->donnees_type;
	auto type_structure = contexte.magasin_types.donnees_types[index_type];

	auto est_pointeur = type_structure.type_base() == id_morceau::POINTEUR;

	if (est_pointeur) {
		type_structure = type_structure.derefence();
	}

	if ((type_structure.type_base() & 0xff) == id_morceau::TABLEAU) {
		auto taille = static_cast<size_t>(type_structure.type_base() >> 8);

		if (taille != 0) {
			return llvm::ConstantInt::get(
						converti_type(contexte, contexte.magasin_types.donnees_types[this->donnees_type]),
						taille);
		}

		/* charge taille de la structure tableau { *type, taille } */
		auto valeur = structure->genere_code_llvm(contexte, true);

		if (est_pointeur) {
			/* déréférence le pointeur en le chargeant */
			auto charge = new llvm::LoadInst(valeur, "", contexte.bloc_courant());
			charge->setAlignment(8);
			valeur = charge;
		}

		if (m_donnees_morceaux.chaine == "pointeur") {
			return accede_membre_structure(contexte, valeur, POINTEUR_TABLEAU);
		}

		return accede_membre_structure(contexte, valeur, true, TAILLE_TABLEAU);
	}

	auto index_structure = size_t(type_structure.type_base() >> 8);

	auto const &nom_membre = membre->chaine();

	auto &donnees_structure = contexte.donnees_structure(index_structure);

	auto const iter = donnees_structure.index_membres.find(nom_membre);

	auto const index_membre = iter->second;

	auto valeur = structure->genere_code_llvm(contexte, true);

	llvm::Value *ret;

	if (est_pointeur) {
		/* déréférence le pointeur en le chargeant */
		auto charge = new llvm::LoadInst(valeur, "", contexte.bloc_courant());
		charge->setAlignment(8);
		valeur = charge;
	}

	ret = accede_membre_structure(contexte, valeur, index_membre);

	if (!expr_gauche) {
		auto charge = new llvm::LoadInst(ret, "", contexte.bloc_courant());
		auto const &dt = contexte.magasin_types.donnees_types[donnees_structure.donnees_types[index_membre]];
		charge->setAlignment(alignement(contexte, dt));
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

	auto const &index_type = structure->donnees_type;
	auto type_structure = contexte.magasin_types.donnees_types[index_type];

	if (type_structure.type_base() == id_morceau::POINTEUR) {
		type_structure = type_structure.derefence();
	}

	if ((type_structure.type_base() & 0xff) == id_morceau::TABLEAU) {
		if (membre->chaine() == "pointeur") {
			auto dt = DonneesType{};
			dt.pousse(id_morceau::POINTEUR);
			dt.pousse(type_structure.derefence());

			this->donnees_type = contexte.magasin_types.ajoute_type(dt);
			return;
		}

		if (membre->chaine() == "taille") {
			auto dt = DonneesType{};
			dt.pousse(id_morceau::N64);

			this->donnees_type = contexte.magasin_types.ajoute_type(dt);
			return;
		}

		erreur::lance_erreur(
					"Le tableau ne possède pas cette propriété !",
					contexte,
					membre->donnees_morceau(),
					erreur::type_erreur::MEMBRE_INCONNU);
	}
	else if ((type_structure.type_base() & 0xff) == id_morceau::CHAINE_CARACTERE) {
		auto const index_structure = size_t(type_structure.type_base() >> 8);

		auto const &nom_membre = membre->chaine();

		auto &donnees_structure = contexte.donnees_structure(index_structure);

		auto const iter = donnees_structure.index_membres.find(nom_membre);

		if (iter == donnees_structure.index_membres.end()) {
			/* À FAIRE : proposer des candidats possibles ou imprimer la structure. */
			erreur::lance_erreur(
						"Membre inconnu",
						contexte,
						m_donnees_morceaux,
						erreur::type_erreur::MEMBRE_INCONNU);
		}

		auto const index_membre = iter->second;

		this->donnees_type = donnees_structure.donnees_types[index_membre];
	}
	else {
		erreur::lance_erreur(
					"Impossible d'accéder au membre d'un objet n'étant pas une structure",
					contexte,
					structure->donnees_morceau(),
					erreur::type_erreur::TYPE_DIFFERENTS);
	}
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

NoeudOperationBinaire::NoeudOperationBinaire(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{}

void NoeudOperationBinaire::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudOperationBinaire : " << m_donnees_morceaux.chaine << " : " << this->donnees_type << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudOperationBinaire::genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche)
{
	auto instr = llvm::Instruction::Add;
	auto predicat = llvm::CmpInst::Predicate::FCMP_FALSE;
	auto est_comp_entier = false;
	auto est_comp_reel = false;

	auto enfant1 = m_enfants.front();
	auto enfant2 = m_enfants.back();

	auto const index_type1 = enfant1->donnees_type;
	auto const index_type2 = enfant2->donnees_type;

	auto const &type1 = contexte.magasin_types.donnees_types[index_type1];
	auto &type2 = contexte.magasin_types.donnees_types[index_type2];

	if ((this->m_donnees_morceaux.identifiant != id_morceau::CROCHET_OUVRANT)) {
		if (!peut_operer(type1, type2, enfant1->type(), enfant2->type())) {
			erreur::lance_erreur_type_operation(
						type1,
						type2,
						contexte,
						m_donnees_morceaux);
		}
	}

	/* À FAIRE : typage */

	/* Ne crée pas d'instruction de chargement si nous avons un tableau. */
	auto const valeur2_brut = ((type2.type_base() & 0xff) == id_morceau::TABLEAU);

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
				valeur = accede_element_tableau(
							 contexte,
							 valeur2,
							 converti_type(contexte, type2),
							 valeur1);
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

	auto const index_type1 = enfant1->donnees_type;
	auto const index_type2 = enfant2->donnees_type;

	auto const &type1 = contexte.magasin_types.donnees_types[index_type1];
	auto const &type2 = contexte.magasin_types.donnees_types[index_type2];

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
			this->donnees_type = index_type1;
			break;
		}
		case id_morceau::CROCHET_OUVRANT:
		{
			auto donnees_enfant = m_enfants.back()->donnees_type;

			auto const &type = contexte.magasin_types.donnees_types[donnees_enfant];
			auto dt = type.derefence();
			this->donnees_type = contexte.magasin_types.ajoute_type(dt);
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
			auto dt = DonneesType{};
			dt.pousse(id_morceau::BOOL);

			this->donnees_type = contexte.magasin_types.ajoute_type(dt);
			break;
		}
	}
}

/* ************************************************************************** */

NoeudOperationUnaire::NoeudOperationUnaire(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{}

void NoeudOperationUnaire::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudOperationUnaire : " << m_donnees_morceaux.chaine
	   << " : " << this->donnees_type << '\n';

	m_enfants.front()->imprime_code(os, tab + 1);
}

llvm::Value *NoeudOperationUnaire::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
{
	llvm::Instruction::BinaryOps instr;
	auto enfant = m_enfants.front();
	auto index_type1 = enfant->donnees_type;
	auto const &type1 = contexte.magasin_types.donnees_types[index_type1];
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
	auto index_type = enfant->donnees_type;
	auto const &type = contexte.magasin_types.donnees_types[index_type];

	if (this->donnees_type == -1ul) {
		switch (this->identifiant()) {
			default:
			{
				this->donnees_type = enfant->donnees_type;
				break;
			}
			case id_morceau::AROBASE:
			{
				auto dt = DonneesType{};
				dt.pousse(id_morceau::POINTEUR);
				dt.pousse(type);

				this->donnees_type = contexte.magasin_types.ajoute_type(dt);
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

				auto dt = DonneesType{};
				dt.pousse(id_morceau::BOOL);
				this->donnees_type = contexte.magasin_types.ajoute_type(dt);
				break;
			}
		}
	}
}

/* ************************************************************************** */

NoeudRetour::NoeudRetour(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{}

void NoeudRetour::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudRetour : " << m_donnees_morceaux.chaine << '\n';
	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudRetour::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
{
	genere_code_extra_pre_retour(contexte, m_donnees_morceaux.module);

	llvm::Value *valeur = nullptr;

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
		auto dt = DonneesType{};
		dt.pousse(id_morceau::RIEN);
		this->donnees_type = contexte.magasin_types.ajoute_type(dt);
		return;
	}

	m_enfants.front()->perfome_validation_semantique(contexte);
	this->donnees_type = m_enfants.front()->donnees_type;
}

/* ************************************************************************** */

NoeudSi::NoeudSi(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{}

void NoeudSi::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudSi : " << m_donnees_morceaux.chaine << '\n';
	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudSi::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
{
	auto const nombre_enfants = m_enfants.size();
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
	auto const nombre_enfants = m_enfants.size();
	auto iter_enfant = m_enfants.begin();

	auto enfant1 = *iter_enfant++;
	auto enfant2 = *iter_enfant++;

	enfant1->perfome_validation_semantique(contexte);
	auto index_type = enfant1->donnees_type;
	auto const &type_condition = contexte.magasin_types.donnees_types[index_type];

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

NoeudBloc::NoeudBloc(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{
	this->valeur_calculee = static_cast<llvm::BasicBlock *>(nullptr);
}

void NoeudBloc::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudBloc : " << m_donnees_morceaux.chaine << '\n';
	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudBloc::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
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
		auto dt = DonneesType{};
		dt.pousse(id_morceau::RIEN);
		this->donnees_type = contexte.magasin_types.ajoute_type(dt);
	}
	else {
		this->donnees_type = m_enfants.back()->donnees_type;
	}

	contexte.depile_nombre_locales();
}

/* ************************************************************************** */

NoeudPour::NoeudPour(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
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
llvm::Value *NoeudPour::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
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

	auto index_type = enfant2->donnees_type;
	auto &type_debut = contexte.magasin_types.donnees_types[index_type];
	auto const type = type_debut.type_base();

	enfant1->donnees_type = index_type;

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

	const auto tableau = (type & 0xff) == id_morceau::TABLEAU;
	const auto taille_tableau = static_cast<uint64_t>(type >> 8);
	auto pointeur_tableau = static_cast<llvm::Value *>(nullptr);

	if (enfant2->type() == type_noeud::PLAGE) {
		noeud_phi = llvm::PHINode::Create(
						converti_type(contexte, type_debut),
						2,
						std::string(enfant1->chaine()),
						contexte.bloc_courant());
	}
	else if (enfant2->type() == type_noeud::VARIABLE) {
		noeud_phi = llvm::PHINode::Create(
						tableau ? llvm::Type::getInt64Ty(contexte.contexte)
								: llvm::Type::getInt32Ty(contexte.contexte),
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

		contexte.pousse_locale(enfant1->chaine(), noeud_phi, index_type, false, false);
	}
	else if (enfant2->type() == type_noeud::VARIABLE) {
		if (tableau) {
			auto valeur_debut = llvm::ConstantInt::get(
									llvm::Type::getInt64Ty(contexte.contexte),
									static_cast<uint64_t>(0),
									false);

			auto valeur_fin = static_cast<llvm::Value *>(nullptr);

			if (taille_tableau != 0) {
				valeur_fin = llvm::ConstantInt::get(
								 llvm::Type::getInt64Ty(contexte.contexte),
								 taille_tableau,
								 false);
			}
			else {
				pointeur_tableau = enfant2->genere_code_llvm(contexte, true);
				valeur_fin = accede_membre_structure(contexte, pointeur_tableau, TAILLE_TABLEAU, true);
			}

			auto condition = llvm::ICmpInst::Create(
								 llvm::Instruction::ICmp,
								 llvm::CmpInst::Predicate::ICMP_SLT,
								 noeud_phi,
								 valeur_fin,
								 "",
								 contexte.bloc_courant());

			noeud_phi->addIncoming(valeur_debut, bloc_pre);

			llvm::BranchInst::Create(
						bloc_corps,
						(bloc_sansarret != nullptr) ? bloc_sansarret : bloc_apres,
						condition,
						contexte.bloc_courant());
		}
		else {
			auto arg = enfant2->genere_code_llvm(contexte);
			contexte.pousse_locale(enfant1->chaine(), arg, index_type, false, false);

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
	}

	/* bloc_corps */
	contexte.bloc_courant(bloc_corps);

	if (tableau) {
		auto valeur_arg = static_cast<llvm::Value *>(nullptr);

		if (taille_tableau != 0) {
			auto valeur_tableau = enfant2->genere_code_llvm(contexte, true);

			valeur_arg = accede_element_tableau(
						 contexte,
						 valeur_tableau,
						 converti_type(contexte, type_debut),
						 noeud_phi);
		}
		else {
			auto pointeur = accede_membre_structure(contexte, pointeur_tableau, POINTEUR_TABLEAU);

			pointeur = new llvm::LoadInst(pointeur, "", contexte.bloc_courant());

			valeur_arg = llvm::GetElementPtrInst::CreateInBounds(
						 pointeur,
						 noeud_phi,
						 "",
						 contexte.bloc_courant());
		}

		contexte.pousse_locale(enfant1->chaine(), valeur_arg, index_type, false, false);
	}

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
					   tableau ? id_morceau::N64 : id_morceau::Z32,
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
	auto const nombre_enfants = m_enfants.size();
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

	enfant2->perfome_validation_semantique(contexte);

	if (enfant2->type() == type_noeud::PLAGE) {
	}
	else if (enfant2->type() == type_noeud::VARIABLE) {
		auto index_type = enfant2->donnees_type;
		auto &type = contexte.magasin_types.donnees_types[index_type];

		if ((type.type_base() & 0xff) == id_morceau::TABLEAU) {
			/* ok. */
		}
		else {
			auto valeur = contexte.est_locale_variadique(enfant2->chaine());

			if (!valeur) {
				erreur::lance_erreur(
							"La variable n'est ni un argument variadic, ni un tableau",
							contexte,
							enfant2->donnees_morceau());
			}
		}
	}
	else {
		erreur::lance_erreur(
					"Expression inattendu dans la boucle 'pour'",
					contexte,
					m_donnees_morceaux);
	}

	contexte.empile_nombre_locales();

	auto index_type = enfant2->donnees_type;
	auto &type = contexte.magasin_types.donnees_types[index_type];

	if ((type.type_base() & 0xff) == id_morceau::TABLEAU) {
		index_type = contexte.magasin_types.ajoute_type(type.derefence());
	}

	auto est_dynamique = false;
	auto iter_locale = contexte.iter_locale(enfant2->chaine());

	if (iter_locale != contexte.fin_locales()) {
		est_dynamique = iter_locale->second.est_dynamique;
	}
	else {
		auto iter_globale = contexte.iter_globale(enfant2->chaine());

		if (iter_globale != contexte.fin_globales()) {
			est_dynamique = iter_globale->second.est_dynamique;
		}
	}

	contexte.pousse_locale(enfant1->chaine(), nullptr, index_type, est_dynamique, false);

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

NoeudContArr::NoeudContArr(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{}

void NoeudContArr::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);
	os << "NoeudContArr : " << m_donnees_morceaux.chaine << '\n';
}

llvm::Value *NoeudContArr::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
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

NoeudBoucle::NoeudBoucle(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
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
		bool const /*expr_gauche*/)
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

NoeudTranstype::NoeudTranstype(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
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

llvm::Value *NoeudTranstype::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
{
	auto enfant = m_enfants.front();
	auto valeur = enfant->genere_code_llvm(contexte);
	auto const &index_type_de = enfant->donnees_type;

	if (index_type_de == this->donnees_type) {
		return valeur;
	}

	auto const &donnees_type_de = contexte.magasin_types.donnees_types[index_type_de];

	using CastOps = llvm::Instruction::CastOps;
	auto &dt = contexte.magasin_types.donnees_types[this->donnees_type];

	auto type = converti_type(contexte, dt);
	auto bloc = contexte.bloc_courant();
	auto type_de = donnees_type_de.type_base();
	auto type_vers = dt.type_base();

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
				dt,
				contexte,
				this->donnees_morceau());
}

type_noeud NoeudTranstype::type() const
{
	return type_noeud::TRANSTYPE;
}

void NoeudTranstype::perfome_validation_semantique(ContexteGenerationCode &contexte)
{
	if (this->donnees_type == -1ul) {
		erreur::lance_erreur(
					"Ne peut transtyper vers un type invalide",
					contexte,
					this->donnees_morceau(),
					erreur::type_erreur::TYPE_INCONNU);
	}

	auto enfant = m_enfants.front();
	enfant->perfome_validation_semantique(contexte);

	if (enfant->donnees_type == -1ul) {
		erreur::lance_erreur(
					"Ne peut calculer le type d'origine",
					contexte,
					enfant->donnees_morceau(),
					erreur::type_erreur::TYPE_INCONNU);
	}
}

/* ************************************************************************** */

NoeudNul::NoeudNul(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{
	auto dt = DonneesType{};
	dt.pousse(id_morceau::POINTEUR);
	dt.pousse(id_morceau::NUL);

	this->donnees_type = contexte.magasin_types.ajoute_type(dt);
}

void NoeudNul::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);
	os << "NoeudNul\n";
}

llvm::Value *NoeudNul::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
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

NoeudTailleDe::NoeudTailleDe(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{
	auto dt = DonneesType{};
	dt.pousse(id_morceau::N32);

	this->donnees_type = contexte.magasin_types.ajoute_type(dt);
}

void NoeudTailleDe::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);
	auto donnees = std::any_cast<DonneesType>(this->valeur_calculee);
	os << "NoeudTailleDe : " << this->donnees_type << '\n';
}

llvm::Value *NoeudTailleDe::genere_code_llvm(
		ContexteGenerationCode &contexte,
		bool const /*expr_gauche*/)
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

NoeudPlage::NoeudPlage(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{}

void NoeudPlage::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);
	os << "NoeudPlage : ...\n";

	for (auto &enfant : m_enfants) {
		enfant->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudPlage::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
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

	auto index_type_debut = enfant1->donnees_type;
	auto index_type_fin   = enfant2->donnees_type;

	if (index_type_debut == -1ul || index_type_fin == -1ul) {
		erreur::lance_erreur(
					"Les types de l'expression sont invalides !",
					contexte,
					m_donnees_morceaux,
					erreur::type_erreur::TYPE_INCONNU);
	}

	auto const &type_debut = contexte.magasin_types.donnees_types[index_type_debut];
	auto const &type_fin   = contexte.magasin_types.donnees_types[index_type_fin];

	if (type_debut.est_invalide() || type_fin.est_invalide()) {
		erreur::lance_erreur(
					"Les types de l'expression sont invalides !",
					contexte,
					m_donnees_morceaux,
					erreur::type_erreur::TYPE_INCONNU);
	}

	if (index_type_debut != index_type_fin) {
		erreur::lance_erreur_type_operation(
					type_debut,
					type_fin,
					contexte,
					m_donnees_morceaux);
	}

	auto const type = type_debut.type_base();

	if (!est_type_entier_naturel(type) && !est_type_entier_relatif(type) && !est_type_reel(type)) {
		erreur::lance_erreur(
					"Attendu des types réguliers dans la plage de la boucle 'pour'",
					contexte,
					this->donnees_morceau(),
					erreur::type_erreur::TYPE_DIFFERENTS);
	}

	this->donnees_type = index_type_debut;

	contexte.pousse_locale("__debut", nullptr, this->donnees_type, false, false);
	contexte.pousse_locale("__fin", nullptr, this->donnees_type, false, false);
}

/* ************************************************************************** */

NoeudAccesMembrePoint::NoeudAccesMembrePoint(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{}

void NoeudAccesMembrePoint::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);
	os << "NoeudAccesMembrePoint : .\n";

	for (auto &enfant : m_enfants) {
		enfant->imprime_code(os, tab + 1);
	}
}

llvm::Value *NoeudAccesMembrePoint::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
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

	auto const nom_module = enfant1->chaine();

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

	auto const nom_fonction = enfant2->chaine();

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

/* ************************************************************************** */

NoeudDefere::NoeudDefere(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
	: Noeud(contexte, morceau)
{}

void NoeudDefere::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);
	os << "NoeudDefere : \n";
	m_enfants.front()->imprime_code(os, tab + 1);
}

llvm::Value *NoeudDefere::genere_code_llvm(ContexteGenerationCode &contexte, bool const /*expr_gauche*/)
{
	auto noeud = m_enfants.front();

	/* La valeur_calculee d'un bloc est son bloc suivant, qui dans le cas d'un
	 * bloc déféré n'en est aucun. */
	noeud->valeur_calculee = static_cast<llvm::BasicBlock *>(nullptr);

	contexte.defere_noeud(noeud);
	return nullptr;
}

type_noeud NoeudDefere::type() const
{
	return type_noeud::DEFERE;
}

/* ************************************************************************** */

NoeudNonSur::NoeudNonSur(ContexteGenerationCode &contexte, const DonneesMorceaux &morceau)
	: Noeud(contexte, morceau)
{}

void NoeudNonSur::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);
	os << "NoeudNonSur : \n";
	m_enfants.front()->imprime_code(os, tab + 1);
}

llvm::Value *NoeudNonSur::genere_code_llvm(ContexteGenerationCode &contexte, const bool /*expr_gauche*/)
{
	return m_enfants.front()->genere_code_llvm(contexte, false);
}

type_noeud NoeudNonSur::type() const
{
	return type_noeud::NONSUR;
}

void NoeudNonSur::perfome_validation_semantique(ContexteGenerationCode &contexte)
{
	contexte.non_sur(true);
	m_enfants.front()->perfome_validation_semantique(contexte);
	contexte.non_sur(false);
}
