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

#include "coulisse_llvm.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/InitializePasses.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#pragma GCC diagnostic pop

#include "biblexternes/iprof/prof.h"

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/structures/dico_desordonne.hh"

#include "compilatrice.hh"

#include "representation_intermediaire/constructrice_ri.hh"
#include "representation_intermediaire/instructions.hh"

/* ************************************************************************** */

static bool est_plus_petit(Type *type1, Type *type2)
{
	return type1->taille_octet < type2->taille_octet;
}

static auto inst_llvm_depuis_operateur(OperateurBinaire::Genre genre)
{
	using Genre = OperateurBinaire::Genre;

	switch (genre) {
		case Genre::Addition:            return llvm::Instruction::Add;
		case Genre::Addition_Reel:       return llvm::Instruction::FAdd;
		case Genre::Soustraction:        return llvm::Instruction::Sub;
		case Genre::Soustraction_Reel:   return llvm::Instruction::FSub;
		case Genre::Multiplication:      return llvm::Instruction::Mul;
		case Genre::Multiplication_Reel: return llvm::Instruction::FMul;
		case Genre::Division_Naturel:    return llvm::Instruction::UDiv;
		case Genre::Division_Reel:       return llvm::Instruction::FDiv;
		case Genre::Division_Relatif:    return llvm::Instruction::SDiv;
		case Genre::Reste_Naturel:       return llvm::Instruction::URem;
		case Genre::Reste_Relatif:       return llvm::Instruction::SRem;
		case Genre::Et_Binaire:          return llvm::Instruction::And;
		case Genre::Ou_Binaire:          return llvm::Instruction::Or;
		case Genre::Ou_Exclusif:         return llvm::Instruction::Xor;
		case Genre::Dec_Gauche:          return llvm::Instruction::Shl;
		case Genre::Dec_Droite_Arithm:   return llvm::Instruction::AShr;
		case Genre::Dec_Droite_Logique:  return llvm::Instruction::LShr;
		case Genre::Invalide:
		case Genre::Comp_Egal:
		case Genre::Comp_Inegal:
		case Genre::Comp_Inf:
		case Genre::Comp_Inf_Egal:
		case Genre::Comp_Sup:
		case Genre::Comp_Sup_Egal:
		case Genre::Comp_Inf_Nat:
		case Genre::Comp_Inf_Egal_Nat:
		case Genre::Comp_Sup_Nat:
		case Genre::Comp_Sup_Egal_Nat:
		case Genre::Comp_Egal_Reel:
		case Genre::Comp_Inegal_Reel:
		case Genre::Comp_Inf_Reel:
		case Genre::Comp_Inf_Egal_Reel:
		case Genre::Comp_Sup_Reel:
		case Genre::Comp_Sup_Egal_Reel:
		case Genre::Indexage:
			break;
	}

	return static_cast<llvm::Instruction::BinaryOps>(0);
}

static auto cmp_llvm_depuis_operateur(OperateurBinaire::Genre genre)
{
	using Genre = OperateurBinaire::Genre;

	switch (genre) {
		case Genre::Comp_Egal:          return llvm::CmpInst::Predicate::ICMP_EQ;
		case Genre::Comp_Inegal:        return llvm::CmpInst::Predicate::ICMP_NE;
		case Genre::Comp_Inf:           return llvm::CmpInst::Predicate::ICMP_SLT;
		case Genre::Comp_Inf_Egal:      return llvm::CmpInst::Predicate::ICMP_SLE;
		case Genre::Comp_Sup:           return llvm::CmpInst::Predicate::ICMP_SGT;
		case Genre::Comp_Sup_Egal:      return llvm::CmpInst::Predicate::ICMP_SGE;
		case Genre::Comp_Inf_Nat:       return llvm::CmpInst::Predicate::ICMP_ULT;
		case Genre::Comp_Inf_Egal_Nat:  return llvm::CmpInst::Predicate::ICMP_ULE;
		case Genre::Comp_Sup_Nat:       return llvm::CmpInst::Predicate::ICMP_UGT;
		case Genre::Comp_Sup_Egal_Nat:  return llvm::CmpInst::Predicate::ICMP_UGE;
		case Genre::Comp_Egal_Reel:     return llvm::CmpInst::Predicate::FCMP_OEQ;
		case Genre::Comp_Inegal_Reel:   return llvm::CmpInst::Predicate::FCMP_ONE;
		case Genre::Comp_Inf_Reel:      return llvm::CmpInst::Predicate::FCMP_OLT;
		case Genre::Comp_Inf_Egal_Reel: return llvm::CmpInst::Predicate::FCMP_OLE;
		case Genre::Comp_Sup_Reel:      return llvm::CmpInst::Predicate::FCMP_OGT;
		case Genre::Comp_Sup_Egal_Reel: return llvm::CmpInst::Predicate::FCMP_OGE;
		case Genre::Invalide:
		case Genre::Addition:
		case Genre::Addition_Reel:
		case Genre::Soustraction:
		case Genre::Soustraction_Reel:
		case Genre::Multiplication:
		case Genre::Multiplication_Reel:
		case Genre::Division_Naturel:
		case Genre::Division_Reel:
		case Genre::Division_Relatif:
		case Genre::Reste_Naturel:
		case Genre::Reste_Relatif:
		case Genre::Et_Binaire:
		case Genre::Ou_Binaire:
		case Genre::Ou_Exclusif:
		case Genre::Dec_Gauche:
		case Genre::Dec_Droite_Arithm:
		case Genre::Dec_Droite_Logique:
		case Genre::Indexage:
			break;
	}

	return static_cast<llvm::CmpInst::Predicate>(0);
}

/* ************************************************************************** */

struct GeneratriceCodeLLVM {
	dls::dico<Atome const *, llvm::Value *> table_valeurs{};
	dls::dico<InstructionLabel const *, llvm::BasicBlock *> table_blocs{};
	dls::dico<Atome const *, llvm::GlobalVariable *> table_globales{};
	dls::dico_desordonne<Type *, llvm::Type *> table_types{};
	dls::dico_desordonne<dls::chaine, llvm::Constant *> valeurs_chaines_globales{};
	EspaceDeTravail &m_espace;

	llvm::Function *m_fonction_courante = nullptr;
	llvm::LLVMContext m_contexte_llvm{};
	llvm::Module *m_module = nullptr;
	llvm::IRBuilder<> m_builder;
	llvm::legacy::FunctionPassManager *manager_fonctions = nullptr;

