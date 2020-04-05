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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "generation_code_llvm.hh"

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/outils/chaine.hh"
#include "biblinternes/outils/conditions.h"

using dls::outils::possede_drapeau;

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
#include <llvm/IR/Module.h>
#pragma GCC diagnostic pop

#include "arbre_syntactic.h"
#include "assembleuse_arbre.h"
#include "contexte_generation_code.h"
#include "conversion_type_llvm.hh"
#include "erreur.h"
#include "info_type_llvm.hh"
#include "modules.hh"
#include "outils_lexemes.hh"
#include "validation_semantique.hh"

#include "contexte_generation_llvm.hh"

#undef NOMME_IR

/* ************************************************************************** */

/* À FAIRE (coulisse LLVM)
 * - prend en compte la portée des blocs pour générer le code des noeuds différés
 * - coroutine, retiens
 * - erreur en cas de débordement des limites, où d'accès à un membre non-actif d'une union
 * - valide que les expressions dans les accès par index sont de type z64 (ou n64 ?)
 * - séparation type structure/type union
 * - construction/extraction implicite des unions
 * - type erreurs, et erreurs en générale
 * - instruction tente
 * - utilisation d'optimisation de valeur de retour pour éviter les chargement
 *   de gros tableaux et éviter d'allouer trop de variables sur la pile
 * - trace d'appels
 * - nouvelle forme d'initialisation des structures (en utilisant expr->exprs)
 * - valeurs constantes => utilisation de la valeur de son expression
 * - voir tous les cas d'utilisation de [].taille comme [].capacité
 * - ajournement pour le nouvel arbre syntaxique des expressions d'appels
 */

/* ************************************************************************** */

namespace noeud {

static void genere_code_extra_pre_retour(ContexteGenerationLLVM &contexte, NoeudBloc *bloc)
{
	/* génère le code pour les blocs déférés */
	while (bloc != nullptr) {
		for (auto i = bloc->noeuds_differes.taille - 1; i >= 0; --i) {
			auto bloc_differe = bloc->noeuds_differes[i];
			bloc_differe->est_differe = false;
			noeud::genere_code_llvm(bloc_differe, contexte, false);
			bloc_differe->est_differe = true;
		}

		bloc = bloc->parent;
	}
}

/* ************************************************************************** */

static bool est_plus_petit(Type *type1, Type *type2)
{
	return type1->taille_octet < type2->taille_octet;
}

static bool est_type_entier(Type *type)
{
	return type->genre == GenreType::ENTIER_NATUREL || type->genre == GenreType::ENTIER_RELATIF;
}

/* ************************************************************************** */

static auto cree_bloc(ContexteGenerationLLVM &contexte, char const *nom)
{
#ifdef NOMME_IR
	return llvm::BasicBlock::Create(contexte.contexte, nom, contexte.fonction);
#else
	static_cast<void>(nom);
	return llvm::BasicBlock::Create(contexte.contexte, "", contexte.fonction);
#endif
}

static bool est_branche_ou_retour(llvm::Value *valeur)
{
	return (valeur != nullptr) && (llvm::isa<llvm::BranchInst>(*valeur) || llvm::isa<llvm::ReturnInst>(*valeur));
}

static bool est_instruction_branche(llvm::Value *valeur)
{
	return (valeur != nullptr) && llvm::isa<llvm::BranchInst>(*valeur);
}

static bool est_instruction_retour(llvm::Value *valeur)
{
	return (valeur != nullptr) && llvm::isa<llvm::ReturnInst>(*valeur);
}

static llvm::FunctionType *obtiens_type_fonction(
		ContexteGenerationLLVM &contexte,
		TypeFonction *type,
		bool est_variadique,
		bool est_externe)
{
	std::vector<llvm::Type *> parametres;
	parametres.reserve(static_cast<size_t>(type->types_entrees.taille));

	POUR (type->types_entrees) {
		if (it->genre == GenreType::VARIADIQUE) {
			/* les arguments variadiques sont transformés en un tableau */
			if (!est_externe) {
				auto type_var = static_cast<TypeVariadique *>(it);
				auto type_tabl = contexte.typeuse.type_tableau_dynamique(type_var->type_pointe);
				parametres.push_back(converti_type_llvm(contexte, type_tabl));
			}

			break;
		}

		parametres.push_back(converti_type_llvm(contexte, it));
	}

	return llvm::FunctionType::get(
				converti_type_llvm(contexte, type->types_sorties[0]),
				parametres,
				est_variadique && est_externe);
}

enum {
	/* TABLEAUX */
	POINTEUR_TABLEAU = 0,
	TAILLE_TABLEAU = 1,
	CAPACITE_TABLEAU = 2,

	/* EINI */
	POINTEUR_EINI = 0,
	TYPE_EINI = 1,

	/* CHAINE */
	POINTEUR_CHAINE = 0,
	TYPE_CHAINE = 1,
};

[[nodiscard]] static llvm::Value *accede_membre_structure(
		ContexteGenerationLLVM &contexte,
		llvm::Value *structure,
		uint64_t index,
		bool charge = false)
{
	std::vector<llvm::Value *> idx;
	idx.reserve(2);
	idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(contexte.contexte), 0));
	idx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(contexte.contexte), index));

	auto ptr = llvm::GetElementPtrInst::CreateInBounds(
				structure,
				idx,
				"",
				contexte.bloc_courant());

	if (charge == true) {
		return new llvm::LoadInst(ptr, "", contexte.bloc_courant());
	}

	return ptr;
}

static int trouve_index_membre(NoeudStruct *noeud_struct, dls::vue_chaine_compacte const &nom_membre)
{
	auto idx_membre = 0;

	POUR (noeud_struct->desc.membres) {
		if (it.nom == nom_membre) {
			break;
		}

		idx_membre += 1;
	}

	return idx_membre;
}

[[nodiscard]] static llvm::Value *accede_membre_union(
		ContexteGenerationLLVM &contexte,
		llvm::Value *structure,
		NoeudStruct *noeud_struct,
		dls::vue_chaine_compacte const &nom_membre,
		bool charge = false)
{
	auto ptr = llvm::GetElementPtrInst::CreateInBounds(
			  structure, {
				  llvm::ConstantInt::get(llvm::Type::getInt32Ty(contexte.contexte), 0),
				  llvm::ConstantInt::get(llvm::Type::getInt32Ty(contexte.contexte), 0)
			  },
			  "",
			  contexte.bloc_courant());

	auto type_membre = static_cast<Type *>(nullptr);

	POUR (noeud_struct->desc.membres) {
		if (it.nom == nom_membre) {
			type_membre = it.type;
			break;
		}
	}

	auto type = converti_type_llvm(contexte, type_membre);

	auto ret = llvm::BitCastInst::Create(llvm::Instruction::BitCast, ptr, type->getPointerTo(), "", contexte.bloc_courant());

	if (charge == true) {
		return new llvm::LoadInst(ret, "", contexte.bloc_courant());
	}

	return ret;
}

