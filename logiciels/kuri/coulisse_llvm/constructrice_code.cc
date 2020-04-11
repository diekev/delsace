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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "constructrice_code.hh"

#include "contexte_generation_code.h"
#include "typage.hh"

#include "contexte_generation_llvm.hh"
#include "conversion_type_llvm.hh"
#include "generation_code_llvm.hh"

ConstructriceCode::ConstructriceCode(ContexteGenerationLLVM &contexte_, llvm::LLVMContext &contexte_llvm)
	: contexte(contexte_)
	, builder(contexte_llvm)
{}

void ConstructriceCode::bloc_courant(llvm::BasicBlock *bloc)
{
	builder.SetInsertPoint(bloc);
}

llvm::Value *ConstructriceCode::alloue_param(IdentifiantCode *ident, Type *type, llvm::Value *valeur)
{
	auto type_llvm = converti_type_llvm(contexte, type);

	auto alloc = builder.CreateAlloca(type_llvm, 0u);
	alloc->setAlignment(type->alignement);

	auto store = builder.CreateStore(valeur, alloc);
	store->setAlignment(type->alignement);

	return alloc;
}

llvm::Value *ConstructriceCode::alloue_variable(IdentifiantCode *ident, Type *type, NoeudExpression *expression)
{
	auto type_llvm = converti_type_llvm(contexte, type);
	auto alloc = builder.CreateAlloca(type_llvm, 0u);
	alloc->setAlignment(type->alignement);

	if (expression != nullptr) {
		auto valeur = noeud::applique_transformation(contexte, expression, false);
		auto store = builder.CreateStore(valeur, alloc);
		store->setAlignment(type->alignement);
	}
	else {
		if (type->genre == GenreType::STRUCTURE) {
			auto alloc_contexte = contexte.valeur(contexte.ident_contexte);
			auto parametres = std::vector<llvm::Value *>();
			parametres.push_back(charge(alloc_contexte, contexte.type_contexte));
			parametres.push_back(alloc);

			auto nom_fonction_init = "initialise_" + dls::vers_chaine(type);
			auto fonction_init = contexte.module_llvm->getFunction(nom_fonction_init.c_str());

			llvm::CallInst::Create(fonction_init, parametres, "", contexte.bloc_courant());
		}
		else if (type->genre != GenreType::TABLEAU_FIXE) {
			auto valeur = cree_valeur_defaut_pour_type(type);
			auto store = builder.CreateStore(valeur, alloc);
			store->setAlignment(type->alignement);
		}
	}

	return alloc;
}

llvm::Value *ConstructriceCode::alloue_globale(IdentifiantCode *ident, Type *type, NoeudExpression *expression, bool est_externe)
{
	auto type_llvm = converti_type_llvm(contexte, type);

	auto vg = new llvm::GlobalVariable(
				*contexte.module_llvm,
				type_llvm,
				false,
				est_externe ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::InternalLinkage,
				nullptr,
				llvm::StringRef(ident->nom.pointeur(), static_cast<size_t>(ident->nom.taille())));

	vg->setAlignment(type->alignement);

	if (expression != nullptr) {
		/* À FAIRE: pour une variable globale, nous devons précaculer
		 * nous-même la valeur et passer le résultat à LLVM sous forme
		 * d'une constante */
		auto valeur = noeud::genere_code_llvm(expression, contexte, false);
		vg->setInitializer(llvm::dyn_cast<llvm::Constant>(valeur));
	}
	else {
		auto valeur = cree_valeur_defaut_pour_type(type);
		vg->setInitializer(llvm::dyn_cast<llvm::Constant>(valeur));
	}

	return vg;
}

llvm::Value *ConstructriceCode::cree_valeur_defaut_pour_type(Type *type)
{
	auto type_llvm = converti_type_llvm(contexte, type);

	switch (type->genre) {
		case GenreType::RIEN:
		case GenreType::VARIADIQUE:
		case GenreType::ENTIER_CONSTANT:
		case GenreType::INVALIDE:
		{
			break;
		}
		case GenreType::OCTET:
		case GenreType::ENTIER_NATUREL:
		{
			return llvm::ConstantInt::get(type_llvm, 0);
		}
		case GenreType::ENTIER_RELATIF:
		{
			return llvm::ConstantInt::get(type_llvm, 0, true);
		}
		case GenreType::BOOL:
		{
			return llvm::ConstantInt::get(type_llvm, 0);
		}
		case GenreType::REEL:
		{
			if (type->taille_octet == 2) {
				return llvm::ConstantInt::get(type_llvm, 0);
			}

			return llvm::ConstantFP::get(type_llvm, 0.0);
		}
		case GenreType::ENUM:
		case GenreType::ERREUR:
		{
			auto type_enum = static_cast<TypeEnum *>(type);
			return cree_valeur_defaut_pour_type(type_enum->type_donnees);
		}
		case GenreType::FONCTION:
		case GenreType::REFERENCE:
		case GenreType::POINTEUR:
		{
			return llvm::ConstantPointerNull::get(static_cast<llvm::PointerType *>(type_llvm));
		}
		case GenreType::CHAINE:
		case GenreType::EINI:
		case GenreType::TABLEAU_DYNAMIQUE:
		case GenreType::TABLEAU_FIXE:
		case GenreType::STRUCTURE:
		case GenreType::UNION:
		{
			return llvm::ConstantAggregateZero::get(type_llvm);
		}
	}

	return nullptr;
}