	GeneratriceCodeLLVM(EspaceDeTravail &espace);

	GeneratriceCodeLLVM(GeneratriceCodeLLVM const &) = delete;
	GeneratriceCodeLLVM &operator=(const GeneratriceCodeLLVM &) = delete;

	llvm::Type *converti_type_llvm(Type *type);

	llvm::FunctionType *converti_type_fonction(TypeFonction *type, bool est_externe);

	llvm::Value *genere_code_pour_atome(Atome *atome, bool pour_globale);

	void genere_code_pour_instruction(Instruction const *inst);

	void genere_code();

	llvm::Constant *valeur_pour_chaine(const dls::chaine &chaine, long taille_chaine);
};

GeneratriceCodeLLVM::GeneratriceCodeLLVM(EspaceDeTravail &espace)
	: m_espace(espace)
	, m_builder(m_contexte_llvm)
{}

llvm::Type *GeneratriceCodeLLVM::converti_type_llvm(Type *type)
{
	auto type_llvm = table_types[type];

	if (type_llvm != nullptr) {
		/* Note: normalement les types des pointeurs vers les fonctions doivent
			 * être des pointeurs, mais les types fonctions sont partagés entre les
			 * fonctions et les variables, donc un type venant d'une fonction n'aura
			 * pas le pointeur. Ajoutons-le. */
		if (type->genre == GenreType::FONCTION && !type_llvm->isPointerTy()) {
			return llvm::PointerType::get(type_llvm, 0);
		}

		return type_llvm;
	}

	if (type == nullptr) {
		return nullptr;
	}

	switch (type->genre) {
		case GenreType::POLYMORPHIQUE:
		case GenreType::TUPLE:
		{
			type_llvm = nullptr;
			table_types[type] = type_llvm;
			break;
		}
		case GenreType::FONCTION:
		{
			auto type_fonc = type->comme_fonction();

			std::vector<llvm::Type *> parametres;
			POUR (type_fonc->types_entrees) {
				auto type_llvm_it = converti_type_llvm(it);
				parametres.push_back(type_llvm_it);
			}

			auto type_retour = converti_type_llvm(type_fonc->type_sortie);

			type_llvm = llvm::FunctionType::get(
						type_retour,
						parametres,
						false);

			type_llvm = llvm::PointerType::get(type_llvm, 0);
			break;
		}
		case GenreType::EINI:
		{
			/* type = structure { *z8, *InfoType } */
			auto type_info_type = m_espace.typeuse.type_info_type_;

			std::vector<llvm::Type *> types_membres(2ul);
			types_membres[0] = llvm::Type::getInt8PtrTy(m_contexte_llvm);
			types_membres[1] = converti_type_llvm(type_info_type)->getPointerTo();

			type_llvm = llvm::StructType::create(
						m_contexte_llvm,
						types_membres,
						"struct.eini",
						false);

			break;
		}
		case GenreType::CHAINE:
		{
			/* type = structure { *z8, z64 } */
			std::vector<llvm::Type *> types_membres(2ul);
			types_membres[0] = llvm::Type::getInt8PtrTy(m_contexte_llvm);
			types_membres[1] = llvm::Type::getInt64Ty(m_contexte_llvm);

			type_llvm = llvm::StructType::create(
						m_contexte_llvm,
						types_membres,
						"struct.chaine",
						false);

			break;
		}
		case GenreType::RIEN:
		{
			type_llvm = llvm::Type::getVoidTy(m_contexte_llvm);
			break;
		}
		case GenreType::BOOL:
		{
			type_llvm = llvm::Type::getInt1Ty(m_contexte_llvm);
			break;
		}
		case GenreType::OCTET:
		{
			type_llvm = llvm::Type::getInt8Ty(m_contexte_llvm);
			break;
		}
		case GenreType::ENTIER_CONSTANT:
		{
			type_llvm = llvm::Type::getInt32Ty(m_contexte_llvm);
			break;
		}
		case GenreType::ENTIER_NATUREL:
		case GenreType::ENTIER_RELATIF:
		{
			if (type->taille_octet == 1) {
				type_llvm = llvm::Type::getInt8Ty(m_contexte_llvm);
			}
			else if (type->taille_octet == 2) {
				type_llvm = llvm::Type::getInt16Ty(m_contexte_llvm);
			}
			else if (type->taille_octet == 4) {
				type_llvm = llvm::Type::getInt32Ty(m_contexte_llvm);
			}
			else if (type->taille_octet == 8) {
				type_llvm = llvm::Type::getInt64Ty(m_contexte_llvm);
			}

			break;
		}
		case GenreType::TYPE_DE_DONNEES:
		{
			type_llvm = llvm::Type::getInt64Ty(m_contexte_llvm);
			break;
		}
		case GenreType::REEL:
		{
			if (type->taille_octet == 2) {
				type_llvm = llvm::Type::getInt16Ty(m_contexte_llvm);
			}
			else if (type->taille_octet == 4) {
				type_llvm = llvm::Type::getFloatTy(m_contexte_llvm);
			}
			else if (type->taille_octet == 8) {
				type_llvm = llvm::Type::getDoubleTy(m_contexte_llvm);
			}

			break;
		}
		case GenreType::REFERENCE:
		case GenreType::POINTEUR:
		{
			auto type_deref = type_dereference_pour(type);

			// Les pointeurs vers rien (void) ne sont pas valides avec LLVM
			// @Incomplet : LLVM n'a pas de pointeur nul
			if (!type_deref || type_deref->genre == GenreType::RIEN) {
				type_deref = m_espace.typeuse[TypeBase::Z8];
			}

			auto type_deref_llvm = converti_type_llvm(type_deref);
			type_llvm = llvm::PointerType::get(type_deref_llvm, 0);
			break;
		}
		case GenreType::UNION:
		{
			auto type_struct = type->comme_union();
			auto decl_struct = type_struct->decl;
			auto nom_nonsur = "union_nonsure." + type_struct->nom->nom;
			auto nom = "union." + type_struct->nom->nom;

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

			auto type_max_llvm = converti_type_llvm(type_max);
			auto type_union = llvm::StructType::create(m_contexte_llvm, { type_max_llvm }, nom_nonsur.c_str());

			if (!decl_struct->est_nonsure) {
				// création d'une structure contenant l'union et une valeur discriminante
				type_union = llvm::StructType::create(m_contexte_llvm, { type_union, llvm::Type::getInt32Ty(m_contexte_llvm) }, nom.c_str());
			}

			type_llvm = type_union;
			break;
		}
		case GenreType::STRUCTURE:
		{
			auto type_struct = type->comme_structure();
			auto nom = "struct." + type_struct->nom->nom;

			/* Pour les structures récursives, il faut créer un type
			 * opaque, dont le corps sera renseigné à la fin */
			auto type_opaque = llvm::StructType::create(m_contexte_llvm, nom.c_str());
			table_types[type] = type_opaque;

			std::vector<llvm::Type *> types_membres;
			types_membres.reserve(static_cast<size_t>(type_struct->membres.taille));

			POUR (type_struct->membres) {
				types_membres.push_back(converti_type_llvm(it.type));
			}

			type_opaque->setBody(types_membres, false);

			/* retourne directement puisque le type a déjà été ajouté à la table de types */
			return type_opaque;
		}
		case GenreType::VARIADIQUE:
		{
			auto type_var = type->comme_variadique();

			// Utilise le type de tableau dynamique afin que le code IR LLVM
			// soit correcte (pointe vers le même type)
			if (type_var->type_pointe != nullptr) {
				auto type_tabl = m_espace.typeuse.type_tableau_dynamique(type_var->type_pointe);
				type_llvm = converti_type_llvm(type_tabl);
			}

			break;
		}
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			auto type_deref_llvm = converti_type_llvm(type_dereference_pour(type));

			/* type = structure { *type, n64, n64 } */
			std::vector<llvm::Type *> types_membres(3ul);
			types_membres[0] = llvm::PointerType::get(type_deref_llvm, 0);
			types_membres[1] = llvm::Type::getInt64Ty(m_contexte_llvm);
			types_membres[2] = llvm::Type::getInt64Ty(m_contexte_llvm);

			type_llvm = llvm::StructType::create(
						m_contexte_llvm,
						types_membres,
						"struct.tableau",
						false);
			break;
		}
		case GenreType::TABLEAU_FIXE:
		{
			auto type_deref_llvm = converti_type_llvm(type_dereference_pour(type));
			auto const taille = type->comme_tableau_fixe()->taille;

			type_llvm = llvm::ArrayType::get(type_deref_llvm, static_cast<unsigned long>(taille));
			break;
		}
		case GenreType::ENUM:
		case GenreType::ERREUR:
		{
			auto type_enum = type->comme_enum();
			type_llvm = converti_type_llvm(type_enum->type_donnees);
			break;
		}
		case GenreType::OPAQUE:
		{
			auto type_opaque = type->comme_opaque();
			type_llvm = converti_type_llvm(type_opaque->type_opacifie);
			break;
		}
	}

	table_types[type] = type_llvm;
	return type_llvm;
}

