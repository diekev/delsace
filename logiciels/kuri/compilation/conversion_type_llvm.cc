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

#include "conversion_type_llvm.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#include <llvm/IR/TypeBuilder.h>
#pragma GCC diagnostic pop

#include "contexte_generation_code.h"

#include "donnees_type.h"

// À FAIRE :
// - unions (utilisation d'une structure avec un seul élément suffisament grand)
//   voir https://stackoverflow.com/questions/19549942/extracting-a-value-from-an-union

static llvm::Type *converti_type(
		ContexteGenerationCode &contexte,
		DonneesTypeFinal &donnees_type)
{
	/* Pointeur vers une fonction, seulement valide lors d'assignement, ou en
	 * paramètre de fonction. */
	if (donnees_type.type_base() == id_morceau::FONC) {
		if (donnees_type.type_llvm() != nullptr) {
			return llvm::PointerType::get(donnees_type.type_llvm(), 0);
		}

		llvm::Type *type = nullptr;
		auto dt = DonneesTypeFinal{};
		std::vector<llvm::Type *> parametres;

		auto nombre_retour = 0l;
		auto dt_params = donnees_types_parametres(contexte.typeuse, donnees_type, nombre_retour);

		for (auto i = 0; i < dt_params.taille() - 1; ++i) {
			type = converti_type(contexte, contexte.typeuse[dt_params[i]]);
			parametres.push_back(type);
		}

		type = converti_type(contexte, contexte.typeuse[dt_params.back()]);
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
		type = converti_type_simple_llvm(contexte, identifiant, type);
	}

	donnees_type.type_llvm(type);

	return type;
}

llvm::Type *converti_type_llvm(
		ContexteGenerationCode &contexte,
		DonneesTypeFinal &donnees)
{
	auto index = contexte.typeuse.ajoute_type(donnees);
	auto &dt = contexte.typeuse[index];
	return converti_type(contexte, dt);
}

llvm::Type *converti_type_llvm(
		ContexteGenerationCode &contexte,
		long index_type)
{
	auto &dt = contexte.typeuse[index_type];
	return converti_type(contexte, dt);
}

llvm::Type *converti_type_simple_llvm(
		ContexteGenerationCode &contexte,
		const id_morceau &identifiant,
		llvm::Type *type_entree)
{
	llvm::Type *type = nullptr;

	switch (identifiant & 0xff) {
		case id_morceau::BOOL:
		{
			type = llvm::Type::getInt1Ty(contexte.contexte);
			break;
		}
		case id_morceau::OCTET:
		case id_morceau::N8:
		case id_morceau::Z8:
		{
			type = llvm::Type::getInt8Ty(contexte.contexte);
			break;
		}
		case id_morceau::N16:
		case id_morceau::Z16:
		{
			type = llvm::Type::getInt16Ty(contexte.contexte);
			break;
		}
		case id_morceau::N32:
		case id_morceau::Z32:
		{
			type = llvm::Type::getInt32Ty(contexte.contexte);
			break;
		}
		case id_morceau::N64:
		case id_morceau::Z64:
		{
			type = llvm::Type::getInt64Ty(contexte.contexte);
			break;
		}
		case id_morceau::R16:
		{
			type = llvm::Type::getInt16Ty(contexte.contexte);
			break;
		}
		case id_morceau::R32:
		{
			type = llvm::Type::getFloatTy(contexte.contexte);
			break;
		}
		case id_morceau::R64:
		{
			type = llvm::Type::getDoubleTy(contexte.contexte);
			break;
		}
		case id_morceau::RIEN:
		{
			type = llvm::Type::getVoidTy(contexte.contexte);
			break;
		}
		case id_morceau::REFERENCE:
		case id_morceau::POINTEUR:
		{
			type = llvm::PointerType::get(type_entree, 0);
			break;
		}
		case id_morceau::CHAINE_CARACTERE:
		{
			auto const &id_structure = (static_cast<long>(identifiant) & 0xffffff00) >> 8;
			auto &donnees_structure = contexte.donnees_structure(id_structure);

			if (donnees_structure.type_llvm == nullptr) {
				if (donnees_structure.est_enum) {
					auto &dt = contexte.typeuse[donnees_structure.noeud_decl->index_type];
					donnees_structure.type_llvm = converti_type_simple_llvm(contexte, dt.type_base(), nullptr);
				}
				else {
					std::vector<llvm::Type *> types_membres;
					types_membres.resize(static_cast<size_t>(donnees_structure.index_types.taille()));

					std::transform(donnees_structure.index_types.debut(),
								   donnees_structure.index_types.fin(),
								   types_membres.begin(),
								   [&](const long index_type)
					{
						auto &dt = contexte.typeuse[index_type];
						return converti_type(contexte, dt);
					});

					auto nom = "struct." + contexte.nom_struct(donnees_structure.id);

					donnees_structure.type_llvm = llvm::StructType::create(
													  contexte.contexte,
													  types_membres,
													  nom.c_str(),
													  false);
				}
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
		case id_morceau::EINI:
		{
			auto dt = DonneesTypeFinal{};
			dt.pousse(id_morceau::EINI);

			auto index_eini = contexte.typeuse.ajoute_type(dt);
			auto &type_eini = contexte.typeuse[index_eini];

			if (type_eini.type_llvm() == nullptr) {
				/* type = structure { *z8, *InfoType } */

				auto index_struct_info = contexte.donnees_structure("InfoType").id;

				auto dt_info = DonneesTypeFinal{};
				dt_info.pousse(id_morceau::POINTEUR);
				dt_info.pousse(id_morceau::CHAINE_CARACTERE | (static_cast<int>(index_struct_info << 8)));

				index_struct_info = contexte.typeuse.ajoute_type(dt_info);
				auto &ref_dt_info = contexte.typeuse[index_struct_info];

				auto type_struct_info = converti_type(contexte, ref_dt_info);

				std::vector<llvm::Type *> types_membres(2ul);
				types_membres[0] = llvm::Type::getInt8PtrTy(contexte.contexte);
				types_membres[1] = type_struct_info;

				type = llvm::StructType::create(
						   contexte.contexte,
						   types_membres,
						   "struct.eini",
						   false);

				type_eini.type_llvm(type);
			}

			type = type_eini.type_llvm();
			break;
		}
		case id_morceau::CHAINE:
		{
			auto index_chaine = contexte.typeuse[TypeBase::CHAINE];
			auto &type_chaine = contexte.typeuse[index_chaine];

			if (type_chaine.type_llvm() == nullptr) {
				/* type = structure { *z8, z64 } */
				std::vector<llvm::Type *> types_membres(2ul);
				types_membres[0] = llvm::Type::getInt8PtrTy(contexte.contexte);
				types_membres[1] = llvm::Type::getInt64Ty(contexte.contexte);

				type = llvm::StructType::create(
						   contexte.contexte,
						   types_membres,
						   "struct.chaine",
						   false);

				type_chaine.type_llvm(type);
			}

			type = type_chaine.type_llvm();
			break;
		}
		case id_morceau::TYPE_DE:
		{
			assert(false && "type_de aurait dû être résolu");
			break;
		}
		default:
		{
			assert(false);
		}
	}

	return type;
}
