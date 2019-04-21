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

#include "info_type.hh"

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
#include "donnees_type.h"

[[nodiscard]] static auto obtiens_type_pour(
		ContexteGenerationCode &contexte,
		std::string_view const &nom_struct)
{
	auto index_struct_info = contexte.donnees_structure(nom_struct).id;

	auto dt_info = DonneesType{};
	dt_info.pousse(id_morceau::CHAINE_CARACTERE | (static_cast<int>(index_struct_info << 8)));

	index_struct_info = contexte.magasin_types.ajoute_type(dt_info);
	auto &ref_dt_info = contexte.magasin_types.donnees_types[index_struct_info];

	return converti_type(contexte, ref_dt_info);
}

[[nodiscard]] static auto cree_info_type_reel(
		ContexteGenerationCode &contexte,
		uint64_t nombre_bits)
{
	auto type_llvm = obtiens_type_pour(contexte, "InfoTypeReel");

	/* { id : e32, nombre_bits : e32 } */

	auto valeur_id = llvm::dyn_cast<llvm::GlobalVariable>(contexte.valeur_globale("TYPE_REEL"));

	auto valeur_nbits = llvm::ConstantInt::get(
						   llvm::Type::getInt32Ty(contexte.contexte),
						   nombre_bits,
						   false);

	llvm::ArrayRef<llvm::Constant *> valeurs{ valeur_id->getInitializer(), valeur_nbits };

	auto constant = llvm::ConstantStruct::get(
						static_cast<llvm::StructType *>(type_llvm),
						valeurs);

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

	auto valeur_id = llvm::dyn_cast<llvm::GlobalVariable>(contexte.valeur_globale("TYPE_ENTIER"));

	auto valeur_signe = llvm::ConstantInt::get(
						   llvm::Type::getInt1Ty(contexte.contexte),
						   est_signe,
						   false);

	auto valeur_nbits = llvm::ConstantInt::get(
						   llvm::Type::getInt32Ty(contexte.contexte),
						   nombre_bits,
						   false);

	llvm::ArrayRef<llvm::Constant *> valeurs{ valeur_id->getInitializer(), valeur_signe, valeur_nbits };

	auto constant = llvm::ConstantStruct::get(
						static_cast<llvm::StructType *>(type_llvm),
						valeurs);

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
		std::string_view const &nom_type)
{
	auto type_llvm = obtiens_type_pour(contexte, "InfoType");

	/* { id : e32 } */

	auto valeur_id = llvm::dyn_cast<llvm::GlobalVariable>(contexte.valeur_globale(nom_type));

	llvm::ArrayRef<llvm::Constant *> valeurs{ valeur_id->getInitializer() };

	auto constant = llvm::ConstantStruct::get(
						static_cast<llvm::StructType *>(type_llvm),
						valeurs);

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

[[nodiscard]] static auto cree_info_type_pointeur(
		ContexteGenerationCode &contexte,
		std::string_view const &nom_type,
		llvm::GlobalVariable *valeur_pointee)
{
	auto type_llvm = obtiens_type_pour(contexte, "InfoTypePointeur");

	/* { id : e32, *InfoStruct } */

	auto valeur_id = llvm::dyn_cast<llvm::GlobalVariable>(contexte.valeur_globale(nom_type));

	llvm::ArrayRef<llvm::Constant *> valeurs{ valeur_id->getInitializer(), valeur_pointee };

	auto constant = llvm::ConstantStruct::get(
						static_cast<llvm::StructType *>(type_llvm),
						valeurs);

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

llvm::Value *cree_info_type(
		ContexteGenerationCode &contexte,
		DonneesType &donnees_type)
{
	auto valeur = static_cast<llvm::Value *>(nullptr);

	switch (donnees_type.type_base() & 0xff) {
		default:
		{
			assert(false);
			break;
		}
		case id_morceau::BOOL:
		{
			valeur = cree_info_type_defaut(contexte, "TYPE_BOOLEEN");
			break;
		}
		case id_morceau::N8:
		{
			valeur = cree_info_type_entier(contexte, 8, false);
			break;
		}
		case id_morceau::Z8:
		{
			valeur = cree_info_type_entier(contexte, 8, true);
			break;
		}
		case id_morceau::N16:
		{
			valeur = cree_info_type_entier(contexte, 16, false);
			break;
		}
		case id_morceau::Z16:
		{
			valeur = cree_info_type_entier(contexte, 16, true);
			break;
		}
		case id_morceau::N32:
		{
			valeur = cree_info_type_entier(contexte, 32, false);
			break;
		}
		case id_morceau::Z32:
		{
			valeur = cree_info_type_entier(contexte, 32, true);
			break;
		}
		case id_morceau::N64:
		{
			valeur = cree_info_type_entier(contexte, 64, false);
			break;
		}
		case id_morceau::Z64:
		{
			valeur = cree_info_type_entier(contexte, 64, true);
			break;
		}
		case id_morceau::R16:
		{
			/* À FAIRE : type r16 */
			valeur = cree_info_type_reel(contexte, 32);
			break;
		}
		case id_morceau::R32:
		{
			valeur = cree_info_type_reel(contexte, 32);
			break;
		}
		case id_morceau::R64:
		{
			valeur = cree_info_type_reel(contexte, 64);
			break;
		}
		case id_morceau::POINTEUR:
		{
			auto deref = donnees_type.derefence();
			auto valeur_pointee = cree_info_type(contexte, deref);
			valeur = cree_info_type_pointeur(contexte, "TYPE_POINTEUR", llvm::dyn_cast<llvm::GlobalVariable>(valeur_pointee));
			break;
		}
		case id_morceau::CHAINE_CARACTERE:
		{
			auto id_structure = static_cast<size_t>(donnees_type.type_base() >> 8);
			auto donnees_structure = contexte.donnees_structure(id_structure);

			/* À FAIRE : tableaux. */

			/* pour chaque membre cree une instance de InfoTypeMembreStructure */
			auto type_struct_membre = obtiens_type_pour(contexte, "InfoTypeMembreStructure");

			std::vector<llvm::Constant *> structes_membres;

			for (auto &arg : donnees_structure.index_membres) {
				/* { nom : []z8, info : *InfoType } */
				auto id_dt = donnees_structure.donnees_types[arg.second];
				auto &ref_membre = contexte.magasin_types.donnees_types[id_dt];

				auto info_type = cree_info_type(contexte, ref_membre);

				auto constante = llvm::ConstantDataArray::getString(
									 contexte.contexte,
									 std::string(arg.first));

				auto dt_nom = DonneesType{};
				dt_nom.pousse(id_morceau::TABLEAU | static_cast<id_morceau>(arg.first.size() << 8));
				dt_nom.pousse(id_morceau::Z8);
				auto type = converti_type(contexte, dt_nom);

				auto nom = new llvm::GlobalVariable(
								   *contexte.module_llvm,
								   type,
								   true,
								   llvm::GlobalValue::PrivateLinkage,
								   constante,
								   ".chn");

				nom->setAlignment(1);
				nom->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);

				llvm::ArrayRef<llvm::Constant *> valeurs{ nom, llvm::dyn_cast<llvm::GlobalVariable>(info_type) };

				auto constant = llvm::ConstantStruct::get(
									static_cast<llvm::StructType *>(type_struct_membre),
									valeurs);

				/* Création d'un InfoType globale. */
				auto globale = new llvm::GlobalVariable(
									*contexte.module_llvm,
									type_struct_membre,
									true,
									llvm::GlobalValue::InternalLinkage,
									constant);

				structes_membres.push_back(globale);
			}

			/* { id : n32, membres : []InfoTypeMembreStructure } */

			auto valeur_id = llvm::dyn_cast<llvm::GlobalVariable>(contexte.valeur_globale("TYPE_STRUCTURE"));

			auto constante = llvm::ConstantArray::get(llvm::ArrayType::get(type_struct_membre, structes_membres.size()), structes_membres);

			auto globale_ = new llvm::GlobalVariable(
								*contexte.module_llvm,
								llvm::ArrayType::get(type_struct_membre, structes_membres.size()),
								true,
								llvm::GlobalValue::InternalLinkage,
								constante);

			llvm::ArrayRef<llvm::Constant *> valeurs{ valeur_id->getInitializer(), globale_ };

			auto type_llvm = converti_type(contexte, donnees_type);

			auto constant = llvm::ConstantStruct::get(
								static_cast<llvm::StructType *>(type_llvm),
								valeurs);

			/* Création d'un InfoType globale. */
			auto globale =  new llvm::GlobalVariable(
								*contexte.module_llvm,
								type_llvm,
								true,
								llvm::GlobalValue::InternalLinkage,
								constant);

			valeur = globale;
			break;
		}
		case id_morceau::TABLEAU:
		{
			auto deref = donnees_type.derefence();
			auto valeur_pointee = cree_info_type(contexte, deref);
			valeur = cree_info_type_pointeur(contexte, "TYPE_TABLEAU", llvm::dyn_cast<llvm::GlobalVariable>(valeur_pointee));
			break;
		}
		case id_morceau::FONCTION:
		{
			valeur = cree_info_type_defaut(contexte, "TYPE_FONCTION");
			break;
		}
		case id_morceau::EINI:
		{
			valeur = cree_info_type_defaut(contexte, "TYPE_EINI");
			break;
		}
		case id_morceau::RIEN:
		{
			valeur = cree_info_type_defaut(contexte, "TYPE_RIEN");
			break;
		}
	}

	return valeur;
}