llvm::FunctionType *GeneratriceCodeLLVM::converti_type_fonction(TypeFonction *type, bool est_externe)
{
	std::vector<llvm::Type *> parametres;
	parametres.reserve(static_cast<size_t>(type->types_entrees.taille));

	auto est_variadique = false;

	POUR (type->types_entrees) {
		if (it->genre == GenreType::VARIADIQUE) {
			est_variadique = true;

			/* les arguments variadiques sont transformés en un tableau */
			if (!est_externe) {
				auto type_var = it->comme_variadique();
				auto type_tabl = m_espace.typeuse.type_tableau_dynamique(type_var->type_pointe);
				parametres.push_back(converti_type_llvm(type_tabl));
			}

			break;
		}

		parametres.push_back(converti_type_llvm(it));
	}

	return llvm::FunctionType::get(
				converti_type_llvm(type->type_sortie),
				parametres,
				est_variadique && est_externe);
}

llvm::Value *GeneratriceCodeLLVM::genere_code_pour_atome(Atome *atome, bool pour_globale)
{
	switch (atome->genre_atome) {
		case Atome::Genre::FONCTION:
		{
			auto atome_fonc = static_cast<AtomeFonction const *>(atome);
			return m_module->getFunction(llvm::StringRef(atome_fonc->nom.c_str()));
		}
		case Atome::Genre::CONSTANTE:
		{
			auto atome_const = static_cast<AtomeConstante const *>(atome);
			auto type_llvm = converti_type_llvm(atome_const->type);

			switch (atome_const->genre) {
				case AtomeConstante::Genre::GLOBALE:
				{
					auto valeur_globale = static_cast<AtomeGlobale const *>(atome);

					if (valeur_globale->ident) {
						return table_globales[valeur_globale];
					}

					return table_valeurs[valeur_globale];
				}
				case AtomeConstante::Genre::TRANSTYPE_CONSTANT:
				{
					auto transtype_const = static_cast<TranstypeConstant const *>(atome_const);
					auto valeur = genere_code_pour_atome(transtype_const->valeur, pour_globale);
					return llvm::ConstantExpr::getBitCast(llvm::cast<llvm::Constant>(valeur), type_llvm);
				}
				case AtomeConstante::Genre::OP_UNAIRE_CONSTANTE:
				{
					break;
				}
				case AtomeConstante::Genre::OP_BINAIRE_CONSTANTE:
				{
					break;
				}
				case AtomeConstante::Genre::ACCES_INDEX_CONSTANT:
				{
					auto acces = static_cast<AccedeIndexConstant const *>(atome_const);
					auto index = genere_code_pour_atome(acces->index, pour_globale);
					assert(index);
					auto accede = genere_code_pour_atome(acces->accede, pour_globale);
					assert(accede);

					if (acces->accede->est_globale()) {
						auto globale = static_cast<AtomeGlobale *>(acces->accede);

						auto init = genere_code_pour_atome(globale, pour_globale);
						assert(init);
						auto globale_llvm = table_globales[globale];

						globale_llvm->setInitializer(llvm::cast<llvm::Constant>(init));
					}

					auto index_array = std::vector<llvm::Value *>(2);
					index_array[0] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_contexte_llvm), 0);
					index_array[1] = index;

					assert(type_llvm);

					return llvm::ConstantExpr::getGetElementPtr(type_llvm, llvm::cast<llvm::Constant>(accede), index_array);
				}
				case AtomeConstante::Genre::VALEUR:
				{
					auto valeur_const = static_cast<AtomeValeurConstante const *>(atome);

					switch (valeur_const->valeur.genre) {
						case AtomeValeurConstante::Valeur::Genre::NULLE:
						{
							return llvm::ConstantPointerNull::get(static_cast<llvm::PointerType *>(converti_type_llvm(valeur_const->type)));
						}
						case AtomeValeurConstante::Valeur::Genre::REELLE:
						{
							return llvm::ConstantFP::get(type_llvm, valeur_const->valeur.valeur_reelle);
						}
						case AtomeValeurConstante::Valeur::Genre::TYPE:
						{
							return llvm::ConstantInt::get(type_llvm, valeur_const->valeur.type->index_dans_table_types);
						}
						case AtomeValeurConstante::Valeur::Genre::ENTIERE:
						{
							return llvm::ConstantInt::get(type_llvm, valeur_const->valeur.valeur_entiere);
						}
						case AtomeValeurConstante::Valeur::Genre::BOOLEENNE:
						{
							return llvm::ConstantInt::get(type_llvm, valeur_const->valeur.valeur_booleenne);
						}
						case AtomeValeurConstante::Valeur::Genre::CARACTERE:
						{
							return llvm::ConstantInt::get(type_llvm, valeur_const->valeur.valeur_entiere);
						}
						case AtomeValeurConstante::Valeur::Genre::INDEFINIE:
						{
							return nullptr;
						}
						case AtomeValeurConstante::Valeur::Genre::STRUCTURE:
						{
							auto type = static_cast<TypeCompose *>(atome->type);
							auto tableau_valeur = valeur_const->valeur.valeur_structure.pointeur;

							auto tableau_membre = std::vector<llvm::Constant *>();

							auto index_membre = 0;
							for (auto i = 0; i < type->membres.taille; ++i) {
								if (type->membres[i].drapeaux & TypeCompose::Membre::EST_CONSTANT) {
									continue;
								}

								auto valeur = static_cast<llvm::Constant *>(nullptr);

								// les tableaux fixes ont une initialisation nulle
								if (tableau_valeur[index_membre] == nullptr) {
									auto type_llvm_valeur = converti_type_llvm(type->membres[i].type);
									valeur = llvm::ConstantAggregateZero::get(type_llvm_valeur);
								}
								else {
									std::cerr << "Génère code pour le membre " << type->membres[i].nom->nom << '\n';
									valeur = llvm::cast<llvm::Constant>(genere_code_pour_atome(tableau_valeur[index_membre], pour_globale));
								}

								tableau_membre.push_back(valeur);
								index_membre += 1;
							}

							return llvm::ConstantStruct::get(
										llvm::cast<llvm::StructType>(converti_type_llvm(type)),
										tableau_membre);
						}
						case AtomeValeurConstante::Valeur::Genre::TABLEAU_FIXE:
						{
							// À FAIRE(tableau fixe)
							return nullptr;
						}
						case AtomeValeurConstante::Valeur::Genre::TABLEAU_DONNEES_CONSTANTES:
						{
							auto pointeur_donnnees = valeur_const->valeur.valeur_tdc.pointeur;
							auto taille_donnees = valeur_const->valeur.valeur_tdc.taille;

							std::vector<unsigned char> donnees;
							donnees.resize(static_cast<size_t>(taille_donnees));

							for (auto i = 0; i < taille_donnees; ++i) {
								donnees[static_cast<size_t>(i)] = static_cast<unsigned char>(pointeur_donnnees[i]);
							}

							return llvm::ConstantDataArray::get(m_contexte_llvm, donnees);
						}
					}
				}
			}

			return nullptr;
		}
		case Atome::Genre::INSTRUCTION:
		{
			auto inst = atome->comme_instruction();
			return table_valeurs[inst];
		}
		case Atome::Genre::GLOBALE:
		{
			return table_globales[atome];
		}
	}

	return nullptr;
}

