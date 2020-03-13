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
#include "biblinternes/langage/nombres.hh"
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
#include <llvm/IR/LegacyPassManager.h>
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

using denombreuse = lng::decoupeuse_nombre<GenreLexeme>;

#undef NOMME_IR

/* ************************************************************************** */

/* À FAIRE (coulisse LLVM)
 * - prend en compte la portée des blocs pour générer le code des noeuds différés
 * - coroutine, retiens
 * - erreur en cas de débordement des limites, où d'accès à un membre non-actif d'une union
 * - stockage temporaire
 * - ajourne les infos types pour avoir une taille en octet pour chaque struct (avec la taille en octet et non en bits !)
 * - ajourne les infos types pour séparer les tableaux fixes des dynamiques
 * - construction de PositionSourceCode
 * - BaseAllocatrice
 * - __ARGC, __ARGV
 * - ajournement pour les nouveau système de type, notamment TypeVariadique, et les types de sorties des fonctions
 * - ajournement pour la nouvelle architecture de l'arbre syntaxique
 */

/* ************************************************************************** */

namespace noeud {
#if 0
static llvm::Value *genere_code_llvm(
		base *b,
		ContexteGenerationCode &contexte,
		const bool expr_gauche);
}

static void genere_code_extra_pre_retour(ContexteGenerationCode &contexte)
{
	/* génère le code pour les blocs déférés */
	auto pile_noeud = contexte.noeuds_differes();

	while (!pile_noeud.est_vide()) {
		auto noeud = pile_noeud.back();
		genere_code_llvm(noeud, contexte, true);
		pile_noeud.pop_back();
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

namespace noeud {

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

static bool est_branche_ou_retour(llvm::Value *valeur)
{
	return (valeur != nullptr) && (llvm::isa<llvm::BranchInst>(*valeur) || llvm::isa<llvm::ReturnInst>(*valeur));
}

static llvm::FunctionType *obtiens_type_fonction(
		ContexteGenerationCode &contexte,
		DonneesFonction const &donnees_fonction,
		Type *type_retour,
		bool est_variadique,
		bool requiers_contexte)
{
	std::vector<llvm::Type *> parametres;
	parametres.reserve(static_cast<size_t>(donnees_fonction.args.taille()));

	if (requiers_contexte) {
		parametres.push_back(converti_type_llvm(contexte, contexte.type_contexte));
	}

	for (auto const &argument : donnees_fonction.args) {
		if (argument.est_variadic) {
			/* les arguments variadics sont transformés en un tableau */
			if (!donnees_fonction.est_externe) {
				auto type_tabl = contexte.typeuse.type_tableau_dynamique(argument.type);
				parametres.push_back(converti_type_llvm(contexte, type_tabl));
			}

			break;
		}

		parametres.push_back(converti_type_llvm(contexte, argument.type));
	}

	return llvm::FunctionType::get(
				converti_type_llvm(contexte, type_retour),
				parametres,
				est_variadique && donnees_fonction.est_externe);
}

enum {
	/* TABLEAUX */
	POINTEUR_TABLEAU = 0,
	TAILLE_TABLEAU = 1,

	/* EINI */
	POINTEUR_EINI = 0,
	TYPE_EINI = 1,

	/* CHAINE */
	POINTEUR_CHAINE = 0,
	TYPE_CHAINE = 1,
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

[[nodiscard]] static llvm::Value *accede_membre_union(
		ContexteGenerationCode &contexte,
		llvm::Value *structure,
		DonneesStructure const &donnees_structure,
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

	auto &dm = donnees_structure.donnees_membres.trouve(nom_membre)->second;
	auto type_membre = donnees_structure.types[dm.index_membre];
	auto type = converti_type_llvm(contexte, type_membre);

	auto ret = llvm::BitCastInst::Create(llvm::Instruction::BitCast, ptr, type->getPointerTo(), "", contexte.bloc_courant());

	if (charge == true) {
		return new llvm::LoadInst(ret, "", contexte.bloc_courant());
	}

	return ret;
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

[[nodiscard]] static llvm::Value *converti_vers_tableau_dyn(
		ContexteGenerationCode &contexte,
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
		ContexteGenerationCode &contexte,
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

	if (llvm::isa<llvm::Constant>(valeur) == false) {
		auto charge_valeur = new llvm::LoadInst(valeur, "", contexte.bloc_courant());
		valeur = charge_valeur->getPointerOperand();
	}
	else {
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

[[nodiscard]] static llvm::Value *applique_transformation(
		ContexteGenerationCode &contexte,
		NoeudBase *b,
		bool expr_gauche)
{
	auto valeur = static_cast<llvm::Value *>(nullptr);

	auto builder = llvm::IRBuilder<>(contexte.contexte);
	builder.SetInsertPoint(contexte.bloc_courant());

	switch (b->transformation.type) {
		default:
		case TypeTransformation::INUTILE:
		case TypeTransformation::PREND_PTR_RIEN:
		{
			valeur = genere_code_llvm(b, contexte, expr_gauche);
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
				valeur = alloc_const;
			}

			valeur = builder.CreateCast(inst, valeur, type_cible_llvm->getPointerTo());
			valeur = builder.CreateLoad(valeur, "");
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

					auto type_cible = converti_type_llvm(contexte, contexte.typeuse[TypeBase::PTR_OCTET]);
					valeur = builder.CreateCast(llvm::Instruction::BitCast, valeur, type_cible->getPointerTo());
					valeur_pointeur = valeur;

					auto taille_type = b->type->taille_octet;
					valeur_taille = builder.getInt64(taille_type);
					break;
				}
				case GenreType::POINTEUR:
				{
					auto type_pointe = static_cast<TypePointeur *>(b->type)->type_pointe;
					valeur = genere_code_llvm(b, contexte, true);
					valeur_pointeur = valeur;
					auto taille_type = type_pointe->taille_octet;
					valeur_taille = builder.getInt64(taille_type);
					break;
				}
				case GenreType::CHAINE:
				{
					auto valeur_chaine = genere_code_llvm(b, contexte, true);
					valeur_pointeur = accede_membre_structure(contexte, valeur_chaine, POINTEUR_TABLEAU);
					valeur_pointeur = builder.CreateLoad(valeur_pointeur);
					valeur_taille = accede_membre_structure(contexte, valeur_chaine, TAILLE_TABLEAU);
					valeur_taille = builder.CreateLoad(valeur_taille);
					break;
				}
				case GenreType::TABLEAU_DYNAMIQUE:
				{
					auto type_pointer = static_cast<TypeTableauDynamique *>(b->type)->type_pointe;

					auto valeur_tableau = genere_code_llvm(b, contexte, true);
					valeur_pointeur = accede_membre_structure(contexte, valeur_tableau, POINTEUR_TABLEAU);
					valeur_pointeur = builder.CreateLoad(valeur_pointeur);
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
			/* À FAIRE: appel fonction */
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
	}

	return valeur;
}

template <typename Conteneur>
llvm::Value *cree_appel(
		ContexteGenerationCode &contexte,
		llvm::Value *fonction,
		Conteneur const &conteneur,
		bool besoin_contexte)
{
	std::vector<llvm::Value *> parametres(static_cast<size_t>(conteneur.taille()));

	auto debut = parametres.begin();

	if (besoin_contexte) {
		auto valeur_contexte = contexte.valeur_locale("contexte");
		parametres.resize(parametres.size() + 1);

		parametres[0] = new llvm::LoadInst(valeur_contexte, "", false, contexte.bloc_courant());

		debut = parametres.begin() + 1;
	}

	std::transform(conteneur.debut(), conteneur.fin(), debut,
				   [&](base *noeud_enfant)
	{
		return applique_transformation(contexte, noeud_enfant, false);
	});

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

static llvm::Value *comparaison_pour_type(
		Type *type,
		llvm::PHINode *noeud_phi,
		llvm::Value *valeur_fin,
		llvm::BasicBlock *bloc_courant)
{
	if (type->genre == GenreType::ENTIER_NATUREL) {
		return llvm::ICmpInst::Create(
					llvm::Instruction::ICmp,
					llvm::CmpInst::Predicate::ICMP_ULE,
					noeud_phi,
					valeur_fin,
					"",
					bloc_courant);
	}

	if (type->genre == GenreType::ENTIER_RELATIF) {
		return llvm::ICmpInst::Create(
					llvm::Instruction::ICmp,
					llvm::CmpInst::Predicate::ICMP_SLE,
					noeud_phi,
					valeur_fin,
					"",
					bloc_courant);
	}

	if (type->genre == GenreType::REEL) {
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
		Type *type,
		ContexteGenerationCode &contexte,
		llvm::PHINode *noeud_phi,
		llvm::BasicBlock *bloc_courant)
{
	auto type_llvm = converti_type_llvm(contexte, type);

	if (type->genre == GenreType::ENTIER_RELATIF || type->genre == GenreType::ENTIER_NATUREL) {
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

	// À FAIRE : r16
	if (type->genre == GenreType::REEL) {
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

/* ************************************************************************** */

static auto genere_code_allocation(
		ContexteGenerationCode &contexte,
		Type *type,
		int mode,
		NoeudBase *b,
		NoeudBase *variable,
		NoeudBase *expression,
		NoeudBase *bloc_sinon)
{
	auto builder = llvm::IRBuilder<>(contexte.contexte);
	builder.SetInsertPoint(contexte.bloc_courant());

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
				val_nouvel_nombre_element = val_expr;

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
			val_acces_pointeur = accede_membre_structure(contexte, val_enfant, 0u);
			val_acces_taille = accede_membre_structure(contexte, val_enfant, 1u);
			val_ancienne_taille_octet = builder.CreateLoad(val_acces_taille, "");

			/* allocation ou réallocation */
			if (expression != nullptr) {
				auto val_expr = genere_code_llvm(expression, contexte, false);
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

	auto ptr_contexte = contexte.valeur_locale("contexte");
	ptr_contexte = builder.CreateLoad(ptr_contexte);

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
	auto arg_val_mode = builder.getInt32(static_cast<unsigned>(mode));
	auto arg_val_ancien_ptr = builder.CreateLoad(val_acces_pointeur, "");

	std::vector<llvm::Value *> parametres;
	parametres.push_back(arg_val_mode);
	parametres.push_back(val_nouvelle_taille_octet);
	parametres.push_back(val_ancienne_taille_octet);
	parametres.push_back(arg_val_ancien_ptr);
	parametres.push_back(arg_ptr_donnees);
	parametres.push_back(arg_ptr_info_type);

	auto ret_pointeur = builder.CreateCall(arg_ptr_allocatrice, parametres);

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

	builder.CreateStore(ret_pointeur, val_acces_pointeur);

	if (val_acces_taille != nullptr) {
		builder.CreateStore(val_nouvel_nombre_element, val_acces_taille);
	}

	return builder.CreateLoad(val_enfant, "");
}

/* ************************************************************************** */

static llvm::Value *genere_code_llvm(
		NoeudBase *b,
		ContexteGenerationCode &contexte,
		const bool expr_gauche)
{
	switch (b->genre) {
		case GenreNoeud::DIRECTIVE_EXECUTION:
		case GenreNoeud::DECLARATION_ENUM:
		case GenreNoeud::DECLARATION_PARAMETRES_FONCTION:
		case GenreNoeud::INSTRUCTION_PAIRE_DISCR:
		case GenreNoeud::RACINE:
		case GenreNoeud::INSTRUCTION_RETOUR:
		case GenreNoeud::INSTRUCTION_SINON:
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
		case GenreNoeud::DECLARATION_FONCTION:
		{
			auto fichier = contexte.fichier(static_cast<size_t>(b->lexeme.fichier));
			auto module = fichier->module;
			auto &vdf = module->donnees_fonction(b->lexeme.chaine);
			auto donnees_fonction = static_cast<DonneesFonction *>(nullptr);

			for (auto &df : vdf) {
				if (df.noeud_decl == b) {
					donnees_fonction = &df;
				}
			}

			auto requiers_contexte = !donnees_fonction->est_externe && !possede_drapeau(b->drapeaux, FORCE_NULCTX);

			auto type_fonc = static_cast<TypeFonction *>(b->type);

			/* Crée le type de la fonction */
			auto type_fonction = obtiens_type_fonction(
						contexte,
						*donnees_fonction,
						type_fonc->types_sorties[0],
						donnees_fonction->est_variadique,
						requiers_contexte);

			donnees_fonction->type->type_llvm = type_fonction;

			auto const est_externe = possede_drapeau(b->drapeaux, EST_EXTERNE);

			/* Crée fonction */
			auto fonction = llvm::Function::Create(
								type_fonction,
								llvm::Function::ExternalLinkage,
								donnees_fonction->nom_broye.c_str(),
								contexte.module_llvm);

			if (est_externe) {
				return fonction;
			}

			contexte.commence_fonction(fonction, donnees_fonction);

			auto block = cree_bloc(contexte, "entree");

			contexte.bloc_courant(block);

			/* Crée code pour les arguments */
			auto valeurs_args = fonction->arg_begin();

			if (requiers_contexte) {
				auto valeur = &(*valeurs_args++);
				valeur->setName("contexte");

				auto type = converti_type_llvm(contexte, contexte.type_contexte);

				auto alloc = new llvm::AllocaInst(
								 type,
								 0,
								 "contexte",
								 contexte.bloc_courant());
				alloc->setAlignment(8);

				auto store = new llvm::StoreInst(valeur, alloc, false, contexte.bloc_courant());
				store->setAlignment(8);

				auto donnees_var = DonneesVariable{};
				donnees_var.valeur = alloc;

				contexte.pousse_locale("contexte", donnees_var);
			}

			for (auto const &argument : donnees_fonction->args) {
				auto align = unsigned{0};
				auto type = static_cast<llvm::Type *>(nullptr);

				if (argument.est_variadic) {
					align = 8;

					auto idx_tabl = contexte.typeuse.type_tableau_dynamique(argument.type);

					type = converti_type_llvm(contexte, idx_tabl);
				}
				else {
					align = argument.type->alignement;
					type = converti_type_llvm(contexte, argument.type);
				}

		#ifdef NOMME_IR
				auto const &nom_argument = argument.chaine;
		#else
				auto const &nom_argument = "";
		#endif

				auto valeur = &(*valeurs_args++);
				valeur->setName(nom_argument);

				auto alloc = new llvm::AllocaInst(
								 type,
								 0,
								 nom_argument,
								 contexte.bloc_courant());

				alloc->setAlignment(align);
				auto store = new llvm::StoreInst(valeur, alloc, false, contexte.bloc_courant());
				store->setAlignment(align);

				auto donnees_var = DonneesVariable{};
				donnees_var.valeur = alloc;

				contexte.pousse_locale(argument.nom, donnees_var);
			}

			/* Crée code pour le bloc. */
			auto bloc = b->enfants.back();
			bloc->valeur_calculee = static_cast<llvm::BasicBlock *>(nullptr);
			auto ret = genere_code_llvm(bloc, contexte, true);

			/* Ajoute une instruction de retour si la dernière n'en est pas une. */
			if (ret == nullptr || !llvm::isa<llvm::ReturnInst>(*ret)) {
				genere_code_extra_pre_retour(contexte);

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
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			auto est_pointeur_fonction = contexte.locale_existe(b->lexeme.chaine);

			if (est_pointeur_fonction) {
				auto valeur = contexte.valeur_locale(b->lexeme.chaine);

				auto charge = new llvm::LoadInst(valeur, "", false, contexte.bloc_courant());
				/* À FAIRE : alignement pointeur. */
				charge->setAlignment(8);

				/* À FAIRE(contexte) */
				return cree_appel(contexte, charge, b->enfants, true);
			}

			auto df = b->df;
			auto fonction = contexte.module_llvm->getFunction(b->nom_fonction_appel.c_str());
			return cree_appel(contexte, fonction, b->enfants, !df->est_externe);
		}
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		{
			auto refexpr = static_cast<NoeudExpressionReference *>(b);
			auto decl = trouve_dans_bloc(refexpr->bloc_parent, refexpr->ident);

			auto valeur = contexte.valeur_locale(b->lexeme.chaine);

			if (valeur == nullptr) {
				valeur = contexte.valeur_globale(b->lexeme.chaine);

				if (valeur == nullptr && b->nom_fonction_appel != "") {
					valeur = contexte.module_llvm->getFunction(b->nom_fonction_appel.c_str());
					return valeur;
				}
			}

			if (expr_gauche || llvm::dyn_cast<llvm::PHINode>(valeur)) {
				return valeur;
			}

			auto charge = new llvm::LoadInst(valeur, "", false, contexte.bloc_courant());
			charge->setAlignment(b->type->alignement);

			return charge;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		{
			auto expr = static_cast<NoeudOperationBinaire *>(b);
			auto structure = expr->expr1;
			auto membre = expr->expr2;

			auto type_structure = structure->type;

			auto est_pointeur = type_structure->genre == GenreType::POINTEUR;

			while (type_structure->genre == GenreType::POINTEUR || type_structure->genre == GenreType::REFERENCE) {
				type_structure = contexte.typeuse.type_dereference_pour(type_structure);
			}

			if (type_structure->genre == GenreType::EINI) {
				auto valeur = genere_code_llvm(structure, contexte, true);

				if (est_pointeur) {
					valeur = new llvm::LoadInst(valeur, "", contexte.bloc_courant());
				}

				if (membre->chaine() == "pointeur") {
					return accede_membre_structure(contexte, valeur, POINTEUR_EINI, true);
				}

				return accede_membre_structure(contexte, valeur, TYPE_EINI, true);
			}

			if (type_structure->genre == GenreType::CHAINE) {
				auto valeur = genere_code_llvm(structure, contexte, true);

				if (est_pointeur) {
					valeur = new llvm::LoadInst(valeur, "", contexte.bloc_courant());
				}

				if (membre->chaine() == "pointeur") {
					return accede_membre_structure(contexte, valeur, POINTEUR_CHAINE, true);
				}

				return accede_membre_structure(contexte, valeur, TYPE_CHAINE, true);
			}

			if (type_structure->genre == GenreType::TABLEAU_FIXE) {
				auto taille = static_cast<TypeTableauFixe *>(type_structure)->taille;

				return llvm::ConstantInt::get(
							converti_type_llvm(contexte, b->type),
							static_cast<unsigned long>(taille));
			}

			if (type_structure->genre == GenreType::TABLEAU_DYNAMIQUE) {
				/* charge taille de la structure tableau { *type, taille } */
				auto valeur = genere_code_llvm(structure, contexte, true);

				if (est_pointeur) {
					/* déréférence le pointeur en le chargeant */
					auto charge = new llvm::LoadInst(valeur, "", contexte.bloc_courant());
					charge->setAlignment(8);
					valeur = charge;
				}

				if (membre->chaine() == "pointeur") {
					return accede_membre_structure(contexte, valeur, POINTEUR_TABLEAU, !expr_gauche);
				}

				return accede_membre_structure(contexte, valeur, TAILLE_TABLEAU, !expr_gauche);
			}

			auto const &nom_membre = membre->chaine();

			if (type_structure->genre == GenreType::ENUM) {
				auto type_enum = static_cast<TypeEnum *>(type_structure);
				auto &donnees_structure = contexte.donnees_structure(type_enum->nom);
				auto builder = llvm::IRBuilder<>(contexte.contexte);
				builder.SetInsertPoint(contexte.bloc_courant());
				return valeur_enum(donnees_structure, nom_membre, builder);
			}

			auto type_struct = static_cast<TypeStructure *>(type_structure);

			auto &donnees_structure = contexte.donnees_structure(type_struct->nom);

			auto valeur = genere_code_llvm(structure, contexte, true);

			auto const iter = donnees_structure.donnees_membres.trouve(nom_membre);

			auto const &donnees_membres = iter->second;
			auto const index_membre = donnees_membres.index_membre;

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
				auto type_membre = donnees_structure.types[index_membre];
				charge->setAlignment(type_membre->alignement);
				ret = charge;
			}

			return ret;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			auto expr = static_cast<NoeudOperationBinaire *>(b);
			auto structure = expr->expr1;
			auto membre = expr->expr2;
			auto const &nom_membre = membre->chaine();

			auto type_structure = static_cast<TypeStructure *>(structure->type);
			auto &donnees_structure = contexte.donnees_structure(type_structure->nom);

			auto valeur = genere_code_llvm(structure, contexte, true);

			if (expr_gauche) {
				auto &dm = donnees_structure.donnees_membres.trouve(nom_membre)->second;
				auto pointeur_membre_actif = accede_membre_structure(contexte, valeur, 1);
				auto valeur_active = llvm::ConstantInt::get(
							llvm::Type::getInt32Ty(contexte.contexte),
							static_cast<unsigned>(dm.index_membre + 1));

				new llvm::StoreInst(valeur_active, pointeur_membre_actif, contexte.bloc_courant());
			}

			return accede_membre_union(contexte, valeur, donnees_structure, nom_membre, !expr_gauche);
		}
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		{
			auto expr = static_cast<NoeudOperationBinaire *>(b);
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
			auto expr = static_cast<NoeudOperationBinaire *>(b);
			auto variable = expr->expr1;
			auto expression = expr->expr2;

			auto type = variable->type;
			auto type_llvm = converti_type_llvm(contexte, type);

			if (contexte.fonction == nullptr) {
				auto est_externe = possede_drapeau(variable->drapeaux, EST_EXTERNE);

				auto vg = new llvm::GlobalVariable(
							*contexte.module_llvm,
							type_llvm,
							false,
							est_externe ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::InternalLinkage,
							nullptr);

				vg->setAlignment(type->alignement);

				auto donnees_var = DonneesVariable{};
				donnees_var.valeur = vg;

				contexte.pousse_globale(variable->chaine(), donnees_var);

				if (expression != nullptr) {
					/* À FAIRE: pour une variable globale, nous devons précaculer
					 * nous-même la valeur et passer le résultat à LLVM sous forme
					 * d'une constante */
					auto valeur = genere_code_llvm(expression, contexte, false);
					vg->setInitializer(llvm::dyn_cast<llvm::Constant>(valeur));
				}

				return vg;
			}

			auto builder = llvm::IRBuilder<>(contexte.contexte);
			builder.SetInsertPoint(contexte.bloc_courant());

			auto alloc = builder.CreateAlloca(type_llvm, 0u);

			if (expression != nullptr) {
				auto valeur = applique_transformation(contexte, expression, false);
				builder.CreateStore(valeur, alloc);
			}

			/* À FAIRE: si pas d'expression, initialise la variable */

			auto donnees_var = DonneesVariable{};
			donnees_var.valeur = alloc;

			contexte.pousse_locale(variable->chaine(), donnees_var);

			return alloc;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		{
			auto const est_calcule = possede_drapeau(b->drapeaux, EST_CALCULE);
			auto const valeur = est_calcule ? std::any_cast<double>(b->valeur_calculee) :
												denombreuse::converti_chaine_nombre_reel(
													b->lexeme.chaine,
													b->lexeme.genre);

			return llvm::ConstantFP::get(
						llvm::Type::getDoubleTy(contexte.contexte),
						valeur);
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		{
			auto const est_calcule = possede_drapeau(b->drapeaux, EST_CALCULE);
			auto const valeur = est_calcule ? std::any_cast<long>(b->valeur_calculee) :
												denombreuse::converti_chaine_nombre_entier(
													b->lexeme.chaine,
													b->lexeme.genre);

			return llvm::ConstantInt::get(
						llvm::Type::getInt32Ty(contexte.contexte),
						static_cast<uint64_t>(valeur),
						false);
		}
		case GenreNoeud::OPERATEUR_BINAIRE:
		{
			auto expr = static_cast<NoeudOperationBinaire *>(b);
			auto enfant1 = expr->expr1;
			auto enfant2 = expr->expr2;
			auto op = expr->op;

			if ((b->drapeaux & EST_ASSIGNATION_OPEREE) != 0) {
				auto ptr_valeur1 = genere_code_llvm(enfant1, contexte, true);
				auto valeur1 = new llvm::LoadInst(ptr_valeur1, "", false, contexte.bloc_courant());
				auto valeur2 = applique_transformation(contexte, enfant2, false);

				if (op->est_basique) {
					auto val_resultat = llvm::BinaryOperator::Create(op->instr_llvm, valeur1, valeur2, "", contexte.bloc_courant());
					new llvm::StoreInst(val_resultat, ptr_valeur1, contexte.bloc_courant());
					return nullptr;
				}

				// À FAIRE: appel fonction (attendre la surcharge d'opérateur)
				return nullptr;
			}

			auto valeur1 = applique_transformation(contexte, enfant1, false);
			auto valeur2 = applique_transformation(contexte, enfant2, false);

			if (op->est_basique) {
				if (op->est_comp_entier) {
					return llvm::ICmpInst::Create(llvm::Instruction::ICmp, op->predicat_llvm, valeur1, valeur2, "", contexte.bloc_courant());
				}

				if (op->est_comp_reel) {
					return llvm::FCmpInst::Create(llvm::Instruction::FCmp, op->predicat_llvm, valeur1, valeur2, "", contexte.bloc_courant());
				}

				return llvm::BinaryOperator::Create(op->instr_llvm, valeur1, valeur2, "", contexte.bloc_courant());
			}

			auto parametres = std::vector<llvm::Value *>(3);
			// À FAIRE : voir si le contexte est nécessaire
			auto valeur_contexte = contexte.valeur_locale("contexte");
			parametres[0] = new llvm::LoadInst(valeur_contexte, "", false, contexte.bloc_courant());
			parametres[1] = valeur1;
			parametres[2] = valeur2;

			auto fonction = contexte.module_llvm->getFunction(op->nom_fonction.c_str());
			return llvm::CallInst::Create(fonction, parametres, "", contexte.bloc_courant());
		}
		case GenreNoeud::OPERATEUR_UNAIRE:
		{
			auto expr = static_cast<NoeudOperationUnaire *>(b);
			auto enfant = expr->expr;
			auto op = expr->op;

			llvm::Instruction::BinaryOps instr;
			auto enfant = b->enfants.front();
			auto type1 = enfant->type;
			auto valeur1 = genere_code_llvm(enfant, contexte, false);
			auto valeur2 = static_cast<llvm::Value *>(nullptr);

			switch (b->lexeme.genre) {
				case GenreLexeme::EXCLAMATION:
				{
					valeur2 = llvm::ConstantInt::get(
								  llvm::Type::getInt1Ty(contexte.contexte),
								  static_cast<uint64_t>(0),
								  false);

					valeur1 = llvm::ICmpInst::Create(llvm::Instruction::ICmp, llvm::CmpInst::Predicate::ICMP_EQ, valeur1, valeur2, "", contexte.bloc_courant());

					instr = llvm::Instruction::Xor;
					valeur2 = llvm::ConstantInt::get(
								  llvm::Type::getInt32Ty(contexte.contexte),
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
					auto inst_load = llvm::dyn_cast<llvm::LoadInst>(valeur1);

					if (inst_load == nullptr) {
						/* Ne devrais pas arriver. */
						return nullptr;
					}

					return inst_load->getPointerOperand();
				}
				case GenreLexeme::PLUS_UNAIRE:
				{
					return valeur1;
				}
				case GenreLexeme::MOINS_UNAIRE:
				{
					valeur2 = valeur1;

					if (est_type_entier(type1)) {
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
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		{
			auto expr = static_cast<NoeudOperationBinaire *>(b);
			auto enfant1 = expr->expr1;
			auto enfant2 = expr->expr2;
			auto op = expr->op;

			auto a_comp_b = genere_code_llvm(enfant1, contexte, expr_gauche);
			auto val_c = genere_code_llvm(enfant2, contexte, expr_gauche);
			auto val_b = genere_code_llvm(enfant1->enfants.back(), contexte, expr_gauche);

			auto op = b->op;

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
				// À FAIRE: appel fonction
				return nullptr;
			}

			return llvm::BinaryOperator::Create(llvm::BinaryOperator::BinaryOps::And, a_comp_b, b_comp_c, "", contexte.bloc_courant());
		}
		case GenreNoeud::EXPRESSION_INDICE:
		{
			auto expr = static_cast<NoeudOperationBinaire *>(b);
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
			llvm::Value *valeur = nullptr;

			if (!b->enfants.est_vide()) {
				assert(b->enfants.taille() == 1);
				valeur = genere_code_llvm(b->enfants.front(), contexte, false);
			}

			/* NOTE : le code différé doit être crée après l'expression de retour, car
			 * nous risquerions par exemple de déloger une variable utilisée dans
			 * l'expression de retour. */
			genere_code_extra_pre_retour(contexte);

			return llvm::ReturnInst::Create(contexte.contexte, valeur, contexte.bloc_courant());
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		{
			auto builder = llvm::IRBuilder<>(contexte.contexte);
			builder.SetInsertPoint(contexte.bloc_courant());

			auto chaine = std::any_cast<dls::chaine>(b->valeur_calculee);

			auto pointeur_chaine_c = builder.CreateGlobalStringPtr(chaine.c_str());

			/* crée la constante pour la taille */
			auto valeur_taille = builder.getInt64(static_cast<uint64_t>(chaine.taille()));

			/* crée la chaine */
			auto type = converti_type_llvm(contexte, b->type);

			auto alloc = builder.CreateAlloca(type, 0u);
			alloc->setAlignment(8);

			/* assigne le pointeur et la taille */
			auto membre_pointeur = accede_membre_structure(contexte, alloc, 0);
			auto charge = builder.CreateStore(pointeur_chaine_c, membre_pointeur);
			charge->setAlignment(8);

			auto membre_taille = accede_membre_structure(contexte, alloc, 1);
			charge = builder.CreateStore(valeur_taille, membre_taille);
			charge->setAlignment(8);

			if (!expr_gauche) {
				return builder.CreateLoad(alloc, "");
			}

			return alloc;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		{
			auto const est_calcule = possede_drapeau(b->drapeaux, EST_CALCULE);
			auto const valeur = est_calcule ? std::any_cast<bool>(b->valeur_calculee)
											  : (b->chaine() == "vrai");
			return llvm::ConstantInt::get(
						llvm::Type::getInt1Ty(contexte.contexte),
						static_cast<uint64_t>(valeur),
						false);
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		{
			auto valeur = dls::caractere_echappe(&b->lexeme.chaine[0]);

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
							  llvm::Type::getInt32Ty(contexte.contexte),
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
			auto ret = genere_code_llvm(enfant2, contexte, false);

			/* noeud 3 : sinon (optionel) */
			if (inst->bloc_si_faux) {
				contexte.bloc_courant(bloc_sinon);

				auto enfant3 = inst->bloc_si_faux;
				enfant3->valeur_calculee = bloc_fusion;
				ret = genere_code_llvm(enfant3, contexte, false);
			}

			contexte.bloc_courant(bloc_fusion);

			return ret;
		}
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			llvm::Value *valeur = nullptr;

			auto bloc_entree = contexte.bloc_courant();

			for (auto enfant : b->enfants) {
				valeur = genere_code_llvm(enfant, contexte, true);

				/* nul besoin de continuer à générer du code pour des expressions qui ne
				 * seront jamais executées. */
				if (est_branche_ou_retour(valeur) && bloc_entree == contexte.bloc_courant()) {
					break;
				}
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
			/* boucle:
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

			contexte.empile_bloc_continue(enfant1->chaine(), bloc_inc);
			contexte.empile_bloc_arrete(enfant1->chaine(), (bloc_sinon != nullptr) ? bloc_sinon : bloc_apres);

			auto bloc_pre = contexte.bloc_courant();

			auto noeud_phi = static_cast<llvm::PHINode *>(nullptr);

			/* bloc_boucle */
			/* on crée une branche explicite dans le bloc */
			llvm::BranchInst::Create(bloc_boucle, contexte.bloc_courant());

			contexte.bloc_courant(bloc_boucle);

			auto pointeur_tableau = static_cast<llvm::Value *>(nullptr);

			auto builder = llvm::IRBuilder<>(contexte.contexte);
			builder.SetInsertPoint(contexte.bloc_courant());
			auto ret = static_cast<llvm::Value *>(nullptr);

			switch (b->aide_generation_code) {
				case GENERE_BOUCLE_PLAGE:
				case GENERE_BOUCLE_PLAGE_INDEX:
				{
					auto var = enfant1;
					auto idx = static_cast<noeud::base *>(nullptr);
					auto valeur_idx = static_cast<llvm::PHINode *>(nullptr);

					if (enfant1->lexeme.genre == GenreLexeme::VIRGULE) {
						var = enfant1->enfants.front();
						idx = enfant1->enfants.back();
					}

					if (idx != nullptr) {
						valeur_idx = builder.CreatePHI(builder.getInt32Ty(), 2, "");
						auto init_zero = builder.getInt32(0);
						valeur_idx->addIncoming(init_zero, bloc_pre);
					}

					/* création du bloc de condition */

					noeud_phi = builder.CreatePHI(converti_type_llvm(contexte, type), 2, dls::chaine(var->chaine()).c_str());

					genere_code_llvm(enfant2, contexte, false);

					auto valeur_debut = contexte.valeur_locale("__debut");
					auto valeur_fin = contexte.valeur_locale("__fin");

					noeud_phi->addIncoming(valeur_debut, bloc_pre);

					auto condition = comparaison_pour_type(
										 type,
										 noeud_phi,
										 valeur_fin,
										 contexte.bloc_courant());

					builder.CreateCondBr(condition, bloc_corps, (bloc_sansarret != nullptr) ? bloc_sansarret : bloc_apres);

					/* création du bloc de corps */

					contexte.bloc_courant(bloc_corps);
					builder.SetInsertPoint(contexte.bloc_courant());

					enfant3->valeur_calculee = bloc_inc;
					ret = genere_code_llvm(enfant3, contexte, false);

					/* bloc_inc */
					contexte.bloc_courant(bloc_inc);
					builder.SetInsertPoint(contexte.bloc_courant());

					auto inc = incremente_pour_type(
								type,
								contexte,
								noeud_phi,
								contexte.bloc_courant());

					noeud_phi->addIncoming(inc, contexte.bloc_courant());

					if (valeur_idx) {
						inc = incremente_pour_type(
									type,
									contexte,
									valeur_idx,
									contexte.bloc_courant());

						valeur_idx->addIncoming(inc, contexte.bloc_courant());
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

					noeud_phi = llvm::PHINode::Create(
									llvm::Type::getInt64Ty(contexte.contexte),
									2,
									dls::chaine(enfant1->chaine()).c_str(),
									contexte.bloc_courant());


					auto valeur_debut = llvm::ConstantInt::get(
											llvm::Type::getInt64Ty(contexte.contexte),
											static_cast<uint64_t>(0),
											false);

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

					contexte.bloc_courant(bloc_corps);

					auto valeur_arg = static_cast<llvm::Value *>(nullptr);

					if (taille_tableau != 0) {
						auto valeur_tableau = genere_code_llvm(enfant2, contexte, true);

						valeur_arg = accede_element_tableau(
									 contexte,
									 valeur_tableau,
									 converti_type_llvm(contexte, type),
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

					/* création du bloc de corps */
					builder.SetInsertPoint(contexte.bloc_courant());

					enfant3->valeur_calculee = bloc_inc;
					ret = genere_code_llvm(enfant3, contexte, false);

					/* bloc_inc */
					contexte.bloc_courant(bloc_inc);
					builder.SetInsertPoint(contexte.bloc_courant());

					auto inc = incremente_pour_type(
								   contexte.typeuse[TypeBase::N64],
								   contexte,
								   noeud_phi,
								   contexte.bloc_courant());

					noeud_phi->addIncoming(inc, contexte.bloc_courant());

					break;
				}
				case GENERE_BOUCLE_COROUTINE:
				case GENERE_BOUCLE_COROUTINE_INDEX:
				{
					/* À FAIRE(coroutine) */
					break;
				}
			}

			ret = llvm::BranchInst::Create(bloc_boucle, contexte.bloc_courant());

			/* 'continue'/'arrête' dans les blocs 'sinon'/'sansarrêt' n'a aucun sens */
			contexte.depile_bloc_continue();
			contexte.depile_bloc_arrete();

			if (enfant_sans_arret) {
				contexte.bloc_courant(bloc_sansarret);
				enfant_sans_arret->valeur_calculee = bloc_apres;
				ret = genere_code_llvm(enfant_sans_arret, contexte, false);
			}

			if (enfant_sinon) {
				contexte.bloc_courant(bloc_sinon);
				enfant_sinon->valeur_calculee = bloc_apres;
				ret = genere_code_llvm(enfant_sinon, contexte, false);
			}

			contexte.bloc_courant(bloc_apres);

			return ret;
		}
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		{
			auto chaine_var = b->enfants.est_vide() ? dls::vue_chaine_compacte{""} : b->enfants.front()->chaine();

			auto bloc = (b->lexeme.genre == GenreLexeme::CONTINUE)
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

			enfant->valeur_calculee = bloc_boucle;
			auto ret = genere_code_llvm(inst->bloc, contexte, false);

			contexte.depile_bloc_continue();
			contexte.depile_bloc_arrete();
			contexte.bloc_courant(bloc_apres);

			return ret;
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

			enfant->valeur_calculee = bloc_tantque;
			auto ret = genere_code_llvm(inst->bloc, contexte, false);

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

			return ret;
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

			enfant_bloc->valeur_calculee = bloc_tantque_cond;
			genere_code_llvm(inst->bloc, contexte, false);

			contexte.depile_bloc_continue();
			contexte.depile_bloc_arrete();
			contexte.bloc_courant(bloc_apres);
			return nullptr;
		}
		case GenreNoeud::EXPRESSION_TRANSTYPE:
		{
			auto enfant = b->enfants.front();
			auto valeur = genere_code_llvm(enfant, contexte, false);
			auto type_de = enfant->type;

			if (type_de == b->type) {
				return valeur;
			}

			using CastOps = llvm::Instruction::CastOps;

			auto type_llvm = converti_type_llvm(contexte, b->type);
			auto bloc = contexte.bloc_courant();
			auto type_vers = b->type;

			if (est_type_entier(type_de)) {
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

					if (type_de->genre == GenreType::ENTIER_NATUREL) {
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
			return llvm::ConstantInt::get(
						llvm::Type::getInt32Ty(contexte.contexte),
						static_cast<uint64_t>(0),
						false);
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
			auto iter = b->enfants.debut();

			auto enfant1 = *iter++;
			auto enfant2 = *iter++;

			auto valeur_debut = genere_code_llvm(enfant1, contexte, false);
			auto valeur_fin = genere_code_llvm(enfant2, contexte, false);

			auto donnees_var = DonneesVariable{};
			donnees_var.valeur = valeur_debut;

			contexte.pousse_locale("__debut", donnees_var);

			donnees_var.valeur = valeur_fin;
			contexte.pousse_locale("__fin", donnees_var);

			return valeur_fin;
		}
		case GenreNoeud::INSTRUCTION_DIFFERE:
		{
			auto noeud = b->enfants.front();

			/* La valeur_calculee d'un bloc est son bloc suivant, qui dans le cas d'un
			 * bloc déféré n'en est aucun. */
			noeud->valeur_calculee = static_cast<llvm::BasicBlock *>(nullptr);

			contexte.differe_noeud(noeud);

			return nullptr;
		}
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		{
			auto taille_tableau = b->enfants.taille();

			auto const est_calcule = possede_drapeau(b->drapeaux, EST_CALCULE);
			if (est_calcule) {
				assert(taille_tableau == std::any_cast<long>(b->valeur_calculee));
			}

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

			for (auto enfant : b->enfants) {
				auto valeur_enfant = applique_transformation(contexte, enfant, false);

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
			dls::tableau<base *> feuilles;
			rassemble_feuilles(b->enfants.front(), feuilles);

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
			auto liste_params = std::any_cast<dls::tableau<dls::vue_chaine_compacte>>(&b->valeur_calculee);

			auto type_struct = static_cast<TypeStructure *>(b->type);
			auto &donnees_structure = contexte.donnees_structure(type_struct->nom);

			auto type_struct_llvm = converti_type_llvm(contexte, b->type);

			auto builder = llvm::IRBuilder<>(contexte.contexte);
			builder.SetInsertPoint(contexte.bloc_courant());

			auto alloc = builder.CreateAlloca(type_struct_llvm, 0u);

			auto enfant = b->enfants.debut();
			auto nom_param = liste_params->debut();

			for (auto i = 0l; i < liste_params->taille(); ++i) {
				auto &dm = donnees_structure.donnees_membres.trouve(*nom_param)->second;
				auto idx_membre = dm.index_membre;

				auto ptr = accede_membre_structure(contexte, alloc, static_cast<size_t>(idx_membre));

				auto val = genere_code_llvm(*enfant, contexte, false);

				builder.CreateStore(val, ptr);

				++enfant;
				++nom_param;
			}

			return builder.CreateLoad(alloc, "");
		}
		case GenreNoeud::EXPRESSION_INFO_DE:
		{
			auto enfant = b->enfants.front();
			return cree_info_type(contexte, enfant->type);
		}
		case GenreNoeud::EXPRESSION_MEMOIRE:
		{
			auto expr = static_cast<NoeudOperationUnaire *>(b);
			auto valeur = genere_code_llvm(expr->expr, contexte, expr_gauche);
			return new llvm::LoadInst(valeur, "", contexte.bloc_courant());
		}
		case GenreNoeud::EXPRESSION_LOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(b);
			return genere_code_allocation(contexte, dt, 0, b->type, expr->expression, expr->expression_chaine, expr->bloc);
		}
		case GenreNoeud::EXPRESSION_DELOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(b);
			return genere_code_allocation(contexte, dt, 2, b->type, expr->expression, expr->expression_chaine, expr->bloc);
		}
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(b);
			return genere_code_allocation(contexte, dt, 1, b->type, expr->expression, expr->expression_chaine, expr->bloc);
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			auto donnees_structure = contexte.donnees_structure(b->chaine());
			converti_type_llvm(contexte, donnees_structure.type);
			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		case GenreNoeud::INSTRUCTION_DISCR:
		{
			/* le premier enfant est l'expression, les suivants les paires */

			auto nombre_enfants = b->enfants.taille();
			auto iter_enfant = b->enfants.debut();
			auto expression = *iter_enfant++;
			auto op = b->op;

			if (nombre_enfants <= 1) {
				return nullptr;
			}

			auto ds = static_cast<DonneesStructure *>(nullptr);

			if (b->genre == GenreNoeud::INSTRUCTION_DISCR_ENUM) {
				auto type_enum = static_cast<TypeEnum *>(expression->type);
				ds = &contexte.donnees_structure(type_enum->nom);
			}
			else if (b->genre == GenreNoeud::INSTRUCTION_DISCR_UNION) {
				auto type_enum = static_cast<TypeStructure *>(expression->type);
				ds = &contexte.donnees_structure(type_enum->nom);
			}

			struct DonneesPaireDiscr {
				base *paire = nullptr;
				llvm::BasicBlock *bloc_de_la_condition = nullptr;
				llvm::BasicBlock *bloc_si_vrai = nullptr;
				llvm::BasicBlock *bloc_si_faux = nullptr;
			};

			auto bloc_post_discr = cree_bloc(contexte, "post_discr");

			dls::tableau<DonneesPaireDiscr> donnees_paires;
			donnees_paires.reserve(nombre_enfants);

			for (auto i = 1l; i < nombre_enfants; ++i) {
				auto paire = *iter_enfant++;
				auto enf0 = paire->enfants.front();

				auto donnees = DonneesPaireDiscr();
				donnees.paire = paire;
				donnees.bloc_de_la_condition = cree_bloc(contexte, "bloc_de_la_condition");

				if (enf0->genre != GenreNoeud::INSTRUCTION_SINON) {
					donnees.bloc_si_vrai = cree_bloc(contexte, "bloc_si_vrai");
				}

				if (!donnees_paires.est_vide()) {
					donnees_paires.back().bloc_si_faux = donnees.bloc_de_la_condition;
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
				auto paire = donnees.paire;
				auto enf0 = paire->enfants.front();
				auto enf1 = paire->enfants.back();

				contexte.bloc_courant(donnees.bloc_de_la_condition);
				builder.SetInsertPoint(contexte.bloc_courant());

				if (enf0->genre != GenreNoeud::INSTRUCTION_SINON) {
					auto feuilles = dls::tableau<base *>();
					rassemble_feuilles(enf0, feuilles);

					auto valeurs_conditions = dls::tableau<llvm::Value *>();
					valeurs_conditions.reserve(feuilles.taille());

					for (auto f : feuilles) {
						auto valeur_f = static_cast<llvm::Value *>(nullptr);

						if (b->genre == GenreNoeud::INSTRUCTION_DISCR_ENUM) {
							valeur_f = valeur_enum(*ds, f->chaine(), builder);
						}
						else if (b->genre == GenreNoeud::INSTRUCTION_DISCR_UNION) {
							auto iter_dm = ds->donnees_membres.trouve(f->chaine());
							auto const &dm = iter_dm->second;
							valeur_f = builder.getInt32(static_cast<unsigned>(dm.index_membre + 1));

							auto dv = DonneesVariable{};
							dv.valeur = accede_membre_union(contexte, ptr_structure, *ds, f->chaine());

							contexte.pousse_locale(f->chaine(), dv);
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

							valeurs_conditions.pousse(condition);
						}
						else {
							auto fonction = contexte.module_llvm->getFunction(op->nom_fonction.c_str());

							std::vector<llvm::Value *> parametres(3);

							// À FAIRE : voir si le contexte est nécessaire
							auto valeur_contexte = contexte.valeur_locale("contexte");
							parametres[0] = new llvm::LoadInst(valeur_contexte, "", false, contexte.bloc_courant());
							parametres[1] = valeur_expression;
							parametres[2] = valeur_f;

							auto valeur_res = builder.CreateCall(fonction, parametres);

							auto condition = llvm::ICmpInst::Create(
										llvm::Instruction::ICmp,
										llvm::CmpInst::Predicate::ICMP_EQ,
										valeur_res,
										builder.getInt1(true),
										"",
										contexte.bloc_courant());

							valeurs_conditions.pousse(condition);
						}
					}

					// construit la condition finale
					auto condition = valeurs_conditions.front();

					for (auto i = 1; i < valeurs_conditions.taille(); ++i) {
						condition = llvm::BinaryOperator::Create(
									llvm::Instruction::Or,
									condition,
									valeurs_conditions[i],
									"",
									contexte.bloc_courant());
					}

					builder.CreateCondBr(condition, donnees.bloc_si_vrai, donnees.bloc_si_faux);
				}

				if (donnees.bloc_si_vrai) {
					contexte.bloc_courant(donnees.bloc_si_vrai);
				}
				else {
					contexte.bloc_courant(donnees.bloc_de_la_condition);
				}

				enf1->valeur_calculee = bloc_post_discr;
				genere_code_llvm(enf1, contexte, false);
			}

			contexte.bloc_courant(bloc_post_discr);

			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			// À FAIRE
			return nullptr;
		}
		case GenreNoeud::EXPANSION_VARIADIQUE:
		{
			// À FAIRE
			return nullptr;
		}
	}

	return nullptr;
}
static void traverse_graphe_pour_generation_code(
		ContexteGenerationCode &contexte,
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

static void genere_fonction_vraie_principale(ContexteGenerationCode &contexte)
{
	auto builder = llvm::IRBuilder<>(contexte.contexte);

	// déclare une fonction de type int(int, char**) appelée main
	auto type_int = llvm::Type::getInt32Ty(contexte.contexte);
	auto type_argc = type_int;

	llvm::Type *type_argv = llvm::Type::getInt8Ty(contexte.contexte);
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
				contexte.module_llvm);

	contexte.fonction = fonction;

	auto block = cree_bloc(contexte, "entree");
	contexte.bloc_courant(block);
	builder.SetInsertPoint(contexte.bloc_courant());

	// crée code pour les arguments

	auto alloc_argc = builder.CreateAlloca(type_int, 0u, nullptr, "argc");
	alloc_argc->setAlignment(4);

	auto alloc_argv = builder.CreateAlloca(type_argv, 0u, nullptr, "argv");
	alloc_argv->setAlignment(8);

	// construit un tableau de type []*z8
	auto type_tabl = contexte.typeuse[TypeBase::PTR_Z8];
	type_tabl = contexte.typeuse.type_tableau_dynamique(type_tabl);

	auto type_tabl_llvm = converti_type_llvm(contexte, type_tabl);
	auto alloc_tabl = builder.CreateAlloca(type_tabl_llvm, 0u, nullptr, "tabl_args");
	alloc_tabl->setAlignment(8);

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
	auto membre_pointeur = accede_membre_structure(contexte, alloc_tabl, 0);
	store_argv = builder.CreateStore(charge_argv, membre_pointeur);
	store_argv->setAlignment(8);

	auto charge_argc = builder.CreateLoad(alloc_argc, "");
	charge_argc->setAlignment(4);
	auto sext = builder.CreateSExt(charge_argc, builder.getInt64Ty());
	auto membre_taille = accede_membre_structure(contexte, alloc_tabl, 1);
	store_argc = builder.CreateStore(sext, membre_taille);
	store_argc->setAlignment(4);

	// construit le contexte du programme
	auto type_contexte = converti_type_llvm(contexte, contexte.type_contexte);

	auto alloc_contexte = builder.CreateAlloca(type_contexte, 0u, nullptr, "contexte");
	alloc_contexte->setAlignment(8);

	// assigne l'allocatrice défaut
	auto &df_fonc_alloc = contexte.module("Kuri")->donnees_fonction("allocatrice_défaut").front();
	auto ptr_fonc_alloc = contexte.module_llvm->getFunction(df_fonc_alloc.nom_broye.c_str());
	auto ptr_alloc = accede_membre_structure(contexte, alloc_contexte, 0u);
	builder.CreateStore(ptr_fonc_alloc, ptr_alloc);

	// assigne les données défaut comme étant nulles
	auto ptr_donnees = accede_membre_structure(contexte, alloc_contexte, 1u);
	builder.CreateStore(builder.getInt64(0), ptr_donnees);

	// appel notre fonction principale en passant le contexte et le tableau
	auto fonc_princ = contexte.module_llvm->getFunction("principale");

	std::vector<llvm::Value *> parametres_appel;
	parametres_appel.push_back(builder.CreateLoad(alloc_contexte));

	auto charge_tabl = builder.CreateLoad(alloc_tabl, "");
	charge_tabl->setAlignment(8);

	parametres_appel.push_back(charge_tabl);

	llvm::ArrayRef<llvm::Value *> args(parametres_appel);

	auto valeur_princ = builder.CreateCall(fonc_princ, args);

	// return
	builder.CreateRet(valeur_princ);
}
#endif

void genere_code_llvm(
		assembleuse_arbre const &arbre,
		ContexteGenerationCode &contexte)
{
#if 0
	auto racine = arbre.racine();

	if (racine == nullptr) {
		return;
	}

	if (racine->genre != GenreNoeud::RACINE) {
		return;
	}

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
		auto const &ds = contexte.donnees_structure(nom_struct);
		auto noeud = graphe_dependance.cherche_noeud_type(ds.type);

		traverse_graphe_pour_generation_code(contexte, noeud);
	}

	traverse_graphe_pour_generation_code(contexte, noeud_fonction_principale);

	genere_fonction_vraie_principale(contexte);

	contexte.temps_generation = temps_generation.temps();
#endif
}

}  /* namespace noeud */
