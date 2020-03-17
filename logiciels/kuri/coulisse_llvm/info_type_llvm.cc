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

#include "info_type_llvm.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#include <llvm/IR/Constants.h>
#include <llvm/IR/TypeBuilder.h>
#include <llvm/IR/GlobalVariable.h>
#pragma GCC diagnostic pop

#include "contexte_generation_llvm.hh"
#include "conversion_type_llvm.hh"

#include "arbre_syntactic.h"
#include "typage.hh"

/* À tenir synchronisé avec l'énum dans info_type.kuri
 * Nous utilisons ceci lors de la génération du code des infos types car nous ne
 * générons pas de code (ou symboles) pour les énums, mais prenons directements
 * leurs valeurs.
 */
struct IDInfoType {
	static constexpr int ENTIER    = 0;
	static constexpr int REEL      = 1;
	static constexpr int BOOLEEN   = 2;
	static constexpr int CHAINE    = 3;
	static constexpr int POINTEUR  = 4;
	static constexpr int STRUCTURE = 5;
	static constexpr int FONCTION  = 6;
	static constexpr int TABLEAU   = 7;
	static constexpr int EINI      = 8;
	static constexpr int RIEN      = 9;
	static constexpr int ENUM      = 10;
	static constexpr int OCTET     = 11;
};

[[nodiscard]] static auto obtiens_type_pour(
		ContexteGenerationLLVM &contexte,
		dls::vue_chaine_compacte const &nom_struct)
{
	auto type_struct_info = contexte.typeuse.type_pour_nom(nom_struct);
	return converti_type_llvm(contexte, type_struct_info);
}

[[nodiscard]] static auto cree_info_type_reel(
		ContexteGenerationLLVM &contexte,
		uint64_t taille_octet)
{
	auto type_llvm = obtiens_type_pour(contexte, "InfoType");

	/* { id : e32, nombre_bits : e32 } */

	auto valeur_id = llvm::ConstantInt::get(
				llvm::Type::getInt32Ty(contexte.contexte),
				IDInfoType::REEL,
				false);

	auto valeur_taille_octet = llvm::ConstantInt::get(
				llvm::Type::getInt32Ty(contexte.contexte),
				taille_octet,
				false);

	auto constant = llvm::ConstantStruct::get(
				static_cast<llvm::StructType *>(type_llvm),
				valeur_id, valeur_taille_octet);

	/* Création d'un InfoTypeEntier globale. */
	auto globale =  new llvm::GlobalVariable(
				*contexte.module_llvm,
				type_llvm,
				true,
				llvm::GlobalValue::InternalLinkage,
				constant);

	globale->setConstant(true);

	return globale;
}

[[nodiscard]] static auto cree_info_type_entier(
		ContexteGenerationLLVM &contexte,
		uint64_t taille_octet,
		bool est_signe)
{
	auto type_llvm = obtiens_type_pour(contexte, "InfoTypeEntier");

	/* { id : e32, est_signé : bool, nombre_bits : e32 } */

	auto valeur_id = llvm::ConstantInt::get(
				llvm::Type::getInt32Ty(contexte.contexte),
				IDInfoType::ENTIER,
				false);

	auto valeur_signe = llvm::ConstantInt::get(
				llvm::Type::getInt1Ty(contexte.contexte),
				est_signe,
				false);

	auto valeur_taille_octet = llvm::ConstantInt::get(
				llvm::Type::getInt32Ty(contexte.contexte),
				taille_octet,
				false);

	auto constant = llvm::ConstantStruct::get(
				static_cast<llvm::StructType *>(type_llvm),
				valeur_id, valeur_taille_octet, valeur_signe);

	/* Création d'un InfoType globale. */
	auto globale =  new llvm::GlobalVariable(
				*contexte.module_llvm,
				type_llvm,
				true,
				llvm::GlobalValue::InternalLinkage,
				constant);

	globale->setConstant(true);

	return globale;
}