void GeneratriceCodeLLVM::genere_code_pour_instruction(const Instruction *inst)
{
	switch (inst->genre) {
		case Instruction::Genre::INVALIDE:
		{
			break;
		}
		case Instruction::Genre::ALLOCATION:
		{
			auto type_pointeur = inst->type->comme_pointeur();
			auto type_llvm = converti_type_llvm(type_pointeur->type_pointe);
			auto alloca = m_builder.CreateAlloca(type_llvm, 0u);
			alloca->setAlignment(type_pointeur->type_pointe->alignement);
			table_valeurs[inst] = alloca;
			break;
		}
		case Instruction::Genre::APPEL:
		{
			auto inst_appel = inst->comme_appel();

//			auto const &lexeme = inst_appel->lexeme;
//			auto fichier = m_contexte.fichier(static_cast<size_t>(lexeme->fichier));
//			auto pos = position_lexeme(*lexeme);

//			if (!m_fonction_courante->sanstrace) {
//				os << "  DEBUTE_RECORD_TRACE_APPEL(";
//				os << pos.numero_ligne << ",";
//				os << pos.pos << ",";
//				os << "\"";

//				auto ligne = fichier->tampon[pos.index_ligne];

//				POUR (ligne) {
//					if (it == '\n') {
//						continue;
//					}

//					if (it == '"') {
//						os << '\\';
//					}

//					os << it;
//				}

//				os << "\",";
//				os << ligne.taille();
//				os << ");\n";
//			}

			auto arguments = std::vector<llvm::Value *>();

			POUR (inst_appel->args) {
				arguments.push_back(genere_code_pour_atome(it, false));
			}

			auto valeur_fonction = genere_code_pour_atome(inst_appel->appele, false);
			table_valeurs[inst] = m_builder.CreateCall(valeur_fonction, arguments);

//			if (!m_fonction_courante->sanstrace) {
//				os << "  TERMINE_RECORD_TRACE_APPEL;\n";
//			}

			break;
		}
		case Instruction::Genre::BRANCHE:
		{
			auto inst_branche = inst->comme_branche();
			m_builder.CreateBr(table_blocs[inst_branche->label]);
			break;
		}
		case Instruction::Genre::BRANCHE_CONDITION:
		{
			auto inst_branche = inst->comme_branche_cond();
			auto condition = genere_code_pour_atome(inst_branche->condition, false);
			auto bloc_si_vrai = table_blocs[inst_branche->label_si_vrai];
			auto bloc_si_faux = table_blocs[inst_branche->label_si_faux];
			m_builder.CreateCondBr(condition, bloc_si_vrai, bloc_si_faux);
			break;
		}
		case Instruction::Genre::CHARGE_MEMOIRE:
		{
			auto inst_charge = inst->comme_charge();
			auto charge = inst_charge->chargee;
			auto valeur = static_cast<llvm::Value *>(nullptr);

			if (charge->genre_atome == Atome::Genre::INSTRUCTION) {
				valeur = table_valeurs[charge];
			}
			else {
				valeur = table_globales[charge];
			}

			assert(valeur != nullptr);

			auto load = m_builder.CreateLoad(valeur, "");
			load->setAlignment(charge->type->alignement);
			table_valeurs[inst_charge] = load;
			break;
		}
		case Instruction::Genre::STOCKE_MEMOIRE:
		{
			auto inst_stocke = inst->comme_stocke_mem();
			auto valeur = genere_code_pour_atome(inst_stocke->valeur, false);
			auto ou = inst_stocke->ou;
			auto valeur_ou = static_cast<llvm::Value *>(nullptr);

			if (ou->genre_atome == Atome::Genre::INSTRUCTION) {
				valeur_ou = table_valeurs[ou];
			}
			else {
				valeur_ou = table_globales[ou];
			}

			m_builder.CreateStore(valeur, valeur_ou);

			break;
		}
		case Instruction::Genre::LABEL:
		{
			auto inst_label = inst->comme_label();
			auto bloc = llvm::BasicBlock::Create(m_contexte_llvm, "", m_fonction_courante);
			table_blocs[inst_label] = bloc;
			//m_builder.SetInsertPoint(bloc);
			break;
		}
		case Instruction::Genre::OPERATION_UNAIRE:
		{
			auto inst_un = inst->comme_op_unaire();
			auto valeur = genere_code_pour_atome(inst_un->valeur, false);
			auto type = inst_un->valeur->type;

			switch (inst_un->op) {
				case OperateurUnaire::Genre::Positif:
				{
					valeur = m_builder.CreateLoad(valeur, "");
					break;
				}
				case OperateurUnaire::Genre::Invalide:
				{
					break;
				}
				case OperateurUnaire::Genre::Complement:
				{
					auto type_llvm = converti_type_llvm(inst_un->valeur->type);
					if (est_type_entier(type)) {
						auto zero = llvm::ConstantInt::get(type_llvm, 0, type->genre == GenreType::ENTIER_RELATIF);
						valeur = m_builder.CreateSub(zero, valeur);
					}
					else {
						auto zero = llvm::ConstantFP::get(type_llvm, 0.0);
						valeur = m_builder.CreateFSub(zero, valeur);
					}
					break;
				}
				case OperateurUnaire::Genre::Non_Binaire:
				{
					auto type_llvm = converti_type_llvm(inst_un->valeur->type);
					valeur = m_builder.CreateXor(valeur, llvm::ConstantInt::get(type_llvm, 0, type->genre == GenreType::ENTIER_RELATIF));
					break;
				}
				case OperateurUnaire::Genre::Non_Logique:
				{
					auto valeur2 = m_builder.getInt1(0);
					valeur = m_builder.CreateICmpEQ(valeur, valeur2);
					valeur = m_builder.CreateXor(valeur, m_builder.getInt1(1));
					break;
				}
				case OperateurUnaire::Genre::Prise_Adresse:
				{
					break;
				}
			}

			table_valeurs[inst] = valeur;
			break;
		}
		case Instruction::Genre::OPERATION_BINAIRE:
		{
			auto inst_bin = inst->comme_op_binaire();
			auto valeur_gauche = genere_code_pour_atome(inst_bin->valeur_gauche, false);
			auto valeur_droite = genere_code_pour_atome(inst_bin->valeur_droite, false);

			if (inst_bin->op >= OperateurBinaire::Genre::Comp_Egal && inst_bin->op <= OperateurBinaire::Genre::Comp_Sup_Egal_Nat) {
				auto cmp = cmp_llvm_depuis_operateur(inst_bin->op);
				table_valeurs[inst] = m_builder.CreateICmp(cmp, valeur_gauche, valeur_droite);
			}
			else if (inst_bin->op >= OperateurBinaire::Genre::Comp_Egal_Reel && inst_bin->op <= OperateurBinaire::Genre::Comp_Sup_Egal_Reel) {
				auto cmp = cmp_llvm_depuis_operateur(inst_bin->op);
				table_valeurs[inst] = m_builder.CreateFCmp(cmp, valeur_gauche, valeur_droite);
			}
			else {
				auto inst_llvm = inst_llvm_depuis_operateur(inst_bin->op);
				table_valeurs[inst] = m_builder.CreateBinOp(inst_llvm, valeur_gauche, valeur_droite);
			}

#if 0
			auto expr = static_cast<NoeudExpressionBinaire *>(b);
			auto enfant1 = expr->expr1;
			auto enfant2 = expr->expr2;
			auto type1 = enfant1->type;
			auto type2 = enfant2->type;
			auto op = expr->op;

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
#endif

			break;
		}
		case Instruction::Genre::RETOUR:
		{
			auto inst_retour = inst->comme_retour();
			if (inst_retour->valeur != nullptr) {
				auto atome = inst_retour->valeur;
				auto valeur_retour = genere_code_pour_atome(atome, false);
				m_builder.CreateRet(valeur_retour);
			}
			else {
				m_builder.CreateRet(nullptr);
			}
			break;
		}
		case Instruction::Genre::ACCEDE_INDEX:
		{
			auto inst_acces = inst->comme_acces_index();
			auto valeur_accede = genere_code_pour_atome(inst_acces->accede, false);
			auto valeur_index = genere_code_pour_atome(inst_acces->index, false);

			auto index = std::vector<llvm::Value *>(2);
			index[0] = m_builder.getInt64(0);
			index[1] = valeur_index;

			table_valeurs[inst] = m_builder.CreateGEP(valeur_accede, index);
			break;
		}
		case Instruction::Genre::ACCEDE_MEMBRE:
		{
			auto inst_acces = inst->comme_acces_membre();

			auto accede = inst_acces->accede;
			auto valeur_accede = genere_code_pour_atome(accede, false);

			auto index_membre = static_cast<AtomeValeurConstante *>(inst_acces->index)->valeur.valeur_entiere;

			if (accede->genre_atome == Atome::Genre::INSTRUCTION) {
				auto index = std::vector<llvm::Value *>(2);
				index[0] = m_builder.getInt32(0);
				index[1] = m_builder.getInt32(static_cast<unsigned int>(index_membre));

				auto valeur_membre = m_builder.CreateInBoundsGEP(valeur_accede, index);
				table_valeurs[inst] = valeur_membre;
			}
			else {
				valeur_accede = table_globales[accede];

				auto index = std::vector<llvm::Value *>(1);
				index[0] = m_builder.getInt32(static_cast<unsigned int>(index_membre));

				table_valeurs[inst] = m_builder.CreateGEP(valeur_accede, index);
			}

			// À FAIRE : type union

			break;
		}
		case Instruction::Genre::TRANSTYPE:
		{
			using CastOps = llvm::Instruction::CastOps;

			auto inst_transtype = inst->comme_transtype();
			auto valeur = genere_code_pour_atome(inst_transtype->valeur, false);
			auto type_de = inst_transtype->valeur->type;
			auto type_vers = inst_transtype->type;
			auto type_llvm = converti_type_llvm(type_vers);
			auto resultat = static_cast<llvm::Value *>(nullptr);

			if (est_type_entier(type_de) || type_de->genre == GenreType::ENTIER_CONSTANT) {
				if (type_vers->genre == GenreType::POINTEUR) {
					resultat = m_builder.CreateCast(CastOps::IntToPtr, valeur, type_llvm);
				}
				else if (type_vers->genre == GenreType::REEL) {
					if (type_de->genre == GenreType::ENTIER_NATUREL) {
						resultat = m_builder.CreateCast(CastOps::UIToFP, valeur, type_llvm);
					}

					resultat = m_builder.CreateCast(CastOps::SIToFP, valeur, type_llvm);
				}
				else if (est_type_entier(type_vers)) {
					if (est_plus_petit(type_vers, type_de)) {
						resultat = m_builder.CreateCast(CastOps::Trunc, valeur, type_llvm);
					}
					else if (type_vers->taille_octet == type_de->taille_octet) {
						resultat = valeur;
					}
					else if (type_vers->genre == GenreType::ENTIER_NATUREL) {
						resultat = m_builder.CreateCast(CastOps::ZExt, valeur, type_llvm);
					}
					else {
						resultat = m_builder.CreateCast(CastOps::SExt, valeur, type_llvm);
					}
				}
			}
			else if (type_de->genre == GenreType::REEL) {
				if (type_vers->genre == GenreType::ENTIER_NATUREL) {
					resultat = m_builder.CreateCast(CastOps::FPToUI, valeur, type_llvm);
				}
				else if (type_vers->genre == GenreType::ENTIER_RELATIF) {
					resultat = m_builder.CreateCast(CastOps::FPToSI, valeur, type_llvm);
				}
				else if (type_vers->genre == GenreType::REEL) {
					if (est_plus_petit(type_vers, type_de)) {
						resultat = m_builder.CreateCast(CastOps::FPTrunc, valeur, type_llvm);
					}
					else {
						resultat = m_builder.CreateCast(CastOps::FPExt, valeur, type_llvm);
					}
				}
			}
			else if (type_de->genre == GenreType::POINTEUR && est_type_entier(type_vers)) {
				resultat = m_builder.CreateCast(CastOps::PtrToInt, valeur, type_llvm);
			}
			else {
				resultat = m_builder.CreateCast(CastOps::BitCast, valeur, type_llvm);
			}

			table_valeurs[inst] = resultat;

			break;
		}
	}
}

