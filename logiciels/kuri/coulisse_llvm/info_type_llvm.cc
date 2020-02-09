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

#include "contexte_generation_code.h"
#include "conversion_type_llvm.hh"
#include "donnees_type.h"

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
};

[[nodiscard]] static auto obtiens_type_pour(
		ContexteGenerationCode &contexte,
		dls::vue_chaine_compacte const &nom_struct)
{
	auto index_struct_info = contexte.donnees_structure(nom_struct).id;

	auto dt_info = DonneesTypeFinal{};
	dt_info.pousse(TypeLexeme::CHAINE_CARACTERE | (static_cast<int>(index_struct_info << 8)));

	index_struct_info = contexte.typeuse.ajoute_type(dt_info);
	auto &ref_dt_info = contexte.typeuse[index_struct_info];

	return converti_type_llvm(contexte, ref_dt_info);
}

[[nodiscard]] static auto cree_info_type_reel(
		ContexteGenerationCode &contexte,
		uint64_t nombre_bits)
{
	auto type_llvm = obtiens_type_pour(contexte, "InfoTypeRéel");

	/* { id : e32, nombre_bits : e32 } */

	auto valeur_id = llvm::ConstantInt::get(
				llvm::Type::getInt32Ty(contexte.contexte),
				IDInfoType::REEL,
				false);

	auto valeur_nbits = llvm::ConstantInt::get(
						   llvm::Type::getInt32Ty(contexte.contexte),
						   nombre_bits,
						   false);

	auto constant = llvm::ConstantStruct::get(
						static_cast<llvm::StructType *>(type_llvm),
						valeur_id, valeur_nbits);

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
		ContexteGenerationCode &contexte,
		uint64_t nombre_bits,
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

	auto valeur_nbits = llvm::ConstantInt::get(
						   llvm::Type::getInt32Ty(contexte.contexte),
						   nombre_bits,
						   false);

	auto constant = llvm::ConstantStruct::get(
						static_cast<llvm::StructType *>(type_llvm),
						valeur_id, valeur_signe, valeur_nbits);

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
		ContexteGenerationCode &contexte,
		int id_info_type)
{
	auto type_llvm = obtiens_type_pour(contexte, "InfoType");

	/* { id : e32 } */

	auto valeur_id = llvm::ConstantInt::get(
				llvm::Type::getInt32Ty(contexte.contexte),
				static_cast<unsigned>(id_info_type),
				false);

	auto constant = llvm::ConstantStruct::get(
						static_cast<llvm::StructType *>(type_llvm),
						valeur_id);

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

[[nodiscard]] static auto cree_chaine_globale(
		ContexteGenerationCode &contexte,
		dls::chaine const &chaine)
{
	auto constante = llvm::ConstantDataArray::getString(
						 contexte.contexte,
						chaine.c_str());

	auto pointeur_chaine_c = new llvm::GlobalVariable(
					   *contexte.module_llvm,
					   converti_type_llvm(contexte, contexte.typeuse[TypeBase::PTR_Z8]),
					   true,
					   llvm::GlobalValue::PrivateLinkage,
					   constante,
					   ".chn");

	auto valeur_taille = llvm::ConstantInt::get(
					llvm::Type::getInt32Ty(contexte.contexte),
					static_cast<uint64_t>(chaine.taille()),
					false);
	auto type_chaine = converti_type_llvm(contexte, contexte.typeuse[TypeBase::CHAINE]);

	auto struct_chaine = llvm::ConstantStruct::get(
						static_cast<llvm::StructType *>(type_chaine),
				static_cast<llvm::Constant *>(pointeur_chaine_c),
						valeur_taille);

	return struct_chaine;
}

[[nodiscard]] static auto cree_tableau_global(
		ContexteGenerationCode &contexte,
		std::vector<llvm::Constant *> const &valeurs_enum_tmp,
		long idx_type_membre)
{
	auto type_valeur_llvm = converti_type_llvm(contexte, idx_type_membre);
	auto type_tableau_valeur = contexte.typeuse.type_tableau_pour(idx_type_membre);

	auto type_tableau_valeur_llvm = converti_type_llvm(contexte, contexte.typeuse[type_tableau_valeur]);

	auto type_tableau = llvm::ArrayType::get(type_valeur_llvm, valeurs_enum_tmp.size());

	auto tableau_constant = llvm::ConstantArray::get(type_tableau, valeurs_enum_tmp);

	auto pointeur_tableau = new llvm::GlobalVariable(
					   *contexte.module_llvm,
					   type_valeur_llvm->getPointerTo(),
					   true,
					   llvm::GlobalValue::PrivateLinkage,
					   tableau_constant,
					   ".tabl");

	static_cast<void>(tableau_constant);
	auto valeur_taille = llvm::ConstantInt::get(
				llvm::Type::getInt32Ty(contexte.contexte),
				valeurs_enum_tmp.size(),
				false);

	auto struct_tableau = llvm::ConstantStruct::get(
						static_cast<llvm::StructType *>(type_tableau_valeur_llvm),
				pointeur_tableau,
						valeur_taille);

	return struct_tableau;
}

[[nodiscard]] static auto cree_info_type_enum(
		ContexteGenerationCode &contexte,
		DonneesStructure const &donnees_structure)
{
	auto builder = llvm::IRBuilder<>(contexte.contexte);

	auto type_llvm = obtiens_type_pour(contexte, "InfoTypeÉnum");

	/* { id: e32, nom: chaine, valeurs: [], membres: [], est_drapeau: bool } */

	auto valeur_id = llvm::ConstantInt::get(
				llvm::Type::getInt32Ty(contexte.contexte),
				IDInfoType::ENUM,
				false);

	auto est_drapeau = llvm::ConstantInt::get(
				llvm::Type::getInt1Ty(contexte.contexte),
				donnees_structure.est_drapeau,
				false);

	auto noeud_decl = donnees_structure.noeud_decl;
	auto struct_chaine = cree_chaine_globale(contexte, donnees_structure.noeud_decl->chaine());

	/* création des tableaux de valeurs et de noms */

	std::vector<llvm::Constant *> valeurs_enum;
	valeurs_enum.reserve(static_cast<size_t>(donnees_structure.donnees_membres.taille()));

	std::vector<llvm::Constant *> noms_enum;
	noms_enum.reserve(static_cast<size_t>(donnees_structure.donnees_membres.taille()));

	for (auto enfant : noeud_decl->enfants) {
		auto enf0 = enfant;

		if (enf0->genre == GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE) {
			enf0 = enf0->enfants.front();
		}

		auto dm = donnees_structure.donnees_membres.trouve(enf0->chaine())->second;

		auto valeur = llvm::ConstantInt::get(
					llvm::Type::getInt32Ty(contexte.contexte),
					static_cast<uint64_t>(dm.resultat_expression.entier),
					false);

		valeurs_enum.push_back(valeur);

		auto chaine_nom = cree_chaine_globale(contexte, enf0->chaine());
		noms_enum.push_back(chaine_nom);
	}

	auto tableau_valeurs = cree_tableau_global(contexte, valeurs_enum, contexte.typeuse[TypeBase::Z32]);
	auto tableau_noms = cree_tableau_global(contexte, noms_enum, contexte.typeuse[TypeBase::CHAINE]);

	/* création de l'info type */

	auto constant = llvm::ConstantStruct::get(
						static_cast<llvm::StructType *>(type_llvm),
						valeur_id, struct_chaine, tableau_valeurs, tableau_noms, est_drapeau);

	auto globale =  new llvm::GlobalVariable(
						*contexte.module_llvm,
						type_llvm,
						true,
						llvm::GlobalValue::InternalLinkage,
						constant);

	globale->setConstant(true);

	return globale;
}

llvm::Value *cree_info_type(
		ContexteGenerationCode &contexte,
		DonneesTypeFinal &donnees_type)
{
	auto valeur = static_cast<llvm::Value *>(nullptr);

	switch (donnees_type.type_base() & 0xff) {
		default:
		{
			assert(false);
			break;
		}
		case TypeLexeme::BOOL:
		{
			valeur = cree_info_type_defaut(contexte, IDInfoType::BOOLEEN);
			break;
		}
		case TypeLexeme::N8:
		{
			valeur = cree_info_type_entier(contexte, 8, false);
			break;
		}
		case TypeLexeme::Z8:
		{
			valeur = cree_info_type_entier(contexte, 8, true);
			break;
		}
		case TypeLexeme::N16:
		{
			valeur = cree_info_type_entier(contexte, 16, false);
			break;
		}
		case TypeLexeme::Z16:
		{
			valeur = cree_info_type_entier(contexte, 16, true);
			break;
		}
		case TypeLexeme::N32:
		{
			valeur = cree_info_type_entier(contexte, 32, false);
			break;
		}
		case TypeLexeme::Z32:
		{
			valeur = cree_info_type_entier(contexte, 32, true);
			break;
		}
		case TypeLexeme::N64:
		{
			valeur = cree_info_type_entier(contexte, 64, false);
			break;
		}
		case TypeLexeme::Z64:
		{
			valeur = cree_info_type_entier(contexte, 64, true);
			break;
		}
		case TypeLexeme::R16:
		{
			valeur = cree_info_type_reel(contexte, 16);
			break;
		}
		case TypeLexeme::R32:
		{
			valeur = cree_info_type_reel(contexte, 32);
			break;
		}
		case TypeLexeme::R64:
		{
			valeur = cree_info_type_reel(contexte, 64);
			break;
		}
		case TypeLexeme::REFERENCE:
		case TypeLexeme::POINTEUR:
		{
			/* { id, type_pointé, est_référence } */

			auto type_pointeur = obtiens_type_pour(contexte, "InfoTypePointeur");

			auto valeur_id = llvm::ConstantInt::get(
						llvm::Type::getInt32Ty(contexte.contexte),
						IDInfoType::POINTEUR,
						false);

			auto idx_type_deref = contexte.typeuse.ajoute_type(donnees_type.dereference());
			auto &type_deref = contexte.typeuse[idx_type_deref];

			auto valeur_type_pointe = cree_info_type(contexte, type_deref);

			auto est_reference = llvm::ConstantInt::get(
								   llvm::Type::getInt1Ty(contexte.contexte),
								   donnees_type.type_base() == TypeLexeme::REFERENCE,
								   false);

			auto constant = llvm::ConstantStruct::get(
								static_cast<llvm::StructType *>(type_pointeur),
								valeur_id, static_cast<llvm::Constant *>(valeur_type_pointe), est_reference);

			auto globale =  new llvm::GlobalVariable(
								*contexte.module_llvm,
								type_pointeur,
								true,
								llvm::GlobalValue::InternalLinkage,
								constant);

			globale->setConstant(true);

			valeur = globale;
			break;
		}
		case TypeLexeme::CHAINE_CARACTERE:
		{
			auto id_structure = static_cast<long>(donnees_type.type_base() >> 8);
			auto donnees_structure = contexte.donnees_structure(id_structure);

			if (donnees_structure.est_enum) {
				valeur = cree_info_type_enum(contexte, donnees_structure);
				break;
			}

			/* pour chaque membre cree une instance de InfoTypeMembreStructure */
			auto type_struct_membre = obtiens_type_pour(contexte, "InfoTypeMembreStructure");

			std::vector<llvm::Constant *> valeurs_membres;

			for (auto &arg : donnees_structure.donnees_membres) {
				/* { nom: chaine, info : *InfoType, décalage } */
				auto id_dt = donnees_structure.index_types[arg.second.index_membre];
				auto &ref_membre = contexte.typeuse[id_dt];

				auto info_type = cree_info_type(contexte, ref_membre);

				auto valeur_nom = cree_chaine_globale(contexte, arg.first);

				auto valeur_decalage = llvm::ConstantInt::get(
							llvm::Type::getInt64Ty(contexte.contexte),
							static_cast<uint64_t>(arg.second.decalage),
							false);

				auto constant = llvm::ConstantStruct::get(
									static_cast<llvm::StructType *>(type_struct_membre),
									valeur_nom, static_cast<llvm::Constant *>(info_type), valeur_decalage);

				/* Création d'un InfoType globale. */
				auto globale = new llvm::GlobalVariable(
									*contexte.module_llvm,
									type_struct_membre,
									true,
									llvm::GlobalValue::InternalLinkage,
									constant);

				valeurs_membres.push_back(globale);
			}

			/* { id : n32, nom: chaine, membres : []InfoTypeMembreStructure } */

			auto valeur_id = llvm::ConstantInt::get(
						llvm::Type::getInt32Ty(contexte.contexte),
						IDInfoType::STRUCTURE,
						false);

			auto valeur_nom = cree_chaine_globale(contexte, donnees_structure.noeud_decl->chaine());

			auto idx_type_membre = contexte.donnees_structure("InfoTypeMembreStructure").index_type;
			auto tableau_membre = cree_tableau_global(contexte, valeurs_membres, idx_type_membre);

			auto type_llvm = obtiens_type_pour(contexte, "InfoTypeStructure");

			auto constant = llvm::ConstantStruct::get(
								static_cast<llvm::StructType *>(type_llvm),
								valeur_id, valeur_nom, tableau_membre);

			auto globale =  new llvm::GlobalVariable(
								*contexte.module_llvm,
								type_llvm,
								true,
								llvm::GlobalValue::InternalLinkage,
								constant);

			valeur = globale;
			break;
		}
		case TypeLexeme::TABLEAU:
		{
			/* { id, type_pointé } */

			auto type_pointeur = obtiens_type_pour(contexte, "InfoTypeTableau");

			auto valeur_id = llvm::ConstantInt::get(
						llvm::Type::getInt32Ty(contexte.contexte),
						IDInfoType::TABLEAU,
						false);

			auto idx_type_deref = contexte.typeuse.ajoute_type(donnees_type.dereference());
			auto &type_deref = contexte.typeuse[idx_type_deref];

			auto valeur_type_pointe = cree_info_type(contexte, type_deref);

			auto constant = llvm::ConstantStruct::get(
								static_cast<llvm::StructType *>(type_pointeur),
								valeur_id, static_cast<llvm::Constant *>(valeur_type_pointe));

			auto globale =  new llvm::GlobalVariable(
								*contexte.module_llvm,
								type_pointeur,
								true,
								llvm::GlobalValue::InternalLinkage,
								constant);

			globale->setConstant(true);

			valeur = globale;
			break;
		}
		case TypeLexeme::FONC:
		{
			valeur = cree_info_type_defaut(contexte, IDInfoType::FONCTION);
			break;
		}
		case TypeLexeme::EINI:
		{
			valeur = cree_info_type_defaut(contexte, IDInfoType::EINI);
			break;
		}
		case TypeLexeme::NUL: /* À FAIRE */
		case TypeLexeme::RIEN:
		{
			valeur = cree_info_type_defaut(contexte, IDInfoType::RIEN);
			break;
		}
		case TypeLexeme::CHAINE:
		{
			valeur = cree_info_type_defaut(contexte, IDInfoType::CHAINE);
			break;
		}
	}

	return valeur;
}

llvm::Value *valeur_enum(
		DonneesStructure const &ds,
		dls::vue_chaine_compacte const &nom,
		llvm::IRBuilder<> &builder)
{
	auto &dm = ds.donnees_membres.trouve(nom)->second;

	if (dm.resultat_expression.type == type_expression::ENTIER) {
		return builder.getInt32(static_cast<unsigned>(dm.resultat_expression.entier));
	}

	return llvm::ConstantFP::get(
							builder.getDoubleTy(),
							dm.resultat_expression.reel);
}
