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

#include "contexte_generation_llvm.hh"

#include "arbre_syntactic.h"
#include "typage.hh"

llvm::Type *converti_type_llvm(
		ContexteGenerationLLVM &contexte,
		Type *type)
{
	if (type->type_llvm != nullptr) {
		/* Note: normalement les types des pointeurs vers les fonctions doivent
		 * être des pointeurs, mais les types fonctions sont partagés entre les
		 * fonctions et les variables, donc un type venant d'une fonction n'aura
		 * pas le pointeur. Ajoutons-le. */
		if (type->genre == GenreType::FONCTION && !type->type_llvm->isPointerTy()) {
			return llvm::PointerType::get(type->type_llvm, 0);
		}

		return type->type_llvm;
	}

	switch (type->genre) {
		case GenreType::INVALIDE:
		{
			type->type_llvm = nullptr;
			break;
		}
		case GenreType::FONCTION:
		{
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

			type->type_llvm = llvm::PointerType::get(type_llvm, 0);
			break;
		}
		case GenreType::EINI:
		{
			/* type = structure { *z8, *InfoType } */
			auto type_info_type = contexte.typeuse.type_pour_nom("InfoType");

			std::vector<llvm::Type *> types_membres(2ul);
			types_membres[0] = llvm::Type::getInt8PtrTy(contexte.contexte);
			types_membres[1] = converti_type_llvm(contexte, type_info_type)->getPointerTo();

			type->type_llvm = llvm::StructType::create(
						contexte.contexte,
						types_membres,
						"struct.eini",
						false);

			break;
		}
		case GenreType::CHAINE:
		{
			/* type = structure { *z8, z64 } */
			std::vector<llvm::Type *> types_membres(2ul);
			types_membres[0] = llvm::Type::getInt8PtrTy(contexte.contexte);
			types_membres[1] = llvm::Type::getInt64Ty(contexte.contexte);

			type->type_llvm = llvm::StructType::create(
						contexte.contexte,
						types_membres,
						"struct.chaine",
						false);

			break;
		}
		case GenreType::RIEN:
		{
			type->type_llvm = llvm::Type::getVoidTy(contexte.contexte);
			break;
		}
		case GenreType::BOOL:
		{
			type->type_llvm = llvm::Type::getInt1Ty(contexte.contexte);
			break;
		}
		case GenreType::OCTET:
		{
			type->type_llvm = llvm::Type::getInt8Ty(contexte.contexte);
			break;
		}
		case GenreType::ENTIER_CONSTANT:
		{
			type->type_llvm = llvm::Type::getInt32Ty(contexte.contexte);
			break;
		}
		case GenreType::ENTIER_NATUREL:
		case GenreType::ENTIER_RELATIF:
		{
			if (type->taille_octet == 1) {
				type->type_llvm = llvm::Type::getInt8Ty(contexte.contexte);
			}
			else if (type->taille_octet == 2) {
				type->type_llvm = llvm::Type::getInt16Ty(contexte.contexte);
			}
			else if (type->taille_octet == 4) {
				type->type_llvm = llvm::Type::getInt32Ty(contexte.contexte);
			}
			else if (type->taille_octet == 8) {
				type->type_llvm = llvm::Type::getInt64Ty(contexte.contexte);
			}

			break;
		}
		case GenreType::REEL:
		{
			if (type->taille_octet == 2) {
				type->type_llvm = llvm::Type::getInt16Ty(contexte.contexte);
			}
			else if (type->taille_octet == 4) {
				type->type_llvm = llvm::Type::getFloatTy(contexte.contexte);
			}
			else if (type->taille_octet == 8) {
				type->type_llvm = llvm::Type::getDoubleTy(contexte.contexte);
			}

			break;
		}
		case GenreType::REFERENCE:
		case GenreType::POINTEUR:
		{
			auto type_deref = contexte.typeuse.type_dereference_pour(type);

			// Les pointeurs vers rien (void) ne sont pas valides avec LLVM
			if (type_deref->genre == GenreType::RIEN) {
				type_deref = contexte.typeuse[TypeBase::Z8];
			}

			auto type_deref_llvm = converti_type_llvm(contexte, type_deref);
			type->type_llvm = llvm::PointerType::get(type_deref_llvm, 0);
			break;
		}
		case GenreType::UNION:
		{
			auto type_struct = static_cast<TypeStructure *>(type);
			auto decl_struct = type_struct->decl;
			auto nom_nonsur = "union_nonsure." + type_struct->nom;
			auto nom = "union." + type_struct->nom;

			// création d'une structure ne contenant que le membre le plus grand
			auto taille_max = 0u;
			auto type_max = static_cast<Type *>(nullptr);

			POUR (type_struct->membres) {
				auto taille_type = it.type->taille_octet;

				if (taille_type > taille_max) {
					taille_max = taille_type;
					type_max = it.type;
				}
			}

			auto type_max_llvm = converti_type_llvm(contexte, type_max);
			auto type_union = llvm::StructType::create(contexte.contexte, { type_max_llvm }, nom_nonsur.c_str());

			if (!decl_struct->est_nonsure) {
				// création d'une structure contenant l'union et une valeur discriminante
				type_union = llvm::StructType::create(contexte.contexte, { type_union, llvm::Type::getInt32Ty(contexte.contexte) }, nom.c_str());
			}

			break;
		}
		case GenreType::STRUCTURE:
		{
			auto type_struct = static_cast<TypeStructure *>(type);
			auto nom = "struct." + type_struct->nom;

			/* Pour les structures récursives, il faut créer un type
			 * opaque, dont le corps sera renseigné à la fin */
			auto type_opaque = llvm::StructType::create(contexte.contexte, nom.c_str());
			type_struct->type_llvm = type_opaque;

			std::vector<llvm::Type *> types_membres;
			types_membres.reserve(static_cast<size_t>(type_struct->membres.taille));

			POUR (type_struct->membres) {
				types_membres.push_back(converti_type_llvm(contexte, it.type));
			}

			type_opaque->setBody(types_membres, false);

			break;
		}
		case GenreType::VARIADIQUE:
		{
			auto type_var = static_cast<TypeVariadique *>(type);

			// Utilise le type de tableau dynamique afin que le code IR LLVM
			// soit correcte (pointe vers le même type)
			if (type_var->type_pointe != nullptr) {
				auto type_tabl = contexte.typeuse.type_tableau_dynamique(type_var->type_pointe);
				type_var->type_llvm = converti_type_llvm(contexte, type_tabl);
			}

			break;
		}
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			auto type_deref_llvm = converti_type_llvm(contexte, contexte.typeuse.type_dereference_pour(type));

			/* type = structure { *type, n64, n64 } */
			std::vector<llvm::Type *> types_membres(3ul);
			types_membres[0] = llvm::PointerType::get(type_deref_llvm, 0);
			types_membres[1] = llvm::Type::getInt64Ty(contexte.contexte);
			types_membres[2] = llvm::Type::getInt64Ty(contexte.contexte);

			type->type_llvm = llvm::StructType::create(
					   contexte.contexte,
					   types_membres,
					   "struct.tableau",
					   false);
			break;
		}
		case GenreType::TABLEAU_FIXE:
		{
			auto type_deref_llvm = converti_type_llvm(contexte, contexte.typeuse.type_dereference_pour(type));
			auto const taille = static_cast<TypeTableauFixe *>(type)->taille;

			type->type_llvm = llvm::ArrayType::get(type_deref_llvm, static_cast<unsigned long>(taille));
			break;
		}
		case GenreType::ENUM:
		case GenreType::ERREUR:
		{
			auto type_enum = static_cast<TypeEnum *>(type);
			type_enum->type_llvm = converti_type_llvm(contexte, type_enum->type_donnees);
			break;
		}
	}

	return type->type_llvm;
}