void GeneratriceCodeLLVM::genere_code()
{
	m_espace.typeuse.construit_table_types();

	POUR_TABLEAU_PAGE (m_espace.globales) {
		auto valeur_globale = &it;

		auto type = valeur_globale->type->comme_pointeur()->type_pointe;
		auto type_llvm = converti_type_llvm(type);

		auto nom_globale = llvm::StringRef();

		if (valeur_globale->ident) {
			nom_globale = llvm::StringRef(valeur_globale->ident->nom.pointeur(), static_cast<size_t>(valeur_globale->ident->nom.taille()));
		}

		auto globale = new llvm::GlobalVariable(
					*m_module,
					type_llvm,
					valeur_globale->est_constante,
					valeur_globale->est_externe ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::InternalLinkage,
					nullptr,
					nom_globale);

		table_globales[valeur_globale] = globale;
	}

	POUR_TABLEAU_PAGE (m_espace.globales) {
		std::cerr << "génère code pour globale " << it.ident << ' ' << chaine_type(it.type) << '\n';
		auto valeur_globale = &it;

		if (valeur_globale->initialisateur) {
			auto valeur_initialisateur = static_cast<llvm::Constant *>(genere_code_pour_atome(valeur_globale->initialisateur, true));
			auto globale = table_globales[valeur_globale];
			globale->setInitializer(valeur_initialisateur);
		}
	}

	POUR_TABLEAU_PAGE (m_espace.fonctions) {
		auto atome_fonc = &it;

		auto type_fonction = atome_fonc->type->comme_fonction();
		auto type_llvm = converti_type_fonction(
					type_fonction,
					atome_fonc->instructions.taille == 0);

		llvm::Function::Create(
					type_llvm,
					llvm::Function::ExternalLinkage,
					atome_fonc->nom.c_str(),
					m_module);
	}

	POUR_TABLEAU_PAGE (m_espace.fonctions) {
		auto atome_fonc = &it;
		table_valeurs.efface();
		table_blocs.efface();

		if (atome_fonc->est_externe) {
			continue;
		}

		//std::cerr << "Génère code pour : " << atome_fonc->nom << '\n';

		auto fonction = m_module->getFunction(atome_fonc->nom.c_str());

		m_fonction_courante = fonction;
		table_valeurs.efface();
		table_blocs.efface();

		/* génère d'abord tous les blocs depuis les labels */
		for (auto inst : atome_fonc->instructions) {
			if (inst->genre != Instruction::Genre::LABEL) {
				continue;
			}

			genere_code_pour_instruction(inst);
		}

		auto bloc_entree = table_blocs[atome_fonc->instructions[0]->comme_label()];
		m_builder.SetInsertPoint(bloc_entree);

		auto valeurs_args = fonction->arg_begin();

		for (auto &param : atome_fonc->params_entrees) {
			auto const &nom_argument = param->ident->nom;

			auto valeur = &(*valeurs_args++);
			valeur->setName(dls::chaine(nom_argument).c_str());

			auto type = param->type->comme_pointeur()->type_pointe;
			auto type_llvm = converti_type_llvm(type);

			auto alloc = m_builder.CreateAlloca(type_llvm, 0u);
			alloc->setAlignment(type->alignement);

			auto store = m_builder.CreateStore(valeur, alloc);
			store->setAlignment(type->alignement);

			table_valeurs[param] = alloc;
		}

//		if (!atome_fonc->sanstrace) {
//			os << "INITIALISE_TRACE_APPEL(\"";

//			if (atome_fonc->lexeme != nullptr) {
//				auto fichier = m_contexte.fichier(static_cast<size_t>(atome_fonc->lexeme->fichier));
//				os << atome_fonc->lexeme->chaine << "\", "
//				   << atome_fonc->lexeme->chaine.taille() << ", \""
//				   << fichier->nom << ".kuri\", "
//				   << fichier->nom.taille() + 5 << ", ";
//			}
//			else {
//				os << atome_fonc->nom << "\", "
//				   << atome_fonc->nom.taille() << ", "
//				   << "\"???\", 3, ";
//			}

//			os << atome_fonc->nom << ");\n";
//		}

		for (auto inst : atome_fonc->instructions) {
			if (inst->genre == Instruction::Genre::LABEL) {
				auto bloc = table_blocs[inst->comme_label()];
				m_builder.SetInsertPoint(bloc);
				continue;
			}

			genere_code_pour_instruction(inst);
		}

		if (manager_fonctions) {
			manager_fonctions->run(*m_fonction_courante);
		}

		m_fonction_courante = nullptr;
	}
}