[[nodiscard]] static auto cree_info_type_defaut(
		ContexteGenerationLLVM &contexte,
		int id_info_type,
		unsigned taille_octet)
{
	auto type_llvm = obtiens_type_pour(contexte, "InfoType");

	/* { id : e32 } */

	auto valeur_id = llvm::ConstantInt::get(
				llvm::Type::getInt32Ty(contexte.contexte),
				static_cast<unsigned>(id_info_type),
				false);

	auto valeur_taille_octet = llvm::ConstantInt::get(
				llvm::Type::getInt32Ty(contexte.contexte),
				taille_octet,
				false);

	auto constant = llvm::ConstantStruct::get(
				static_cast<llvm::StructType *>(type_llvm),
				valeur_id, valeur_taille_octet);

	/* Création d'un InfoType globale. */
	auto globale =  new llvm::GlobalVariable(
				*contexte.module_llvm,
				type_llvm,
				true,
				llvm::GlobalValue::InternalLinkage,
				constant);

	globale->setConstant(true);

	return globale;
}

[[nodiscard]] static auto cree_tableau_global(
		ContexteGenerationLLVM &contexte,
		std::vector<llvm::Constant *> const &valeurs_enum_tmp,
		Type *type_membre)
{
	auto type_valeur_llvm = converti_type_llvm(contexte, type_membre);
	auto type_tableau_valeur = contexte.typeuse.type_tableau_dynamique(type_membre);

	auto type_tableau_valeur_llvm = converti_type_llvm(contexte, type_tableau_valeur);

	// @.tabl [N x T] [...]
	auto type_tableau = llvm::ArrayType::get(type_valeur_llvm, valeurs_enum_tmp.size());

	auto init_tableau_constant = llvm::ConstantArray::get(type_tableau, valeurs_enum_tmp);

	auto tableau_constant = new llvm::GlobalVariable(
				*contexte.module_llvm,
				type_tableau,
				true,
				llvm::GlobalValue::PrivateLinkage,
				init_tableau_constant,
				".tabl");

	// prend le pointeur vers le premier élément.
	auto idx = std::vector<llvm::Constant *>{
			llvm::ConstantInt::get(llvm::Type::getInt32Ty(contexte.contexte), 0),
			llvm::ConstantInt::get(llvm::Type::getInt32Ty(contexte.contexte), 0)};

	auto pointeur_tableau = llvm::ConstantExpr::getGetElementPtr(
				type_tableau,
				tableau_constant,
				idx,
				true);

	auto valeur_taille = llvm::ConstantInt::get(
				llvm::Type::getInt64Ty(contexte.contexte),
				valeurs_enum_tmp.size(),
				false);

	auto struct_tableau = llvm::ConstantStruct::get(
				static_cast<llvm::StructType *>(type_tableau_valeur_llvm),
				pointeur_tableau,
				valeur_taille);

	return struct_tableau;
}

[[nodiscard]] static auto cree_info_type_enum(
		ContexteGenerationLLVM &contexte,
		NoeudEnum *noeud_decl,
		unsigned taille_octet)
{
	auto builder = llvm::IRBuilder<>(contexte.contexte);

	auto type_llvm = obtiens_type_pour(contexte, "InfoTypeÉnum");

	/* { id: e32, taille_en_octet, nom: chaine, valeurs: [], membres: [], est_drapeau: bool } */

	auto valeur_id = llvm::ConstantInt::get(
				llvm::Type::getInt32Ty(contexte.contexte),
				IDInfoType::ENUM,
				false);

	auto valeur_taille_octet = llvm::ConstantInt::get(
				llvm::Type::getInt32Ty(contexte.contexte),
				taille_octet,
				false);

	auto est_drapeau = llvm::ConstantInt::get(
				llvm::Type::getInt1Ty(contexte.contexte),
				noeud_decl->desc.est_drapeau,
				false);

	auto struct_chaine = contexte.constructrice.cree_chaine(noeud_decl->lexeme->chaine);

	/* création des tableaux de valeurs et de noms */

	std::vector<llvm::Constant *> valeurs_enum;
	valeurs_enum.reserve(static_cast<size_t>(noeud_decl->desc.valeurs.taille));

	POUR (noeud_decl->desc.valeurs) {
		auto valeur = llvm::ConstantInt::get(
					llvm::Type::getInt32Ty(contexte.contexte),
					static_cast<uint64_t>(it),
					false);

		valeurs_enum.push_back(valeur);
	}

	std::vector<llvm::Constant *> noms_enum;
	noms_enum.reserve(static_cast<size_t>(noeud_decl->desc.noms.taille));

	POUR (noeud_decl->desc.noms) {
		auto chaine_nom = contexte.constructrice.cree_chaine(dls::chaine(it.pointeur, it.taille));
		noms_enum.push_back(chaine_nom);
	}

	auto tableau_valeurs = cree_tableau_global(contexte, valeurs_enum, contexte.typeuse[TypeBase::Z32]);
	auto tableau_noms = cree_tableau_global(contexte, noms_enum, contexte.typeuse[TypeBase::CHAINE]);

	/* création de l'info type */

	auto constant = llvm::ConstantStruct::get(
				static_cast<llvm::StructType *>(type_llvm),
				valeur_id, valeur_taille_octet, struct_chaine, tableau_valeurs, tableau_noms, est_drapeau);

	auto globale =  new llvm::GlobalVariable(
				*contexte.module_llvm,
				type_llvm,
				true,
				llvm::GlobalValue::InternalLinkage,
				constant);

	globale->setConstant(true);

	return globale;
}

