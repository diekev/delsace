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

#include "typage.hh"

#if 0
llvm::Type *converti_type_llvm(
		ContexteGenerationCode &contexte,
		Type *type)
{
	switch (type->genre) {
		case GenreType::INVALIDE:
		{
			return nullptr;
		}
		case GenreType::FONCTION:
		{
			if (type->type_llvm != nullptr) {
				return llvm::PointerType::get(type->type_llvm, 0);
			}

			auto type_fonc = static_cast<TypeFonction *>(type);

			std::vector<llvm::Type *> parametres;
			POUR (type_fonc->types_entrees) {
				auto type_llvm = converti_type_llvm(contexte, it);
				parametres.push_back(type_llvm);
			}

			// À FAIRE : multiples types de retours
			auto type_retour = converti_type_llvm(contexte, type_fonc->types_sorties[0]);

			auto type_llvm = llvm::FunctionType::get(
						type_retour,
						parametres,
						false);

			return llvm::PointerType::get(type_llvm, 0);
		}
		case GenreType::EINI:
		{
			/* type = structure { *z8, *InfoType } */
			auto type_info_type = contexte.typeuse.type_pour_nom("InfoType");

			std::vector<llvm::Type *> types_membres(2ul);
			types_membres[0] = llvm::Type::getInt8PtrTy(contexte.contexte);
			types_membres[1] = converti_type_llvm(contexte, type_info_type);

			return llvm::StructType::create(
						contexte.contexte,
						types_membres,
						"struct.eini",
						false);

		}
		case GenreType::CHAINE:
		{
			/* type = structure { *z8, z64 } */
			std::vector<llvm::Type *> types_membres(2ul);
			types_membres[0] = llvm::Type::getInt8PtrTy(contexte.contexte);
			types_membres[1] = llvm::Type::getInt64Ty(contexte.contexte);

			return llvm::StructType::create(
						contexte.contexte,
						types_membres,
						"struct.chaine",
						false);
		}
		case GenreType::RIEN:
		{
			return llvm::Type::getVoidTy(contexte.contexte);
		}
		case GenreType::BOOL:
		{
			return llvm::Type::getInt1Ty(contexte.contexte);
		}
		case GenreType::OCTET:
		{
			return llvm::Type::getInt8Ty(contexte.contexte);
		}
		case GenreType::ENTIER_CONSTANT:
		{
			return llvm::Type::getInt32Ty(contexte.contexte);
		}
		case GenreType::ENTIER_NATUREL:
		case GenreType::ENTIER_RELATIF:
		{
			if (type->taille_octet == 1) {
				return llvm::Type::getInt8Ty(contexte.contexte);
			}

			if (type->taille_octet == 2) {
				return llvm::Type::getInt16Ty(contexte.contexte);
			}

			if (type->taille_octet == 4) {
				return llvm::Type::getInt32Ty(contexte.contexte);
			}

			if (type->taille_octet == 8) {
				return llvm::Type::getInt64Ty(contexte.contexte);
			}

			return nullptr;
		}
		case GenreType::REEL:
		{
			if (type->taille_octet == 2) {
				return llvm::Type::getInt16Ty(contexte.contexte);
			}

			if (type->taille_octet == 4) {
				return llvm::Type::getFloatTy(contexte.contexte);
			}

			if (type->taille_octet == 8) {
				return llvm::Type::getDoubleTy(contexte.contexte);
			}

			return nullptr;
		}
		case GenreType::REFERENCE:
		case GenreType::POINTEUR:
		{
			auto type_deref_llvm = converti_type_llvm(contexte, contexte.typeuse.type_dereference_pour(type));
			return llvm::PointerType::get(type_deref_llvm, 0);
		}
		case GenreType::UNION:
		case GenreType::STRUCTURE:
		{
			auto nom_struct = static_cast<TypeStructure *>(type)->nom;
			auto &donnees_structure = contexte.donnees_structure(nom_struct);

			if (donnees_structure.type_llvm == nullptr) {
				if (donnees_structure.est_union) {
					auto nom_nonsur = "union_nonsure." + contexte.nom_struct(donnees_structure.id);
					auto nom = "union." + contexte.nom_struct(donnees_structure.id);

					// création d'une structure ne contenant que le membre le plus grand
					auto taille_max = 0u;
					auto type_max = static_cast<Type *>(nullptr);

					for (auto &id : donnees_structure.types) {
						auto taille_type = id->taille_octet;

						if (taille_type > taille_max) {
							taille_max = taille_type;
							type_max = id;
						}
					}

					auto type_max_llvm = converti_type_llvm(contexte, type_max);
					auto type_union = llvm::StructType::create(contexte.contexte, { type_max_llvm }, nom_nonsur.c_str());

					if (!donnees_structure.est_nonsur) {
						// création d'une structure contenant l'union et une valeur discriminante
						type_union = llvm::StructType::create(contexte.contexte, { type_union, llvm::Type::getInt32Ty(contexte.contexte) }, nom.c_str());
					}

					donnees_structure.type_llvm = type_union;
				}
				else {
					auto nom = "struct." + contexte.nom_struct(donnees_structure.id);

					/* Pour les structures récursives, il faut créer un type
					 * opaque, dont le corps sera renseigné à la fin */
					auto type_opaque = llvm::StructType::create(contexte.contexte, nom.c_str());
					donnees_structure.type_llvm = type_opaque;

					std::vector<llvm::Type *> types_membres;
					types_membres.resize(static_cast<size_t>(donnees_structure.types.taille()));

					std::transform(donnees_structure.types.debut(),
								   donnees_structure.types.fin(),
								   types_membres.begin(),
								   [&](Type *type_membre)
					{
						return converti_type_llvm(contexte, type_membre);
					});

					type_opaque->setBody(types_membres, false);
				}
			}

			return donnees_structure.type_llvm;
		}
		case GenreType::VARIADIQUE:
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			auto type_deref_llvm = converti_type_llvm(contexte, contexte.typeuse.type_dereference_pour(type));

			/* type = structure { *type, n64 } */
			std::vector<llvm::Type *> types_membres(2ul);
			types_membres[0] = llvm::PointerType::get(type_deref_llvm, 0);
			types_membres[1] = llvm::Type::getInt64Ty(contexte.contexte);

			return llvm::StructType::create(
					   contexte.contexte,
					   types_membres,
					   "struct.tableau",
					   false);
		}
		case GenreType::TABLEAU_FIXE:
		{
			auto type_deref_llvm = converti_type_llvm(contexte, contexte.typeuse.type_dereference_pour(type));
			auto const taille = static_cast<TypeTableauFixe *>(type)->taille;

			return  llvm::ArrayType::get(type_deref_llvm, static_cast<unsigned long>(taille));
		}
		case GenreType::ENUM:
		{
			auto nom_struct = static_cast<TypeStructure *>(type)->nom;
			auto &donnees_structure = contexte.donnees_structure(nom_struct);
			donnees_structure.type_llvm = converti_type_llvm(contexte, donnees_structure.type);
			return nullptr;
		}
	}

	return nullptr;
}
#endif