llvm::Constant *GeneratriceCodeLLVM::valeur_pour_chaine(const dls::chaine &chaine, long taille_chaine)
{
	auto iter = valeurs_chaines_globales.trouve(chaine);

	if (iter != valeurs_chaines_globales.fin()) {
		return iter->second;
	}

	// @.chn [N x i8] c"...0"
	auto type_tableau = m_espace.typeuse.type_tableau_fixe(m_espace.typeuse[TypeBase::Z8], taille_chaine + 1);

	auto constante = llvm::ConstantDataArray::getString(
				m_contexte_llvm,
				chaine.c_str());

	auto tableu_chaine_c = new llvm::GlobalVariable(
				*m_module,
				converti_type_llvm(type_tableau),
				true,
				llvm::GlobalValue::PrivateLinkage,
				constante,
				".chn");

	// prend le pointeur vers le premier élément.
	auto idx = std::vector<llvm::Constant *>{
			llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_contexte_llvm), 0),
			llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_contexte_llvm), 0)};

	auto pointeur_chaine_c = llvm::ConstantExpr::getGetElementPtr(
				converti_type_llvm(type_tableau),
				tableu_chaine_c,
				idx,
				true);

	auto valeur_taille = llvm::ConstantInt::get(
				llvm::Type::getInt64Ty(m_contexte_llvm),
				static_cast<uint64_t>(chaine.taille()),
				false);

	auto type_chaine = converti_type_llvm(m_espace.typeuse[TypeBase::CHAINE]);

	auto struct_chaine = llvm::ConstantStruct::get(
				static_cast<llvm::StructType *>(type_chaine),
				pointeur_chaine_c,
				valeur_taille);

	valeurs_chaines_globales.insere({ chaine, struct_chaine });

	return struct_chaine;
}