llvm::Value *ConstructriceCode::charge(llvm::Value *valeur, Type *type)
{
	auto inst = builder.CreateLoad(valeur);
	inst->setAlignment(type->alignement);
	return inst;
}

void ConstructriceCode::stocke(llvm::Value *valeur, llvm::Value *ptr)
{
	builder.CreateStore(valeur, ptr);
}

llvm::Constant *ConstructriceCode::cree_chaine(dls::vue_chaine_compacte const &chaine)
{
	return this->valeur_pour_chaine(chaine);
}

llvm::Value *ConstructriceCode::accede_membre_structure(llvm::Value *structure, unsigned index, bool expr_gauche)
{
	auto ptr = builder.CreateGEP(structure, {
						  llvm::ConstantInt::get(llvm::Type::getInt32Ty(contexte.contexte), 0),
						  llvm::ConstantInt::get(llvm::Type::getInt32Ty(contexte.contexte), index)
					  });

	if (!expr_gauche) {
		auto charge = builder.CreateLoad(ptr);
		// À FAIRE
		//charge->setAlignment(type_chaine->alignement);
		return charge;
	}

	return ptr;
}

llvm::Value *ConstructriceCode::nombre_entier(long valeur)
{
	return builder.getInt32(static_cast<unsigned>(valeur));
}

void ConstructriceCode::incremente(llvm::Value *valeur, Type * type)
{
	auto type_llvm = converti_type_llvm(contexte, type);

	if (type->genre == GenreType::ENTIER_NATUREL) {
		auto val_inc = llvm::ConstantInt::get(type_llvm, 1, false);
		auto inc = builder.CreateBinOp(llvm::Instruction::Add, builder.CreateLoad(valeur), val_inc);
		auto charge = builder.CreateStore(inc, valeur);
		charge->setAlignment(type->alignement);
	}

	if (type->genre == GenreType::ENTIER_RELATIF) {
		auto val_inc = llvm::ConstantInt::get(type_llvm, 1, true);
		auto inc = builder.CreateBinOp(llvm::Instruction::Add, builder.CreateLoad(valeur), val_inc);
		auto charge = builder.CreateStore(inc, valeur);
		charge->setAlignment(type->alignement);
	}

	// À FAIRE : r16
	if (type->genre == GenreType::REEL) {
		auto val_inc = llvm::ConstantFP::get(type_llvm, 1.0);
		auto inc = builder.CreateBinOp(llvm::Instruction::Add, builder.CreateLoad(valeur), val_inc);
		auto charge = builder.CreateStore(inc, valeur);
		charge->setAlignment(type->alignement);
	}
}

llvm::Constant *ConstructriceCode::valeur_pour_chaine(const dls::chaine &chaine)
{
	auto iter = valeurs_chaines_globales.trouve(chaine);

	if (iter != valeurs_chaines_globales.fin()) {
		return iter->second;
	}

	// @.chn [N x i8] c"...0"
	auto type_tableau = contexte.typeuse.type_tableau_fixe(contexte.typeuse[TypeBase::Z8], chaine.taille() + 1);

	auto constante = llvm::ConstantDataArray::getString(
				contexte.contexte,
				chaine.c_str());

	auto tableu_chaine_c = new llvm::GlobalVariable(
				*contexte.module_llvm,
				converti_type_llvm(contexte, type_tableau),
				true,
				llvm::GlobalValue::PrivateLinkage,
				constante,
				".chn");

	// prend le pointeur vers le premier élément.
	auto idx = std::vector<llvm::Constant *>{
			llvm::ConstantInt::get(llvm::Type::getInt32Ty(contexte.contexte), 0),
			llvm::ConstantInt::get(llvm::Type::getInt32Ty(contexte.contexte), 0)};

	auto pointeur_chaine_c = llvm::ConstantExpr::getGetElementPtr(
				converti_type_llvm(contexte, type_tableau),
				tableu_chaine_c,
				idx,
				true);

	auto valeur_taille = llvm::ConstantInt::get(
				llvm::Type::getInt64Ty(contexte.contexte),
				static_cast<uint64_t>(chaine.taille()),
				false);

	auto type_chaine = converti_type_llvm(contexte, contexte.typeuse[TypeBase::CHAINE]);

	auto struct_chaine = llvm::ConstantStruct::get(
				static_cast<llvm::StructType *>(type_chaine),
				pointeur_chaine_c,
				valeur_taille);

	valeurs_chaines_globales.insere({ chaine, struct_chaine });

	return struct_chaine;
}

llvm::Value *ConstructriceCode::appel_operateur(OperateurBinaire const *op, llvm::Value *valeur1, llvm::Value *valeur2)
{
	auto parametres = std::vector<llvm::Value *>(3);
	// À FAIRE : voir si le contexte est nécessaire
	auto valeur_contexte = contexte.valeur(contexte.ident_contexte);
	parametres[0] = new llvm::LoadInst(valeur_contexte, "", false, contexte.bloc_courant());
	parametres[1] = valeur1;
	parametres[2] = valeur2;

	auto fonction = contexte.module_llvm->getFunction(op->nom_fonction.c_str());
	return llvm::CallInst::Create(fonction, parametres, "", contexte.bloc_courant());
}

#if 0
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
#endif