llvm::Value *cree_info_type(ContexteGenerationLLVM &contexte, Type *type)
{
	if (type->info_type_llvm != nullptr) {
		return type->info_type_llvm;
	}

	switch (type->genre) {
		case GenreType::INVALIDE:
		{
			assert(false);
			break;
		}
		case GenreType::BOOL:
		{
			type->info_type_llvm = cree_info_type_defaut(contexte, IDInfoType::BOOLEEN, type->taille_octet);
			break;
		}
		case GenreType::OCTET:
		{
			type->info_type_llvm = cree_info_type_defaut(contexte, IDInfoType::OCTET, type->taille_octet);
			break;
		}
		case GenreType::ENTIER_CONSTANT:
		{
			type->info_type_llvm = cree_info_type_entier(contexte, 4, true);
			break;
		}
		case GenreType::ENTIER_NATUREL:
		{
			type->info_type_llvm = cree_info_type_entier(contexte, type->taille_octet, false);
			break;
		}
		case GenreType::ENTIER_RELATIF:
		{
			type->info_type_llvm = cree_info_type_entier(contexte, type->taille_octet, true);
			break;
		}
		case GenreType::REEL:
		{
			type->info_type_llvm = cree_info_type_reel(contexte, type->taille_octet);
			break;
		}
		case GenreType::REFERENCE:
		case GenreType::POINTEUR:
		{
			/* { id, taille_en_octet type_pointé, est_référence } */

			auto type_pointeur = obtiens_type_pour(contexte, "InfoTypePointeur");

			auto valeur_id = llvm::ConstantInt::get(
						llvm::Type::getInt32Ty(contexte.contexte),
						IDInfoType::POINTEUR,
						false);

			auto valeur_taille_octet = llvm::ConstantInt::get(
						llvm::Type::getInt32Ty(contexte.contexte),
						type->taille_octet,
						false);

			auto type_deref = contexte.typeuse.type_dereference_pour(type);

			auto valeur_type_pointe = cree_info_type(contexte, type_deref);

			valeur_type_pointe = llvm::ConstantExpr::getCast(
						llvm::CastInst::Instruction::BitCast,
						static_cast<llvm::Constant *>(valeur_type_pointe),
						obtiens_type_pour(contexte, "InfoType")->getPointerTo());

			auto est_reference = llvm::ConstantInt::get(
						llvm::Type::getInt1Ty(contexte.contexte),
						type->genre == GenreType::REFERENCE,
						false);

			auto constant = llvm::ConstantStruct::get(
						static_cast<llvm::StructType *>(type_pointeur),
						valeur_id, valeur_taille_octet, static_cast<llvm::Constant *>(valeur_type_pointe), est_reference);

			auto globale =  new llvm::GlobalVariable(
						*contexte.module_llvm,
						type_pointeur,
						true,
						llvm::GlobalValue::InternalLinkage,
						constant);

			globale->setConstant(true);

			type->info_type_llvm = globale;
			break;
		}
		case GenreType::ENUM:
		case GenreType::ERREUR:
		{
			auto type_enum = static_cast<TypeEnum *>(type);
			type->info_type_llvm = cree_info_type_enum(contexte, type_enum->decl, type->taille_octet);
			break;
		}
		case GenreType::UNION:
		case GenreType::STRUCTURE:
		{
			auto type_struct = static_cast<TypeStructure *>(type);
			auto decl_struct = type_struct->decl;

			// ------------------------------------
			// Commence par assigner une globale non-initialisée comme info type
			// pour éviter de recréer plusieurs fois le même info type.
			auto type_llvm = obtiens_type_pour(contexte, "InfoTypeStructure");

			auto globale = new llvm::GlobalVariable(
						*contexte.module_llvm,
						type_llvm,
						true,
						llvm::GlobalValue::InternalLinkage,
						nullptr);

			type->info_type_llvm = globale;

			// ------------------------------------
			/* pour chaque membre cree une instance de InfoTypeMembreStructure */
			auto type_struct_membre = obtiens_type_pour(contexte, "InfoTypeMembreStructure");

			std::vector<llvm::Constant *> valeurs_membres;

			POUR (decl_struct->desc.membres) {
				/* { nom: chaine, info : *InfoType, décalage } */
				auto type_membre = it.type;

				auto info_type = cree_info_type(contexte, type_membre);

				info_type = llvm::ConstantExpr::getCast(
							llvm::CastInst::Instruction::BitCast,
							static_cast<llvm::Constant *>(info_type),
							obtiens_type_pour(contexte, "InfoType")->getPointerTo());

				auto valeur_nom = contexte.constructrice.cree_chaine(dls::chaine(it.nom.pointeur, it.nom.taille));

				auto valeur_decalage = llvm::ConstantInt::get(
							llvm::Type::getInt64Ty(contexte.contexte),
							static_cast<uint64_t>(it.decalage),
							false);

				auto constant = llvm::ConstantStruct::get(
							static_cast<llvm::StructType *>(type_struct_membre),
							valeur_nom, static_cast<llvm::Constant *>(info_type), valeur_decalage);

				/* Création d'un InfoType globale. */
				auto globale_membre = new llvm::GlobalVariable(
							*contexte.module_llvm,
							type_struct_membre,
							true,
							llvm::GlobalValue::InternalLinkage,
							constant);

				valeurs_membres.push_back(globale_membre);
			}

			/* { id : n32, taille_en_octet, nom: chaine, membres : []InfoTypeMembreStructure } */

			auto valeur_id = llvm::ConstantInt::get(
						llvm::Type::getInt32Ty(contexte.contexte),
						IDInfoType::STRUCTURE,
						false);

			auto valeur_taille_octet = llvm::ConstantInt::get(
						llvm::Type::getInt32Ty(contexte.contexte),
						type->taille_octet,
						false);

			auto valeur_nom = contexte.constructrice.cree_chaine(decl_struct->lexeme->chaine);

			// Pour les références à des globales, nous devons avoir un type pointeur.
			auto type_membre = contexte.typeuse.type_pour_nom("InfoTypeMembreStructure");
			type_membre = contexte.typeuse.type_pointeur_pour(type_membre);

			auto tableau_membre = cree_tableau_global(contexte, valeurs_membres, type_membre);

			auto constant = llvm::ConstantStruct::get(
						static_cast<llvm::StructType *>(type_llvm),
						valeur_id, valeur_taille_octet, valeur_nom, tableau_membre);

			globale->setInitializer(constant);

			break;
		}
		case GenreType::TABLEAU_DYNAMIQUE:
		case GenreType::VARIADIQUE:
		{
			/* { id, taille_en_octet, type_pointé, est_tableau_fixe, taille_fixe } */

			auto type_pointeur = obtiens_type_pour(contexte, "InfoTypeTableau");

			auto valeur_id = llvm::ConstantInt::get(
						llvm::Type::getInt32Ty(contexte.contexte),
						IDInfoType::TABLEAU,
						false);

			auto valeur_taille_octet = llvm::ConstantInt::get(
						llvm::Type::getInt32Ty(contexte.contexte),
						type->taille_octet,
						false);

			auto type_deref = contexte.typeuse.type_dereference_pour(type);

			auto valeur_type_pointe = cree_info_type(contexte, type_deref);

			valeur_type_pointe = llvm::ConstantExpr::getCast(
						llvm::CastInst::Instruction::BitCast,
						static_cast<llvm::Constant *>(valeur_type_pointe),
						obtiens_type_pour(contexte, "InfoType")->getPointerTo());

			auto valeur_est_fixe = llvm::ConstantInt::get(
						llvm::Type::getInt1Ty(contexte.contexte),
						false,
						false);

			auto valeur_taille_fixe = llvm::ConstantInt::get(
						llvm::Type::getInt32Ty(contexte.contexte),
						0,
						false);

			auto constant = llvm::ConstantStruct::get(
						static_cast<llvm::StructType *>(type_pointeur),
						valeur_id, valeur_taille_octet, static_cast<llvm::Constant *>(valeur_type_pointe), valeur_est_fixe, valeur_taille_fixe);

			auto globale =  new llvm::GlobalVariable(
						*contexte.module_llvm,
						type_pointeur,
						true,
						llvm::GlobalValue::InternalLinkage,
						constant);

			globale->setConstant(true);

			type->info_type_llvm = globale;
			break;
		}
		case GenreType::TABLEAU_FIXE:
		{
			auto type_tableau = static_cast<TypeTableauFixe *>(type);
			/* { id, taille_en_octet, type_pointé, est_tableau_fixe, taille_fixe } */

			auto type_pointeur = obtiens_type_pour(contexte, "InfoTypeTableau");

			auto valeur_id = llvm::ConstantInt::get(
						llvm::Type::getInt32Ty(contexte.contexte),
						IDInfoType::TABLEAU,
						false);

			auto valeur_taille_octet = llvm::ConstantInt::get(
						llvm::Type::getInt32Ty(contexte.contexte),
						type->taille_octet,
						false);

			auto valeur_type_pointe = cree_info_type(contexte, type_tableau->type_pointe);

			valeur_type_pointe = llvm::ConstantExpr::getCast(
						llvm::CastInst::Instruction::BitCast,
						static_cast<llvm::Constant *>(valeur_type_pointe),
						obtiens_type_pour(contexte, "InfoType")->getPointerTo());

			auto valeur_est_fixe = llvm::ConstantInt::get(
						llvm::Type::getInt1Ty(contexte.contexte),
						true,
						false);

			auto valeur_taille_fixe = llvm::ConstantInt::get(
						llvm::Type::getInt32Ty(contexte.contexte),
						static_cast<size_t>(type_tableau->taille),
						false);

			auto constant = llvm::ConstantStruct::get(
						static_cast<llvm::StructType *>(type_pointeur),
						valeur_id, valeur_taille_octet, static_cast<llvm::Constant *>(valeur_type_pointe), valeur_est_fixe, valeur_taille_fixe);

			auto globale =  new llvm::GlobalVariable(
						*contexte.module_llvm,
						type_pointeur,
						true,
						llvm::GlobalValue::InternalLinkage,
						constant);

			globale->setConstant(true);

			type->info_type_llvm = globale;
			break;
		}
		case GenreType::FONCTION:
		{
			type->info_type_llvm = cree_info_type_defaut(contexte, IDInfoType::FONCTION, type->taille_octet);
			break;
		}
		case GenreType::EINI:
		{
			type->info_type_llvm = cree_info_type_defaut(contexte, IDInfoType::EINI, type->taille_octet);
			break;
		}
		case GenreType::RIEN:
		{
			type->info_type_llvm = cree_info_type_defaut(contexte, IDInfoType::RIEN, type->taille_octet);
			break;
		}
		case GenreType::CHAINE:
		{
			type->info_type_llvm = cree_info_type_defaut(contexte, IDInfoType::CHAINE, type->taille_octet);
			break;
		}
	}

	return type->info_type_llvm;
}

llvm::Value *valeur_enum(
		NoeudEnum *noeud_decl,
		dls::vue_chaine_compacte const &nom,
		llvm::IRBuilder<> &builder)
{
	for (auto i = 0; i < noeud_decl->desc.noms.taille; ++i) {
		if (noeud_decl->desc.noms[i] == nom) {
			return builder.getInt32(static_cast<unsigned>(noeud_decl->desc.valeurs[i]));
		}
	}

	return nullptr;
}