bool initialise_llvm()
{
	if (llvm::InitializeNativeTarget()
			|| llvm::InitializeNativeTargetAsmParser()
			|| llvm::InitializeNativeTargetAsmPrinter())
	{
		std::cerr << "Ne peut pas initialiser LLVM !\n";
		return false;
	}

	/* Initialise les passes. */
	auto &registre = *llvm::PassRegistry::getPassRegistry();
	llvm::initializeCore(registre);
	llvm::initializeScalarOpts(registre);
	llvm::initializeObjCARCOpts(registre);
	llvm::initializeVectorization(registre);
	llvm::initializeIPO(registre);
	llvm::initializeAnalysis(registre);
	llvm::initializeTransformUtils(registre);
	llvm::initializeInstCombine(registre);
	llvm::initializeInstrumentation(registre);
	llvm::initializeTarget(registre);

	/* Pour les passes de transformation de code, seuls celles d'IR à IR sont
	 * supportées. */
	llvm::initializeCodeGenPreparePass(registre);
	llvm::initializeAtomicExpandPass(registre);
	llvm::initializeWinEHPreparePass(registre);
	llvm::initializeDwarfEHPreparePass(registre);
	llvm::initializeSjLjEHPreparePass(registre);
	llvm::initializePreISelIntrinsicLoweringLegacyPassPass(registre);
	llvm::initializeGlobalMergePass(registre);
	llvm::initializeInterleavedAccessPass(registre);
	llvm::initializeUnreachableBlockElimLegacyPassPass(registre);

	return true;
}

void issitialise_llvm()
{
	llvm::llvm_shutdown();
}

/**
 * Ajoute les passes d'optimisation au ménageur en fonction du niveau
 * d'optimisation.
 */
static void ajoute_passes(
		llvm::legacy::FunctionPassManager &manager_fonctions,
		uint niveau_optimisation,
		uint niveau_taille)
{
	llvm::PassManagerBuilder builder;
	builder.OptLevel = niveau_optimisation;
	builder.SizeLevel = niveau_taille;
	builder.DisableUnrollLoops = (niveau_optimisation == 0);

	/* Pour plus d'informations sur les vectoriseurs, suivre le lien :
	 * http://llvm.org/docs/Vectorizers.html */
	builder.LoopVectorize = (niveau_optimisation > 1 && niveau_taille < 2);
	builder.SLPVectorize = (niveau_optimisation > 1 && niveau_taille < 2);

	builder.populateFunctionPassManager(manager_fonctions);
}

/**
 * Initialise le ménageur de passes fonctions du contexte selon le niveau
 * d'optimisation.
 */
static void initialise_optimisation(
		NiveauOptimisation optimisation,
		GeneratriceCodeLLVM &contexte)
{
	if (contexte.manager_fonctions == nullptr) {
		contexte.manager_fonctions = new llvm::legacy::FunctionPassManager(contexte.m_module);
	}

	switch (optimisation) {
		case NiveauOptimisation::AUCUN:
			break;
		case NiveauOptimisation::O0:
			ajoute_passes(*contexte.manager_fonctions, 0, 0);
			break;
		case NiveauOptimisation::O1:
			ajoute_passes(*contexte.manager_fonctions, 1, 0);
			break;
		case NiveauOptimisation::O2:
			ajoute_passes(*contexte.manager_fonctions, 2, 0);
			break;
		case NiveauOptimisation::Os:
			ajoute_passes(*contexte.manager_fonctions, 2, 1);
			break;
		case NiveauOptimisation::Oz:
			ajoute_passes(*contexte.manager_fonctions, 2, 2);
			break;
		case NiveauOptimisation::O3:
			ajoute_passes(*contexte.manager_fonctions, 3, 0);
			break;
	}
}