[[nodiscard]] static llvm::Value *accede_element_tableau(
		ContexteGenerationLLVM &contexte,
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
		ContexteGenerationLLVM &contexte,
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

[[nodiscard]] static llvm::Value *converti_vers_tableau_dyn(
		ContexteGenerationLLVM &contexte,
		llvm::Value *tableau,
		Type *type,
		bool charge_ = true)
{
	auto type_tabl_fixe = static_cast<TypeTableauFixe *>(type);
	/* trouve le type de la structure tableau */
	auto type_tabl = contexte.typeuse.type_tableau_dynamique(type_tabl_fixe->type_pointe);

	auto type_llvm = converti_type_llvm(contexte, type_tabl);

	/* alloue de l'espace pour ce type */
	auto alloc = new llvm::AllocaInst(type_llvm, 0, "", contexte.bloc_courant());
	alloc->setAlignment(8);

	/* copie le pointeur du début du tableau */
	auto ptr_valeur = accede_membre_structure(contexte, alloc, POINTEUR_TABLEAU);

	/* charge le premier élément du tableau */
	auto premier_elem = accede_element_tableau(
							contexte,
							tableau,
							type->type_llvm,
							0ul);

	auto stocke = new llvm::StoreInst(premier_elem, ptr_valeur, contexte.bloc_courant());
	stocke->setAlignment(8);

	/* copie la taille du tableau */
	auto ptr_taille = accede_membre_structure(contexte, alloc, TAILLE_TABLEAU);

	auto constante = llvm::ConstantInt::get(
						 llvm::Type::getInt64Ty(contexte.contexte),
						 uint64_t(type_tabl_fixe->taille),
						 false);

	stocke = new llvm::StoreInst(constante, ptr_taille, contexte.bloc_courant());
	stocke->setAlignment(8);

	if (charge_) {
		auto charge = new llvm::LoadInst(alloc, "", contexte.bloc_courant());
		charge->setAlignment(8);
		return charge;
	}

	return alloc;
}

[[nodiscard]] static llvm::Value *converti_vers_eini(
		ContexteGenerationLLVM &contexte,
		llvm::Value *valeur,
		Type *type,
		bool charge_)
{
	/* alloue de l'espace pour un eini */
	auto type_eini_llvm = converti_type_llvm(contexte, contexte.typeuse[TypeBase::EINI]);

	auto alloc_eini = new llvm::AllocaInst(type_eini_llvm, 0, "", contexte.bloc_courant());
	alloc_eini->setAlignment(8);

	/* copie le pointeur de la valeur vers le type eini */
	auto ptr_eini = accede_membre_structure(contexte, alloc_eini, POINTEUR_EINI);

	if (!llvm::isa<llvm::AllocaInst>(valeur) && !llvm::isa<llvm::GetElementPtrInst>(valeur)) {
		auto type_donnees_llvm = converti_type_llvm(contexte, type);
		auto alloc_const = new llvm::AllocaInst(type_donnees_llvm, 0, "", contexte.bloc_courant());
		new llvm::StoreInst(valeur, alloc_const, contexte.bloc_courant());
		valeur = alloc_const;
	}

	auto transtype = new llvm::BitCastInst(
						 valeur,
						 llvm::Type::getInt8PtrTy(contexte.contexte),
						 "",
						 contexte.bloc_courant());

	auto stocke = new llvm::StoreInst(transtype, ptr_eini, contexte.bloc_courant());
	stocke->setAlignment(8);

	/* copie le pointeur vers les infos du type du eini */
	auto tpe_eini = accede_membre_structure(contexte, alloc_eini, TYPE_EINI);

	auto ptr_info_type = cree_info_type(contexte, type);

	ptr_info_type = new llvm::BitCastInst(
				ptr_info_type,
				converti_type_llvm(contexte, contexte.typeuse.type_pour_nom("InfoType"))->getPointerTo(),
				"",
				contexte.bloc_courant());

	stocke = new llvm::StoreInst(ptr_info_type, tpe_eini, contexte.bloc_courant());
	stocke->setAlignment(8);

	/* charge et retourne */
	if (charge_) {
		auto charge = new llvm::LoadInst(alloc_eini, "", contexte.bloc_courant());
		charge->setAlignment(8);
		return charge;
	}

	return alloc_eini;
}

[[nodiscard]] llvm::Value *applique_transformation(
		ContexteGenerationLLVM &contexte,
		NoeudExpression *b,
		bool expr_gauche)
{
	auto valeur = static_cast<llvm::Value *>(nullptr);

	auto builder = llvm::IRBuilder<>(contexte.contexte);
	builder.SetInsertPoint(contexte.bloc_courant());

	switch (b->transformation.type) {
		case TypeTransformation::IMPOSSIBLE:
		{
			break;
		}
		case TypeTransformation::CONSTRUIT_UNION:
		{
			// À FAIRE
			break;
		}
		case TypeTransformation::EXTRAIT_UNION:
		{
			// À FAIRE
			break;
		}
		case TypeTransformation::INUTILE:
		case TypeTransformation::PREND_PTR_RIEN:
		{
			valeur = genere_code_llvm(b, contexte, expr_gauche);
			break;
		}
		case TypeTransformation::CONVERTI_VERS_BASE:
		case TypeTransformation::CONVERTI_VERS_TYPE_CIBLE:
		{
			auto type_cible_llvm = converti_type_llvm(contexte, b->transformation.type_cible);
			valeur = genere_code_llvm(b, contexte, false);
			valeur = builder.CreateCast(llvm::Instruction::BitCast, valeur, type_cible_llvm);
			break;
		}
		case TypeTransformation::AUGMENTE_TAILLE_TYPE:
		{
			auto type_cible = b->transformation.type_cible;
			auto type_cible_llvm = converti_type_llvm(contexte, type_cible);

			auto inst = llvm::Instruction::CastOps{};

			if (type_cible->genre == GenreType::ENTIER_NATUREL) {
				inst = llvm::Instruction::ZExt;
			}
			else if (type_cible->genre == GenreType::ENTIER_RELATIF) {
				inst = llvm::Instruction::SExt;
			}
			else {
				inst = llvm::Instruction::FPExt;
			}

			valeur = genere_code_llvm(b, contexte, true);

			if (llvm::isa<llvm::Constant>(valeur)) {
				auto type_donnees_llvm = converti_type_llvm(contexte, b->type);
				auto alloc_const = builder.CreateAlloca(type_donnees_llvm, 0u);
				builder.CreateStore(valeur, alloc_const);
				valeur = builder.CreateLoad(alloc_const, "");
			}
			else if (llvm::isa<llvm::AllocaInst>(valeur) || llvm::isa<llvm::GetElementPtrInst>(valeur)) {
				valeur = builder.CreateLoad(valeur, "");
			}

			valeur = builder.CreateCast(inst, valeur, type_cible_llvm);
			break;
		}
		case TypeTransformation::CONSTRUIT_EINI:
		{
			valeur = genere_code_llvm(b, contexte, true);
			valeur = converti_vers_eini(contexte, valeur, b->type, !expr_gauche);
			break;
		}
		case TypeTransformation::EXTRAIT_EINI:
		{
			valeur = genere_code_llvm(b, contexte, true);
			valeur = accede_membre_structure(contexte, valeur, POINTEUR_EINI, !expr_gauche);
			auto type_cible = converti_type_llvm(contexte, b->transformation.type_cible);
			valeur = builder.CreateCast(llvm::Instruction::BitCast, valeur, type_cible->getPointerTo());
			valeur = builder.CreateLoad(valeur, "");
			break;
		}
		case TypeTransformation::CONSTRUIT_TABL_OCTET:
		{
			auto valeur_pointeur = static_cast<llvm::Value *>(nullptr);
			auto valeur_taille = static_cast<llvm::Value *>(nullptr);

			auto type_cible = converti_type_llvm(contexte, contexte.typeuse[TypeBase::PTR_OCTET]);

			switch (b->type->genre) {
				default:
				{
					valeur = genere_code_llvm(b, contexte, true);

					if (llvm::isa<llvm::Constant>(valeur)) {
						auto type_donnees_llvm = converti_type_llvm(contexte, b->type);
						auto alloc_const = builder.CreateAlloca(type_donnees_llvm, 0u);
						builder.CreateStore(valeur, alloc_const);
						valeur = alloc_const;
					}

					valeur = builder.CreateCast(llvm::Instruction::BitCast, valeur, type_cible);
					valeur_pointeur = valeur;

					if (b->type->genre == GenreType::ENTIER_CONSTANT) {
						valeur_taille = builder.getInt64(4);
					}
					else {
						valeur_taille = builder.getInt64(b->type->taille_octet);
					}

					break;
				}
				case GenreType::POINTEUR:
				{
					auto type_pointe = static_cast<TypePointeur *>(b->type)->type_pointe;
					valeur = genere_code_llvm(b, contexte, true);
					valeur = builder.CreateCast(llvm::Instruction::BitCast, valeur, type_cible);
					valeur_pointeur = valeur;
					auto taille_type = type_pointe->taille_octet;
					valeur_taille = builder.getInt64(taille_type);
					break;
				}
				case GenreType::CHAINE:
				{
					auto valeur_chaine = genere_code_llvm(b, contexte, true);

					if (llvm::isa<llvm::Constant>(valeur_chaine)) {
						auto const_struct = static_cast<llvm::ConstantStruct *>(valeur_chaine);
						valeur_pointeur = const_struct->getAggregateElement(0u);
						valeur_taille = const_struct->getAggregateElement(1);
					}
					else {
						valeur_pointeur = accede_membre_structure(contexte, valeur_chaine, POINTEUR_TABLEAU);
						valeur_pointeur = builder.CreateLoad(valeur_pointeur);
						valeur_taille = accede_membre_structure(contexte, valeur_chaine, TAILLE_TABLEAU);
						valeur_taille = builder.CreateLoad(valeur_taille);
					}

					break;
				}
				case GenreType::TABLEAU_DYNAMIQUE:
				{
					auto type_pointer = static_cast<TypeTableauDynamique *>(b->type)->type_pointe;

					auto valeur_tableau = genere_code_llvm(b, contexte, true);
					valeur_pointeur = accede_membre_structure(contexte, valeur_tableau, POINTEUR_TABLEAU);
					valeur_pointeur = builder.CreateLoad(valeur_pointeur);
					valeur_pointeur = builder.CreateCast(llvm::Instruction::BitCast, valeur_pointeur, type_cible);
					valeur_taille = accede_membre_structure(contexte, valeur_tableau, TAILLE_TABLEAU);

					auto taille_type = type_pointer->taille_octet;

					valeur_taille = builder.CreateBinOp(
								llvm::Instruction::Mul,
								builder.CreateLoad(valeur_taille),
								builder.getInt64(taille_type));

					break;
				}
				case GenreType::TABLEAU_FIXE:
				{
					auto type_tabl = static_cast<TypeTableauFixe *>(b->type);
					auto type_pointe = type_tabl->type_pointe;
					auto taille_type = type_pointe->taille_octet;

					auto valeur_tableau = genere_code_llvm(b, contexte, true);

					valeur_pointeur = accede_element_tableau(
								contexte,
								valeur_tableau,
								converti_type_llvm(contexte, b->type),
								0ul);

					valeur_pointeur = builder.CreateCast(llvm::Instruction::BitCast, valeur_pointeur, type_cible);

					valeur_taille = builder.getInt64(static_cast<unsigned>(type_tabl->taille) * taille_type);
				}
			}

			auto type_llvm_tabl_octet = converti_type_llvm(contexte, contexte.typeuse[TypeBase::TABL_OCTET]);

			/* alloue de l'espace pour ce type */
			auto tabl_octet = builder.CreateAlloca(type_llvm_tabl_octet, 0u);
			tabl_octet->setAlignment(8);

			auto pointeur_tabl_octet = accede_membre_structure(contexte, tabl_octet, POINTEUR_TABLEAU);
			builder.CreateStore(valeur_pointeur, pointeur_tabl_octet);

			auto taille_tabl_octet = accede_membre_structure(contexte, tabl_octet, TAILLE_TABLEAU);
			builder.CreateStore(valeur_taille, taille_tabl_octet);

			valeur = builder.CreateLoad(tabl_octet, "");

			break;
		}
		case TypeTransformation::CONVERTI_TABLEAU:
		{
			valeur = genere_code_llvm(b, contexte, true);
			valeur = converti_vers_tableau_dyn(contexte, valeur, b->type, !expr_gauche);
			break;
		}
		case TypeTransformation::FONCTION:
		{
			valeur = genere_code_llvm(b, contexte, false);

			auto nom_fonction = b->transformation.nom_fonction;
			auto fonction = contexte.module_llvm->getFunction(llvm::StringRef(nom_fonction.pointeur(), static_cast<size_t>(nom_fonction.taille())));

			auto parametres = std::vector<llvm::Value *>();
			// À FAIRE : contexte
			parametres.push_back(valeur);

			valeur = llvm::CallInst::Create(fonction, parametres, "", contexte.bloc_courant());

			break;
		}
		case TypeTransformation::PREND_REFERENCE:
		{
			/* la référence est le pointeur sur la pile */
			valeur = genere_code_llvm(b, contexte, true);
			break;
		}
		case TypeTransformation::DEREFERENCE:
		{
			valeur = genere_code_llvm(b, contexte, true);

			/* le déréférencement se fait via un chargement si nous sommes dans
			 * une expression droite,
			 * pour une expression gauche le déréférencement se fera lors du
			 * stockage d'une valeur */
			if (!expr_gauche) {
				valeur = builder.CreateLoad(valeur, "");
			}

			break;
		}
		case TypeTransformation::CONVERTI_ENTIER_CONSTANT:
		{
			b->type = b->transformation.type_cible;
			valeur = genere_code_llvm(b, contexte, true);
			break;
		}
		case TypeTransformation::CONVERTI_VERS_PTR_RIEN:
		{
			valeur = genere_code_llvm(b, contexte, false);

			if (b->genre != GenreNoeud::EXPRESSION_LITTERALE_NUL) {
				auto type_llvm = converti_type_llvm(contexte, contexte.typeuse[TypeBase::PTR_RIEN]);
				valeur = builder.CreateCast(llvm::Instruction::BitCast, valeur, type_llvm);
			}

			break;
		}
	}

	return valeur;
}

template <typename Conteneur>
llvm::Value *cree_appel(
		ContexteGenerationLLVM &contexte,
		llvm::Value *fonction,
		Conteneur const &conteneur,
		long nombre_parametres,
		bool besoin_contexte)
{
	std::vector<llvm::Value *> parametres(static_cast<size_t>(nombre_parametres));

	auto debut = 0u;

	if (besoin_contexte) {
		auto valeur_contexte = contexte.valeur(contexte.ident_contexte);
		parametres.resize(parametres.size() + 1);

		parametres[0] = new llvm::LoadInst(valeur_contexte, "", false, contexte.bloc_courant());

		debut = 1;
	}

	for (auto i = 0; i < nombre_parametres; ++i) {
		parametres[debut + static_cast<size_t>(i)] = applique_transformation(contexte, conteneur[i], false);
	}

	llvm::ArrayRef<llvm::Value *> args(parametres);

	return llvm::CallInst::Create(fonction, args, "", contexte.bloc_courant());
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

/* ************************************************************************** */

static llvm::Value *genere_code_position_source(ContexteGenerationLLVM &contexte, NoeudExpression *b)
{
	auto &constructrice = contexte.constructrice;

	auto type_position = contexte.typeuse.type_pour_nom("PositionCodeSource");

	auto alloc = constructrice.alloue_variable(nullptr, type_position, nullptr);

	// fichier
	auto fichier = contexte.fichiers[b->lexeme->fichier];
	auto chaine_nom_fichier = constructrice.cree_chaine(fichier->nom);
	auto ptr_fichier = constructrice.accede_membre_structure(alloc, 0, true);
	constructrice.stocke(chaine_nom_fichier, ptr_fichier);

	// fonction
	auto fonction_courante = contexte.donnees_fonction;
	auto nom_fonction = dls::vue_chaine_compacte("");

	if (fonction_courante != nullptr) {
		nom_fonction = fonction_courante->lexeme->chaine;
	}

	auto chaine_fonction = constructrice.cree_chaine(nom_fonction);
	auto ptr_fonction = constructrice.accede_membre_structure(alloc, 1, true);
	constructrice.stocke(chaine_fonction, ptr_fonction);

	// ligne
	auto pos = position_lexeme(*b->lexeme);
	auto ligne = constructrice.nombre_entier(pos.numero_ligne);
	auto ptr_ligne = constructrice.accede_membre_structure(alloc, 2, true);
	constructrice.stocke(ligne, ptr_ligne);

	// colonne
	auto colonne = constructrice.nombre_entier(pos.pos);
	auto ptr_colonne = constructrice.accede_membre_structure(alloc, 3, true);
	constructrice.stocke(colonne, ptr_colonne);

	return constructrice.charge(alloc, type_position);
}

/* ************************************************************************** */

static auto genere_code_allocation(
		ContexteGenerationLLVM &contexte,
		Type *type,
		int mode,
		NoeudBase *b,
		NoeudExpression *variable,
		NoeudExpression *expression,
		NoeudExpression *bloc_sinon)
{
	auto builder = llvm::IRBuilder<>(contexte.contexte);
	builder.SetInsertPoint(contexte.bloc_courant());

	auto type_du_pointeur_retour = type;
	auto val_enfant = static_cast<llvm::Value *>(nullptr);
	auto val_acces_pointeur = static_cast<llvm::Value *>(nullptr);
	auto val_acces_taille = static_cast<llvm::Value *>(nullptr);
	auto val_ancienne_taille_octet = static_cast<llvm::Value *>(nullptr);
	auto val_nouvelle_taille_octet = static_cast<llvm::Value *>(nullptr);
	auto val_ancien_nombre_element = static_cast<llvm::Value *>(nullptr);
	auto val_nouvel_nombre_element = static_cast<llvm::Value *>(nullptr);

	/* variable n'est nul que pour les allocations simples */
	if (variable != nullptr) {
		assert(mode == 1 || mode == 2);
		val_enfant = genere_code_llvm(variable, contexte, true);
	}
	else {
		assert(mode == 0);
		val_enfant = builder.CreateAlloca(converti_type_llvm(contexte, type));
	}

	switch (type->genre) {
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			auto type_deref = contexte.typeuse.type_dereference_pour(type);
			type_du_pointeur_retour = contexte.typeuse.type_pointeur_pour(type_deref);

			val_acces_pointeur = accede_membre_structure(contexte, val_enfant, 0u);
			val_acces_taille = accede_membre_structure(contexte, val_enfant, 1u);

			auto taille_type = type_deref->taille_octet;

			val_ancien_nombre_element = builder.CreateLoad(val_acces_taille);
			val_ancienne_taille_octet = builder.CreateBinOp(
						llvm::Instruction::Mul,
						val_ancien_nombre_element,
						builder.getInt64(taille_type));

			/* allocation ou réallocation */
			if (!b->type_declare.expressions.est_vide()) {
				expression = b->type_declare.expressions[0];
				auto val_expr = genere_code_llvm(expression, contexte, false);

				// À FAIRE : déplace cela dans la validation sémantique
				if (expression->type->taille_octet != 8) {
					val_expr = builder.CreateCast(llvm::Instruction::ZExt, val_expr, converti_type_llvm(contexte, contexte.typeuse[TypeBase::Z64]));
				}

				val_nouvel_nombre_element = val_expr;

				val_nouvelle_taille_octet = builder.CreateBinOp(
							llvm::Instruction::Mul,
							val_nouvel_nombre_element,
							builder.getInt64(taille_type));
			}
			/* désallocation */
			else {
				val_nouvel_nombre_element = builder.getInt64(0);
				val_nouvelle_taille_octet = builder.getInt64(0);
			}

			break;
		}
		case GenreType::CHAINE:
		{
			type_du_pointeur_retour = contexte.typeuse[TypeBase::PTR_Z8];
			val_acces_pointeur = accede_membre_structure(contexte, val_enfant, 0u);
			val_acces_taille = accede_membre_structure(contexte, val_enfant, 1u);
			val_ancienne_taille_octet = builder.CreateLoad(val_acces_taille, "");

			/* allocation ou réallocation */
			if (expression != nullptr) {
				auto val_expr = genere_code_llvm(expression, contexte, false);

				// À FAIRE : déplace cela dans la validation sémantique
				if (expression->type->taille_octet != 8) {
					val_expr = builder.CreateCast(llvm::Instruction::ZExt, val_expr, converti_type_llvm(contexte, contexte.typeuse[TypeBase::Z64]));
				}

				val_nouvel_nombre_element = val_expr;
				val_nouvelle_taille_octet = val_nouvel_nombre_element;
			}
			/* désallocation */
			else {
				val_nouvel_nombre_element = builder.getInt64(0);
				val_nouvelle_taille_octet = builder.getInt64(0);
			}

			break;
		}
		default:
		{
			val_acces_pointeur = val_enfant;

			auto type_deref = contexte.typeuse.type_dereference_pour(type);

			auto taille_octet = type_deref->taille_octet;
			val_ancienne_taille_octet = builder.getInt64(taille_octet);

			/* allocation ou réallocation */
			val_nouvelle_taille_octet = val_ancienne_taille_octet;
		}
	}

	auto ptr_contexte = contexte.valeur(contexte.ident_contexte);
	//ptr_contexte = builder.CreateLoad(ptr_contexte);

	// int mode = ...;
	// long nouvelle_taille_octet = ...;
	// long ancienne_taille_octet = ...;
	// void *pointeur = ...;
	// void *données = contexte->données_allocatrice;
	// InfoType *info_type = ...;
	// contexte->allocatrice(mode, nouvelle_taille_octet, ancienne_taille_octet, pointeur, données, info_type);

	auto arg_ptr_allocatrice = accede_membre_structure(contexte, ptr_contexte, 0u, true);
	auto arg_ptr_donnees = accede_membre_structure(contexte, ptr_contexte, 1u, true);

	auto arg_ptr_info_type = cree_info_type(contexte, type);
	arg_ptr_info_type = builder.CreateCast(
				llvm::Instruction::BitCast,
				arg_ptr_info_type,
				converti_type_llvm(contexte, contexte.typeuse.type_pour_nom("InfoType"))->getPointerTo());

	auto arg_val_mode = builder.getInt32(static_cast<unsigned>(mode));

	// À FAIRE : nous devrions avoir une transformation ici
	auto arg_val_ancien_ptr = builder.CreateCast(
				llvm::Instruction::BitCast,
				builder.CreateLoad(val_acces_pointeur, ""),
				converti_type_llvm(contexte, contexte.typeuse[TypeBase::PTR_RIEN]));

	auto arg_position = genere_code_position_source(contexte, static_cast<NoeudExpression *>(b));

	std::vector<llvm::Value *> parametres;
	parametres.push_back(builder.CreateLoad(ptr_contexte));
	parametres.push_back(arg_val_mode);
	parametres.push_back(val_nouvelle_taille_octet);
	parametres.push_back(val_ancienne_taille_octet);
	parametres.push_back(arg_val_ancien_ptr);
	parametres.push_back(arg_ptr_donnees);
	parametres.push_back(arg_ptr_info_type);
	parametres.push_back(arg_position);

#if 0
	{
		auto format_printf = builder.CreateGlobalStringPtr("[allocation] pré-appel\n");
		auto args_printf = std::vector<llvm::Value *>();
		args_printf.push_back(format_printf);

		auto fonc_printf = contexte.module_llvm->getFunction("printf");
		builder.CreateCall(fonc_printf, args_printf);
	}
#endif

	auto ret_pointeur = builder.CreateCall(arg_ptr_allocatrice, parametres);

#if 0
	{
		auto format_printf = builder.CreateGlobalStringPtr("[allocation] ptr : %p\n");
		auto args_printf = std::vector<llvm::Value *>();
		args_printf.push_back(format_printf);
		args_printf.push_back(ret_pointeur);

		auto fonc_printf = contexte.module_llvm->getFunction("printf");
		builder.CreateCall(fonc_printf, args_printf);
	}
#endif

	// À FAIRE: ajourne variable globales, vérifie validité pointeur

	switch (mode) {
		case 0:
		{
//			genere_code_echec_logement(
//						contexte,
//						generatrice,
//						expr_pointeur,
//						b,
//						bloc_sinon);

			break;
		}
		case 1:
		{
//			genere_code_echec_logement(
//						contexte,
//						generatrice,
//						expr_pointeur,
//						b,
//						bloc_sinon);

			break;
		}
	}

	builder.CreateStore(builder.CreateCast(llvm::Instruction::BitCast, ret_pointeur, converti_type_llvm(contexte, type_du_pointeur_retour)), val_acces_pointeur);

	if (val_acces_taille != nullptr) {
		builder.CreateStore(val_nouvel_nombre_element, val_acces_taille);
	}

	return builder.CreateLoad(val_enfant, "");
}

/* ************************************************************************** */

llvm::Value *genere_code_llvm(
		NoeudExpression *b,
		ContexteGenerationLLVM &contexte,
		const bool expr_gauche)
{
	auto &constructrice = contexte.constructrice;

	switch (b->genre) {
		case GenreNoeud::DIRECTIVE_EXECUTION:
		case GenreNoeud::DECLARATION_ENUM:
		case GenreNoeud::INSTRUCTION_RETOUR:
		case GenreNoeud::INSTRUCTION_SINON:
		case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
		{
			/* RÀF pour ces types de noeuds */
			return nullptr;
		}
		case GenreNoeud::DECLARATION_COROUTINE:
		case GenreNoeud::INSTRUCTION_RETIENS:
		{
			/* À FAIRE(coroutine) */
			return nullptr;
		}
		case GenreNoeud::EXPRESSION_PARENTHESE:
		{
			auto expr = static_cast<NoeudExpressionParenthese *>(b);
			return genere_code_llvm(expr->expr, contexte, expr_gauche);
		}
		case GenreNoeud::DECLARATION_OPERATEUR:
		case GenreNoeud::DECLARATION_FONCTION:
		{
			using dls::outils::possede_drapeau;

			auto decl = static_cast<NoeudDeclarationFonction *>(b);
			auto const est_externe = possede_drapeau(b->drapeaux, EST_EXTERNE);

			auto requiers_contexte = !est_externe && !possede_drapeau(b->drapeaux, FORCE_NULCTX);

			/* Crée fonction */
			auto fonction = contexte.module_llvm->getFunction(decl->nom_broye.c_str());

			if (est_externe) {
				return fonction;
			}

			contexte.commence_fonction(fonction, decl);

			auto block = cree_bloc(contexte, "entree");

			contexte.bloc_courant(block);

			/* Crée code pour les arguments */
			auto valeurs_args = fonction->arg_begin();

			if (requiers_contexte) {
				auto valeur = &(*valeurs_args++);
				valeur->setName("contexte");

				auto alloc = constructrice.alloue_param(contexte.ident_contexte, contexte.type_contexte, valeur);
				contexte.ajoute_locale(contexte.ident_contexte, alloc);
			}

			POUR (decl->params) {
#ifdef NOMME_IR
				auto const &nom_argument = it->ident->nom;
#else
				auto const &nom_argument = "";
#endif
				auto valeur = &(*valeurs_args++);
				valeur->setName(nom_argument);

				auto alloc = constructrice.alloue_param(it->ident, it->type, valeur);

				contexte.ajoute_locale(it->ident, alloc);
			}

			/* Crée code pour le bloc. */
			auto bloc = decl->bloc;
			bloc->valeur_calculee = static_cast<llvm::BasicBlock *>(nullptr);
			auto ret = genere_code_llvm(bloc, contexte, true);

			/* Ajoute une instruction de retour si la dernière n'en est pas une. */
			if (ret == nullptr || !llvm::isa<llvm::ReturnInst>(*ret)) {
				genere_code_extra_pre_retour(contexte, bloc);

				llvm::ReturnInst::Create(
							contexte.contexte,
							nullptr,
							contexte.bloc_courant());
			}

			contexte.termine_fonction();

			/* optimise la fonction */
			if (contexte.manager_fonctions != nullptr) {
				contexte.manager_fonctions->run(*fonction);
			}

			return nullptr;
		}
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			auto expr = static_cast<NoeudExpressionAppel *>(b);
			auto valeur = contexte.valeur(expr->ident);

			if (valeur != nullptr) {
				auto charge = constructrice.charge(valeur, expr->type);

				// À FAIRE : contexte
				return cree_appel(contexte, charge, expr->params, expr->params.taille, true);
			}

			// À FAIRE : trouve la fonction
//			auto fonction = contexte.module_llvm->getFunction(b->nom_fonction_appel.c_str());
//			auto decl = static_cast<NoeudDeclarationFonction *>(expr->noeud_fonction_appelee);
//			return cree_appel(contexte, fonction, expr->exprs, expr->exprs.taille, !decl->est_externe && !possede_drapeau(decl->drapeaux, FORCE_NULCTX));
			return nullptr;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		{
			auto refexpr = static_cast<NoeudExpressionReference *>(b);

			auto valeur = contexte.valeur(refexpr->ident);

			// À FAIRE : trouve la fonction -> déplace dans global
//			if (valeur == nullptr && b->nom_fonction_appel != "") {
//				valeur = contexte.module_llvm->getFunction(b->nom_fonction_appel.c_str());
//				return valeur;
//			}

			if (expr_gauche || llvm::dyn_cast<llvm::PHINode>(valeur)) {
				return valeur;
			}

			auto charge = new llvm::LoadInst(valeur, "", false, contexte.bloc_courant());
			charge->setAlignment(b->type->alignement);

			return charge;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		{
			auto expr = static_cast<NoeudExpressionBinaire *>(b);
			auto structure = expr->expr1;
			auto membre = expr->expr2;

			if (possede_drapeau(b->drapeaux, EST_APPEL_SYNTAXE_UNIFORME)) {
				return genere_code_llvm(membre, contexte, expr_gauche);
			}

			auto type_structure = structure->type;

			auto est_pointeur = type_structure->genre == GenreType::POINTEUR || type_structure->genre == GenreType::REFERENCE;

			while (type_structure->genre == GenreType::POINTEUR || type_structure->genre == GenreType::REFERENCE) {
				type_structure = contexte.typeuse.type_dereference_pour(type_structure);
			}

			if (type_structure->genre == GenreType::EINI) {
				auto valeur = genere_code_llvm(structure, contexte, true);

				if (est_pointeur) {
					valeur = new llvm::LoadInst(valeur, "", contexte.bloc_courant());
				}

				if (membre->ident->nom == "pointeur") {
					return accede_membre_structure(contexte, valeur, POINTEUR_EINI, !expr_gauche);
				}

				return accede_membre_structure(contexte, valeur, TYPE_EINI, !expr_gauche);
			}

			if (type_structure->genre == GenreType::CHAINE) {
				auto valeur = genere_code_llvm(structure, contexte, true);

				if (est_pointeur) {
					valeur = new llvm::LoadInst(valeur, "", contexte.bloc_courant());
				}

				if (llvm::isa<llvm::Constant>(valeur)) {
					auto const_struct = static_cast<llvm::ConstantStruct *>(valeur);

					if (membre->ident->nom == "pointeur") {
						return const_struct->getAggregateElement(0u);
					}

					return const_struct->getAggregateElement(1);
				}

				if (membre->ident->nom == "pointeur") {
					return accede_membre_structure(contexte, valeur, POINTEUR_CHAINE, !expr_gauche);
				}

				return accede_membre_structure(contexte, valeur, TYPE_CHAINE, !expr_gauche);
			}

			if (type_structure->genre == GenreType::TABLEAU_FIXE) {
				auto taille = static_cast<TypeTableauFixe *>(type_structure)->taille;

				return llvm::ConstantInt::get(
							converti_type_llvm(contexte, b->type),
							static_cast<unsigned long>(taille));
			}

			if (type_structure->genre == GenreType::TABLEAU_DYNAMIQUE || type_structure->genre == GenreType::VARIADIQUE) {
				/* charge taille de la structure tableau { *type, taille } */
				auto valeur = genere_code_llvm(structure, contexte, true);

				if (est_pointeur) {
					/* déréférence le pointeur en le chargeant */
					auto charge = new llvm::LoadInst(valeur, "", contexte.bloc_courant());
					charge->setAlignment(8);
					valeur = charge;
				}

				if (membre->ident->nom == "pointeur") {
					return accede_membre_structure(contexte, valeur, POINTEUR_TABLEAU, !expr_gauche);
				}

				if (membre->ident->nom == "capacité") {
					return accede_membre_structure(contexte, valeur, CAPACITE_TABLEAU, !expr_gauche);
				}

				return accede_membre_structure(contexte, valeur, TAILLE_TABLEAU, !expr_gauche);
			}

			auto const &nom_membre = membre->ident->nom;

			if (type_structure->genre == GenreType::ENUM || type_structure->genre == GenreType::ERREUR) {
				auto type_enum = static_cast<TypeEnum *>(type_structure);
				auto decl_enum = type_enum->decl;
				auto builder = llvm::IRBuilder<>(contexte.contexte);
				builder.SetInsertPoint(contexte.bloc_courant());
				return valeur_enum(decl_enum, nom_membre, builder);
			}

			auto type_struct = static_cast<TypeStructure *>(type_structure);
			auto decl_struct = type_struct->decl;

			auto valeur = genere_code_llvm(structure, contexte, true);

			auto const index_membre = trouve_index_membre(decl_struct, nom_membre);

			llvm::Value *ret;

			if (est_pointeur) {
				/* déréférence le pointeur en le chargeant */
				auto charge = new llvm::LoadInst(valeur, "", contexte.bloc_courant());
				charge->setAlignment(8);
				valeur = charge;
			}

			ret = accede_membre_structure(contexte, valeur, static_cast<size_t>(index_membre));

			if (!expr_gauche) {
				auto charge = new llvm::LoadInst(ret, "", contexte.bloc_courant());
				auto type_membre = decl_struct->desc.membres[index_membre].type;
				charge->setAlignment(type_membre->alignement);
				ret = charge;
			}

			return ret;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			auto expr = static_cast<NoeudExpressionBinaire *>(b);
			auto structure = expr->expr1;
			auto membre = expr->expr2;
			auto const &nom_membre = membre->ident->nom;

			auto type_structure = static_cast<TypeStructure *>(structure->type);
			auto decl_struct = type_structure->decl;

			auto valeur = genere_code_llvm(structure, contexte, true);

			if (expr_gauche) {
				auto idx_membre = trouve_index_membre(decl_struct, nom_membre);
				auto pointeur_membre_actif = accede_membre_structure(contexte, valeur, 1);
				auto valeur_active = llvm::ConstantInt::get(
							llvm::Type::getInt32Ty(contexte.contexte),
							static_cast<unsigned>(idx_membre + 1));

				new llvm::StoreInst(valeur_active, pointeur_membre_actif, contexte.bloc_courant());
			}

			return accede_membre_union(contexte, valeur, decl_struct, nom_membre, !expr_gauche);
		}
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		{
			auto expr = static_cast<NoeudExpressionBinaire *>(b);
			auto variable = expr->expr1;
			auto expression = expr->expr2;

			/* Génère d'abord le code de l'enfant afin que l'instruction d'allocation de
			 * la variable sur la pile et celle de stockage de la valeur soit côte à
			 * côte. */
			auto valeur = applique_transformation(contexte, expression, false);

			auto alloc = genere_code_llvm(variable, contexte, true);

			auto store = new llvm::StoreInst(valeur, alloc, false, contexte.bloc_courant());

			store->setAlignment(expression->type->alignement);

			return store;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			auto expr = static_cast<NoeudDeclarationVariable *>(b);
			auto variable = expr->valeur;
			auto expression = expr->expression;
			auto type = variable->type;

			if (contexte.fonction == nullptr) {
				auto est_externe = possede_drapeau(variable->drapeaux, EST_EXTERNE);
				auto vg = constructrice.alloue_globale(variable->ident, type, expression, est_externe);

				contexte.ajoute_globale(variable->ident, vg);

				return vg;
			}

			auto alloc = constructrice.alloue_variable(variable->ident, type, expression);
			contexte.ajoute_locale(variable->ident, alloc);

			return alloc;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		{
			return llvm::ConstantFP::get(
						llvm::Type::getFloatTy(contexte.contexte),
						b->lexeme->valeur_reelle);
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		{
			return llvm::ConstantInt::get(
						converti_type_llvm(contexte, b->type),
						b->lexeme->valeur_entiere,
						false);
		}
		case GenreNoeud::OPERATEUR_BINAIRE:
		{
			auto expr = static_cast<NoeudExpressionBinaire *>(b);
			auto enfant1 = expr->expr1;
			auto enfant2 = expr->expr2;
			auto type1 = enfant1->type;
			auto type2 = enfant2->type;
			auto op = expr->op;

			if ((b->drapeaux & EST_ASSIGNATION_COMPOSEE) != 0) {
				auto ptr_valeur1 = genere_code_llvm(enfant1, contexte, true);
				auto valeur1 = new llvm::LoadInst(ptr_valeur1, "", false, contexte.bloc_courant());
				auto valeur2 = applique_transformation(contexte, enfant2, false);
				auto val_resultat = static_cast<llvm::Value *>(nullptr);

				if (op->est_basique) {
					// détecte arithmétique de pointeur
					if (type1->genre == GenreType::POINTEUR && (est_type_entier(type2) || type2->genre == GenreType::ENTIER_CONSTANT)) {
						val_resultat = llvm::GetElementPtrInst::CreateInBounds(
									valeur1,
									valeur2,
									"",
									contexte.bloc_courant());
					}
					else if (type2->genre == GenreType::POINTEUR && (est_type_entier(type1) || type1->genre == GenreType::ENTIER_CONSTANT)) {
						val_resultat = llvm::GetElementPtrInst::CreateInBounds(
									valeur2,
									valeur1,
									"",
									contexte.bloc_courant());
					}
					else {
						val_resultat = llvm::BinaryOperator::Create(op->instr_llvm, valeur1, valeur2, "", contexte.bloc_courant());
					}
				}
				else {
					val_resultat = constructrice.appel_operateur(op, valeur1, valeur2);
				}

				new llvm::StoreInst(val_resultat, ptr_valeur1, contexte.bloc_courant());
				return nullptr;
			}

			auto valeur1 = applique_transformation(contexte, enfant1, false);
			auto valeur2 = applique_transformation(contexte, enfant2, false);

			if (op->est_basique) {
				if (op->est_comp_entier) {
					// détecte comparaison de pointeurs avec nul
					if (type1->genre == GenreType::POINTEUR && type2->genre == GenreType::POINTEUR) {
						auto type_pointe1 = static_cast<TypePointeur *>(type1)->type_pointe;
						auto type_pointe2 = static_cast<TypePointeur *>(type2)->type_pointe;

						if (type_pointe1 == nullptr) {
							valeur1 = new llvm::BitCastInst(valeur1, converti_type_llvm(contexte, type2), "", contexte.bloc_courant());
						}
						else if (type_pointe2 == nullptr) {
							valeur2 = new llvm::BitCastInst(valeur2, converti_type_llvm(contexte, type1), "", contexte.bloc_courant());
						}
					}

					return llvm::ICmpInst::Create(llvm::Instruction::ICmp, op->predicat_llvm, valeur1, valeur2, "", contexte.bloc_courant());
				}

				if (op->est_comp_reel) {
					return llvm::FCmpInst::Create(llvm::Instruction::FCmp, op->predicat_llvm, valeur1, valeur2, "", contexte.bloc_courant());
				}

				// détecte arithmétique de pointeur
				if (type1->genre != type2->genre) {
					if (type1->genre == GenreType::POINTEUR && (est_type_entier(type2) || type2->genre == GenreType::ENTIER_CONSTANT)) {
						return llvm::GetElementPtrInst::CreateInBounds(
									valeur1,
									valeur2,
									"",
									contexte.bloc_courant());
					}

					if (type2->genre == GenreType::POINTEUR && (est_type_entier(type1) || type1->genre == GenreType::ENTIER_CONSTANT)) {
						return llvm::GetElementPtrInst::CreateInBounds(
									valeur2,
									valeur1,
									"",
									contexte.bloc_courant());
					}
				}

				return llvm::BinaryOperator::Create(op->instr_llvm, valeur1, valeur2, "", contexte.bloc_courant());
			}

			return constructrice.appel_operateur(op, valeur1, valeur2);
		}
		case GenreNoeud::OPERATEUR_UNAIRE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(b);
			auto enfant = expr->expr;
			//auto op = expr->op;

			llvm::Instruction::BinaryOps instr;
			auto type1 = enfant->type;
			// retourne une valeur gauche pour les prises d'adresses afin d'éviter
			// de charger des larges tableaux fixes
			auto valeur1 = genere_code_llvm(enfant, contexte, b->lexeme->genre == GenreLexeme::AROBASE);
			auto valeur2 = static_cast<llvm::Value *>(nullptr);

			switch (b->lexeme->genre) {
				case GenreLexeme::EXCLAMATION:
				{
					valeur2 = llvm::ConstantInt::get(
								  llvm::Type::getInt1Ty(contexte.contexte),
								  static_cast<uint64_t>(0),
								  false);

					valeur1 = llvm::ICmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::Predicate::ICMP_EQ, valeur1, valeur2, "", contexte.bloc_courant());

					instr = llvm::Instruction::Xor;
					valeur2 = llvm::ConstantInt::get(
								  llvm::Type::getInt1Ty(contexte.contexte),
								  static_cast<uint64_t>(1),
								  false);
					break;
				}
				case GenreLexeme::TILDE:
				{
					instr = llvm::Instruction::Xor;
					valeur2 = llvm::ConstantInt::get(
								  llvm::Type::getInt32Ty(contexte.contexte),
								  static_cast<uint64_t>(0),
								  false);
					break;
				}
				case GenreLexeme::AROBASE:
				{
					assert(llvm::isa<llvm::AllocaInst>(valeur1) || llvm::isa<llvm::GetElementPtrInst>(valeur1) || llvm::isa<llvm::GlobalObject>(valeur1));
					return valeur1;
				}
				case GenreLexeme::PLUS_UNAIRE:
				{
					return valeur1;
				}
				case GenreLexeme::MOINS_UNAIRE:
				{
					valeur2 = valeur1;

					if (est_type_entier(type1) || type1->genre == GenreType::ENTIER_CONSTANT) {
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

			if (contexte.fonction == nullptr) {
				return llvm::ConstantExpr::get(instr, static_cast<llvm::Constant *>(valeur1), static_cast<llvm::Constant *>(valeur2));
			}

			return llvm::BinaryOperator::Create(instr, valeur1, valeur2, "", contexte.bloc_courant());
		}
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		{
			auto expr = static_cast<NoeudExpressionBinaire *>(b);
			auto enfant1 = expr->expr1;
			auto enfant2 = expr->expr2;
			auto op = expr->op;

			auto a_comp_b = genere_code_llvm(enfant1, contexte, expr_gauche);
			auto val_c = applique_transformation(contexte, enfant2, expr_gauche);

			// À FAIRE : évite de regénérer ce code.
			auto expr_a = static_cast<NoeudExpressionBinaire *>(enfant1);
			auto val_b = applique_transformation(contexte, expr_a->expr2, expr_gauche);

			auto b_comp_c = static_cast<llvm::Value *>(nullptr);

			if (op->est_basique) {
				if (op->est_comp_entier) {
					b_comp_c = llvm::ICmpInst::Create(llvm::Instruction::ICmp, op->predicat_llvm, val_b, val_c, "", contexte.bloc_courant());
				}
				else if (op->est_comp_reel) {
					b_comp_c = llvm::FCmpInst::Create(llvm::Instruction::FCmp, op->predicat_llvm, val_b, val_c, "", contexte.bloc_courant());
				}
				else {
					b_comp_c = llvm::BinaryOperator::Create(op->instr_llvm, val_b, val_c, "", contexte.bloc_courant());
				}
			}
			else {
				b_comp_c = constructrice.appel_operateur(op, val_b, val_c);
			}

			return llvm::BinaryOperator::Create(llvm::BinaryOperator::BinaryOps::And, a_comp_b, b_comp_c, "", contexte.bloc_courant());
		}
		case GenreNoeud::EXPRESSION_INDICE:
		{
			auto expr = static_cast<NoeudExpressionBinaire *>(b);
			auto enfant1 = expr->expr1;
			auto enfant2 = expr->expr2;

			auto const type1 = enfant1->type;

			auto valeur1 = genere_code_llvm(enfant1, contexte, true);
			auto valeur2 = genere_code_llvm(enfant2, contexte, false);

			llvm::Value *valeur;

			if (type1->genre == GenreType::POINTEUR) {
				valeur1 = new llvm::LoadInst(valeur1, "", false, contexte.bloc_courant());
				valeur = llvm::GetElementPtrInst::CreateInBounds(
							 valeur1,
							 valeur2,
							 "",
							 contexte.bloc_courant());
			}
			else {
				if (type1->genre == GenreType::TABLEAU_FIXE) {
					valeur = accede_element_tableau(
								 contexte,
								 valeur1,
								 converti_type_llvm(contexte, type1),
								 valeur2);
				}
				else {
					auto pointeur = accede_membre_structure(contexte, valeur1, 0, true);

					valeur = llvm::GetElementPtrInst::CreateInBounds(
								pointeur,
								valeur2,
								"",
								contexte.bloc_courant());
				}
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
			charge->setAlignment(type1->alignement);
			return charge;
		}
		case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:
		{
			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_RETOUR_SIMPLE:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(b);
			llvm::Value *valeur = nullptr;

			if (inst->expr != nullptr) {
				valeur = applique_transformation(contexte, inst->expr, false);
			}

			/* NOTE : le code différé doit être crée après l'expression de retour, car
			 * nous risquerions par exemple de déloger une variable utilisée dans
			 * l'expression de retour. */
			genere_code_extra_pre_retour(contexte, b->bloc_parent);

			return llvm::ReturnInst::Create(contexte.contexte, valeur, contexte.bloc_courant());
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		{
			auto chaine = std::any_cast<dls::chaine>(b->valeur_calculee);
			return constructrice.cree_chaine(chaine);
		}
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		{
			auto const est_calcule = possede_drapeau(b->drapeaux, EST_CALCULE);
			auto const valeur = est_calcule ? std::any_cast<bool>(b->valeur_calculee)
											  : (b->lexeme->chaine == "vrai");
			return llvm::ConstantInt::get(
						llvm::Type::getInt1Ty(contexte.contexte),
						static_cast<uint64_t>(valeur),
						false);
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		{
			auto valeur = dls::caractere_echappe(&b->lexeme->chaine[0]);

			return llvm::ConstantInt::get(
						llvm::Type::getInt8Ty(contexte.contexte),
						static_cast<uint64_t>(valeur),
						false);
		}
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		{
			auto inst = static_cast<NoeudSi *>(b);

			auto condition = genere_code_llvm(inst->condition, contexte, false);

			if (b->genre == GenreNoeud::INSTRUCTION_SAUFSI) {
				auto valeur2 = llvm::ConstantInt::get(
							  llvm::Type::getInt1Ty(contexte.contexte),
							  static_cast<uint64_t>(0),
							  false);

				condition = llvm::ICmpInst::Create(
							llvm::Instruction::ICmp,
							llvm::CmpInst::Predicate::ICMP_EQ,
							condition,
							valeur2,
							"",
							contexte.bloc_courant());
			}

			auto bloc_alors = cree_bloc(contexte, "alors");

			auto bloc_sinon = inst->bloc_si_faux
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
			auto enfant2 = inst->bloc_si_vrai;
			enfant2->valeur_calculee = bloc_fusion;
			genere_code_llvm(enfant2, contexte, false);

			/* noeud 3 : sinon (optionel) */
			if (inst->bloc_si_faux) {
				contexte.bloc_courant(bloc_sinon);

				auto enfant3 = inst->bloc_si_faux;
				enfant3->valeur_calculee = bloc_fusion;
				genere_code_llvm(enfant3, contexte, false);
			}

			contexte.bloc_courant(bloc_fusion);

			// ne retourne rien, car cela fais échoué la logique d'arrêt de
			// génération de code dans GenreNoeud::INSTRUCTION_COMPOSEE
			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			auto inst = static_cast<NoeudBloc *>(b);

			if (inst->est_differe) {
				return nullptr;
			}

			llvm::Value *valeur = nullptr;

			auto bloc_entree = contexte.bloc_courant();

			POUR (inst->expressions) {
				// un bloc « orphelin », qui n'est pas attaché à une fonction,
				// ou instruction, sans doute utilisé pour confiner une portion
				// de code
				if (it->genre == GenreNoeud::INSTRUCTION_COMPOSEE) {
					it->valeur_calculee = static_cast<llvm::BasicBlock *>(nullptr);
				}

				valeur = genere_code_llvm(it, contexte, true);

				/* nul besoin de continuer à générer du code pour des expressions qui ne
				 * seront jamais executées. */
				if (est_instruction_retour(valeur)) {
					return valeur;
				}
			}

			if (est_instruction_branche(valeur)) {
				return valeur;
			}

			auto bloc_suivant = std::any_cast<llvm::BasicBlock *>(b->valeur_calculee);

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

			return valeur;
		}
		case GenreNoeud::INSTRUCTION_POUR:
		{
			/* valeur_debut
			 * valeur_index
			 *
			 * boucle:
			 *	cmp valeur_debut, valeur_fin
			 *	br corps_boucle, apres_boucle
			 *
			 * corps_boucle:
			 *	...
			 *	br inc_boucle
			 *
			 * inc_boucle:
			 *	inc valeur_debut
			 *	br boucle
			 *
			 * apres_boucle:
			 *	...
			 */

			auto inst = static_cast<NoeudPour *>(b);

			/* on génère d'abord le type de la variable */
			auto enfant1 = inst->variable;
			auto enfant2 = inst->expression;
			auto enfant3 = inst->bloc;
			auto enfant_sans_arret = inst->bloc_sansarret;
			auto enfant_sinon = inst->bloc_sinon;

			auto type = enfant2->type;
			enfant1->type = type;

			/* création des blocs */
			auto bloc_boucle = cree_bloc(contexte, "boucle");
			auto bloc_corps = cree_bloc(contexte, "corps_boucle");
			auto bloc_inc = cree_bloc(contexte, "inc_boucle");

			auto bloc_sansarret = static_cast<llvm::BasicBlock *>(nullptr);
			auto bloc_sinon = static_cast<llvm::BasicBlock *>(nullptr);

			if (enfant_sans_arret) {
				bloc_sansarret = cree_bloc(contexte, "sansarret_boucle");
			}

			if (enfant_sinon) {
				bloc_sinon = cree_bloc(contexte, "sinon_boucle");
			}

			auto bloc_apres = cree_bloc(contexte, "apres_boucle");

			auto var = enfant1;
			auto idx = static_cast<NoeudExpression *>(nullptr);

			if (enfant1->lexeme->genre == GenreLexeme::VIRGULE) {
				auto expr_bin = static_cast<NoeudExpressionBinaire *>(var);
				var = expr_bin->expr1;
				idx = expr_bin->expr2;
			}

			contexte.empile_bloc_continue(var->ident->nom, bloc_inc);
			contexte.empile_bloc_arrete(var->ident->nom, (bloc_sinon != nullptr) ? bloc_sinon : bloc_apres);

			auto type_de_la_variable = static_cast<Type *>(nullptr);
			auto type_de_l_index = static_cast<Type *>(nullptr);

			if (b->aide_generation_code == GENERE_BOUCLE_PLAGE || b->aide_generation_code == GENERE_BOUCLE_PLAGE_INDEX) {
				type_de_la_variable = enfant2->type;
				type_de_l_index = enfant2->type;
			}
			else {
				type_de_la_variable = contexte.typeuse[TypeBase::Z64];
				type_de_l_index = contexte.typeuse[TypeBase::Z64];
			}

			auto valeur_debut = constructrice.alloue_variable(var->ident, type_de_la_variable, nullptr);
			auto valeur_index = static_cast<llvm::Value *>(nullptr);

			if (idx != nullptr) {
				valeur_index = constructrice.alloue_variable(idx->ident, type_de_l_index, nullptr);
			}

			auto builder = llvm::IRBuilder<>(contexte.contexte);
			builder.SetInsertPoint(contexte.bloc_courant());
			if (b->aide_generation_code == GENERE_BOUCLE_PLAGE || b->aide_generation_code == GENERE_BOUCLE_PLAGE_INDEX) {
				auto expr_plage = static_cast<NoeudExpressionBinaire *>(enfant2);
				auto init_debut = genere_code_llvm(expr_plage->expr1, contexte, false);
				builder.CreateStore(init_debut, valeur_debut);
			}

			/* bloc_boucle */
			/* on crée une branche explicite dans le bloc */
			llvm::BranchInst::Create(bloc_boucle, contexte.bloc_courant());

			contexte.bloc_courant(bloc_boucle);

			auto pointeur_tableau = static_cast<llvm::Value *>(nullptr);

			builder.SetInsertPoint(contexte.bloc_courant());

			switch (b->aide_generation_code) {
				case GENERE_BOUCLE_PLAGE:
				case GENERE_BOUCLE_PLAGE_INDEX:
				{
					/* création du bloc de condition */

					auto expr_plage = static_cast<NoeudExpressionBinaire *>(enfant2);
					auto valeur_fin = genere_code_llvm(expr_plage->expr2, contexte, false);

					auto condition = llvm::ICmpInst::Create(
										 llvm::Instruction::ICmp,
										 llvm::CmpInst::Predicate::ICMP_SLE,
										 builder.CreateLoad(valeur_debut),
										 valeur_fin,
										 "",
										 contexte.bloc_courant());

					builder.CreateCondBr(condition, bloc_corps, (bloc_sansarret != nullptr) ? bloc_sansarret : bloc_apres);

					if (b->aide_generation_code == GENERE_BOUCLE_PLAGE_INDEX) {
						contexte.ajoute_locale(var->ident, valeur_debut);
						contexte.ajoute_locale(idx->ident, valeur_index);
					}
					else {
						contexte.ajoute_locale(enfant1->ident, valeur_debut);
					}

					/* création du bloc de corps */

					contexte.bloc_courant(bloc_corps);

					enfant3->valeur_calculee = bloc_inc;
					genere_code_llvm(enfant3, contexte, false);

					/* bloc_inc */
					contexte.bloc_courant(bloc_inc);

					constructrice.incremente(valeur_debut, type_de_la_variable);

					if (valeur_index) {
						constructrice.incremente(valeur_index, type_de_l_index);
					}

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

#if 0
					{
						auto format_printf = builder.CreateGlobalStringPtr("[boucle pour] taille : %d, phi : %d\n");
						auto args_printf = std::vector<llvm::Value *>();
						args_printf.push_back(format_printf);
						args_printf.push_back(valeur_fin);
						args_printf.push_back(builder.CreateLoad(valeur_debut));

						auto fonc_printf = contexte.module_llvm->getFunction("printf");
						builder.CreateCall(fonc_printf, args_printf);
					}
#endif

					auto condition = llvm::ICmpInst::Create(
										 llvm::Instruction::ICmp,
										 llvm::CmpInst::Predicate::ICMP_SLT,
										 builder.CreateLoad(valeur_debut),
										 valeur_fin,
										 "",
										 contexte.bloc_courant());

#if 0
					{
						auto format_printf = builder.CreateGlobalStringPtr("[boucle pour, cond] cond : %d\n");
						auto args_printf = std::vector<llvm::Value *>();
						args_printf.push_back(format_printf);
						args_printf.push_back(condition);

						auto fonc_printf = contexte.module_llvm->getFunction("printf");
						builder.CreateCall(fonc_printf, args_printf);
					}
#endif

					llvm::BranchInst::Create(
								bloc_corps,
								(bloc_sansarret != nullptr) ? bloc_sansarret : bloc_apres,
								condition,
								contexte.bloc_courant());

					contexte.bloc_courant(bloc_corps);
					builder.SetInsertPoint(bloc_corps);

					auto valeur_arg = static_cast<llvm::Value *>(nullptr);

					if (taille_tableau != 0) {
						auto valeur_tableau = genere_code_llvm(enfant2, contexte, true);

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

					if (b->aide_generation_code == GENERE_BOUCLE_TABLEAU_INDEX) {
						contexte.ajoute_locale(var->ident, valeur_arg);
						contexte.ajoute_locale(idx->ident, valeur_index);
					}
					else {
						contexte.ajoute_locale(var->ident, valeur_arg);
					}

					/* création du bloc de corps */
					enfant3->valeur_calculee = bloc_inc;
					genere_code_llvm(enfant3, contexte, false);

					/* bloc_inc */
					contexte.bloc_courant(bloc_inc);

					constructrice.incremente(valeur_debut, type_de_la_variable);

					if (valeur_index) {
						constructrice.incremente(valeur_index, type_de_l_index);
					}
#if 0
					{
						auto format_printf = builder.CreateGlobalStringPtr("[boucle pour, inc] taille : %d, phi : %d\n");
						auto args_printf = std::vector<llvm::Value *>();
						args_printf.push_back(format_printf);
						args_printf.push_back(valeur_fin);
						args_printf.push_back(builder.CreateLoad(valeur_debut));

						auto fonc_printf = contexte.module_llvm->getFunction("printf");
						builder.CreateCall(fonc_printf, args_printf);
					}
#endif

					break;
				}
				case GENERE_BOUCLE_COROUTINE:
				case GENERE_BOUCLE_COROUTINE_INDEX:
				{
					/* À FAIRE(coroutine) */
					break;
				}
			}

			llvm::BranchInst::Create(bloc_boucle, contexte.bloc_courant());

			/* 'continue'/'arrête' dans les blocs 'sinon'/'sansarrêt' n'a aucun sens */
			contexte.depile_bloc_continue();
			contexte.depile_bloc_arrete();

			if (enfant_sans_arret) {
				contexte.bloc_courant(bloc_sansarret);
				enfant_sans_arret->valeur_calculee = bloc_apres;
				genere_code_llvm(enfant_sans_arret, contexte, false);
			}

			if (enfant_sinon) {
				contexte.bloc_courant(bloc_sinon);
				enfant_sinon->valeur_calculee = bloc_apres;
				genere_code_llvm(enfant_sinon, contexte, false);
			}

			contexte.bloc_courant(bloc_apres);

			// ne retourne rien, car cela fais échoué la logique d'arrêt de
			// génération de code dans GenreNoeud::INSTRUCTION_COMPOSEE
			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(b);
			auto chaine_var = inst->expr == nullptr ? dls::vue_chaine_compacte{""} : inst->expr->ident->nom;

			auto bloc = (b->lexeme->genre == GenreLexeme::CONTINUE)
						? contexte.bloc_continue(chaine_var)
						: contexte.bloc_arrete(chaine_var);

			return llvm::BranchInst::Create(bloc, contexte.bloc_courant());
		}
		case GenreNoeud::INSTRUCTION_BOUCLE:
		{
			/* boucle:
			 *	corps
			 *  br boucle
			 *
			 * apres_boucle:
			 *	...
			 */

			auto inst = static_cast<NoeudBoucle *>(b);

			auto bloc_boucle = cree_bloc(contexte, "boucle");
			auto bloc_apres = cree_bloc(contexte, "apres_boucle");

			contexte.empile_bloc_continue("", bloc_boucle);
			contexte.empile_bloc_arrete("", bloc_apres);

			/* on crée une branche explicite dans le bloc */
			llvm::BranchInst::Create(bloc_boucle, contexte.bloc_courant());

			contexte.bloc_courant(bloc_boucle);

			inst->bloc->valeur_calculee = bloc_boucle;
			genere_code_llvm(inst->bloc, contexte, false);

			contexte.depile_bloc_continue();
			contexte.depile_bloc_arrete();
			contexte.bloc_courant(bloc_apres);

			// ne retourne rien, car cela fais échoué la logique d'arrêt de
			// génération de code dans GenreNoeud::INSTRUCTION_COMPOSEE
			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_REPETE:
		{
			auto inst = static_cast<NoeudBoucle *>(b);

			auto bloc_boucle = cree_bloc(contexte, "repete");
			auto bloc_tantque = cree_bloc(contexte, "tantque_boucle");
			auto bloc_apres = cree_bloc(contexte, "apres_repete");

			contexte.empile_bloc_continue("", bloc_boucle);
			contexte.empile_bloc_arrete("", bloc_apres);

			/* on crée une branche explicite dans le bloc */
			llvm::BranchInst::Create(bloc_boucle, contexte.bloc_courant());

			contexte.bloc_courant(bloc_boucle);

			inst->bloc->valeur_calculee = bloc_tantque;
			genere_code_llvm(inst->bloc, contexte, false);

			contexte.bloc_courant(bloc_tantque);

			auto condition = genere_code_llvm(inst->condition, contexte, false);

			llvm::BranchInst::Create(
						bloc_boucle,
						bloc_apres,
						condition,
						contexte.bloc_courant());

			contexte.depile_bloc_continue();
			contexte.depile_bloc_arrete();
			contexte.bloc_courant(bloc_apres);

			// ne retourne rien, car cela fais échoué la logique d'arrêt de
			// génération de code dans GenreNoeud::INSTRUCTION_COMPOSEE
			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_TANTQUE:
		{
			auto inst = static_cast<NoeudBoucle *>(b);

			auto bloc_tantque_cond = cree_bloc(contexte, "tantque_cond");
			auto bloc_tantque_corps = cree_bloc(contexte, "tantque_corps");
			auto bloc_apres = cree_bloc(contexte, "apres_tantque");

			contexte.empile_bloc_continue("", bloc_tantque_cond);
			contexte.empile_bloc_arrete("", bloc_apres);

			/* on crée une branche explicite dans le bloc */
			llvm::BranchInst::Create(bloc_tantque_cond, contexte.bloc_courant());

			contexte.bloc_courant(bloc_tantque_cond);

			auto condition = genere_code_llvm(inst->condition, contexte, false);

			llvm::BranchInst::Create(
						bloc_tantque_corps,
						bloc_apres,
						condition,
						contexte.bloc_courant());

			contexte.bloc_courant(bloc_tantque_corps);

			inst->bloc->valeur_calculee = bloc_tantque_cond;
			genere_code_llvm(inst->bloc, contexte, false);

			contexte.depile_bloc_continue();
			contexte.depile_bloc_arrete();
			contexte.bloc_courant(bloc_apres);

			// ne retourne rien, car cela fais échoué la logique d'arrêt de
			// génération de code dans GenreNoeud::INSTRUCTION_COMPOSEE
			return nullptr;
		}
		case GenreNoeud::EXPRESSION_COMME:
		{
			auto inst = static_cast<NoeudExpressionBinaire *>(b);
			auto enfant = inst->expr1;
			auto valeur = genere_code_llvm(enfant, contexte, false);
			auto type_de = enfant->type;

			if (type_de == b->type) {
				return valeur;
			}

			using CastOps = llvm::Instruction::CastOps;

			auto type_llvm = converti_type_llvm(contexte, b->type);
			auto bloc = contexte.bloc_courant();
			auto type_vers = b->type;

			if (est_type_entier(type_de) || type_de->genre == GenreType::ENTIER_CONSTANT) {
				/* un nombre entier peut être converti en l'adresse d'un pointeur */
				if (type_vers->genre == GenreType::POINTEUR) {
					return cree_instruction<CastOps::IntToPtr>(valeur, type_llvm, bloc);
				}

				if (type_vers->genre == GenreType::REEL) {
					if (type_de->genre == GenreType::ENTIER_NATUREL) {
						return cree_instruction<CastOps::UIToFP>(valeur, type_llvm, bloc);
					}

					return cree_instruction<CastOps::SIToFP>(valeur, type_llvm, bloc);
				}

				if (est_type_entier(type_vers)) {
					if (est_plus_petit(type_vers, type_de)) {
						return cree_instruction<CastOps::Trunc>(valeur, type_llvm, bloc);
					}

					if (type_vers->taille_octet == type_de->taille_octet) {
						return valeur;
					}

					if (type_vers->genre == GenreType::ENTIER_NATUREL) {
						return cree_instruction<CastOps::ZExt>(valeur, type_llvm, bloc);
					}

					return cree_instruction<CastOps::SExt>(valeur, type_llvm, bloc);
				}
			}

			if (type_de->genre == GenreType::REEL) {
				if (type_vers->genre == GenreType::ENTIER_NATUREL) {
					return cree_instruction<CastOps::FPToUI>(valeur, type_llvm, bloc);
				}

				if (type_vers->genre == GenreType::ENTIER_RELATIF) {
					return cree_instruction<CastOps::FPToSI>(valeur, type_llvm, bloc);
				}

				if (type_vers->genre == GenreType::REEL) {
					if (est_plus_petit(type_vers, type_de)) {
						return cree_instruction<CastOps::FPTrunc>(valeur, type_llvm, bloc);
					}

					return cree_instruction<CastOps::FPExt>(valeur, type_llvm, bloc);
				}
			}

			if (type_de->genre == GenreType::POINTEUR && est_type_entier(type_vers)) {
				return cree_instruction<CastOps::PtrToInt>(valeur, type_llvm, bloc);
			}

			return cree_instruction<CastOps::BitCast>(valeur, type_llvm, bloc);
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		{
			auto type_pointeur = contexte.typeuse[TypeBase::PTR_RIEN];
			auto type_llvm = converti_type_llvm(contexte, type_pointeur);
			return llvm::ConstantPointerNull::get(static_cast<llvm::PointerType *>(type_llvm));
		}
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		{
			auto dl = llvm::DataLayout(contexte.module_llvm);
			auto type = std::any_cast<Type *>(b->valeur_calculee);
			auto type_llvm = converti_type_llvm(contexte, type);
			auto taille = dl.getTypeAllocSize(type_llvm);

			return llvm::ConstantInt::get(
						llvm::Type::getInt32Ty(contexte.contexte),
						taille,
						false);
		}
		case GenreNoeud::EXPRESSION_PLAGE:
		{
			// prise en charge dans EXPRESSION_POUR
			return nullptr;
		}
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		{
			auto noeud_tableau = static_cast<NoeudTableauArgsVariadiques *>(b);
			auto taille_tableau = noeud_tableau->exprs.taille;

			auto type = b->type;

			/* alloue un tableau fixe */
			auto type_tableau_fixe = contexte.typeuse.type_tableau_fixe(type, taille_tableau);

			auto type_llvm = converti_type_llvm(contexte, type_tableau_fixe);

			auto pointeur_tableau = new llvm::AllocaInst(
										type_llvm,
										0,
										nullptr,
										"",
										contexte.bloc_courant());

			/* copie les valeurs dans le tableau fixe */
			auto index = 0ul;

			POUR (noeud_tableau->exprs) {
				auto valeur_enfant = applique_transformation(contexte, it, false);

				auto index_tableau = accede_element_tableau(
										 contexte,
										 pointeur_tableau,
										 type_llvm,
										 index++);

				new llvm::StoreInst(valeur_enfant, index_tableau, contexte.bloc_courant());
			}

			/* alloue un tableau dynamique */
			return converti_vers_tableau_dyn(contexte, pointeur_tableau, type_tableau_fixe);
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(b);

			dls::tablet<NoeudExpression *, 10> feuilles;
			rassemble_feuilles(expr->expr, feuilles);

			/* alloue de la place pour le tableau */
			auto type = b->type;
			auto type_llvm = converti_type_llvm(contexte, type);

			auto pointeur_tableau = new llvm::AllocaInst(
										type_llvm,
										0,
										nullptr,
										"",
										contexte.bloc_courant());
			pointeur_tableau->setAlignment(type->alignement);

			/* stocke les valeurs des feuilles */
			auto index = 0ul;

			for (auto f : feuilles) {
				auto valeur = applique_transformation(contexte, f, false);

				auto index_tableau = accede_element_tableau(
										 contexte,
										 pointeur_tableau,
										 type_llvm,
										 index++);

				auto stocke = new llvm::StoreInst(valeur, index_tableau, contexte.bloc_courant());
				stocke->setAlignment(type->alignement);
			}

			assert(expr_gauche == false);
			return new llvm::LoadInst(pointeur_tableau, "", false, contexte.bloc_courant());
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		{
			auto expr = static_cast<NoeudExpressionAppel *>(b);
			auto type_struct = static_cast<TypeStructure *>(b->type);
			auto decl_struct = type_struct->decl;
			auto type_struct_llvm = converti_type_llvm(contexte, type_struct);

			auto builder = llvm::IRBuilder<>(contexte.contexte);
			builder.SetInsertPoint(contexte.bloc_courant());

			auto alloc = builder.CreateAlloca(type_struct_llvm, 0u);

			if (b->lexeme->chaine == "PositionCodeSource") {
				return genere_code_position_source(contexte, b);
			}

			for (auto i = 0l; i < expr->params.taille; ++i) {
				auto param = static_cast<NoeudExpressionBinaire *>(expr->params[i]);
				auto idx_membre = trouve_index_membre(decl_struct, param->expr1->ident->nom);

				auto ptr = accede_membre_structure(contexte, alloc, static_cast<size_t>(idx_membre));

				auto val = applique_transformation(contexte, param->expr2, false);

				builder.CreateStore(val, ptr);
			}

			return builder.CreateLoad(alloc, "");
		}
		case GenreNoeud::EXPRESSION_INFO_DE:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(b);
			auto enfant = inst->expr;
			return cree_info_type(contexte, enfant->type);
		}
		case GenreNoeud::EXPRESSION_MEMOIRE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(b);
			auto valeur = genere_code_llvm(expr->expr, contexte, expr_gauche);
			return new llvm::LoadInst(valeur, "", contexte.bloc_courant());
		}
		case GenreNoeud::EXPRESSION_LOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(b);
			return genere_code_allocation(contexte, b->type, 0, b, expr->expr, expr->expr_chaine, expr->bloc);
		}
		case GenreNoeud::EXPRESSION_DELOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(b);
			return genere_code_allocation(contexte, expr->expr->type, 2, expr->expr, expr->expr, expr->expr_chaine, expr->bloc);
		}
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(b);
			return genere_code_allocation(contexte, expr->expr->type, 1, b, expr->expr, expr->expr_chaine, expr->bloc);
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			auto type_struct = static_cast<TypeStructure *>(b->type);

			if (type_struct->deja_genere) {
				return nullptr;
			}

			type_struct->type_llvm = converti_type_llvm(contexte, b->type);

			auto type_pointeur_struct = contexte.typeuse.type_pointeur_pour(type_struct);

			auto nom_fonction = "initialise_" + dls::vers_chaine(b->type);

			auto types_entrees = kuri::tableau<Type *>();
			types_entrees.pousse(contexte.type_contexte);
			types_entrees.pousse(type_pointeur_struct);

			auto types_sorties = kuri::tableau<Type *>();
			types_sorties.pousse(contexte.typeuse[TypeBase::RIEN]);

			auto type_fonction = contexte.typeuse.type_fonction(std::move(types_entrees), std::move(types_sorties));

			auto type_fonction_llvm = obtiens_type_fonction(
						contexte,
						type_fonction,
						false,
						false);

			auto fonction = llvm::Function::Create(
						type_fonction_llvm,
						llvm::Function::ExternalLinkage,
						nom_fonction.c_str(),
						contexte.module_llvm);

			auto ancienne_fonction = contexte.fonction;
			contexte.fonction = fonction;

			auto valeur_args = fonction->args().begin();

			auto ancien_bloc = contexte.bloc_courant();
			auto bloc = cree_bloc(contexte, "");
			contexte.bloc_courant(bloc);

			auto alloc_contexte = constructrice.alloue_param(nullptr, contexte.type_contexte, valeur_args);
			valeur_args++;

			auto alloc_struct = constructrice.alloue_param(nullptr, type_pointeur_struct, valeur_args);
			auto charge_struct = constructrice.charge(alloc_struct, b->type);

			auto &desc = type_struct->decl->desc;

			for (auto i = 0; i < desc.membres.taille; ++i) {
				auto type_membre = desc.membres[i].type;
				auto ptr_membre = accede_membre_structure(contexte, charge_struct, static_cast<size_t>(i));
				auto valeur_membre = static_cast<llvm::Value *>(nullptr);

				if (type_membre->genre == GenreType::STRUCTURE) {
					auto parametres = std::vector<llvm::Value *>();
					parametres.push_back(constructrice.charge(alloc_contexte, contexte.type_contexte));
					parametres.push_back(ptr_membre);

					auto nom_fonction_init = "initialise_" + dls::vers_chaine(type_membre);
					auto fonction_init = contexte.module_llvm->getFunction(nom_fonction_init.c_str());

					llvm::CallInst::Create(fonction_init, parametres, "", contexte.bloc_courant());
				}
				else if (type_membre->genre == GenreType::TABLEAU_FIXE) {
					// À FAIRE : défini une taille minimum car les instructions pour les tableaux trop gros prennent du temps à compiler
					// peut-être utiliser memset ?
				}
				else {
					valeur_membre = constructrice.cree_valeur_defaut_pour_type(desc.membres[i].type);
				}

				if (valeur_membre != nullptr) {
					constructrice.stocke(valeur_membre, ptr_membre);
				}
			}

			llvm::ReturnInst::Create(contexte.contexte, nullptr, bloc);

			contexte.fonction = ancienne_fonction;
			contexte.bloc_courant(ancien_bloc);
			type_struct->deja_genere = true;

			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		case GenreNoeud::INSTRUCTION_DISCR:
		{
			auto inst = static_cast<NoeudDiscr *>(b);
			auto expression = inst->expr;
			auto op = inst->op;
			auto decl_enum = static_cast<NoeudEnum *>(nullptr);
			auto decl_struct = static_cast<NoeudStruct *>(nullptr);

			if (b->genre == GenreNoeud::INSTRUCTION_DISCR_ENUM) {
				auto type_enum = static_cast<TypeEnum *>(expression->type);
				decl_enum = type_enum->decl;
			}
			else if (b->genre == GenreNoeud::INSTRUCTION_DISCR_UNION) {
				auto type_struct = static_cast<TypeStructure *>(expression->type);
				decl_struct = type_struct->decl;
			}

			struct DonneesPaireDiscr {
				NoeudExpression *expr = nullptr;
				NoeudBloc *bloc = nullptr;
				llvm::BasicBlock *bloc_de_la_condition = nullptr;
				llvm::BasicBlock *bloc_si_vrai = nullptr;
				llvm::BasicBlock *bloc_si_faux = nullptr;
			};

			auto bloc_post_discr = cree_bloc(contexte, "post_discr");

			dls::tableau<DonneesPaireDiscr> donnees_paires;
			donnees_paires.reserve(inst->paires_discr.taille + inst->bloc_sinon != nullptr);

			POUR (inst->paires_discr) {
				auto donnees = DonneesPaireDiscr();
				donnees.expr = it.first;
				donnees.bloc = it.second;
				donnees.bloc_de_la_condition = cree_bloc(contexte, "bloc_de_la_condition");
				donnees.bloc_si_vrai = cree_bloc(contexte, "bloc_si_vrai");

				if (!donnees_paires.est_vide()) {
					donnees_paires.back().bloc_si_faux = donnees.bloc_de_la_condition;
				}

				donnees_paires.pousse(donnees);
			}

			if (inst->bloc_sinon) {
				auto donnees = DonneesPaireDiscr();
				donnees.bloc = inst->bloc_sinon;
				donnees.bloc_si_vrai = cree_bloc(contexte, "bloc_si_vrai");

				if (!donnees_paires.est_vide()) {
					donnees_paires.back().bloc_si_faux = donnees.bloc_si_vrai;
				}

				donnees_paires.pousse(donnees);
			}

			donnees_paires.back().bloc_si_faux = bloc_post_discr;

			auto builder = llvm::IRBuilder<>(contexte.contexte);
			builder.SetInsertPoint(contexte.bloc_courant());

			auto valeur_expression = genere_code_llvm(expression, contexte, b->genre == GenreNoeud::INSTRUCTION_DISCR_UNION);
			auto ptr_structure = valeur_expression;

			if (b->genre == GenreNoeud::INSTRUCTION_DISCR_UNION) {
				valeur_expression = accede_membre_structure(contexte, valeur_expression, 1, true);
			}

			builder.CreateBr(donnees_paires.front().bloc_de_la_condition);

			for (auto &donnees : donnees_paires) {
				auto enf0 = donnees.expr;
				auto enf1 = donnees.bloc;

				contexte.bloc_courant(donnees.bloc_de_la_condition);
				builder.SetInsertPoint(contexte.bloc_courant());

				if (enf0 != nullptr) {
					auto feuilles = dls::tablet<NoeudExpression *, 10>();
					rassemble_feuilles(enf0, feuilles);

					// les différentes feuilles sont évaluées dans des blocs
					// séparés afin de pouvoir éviter de tester trop de conditions
					// dès qu'une condition est vraie, nous allons dans le bloc_si_vrai
					// sinon nous allons dans le bloc pour la feuille suivante
					for (auto f : feuilles) {
						auto bloc_si_faux = donnees.bloc_si_faux;

						if (f != feuilles.back()) {
							auto nouveau_bloc_condition = cree_bloc(contexte, "");
							bloc_si_faux = nouveau_bloc_condition;
						}

						auto valeur_f = static_cast<llvm::Value *>(nullptr);

						if (b->genre == GenreNoeud::INSTRUCTION_DISCR_ENUM) {
							valeur_f = valeur_enum(decl_enum, f->ident->nom, builder);
						}
						else if (b->genre == GenreNoeud::INSTRUCTION_DISCR_UNION) {
							auto idx_membre = trouve_index_membre(decl_struct, f->ident->nom);

							valeur_f = builder.getInt32(static_cast<unsigned>(idx_membre + 1));

							auto valeur = accede_membre_union(contexte, ptr_structure, decl_struct, f->ident->nom);

							contexte.ajoute_locale(f->ident, valeur);
						}
						else {
							valeur_f = genere_code_llvm(f, contexte, false);
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

							builder.CreateCondBr(condition, donnees.bloc_si_vrai, bloc_si_faux);
						}
						else {
							auto condition = constructrice.appel_operateur(op, valeur_expression, valeur_f);
							builder.CreateCondBr(condition, donnees.bloc_si_vrai, bloc_si_faux);
						}

						if (f != feuilles.back()) {
							contexte.bloc_courant(bloc_si_faux);
							builder.SetInsertPoint(bloc_si_faux);
						}
					}
				}

				if (donnees.bloc_si_vrai) {
					contexte.bloc_courant(donnees.bloc_si_vrai);
				}

				enf1->valeur_calculee = bloc_post_discr;
				genere_code_llvm(enf1, contexte, false);
			}

			contexte.bloc_courant(bloc_post_discr);

			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			auto inst = static_cast<NoeudPousseContexte *>(b);
			auto variable = inst->expr;

			auto ancien_contexte = contexte.valeur(contexte.ident_contexte);

			auto valeur = contexte.valeur(variable->ident);
			contexte.ajoute_locale(contexte.ident_contexte, valeur);

			auto bloc = inst->bloc;
			bloc->valeur_calculee = static_cast<llvm::BasicBlock *>(nullptr);

			auto ret = genere_code_llvm(bloc, contexte, false);

			contexte.ajoute_locale(contexte.ident_contexte, ancien_contexte);

			return ret;
		}
		case GenreNoeud::EXPANSION_VARIADIQUE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(b);
			return applique_transformation(contexte, expr->expr, expr_gauche);
		}
		case GenreNoeud::INSTRUCTION_TENTE:
		{
			// À FAIRE(tente)
			return nullptr;
		}
	}

	return nullptr;
}

static void traverse_graphe_pour_generation_code(
		ContexteGenerationLLVM &contexte,
		NoeudDependance *noeud)
{
	noeud->fut_visite = true;

	for (auto const &relation : noeud->relations) {
		auto accepte = relation.type == TypeRelation::UTILISE_TYPE;
		accepte |= relation.type == TypeRelation::UTILISE_FONCTION;
		accepte |= relation.type == TypeRelation::UTILISE_GLOBALE;

		if (!accepte) {
			continue;
		}

		/* évite les boucles infinies dues aux dépendances cycliques de types
		 * et fonctions recursives
		 */
		if (relation.noeud_fin->fut_visite) {
			continue;
		}

		traverse_graphe_pour_generation_code(contexte, relation.noeud_fin);
	}

	if (noeud->noeud_syntactique == nullptr) {
		// les types de bases n'ont pas de noeud syntaxique
		return;
	}

	genere_code_llvm(noeud->noeud_syntactique, contexte, false);
}

static void genere_fonction_vraie_principale(
		ContexteGenerationCode &contexte,
		ContexteGenerationLLVM &contexte_llvm)
{
	auto &constructrice = contexte_llvm.constructrice;

	// ----------------------------------

	auto builder = llvm::IRBuilder<>(contexte_llvm.contexte);

	// déclare une fonction de type int(int, char**) appelée main
	auto type_int = llvm::Type::getInt32Ty(contexte_llvm.contexte);
	auto type_argc = type_int;

	llvm::Type *type_argv = llvm::Type::getInt8Ty(contexte_llvm.contexte);
	type_argv = llvm::PointerType::get(type_argv, 0);
	type_argv = llvm::PointerType::get(type_argv, 0);

	std::vector<llvm::Type *> parametres;
	parametres.push_back(type_argc);
	parametres.push_back(type_argv);

	auto type_fonction = llvm::FunctionType::get(type_int, parametres, false);

	auto fonction = llvm::Function::Create(
				type_fonction,
				llvm::Function::ExternalLinkage,
				"vraie_principale",
				contexte_llvm.module_llvm);

	contexte_llvm.fonction = fonction;

	auto block = cree_bloc(contexte_llvm, "entree");
	contexte_llvm.bloc_courant(block);
	builder.SetInsertPoint(contexte_llvm.bloc_courant());

	// crée code pour les arguments

	auto alloc_argc = builder.CreateAlloca(type_int, 0u, nullptr, "argc");
	alloc_argc->setAlignment(4);

	auto alloc_argv = builder.CreateAlloca(type_argv, 0u, nullptr, "argv");
	alloc_argv->setAlignment(8);

	// construit un tableau de type []*z8
	auto valeurs_args = fonction->arg_begin();

	auto valeur = &(*valeurs_args++);
	valeur->setName("argc");

	auto store_argc = builder.CreateStore(valeur, alloc_argc);
	store_argc->setAlignment(4);

	valeur = &(*valeurs_args++);
	valeur->setName("argv");

	auto store_argv = builder.CreateStore(valeur, alloc_argv);
	store_argv->setAlignment(8);

	auto charge_argv = builder.CreateLoad(alloc_argv, "");
	charge_argv->setAlignment(8);

	auto charge_argc = builder.CreateLoad(alloc_argc, "");
	charge_argc->setAlignment(4);

	auto valeur_ARGV = contexte_llvm.valeur(contexte.table_identifiants.identifiant_pour_chaine("__ARGV"));
	auto valeur_ARGC = contexte_llvm.valeur(contexte.table_identifiants.identifiant_pour_chaine("__ARGC"));

	if (valeur_ARGV) {
		builder.CreateStore(charge_argv, valeur_ARGV);
	}

	if (valeur_ARGC) {
		builder.CreateStore(charge_argc, valeur_ARGC);
	}

	auto type_contexte = converti_type_llvm(contexte_llvm, contexte.type_contexte);

	auto alloc_contexte = builder.CreateAlloca(type_contexte, 0u, nullptr, "contexte");
	alloc_contexte->setAlignment(8);

	contexte_llvm.ajoute_locale(contexte_llvm.ident_contexte, alloc_contexte);

	// ----------------------------------
	// création du stockage temporaire
	auto ident_stock_temp = contexte.table_identifiants.identifiant_pour_chaine("STOCKAGE_TEMPORAIRE");
	auto type_tabl_stock_temp = contexte.typeuse[TypeBase::Z8];
	type_tabl_stock_temp = contexte.typeuse.type_tableau_fixe(type_tabl_stock_temp, 16384);
	auto tabl_stock_temp = constructrice.alloue_globale(ident_stock_temp, type_tabl_stock_temp, nullptr, false);

	auto type_stock_temp = contexte.typeuse.type_pour_nom("StockageTemporaire");

	auto alloc_stocke_temp = constructrice.alloue_variable(nullptr, type_stock_temp, nullptr);
//	auto ptr_stocke_temp = accede_element_tableau(contexte_llvm, alloc_stocke_temp, converti_type_llvm(contexte_llvm, type_tabl_stock_temp), 0ul);
	auto ptr_stocke_temp = accede_membre_structure(contexte_llvm, alloc_stocke_temp, 0ul);

	tabl_stock_temp = accede_element_tableau(contexte_llvm, tabl_stock_temp, converti_type_llvm(contexte_llvm, type_tabl_stock_temp), 0ul);
	builder.CreateStore(tabl_stock_temp, ptr_stocke_temp);

	auto ptr_taille_stocke_temp = accede_membre_structure(contexte_llvm, alloc_stocke_temp, 1ul);
	builder.CreateStore(builder.getInt32(16384), ptr_taille_stocke_temp);

	// ----------------------------------
	// création de l'allocatrice de base
	auto type_base_alloc = contexte.typeuse.type_pour_nom("BaseAllocatrice");
	auto alloc_base_alloc = constructrice.alloue_variable(nullptr, type_base_alloc, nullptr);

	// ----------------------------------
	// construit le contexte du programme

	// assigne l'allocatrice défaut
	auto fonc_alloc = cherche_fonction_dans_module(contexte, "Kuri", "allocatrice_défaut");
	auto ptr_fonc_alloc = contexte_llvm.module_llvm->getFunction(fonc_alloc->nom_broye.c_str());
	auto ptr_alloc = accede_membre_structure(contexte_llvm, alloc_contexte, 0u);
	builder.CreateStore(ptr_fonc_alloc, ptr_alloc);

	// assigne les données défaut comme étant nulles
	auto ptr_donnees = accede_membre_structure(contexte_llvm, alloc_contexte, 1u);
	builder.CreateStore(alloc_base_alloc, ptr_donnees);

	// assigne le stockage temporaire
	auto ptr_stocke = accede_membre_structure(contexte_llvm, alloc_contexte, 4u);
	builder.CreateStore(alloc_stocke_temp, ptr_stocke);

	// appel notre fonction principale en passant le contexte et le tableau
	auto fonc_princ = contexte_llvm.module_llvm->getFunction("principale");

	std::vector<llvm::Value *> parametres_appel;
	parametres_appel.push_back(builder.CreateLoad(alloc_contexte));

	llvm::ArrayRef<llvm::Value *> args(parametres_appel);

	auto valeur_princ = builder.CreateCall(fonc_princ, args);

	// return
	builder.CreateRet(valeur_princ);
}

void genere_code_llvm(
		ContexteGenerationCode &contexte,
		ContexteGenerationLLVM &contexte_llvm)
{
	contexte_llvm.ident_contexte = contexte.table_identifiants.identifiant_pour_chaine("contexte");
	contexte_llvm.type_contexte = contexte.type_contexte;

	auto &graphe_dependance = contexte.graphe_dependance;
	auto noeud_fonction_principale = graphe_dependance.cherche_noeud_fonction("principale");

	if (noeud_fonction_principale == nullptr) {
		erreur::fonction_principale_manquante();
	}

	auto temps_generation = dls::chrono::compte_seconde();

	/* il faut d'abord créer le code pour les structures InfoType */
	const char *noms_structs_infos_types[] = {
		"InfoType",
		"InfoTypeEntier",
		"InfoTypePointeur",
		"InfoTypeÉnum",
		"InfoTypeStructure",
		"InfoTypeTableau",
		"InfoTypeFonction",
		"InfoTypeMembreStructure",
	};

	for (auto nom_struct : noms_structs_infos_types) {
		auto type_struct = contexte.typeuse.type_pour_nom(nom_struct);
		auto noeud = graphe_dependance.cherche_noeud_type(type_struct);

		traverse_graphe_pour_generation_code(contexte_llvm, noeud);
	}

	/* génère ensuite les fonctions, en les prédéclarant, pour éviter les crash
	 * lorsque nous cherchons une fonction qui n'a pas encore été déclarée (p.e.
	 * dans le cas des fonctions mutuellement récursives) */
	for (auto noeud_dep : graphe_dependance.noeuds) {
		if (noeud_dep->type == TypeNoeudDependance::FONCTION) {
			auto decl = static_cast<NoeudDeclarationFonction *>(noeud_dep->noeud_syntactique);

			/* Crée le type de la fonction */
			auto type_fonction = obtiens_type_fonction(
						contexte_llvm,
						decl->type_fonc,
						decl->est_variadique,
						possede_drapeau(decl->drapeaux, EST_EXTERNE));

			decl->type_fonc->type_llvm = type_fonction;

			/* Crée fonction */
			llvm::Function::Create(
								type_fonction,
								llvm::Function::ExternalLinkage,
								decl->nom_broye.c_str(),
								contexte_llvm.module_llvm);
		}
	}

	traverse_graphe_pour_generation_code(contexte_llvm, noeud_fonction_principale);

	genere_fonction_vraie_principale(contexte, contexte_llvm);

	contexte.temps_generation = temps_generation.temps();
}

}  /* namespace noeud */