static bool ecris_fichier_objet(llvm::TargetMachine *machine_cible, llvm::Module &module)
{
#if 1
	auto chemin_sortie = "/tmp/kuri.o";
	std::error_code ec;

	llvm::raw_fd_ostream dest(chemin_sortie, ec, llvm::sys::fs::F_None);

	if (ec) {
		std::cerr << "Ne put pas ouvrir le fichier '" << chemin_sortie << "'\n";
		return false;
	}

	llvm::legacy::PassManager pass;
	auto type_fichier = llvm::TargetMachine::CGFT_ObjectFile;

	if (machine_cible->addPassesToEmitFile(pass, dest, type_fichier)) {
		std::cerr << "La machine cible ne peut pas émettre ce type de fichier\n";
		return false;
	}

	pass.run(module);
	dest.flush();
#else
	// https://stackoverflow.com/questions/1419139/llvm-linking-problem?rq=1
	std::error_code ec;
	llvm::raw_fd_ostream dest("/tmp/kuri.ll", ec, llvm::sys::fs::F_None);
	module.print(dest, nullptr);

	system("llvm-as /tmp/kuri.ll -o /tmp/kuri.bc");
	system("llc /tmp/kuri.bc -o /tmp/kuri.s");
#endif
	return true;
}

#ifndef NDEBUG
static bool valide_llvm_ir(llvm::Module &module)
{
	std::error_code ec;
	llvm::raw_fd_ostream dest("/tmp/kuri.ll", ec, llvm::sys::fs::F_None);
	module.print(dest, nullptr);

	auto err = system("llvm-as /tmp/kuri.ll -o /tmp/kuri.bc");
	return err == 0;
}
#endif

static bool cree_executable(const kuri::chaine &dest, const std::filesystem::path &racine_kuri)
{
	/* Compile le fichier objet qui appelera 'fonction principale'. */
	if (!std::filesystem::exists("/tmp/execution_kuri.o")) {
		auto const &chemin_execution_S = racine_kuri / "fichiers/execution_kuri.S";

		dls::flux_chaine ss;
		ss << "as -o /tmp/execution_kuri.o ";
		ss << chemin_execution_S;

		auto err = system(ss.chn().c_str());

		if (err != 0) {
			std::cerr << "Ne peut pas créer /tmp/execution_kuri.o !\n";
			return false;
		}
	}

	if (!std::filesystem::exists("/tmp/kuri.o")) {
		std::cerr << "Le fichier objet n'a pas été émis !\n Utiliser la commande -o !\n";
		return false;
	}

	dls::flux_chaine ss;
#if 1
	ss << "gcc ";
	ss << racine_kuri / "fichiers/point_d_entree.c";
	ss << " /tmp/kuri.o /tmp/r16_tables.o -o " << dest;
#else
	ss << "ld ";
	/* ce qui chargera le programme */
	ss << "-dynamic-linker /lib64/ld-linux-x86-64.so.2 ";
	ss << "-m elf_x86_64 ";
	ss << "--hash-style=gnu ";
	ss << "-lc ";
	ss << "/tmp/execution_kuri.o ";
	ss << "/tmp/kuri.o ";
	ss << "-o " << dest;
#endif

	auto err = system(ss.chn().c_str());

	if (err != 0) {
		std::cerr << "Ne peut pas créer l'executable !\n";
		return false;
	}

	return true;
}

bool CoulisseLLVM::cree_fichier_objet(Compilatrice &compilatrice, EspaceDeTravail &espace, ConstructriceRI &/*constructrice_ri*/)
{
	auto const triplet_cible = llvm::sys::getDefaultTargetTriple();

	if (!initialise_llvm()) {
		return false;
	}

	auto erreur = std::string{""};
	auto cible = llvm::TargetRegistry::lookupTarget(triplet_cible, erreur);

	if (!cible) {
		std::cerr << erreur << '\n';
		return 1;
	}

	auto CPU = "generic";
	auto feature = "";
	auto options_cible = llvm::TargetOptions{};
	auto RM = llvm::Optional<llvm::Reloc::Model>(llvm::Reloc::PIC_);
	auto machine_cible = std::unique_ptr<llvm::TargetMachine>(
				cible->createTargetMachine(
					triplet_cible, CPU, feature, options_cible, RM));

	auto generatrice = GeneratriceCodeLLVM(espace);

	auto module_llvm = llvm::Module("Module", generatrice.m_contexte_llvm);
	module_llvm.setDataLayout(machine_cible->createDataLayout());
	module_llvm.setTargetTriple(triplet_cible);

	generatrice.m_module = &module_llvm;

	std::cout << "Génération du code..." << std::endl;
	auto debut_generation_code = dls::chrono::compte_seconde();

	initialise_optimisation(espace.options.niveau_optimisation, generatrice);

	generatrice.genere_code();

	delete generatrice.manager_fonctions;

	temps_generation_code = debut_generation_code.temps();

#ifndef NDEBUG
	if (!valide_llvm_ir(module_llvm)) {
		return false;
	}
#endif

	if (espace.options.objet_genere == ObjetGenere::Executable) {
		std::cout << "Écriture du code dans un fichier..." << std::endl;
		auto debut_fichier_objet = dls::chrono::compte_seconde();
		if (!ecris_fichier_objet(machine_cible.get(), module_llvm)) {
			compilatrice.possede_erreur = true;
			return 1;
		}
		temps_fichier_objet = debut_fichier_objet.temps();
	}

	return true;
}

bool CoulisseLLVM::cree_executable(Compilatrice &compilatrice, EspaceDeTravail &espace)
{
	auto debut_executable = dls::chrono::compte_seconde();
	if (!::cree_executable(espace.options.nom_sortie, compilatrice.racine_kuri.c_str())) {
		compilatrice.possede_erreur = true;
		return false;
	}

	temps_executable = debut_executable.temps();
	return true;
}
