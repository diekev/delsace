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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#include <llvm/IR/Module.h>
#pragma GCC diagnostic pop

#include "compilatrice.hh"

#include "representation_intermediaire/constructrice_ri.hh"
#include "representation_intermediaire/instructions.hh"

/* ************************************************************************** */

static bool est_plus_petit(Type *type1, Type *type2)
{
	return type1->taille_octet < type2->taille_octet;
}

static bool est_type_entier(Type *type)
{
	return type->genre == GenreType::ENTIER_NATUREL || type->genre == GenreType::ENTIER_RELATIF;
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
		case Genre::Et_Logique:          return llvm::Instruction::And;
		case Genre::Ou_Logique:          return llvm::Instruction::Or;
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
		case Genre::Et_Logique:
		case Genre::Ou_Logique:
		case Genre::Et_Binaire:
		case Genre::Ou_Binaire:
		case Genre::Ou_Exclusif:
		case Genre::Dec_Gauche:
		case Genre::Dec_Droite_Arithm:
		case Genre::Dec_Droite_Logique:
			break;
	}

	return static_cast<llvm::CmpInst::Predicate>(0);
}

/* ************************************************************************** */

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

	switch (type->genre) {
		case GenreType::POLYMORPHIQUE:
		case GenreType::INVALIDE:
		{
			type_llvm = nullptr;
			table_types[type] = type_llvm;
			break;
		}
		case GenreType::FONCTION:
		{
			auto type_fonc = static_cast<TypeFonction *>(type);

			std::vector<llvm::Type *> parametres;
			POUR (type_fonc->types_entrees) {
				auto type_llvm_it = converti_type_llvm(it);
				parametres.push_back(type_llvm_it);
			}

			// À FAIRE : multiples types de retours
			auto type_retour = converti_type_llvm(type_fonc->types_sorties[0]);

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
			if (type_deref->genre == GenreType::RIEN) {
				type_deref = m_espace.typeuse[TypeBase::Z8];
			}

			auto type_deref_llvm = converti_type_llvm(type_deref);
			type_llvm = llvm::PointerType::get(type_deref_llvm, 0);
			break;
		}
		case GenreType::UNION:
		{
			auto type_struct = static_cast<TypeUnion *>(type);
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
			auto type_struct = static_cast<TypeStructure *>(type);
			auto nom = "struct." + type_struct->nom;

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
			auto type_var = static_cast<TypeVariadique *>(type);

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
			auto const taille = static_cast<TypeTableauFixe *>(type)->taille;

			type_llvm = llvm::ArrayType::get(type_deref_llvm, static_cast<unsigned long>(taille));
			break;
		}
		case GenreType::ENUM:
		case GenreType::ERREUR:
		{
			auto type_enum = static_cast<TypeEnum *>(type);
			type_llvm = converti_type_llvm(type_enum->type_donnees);
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
				auto type_var = static_cast<TypeVariadique *>(it);
				auto type_tabl = m_espace.typeuse.type_tableau_dynamique(type_var->type_pointe);
				parametres.push_back(converti_type_llvm(type_tabl));
			}

			break;
		}

		parametres.push_back(converti_type_llvm(it));
	}

	return llvm::FunctionType::get(
				converti_type_llvm(type->types_sorties[0]),
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
					break;
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

							for (auto i = 0; i < type->membres.taille; ++i) {
								auto valeur = static_cast<llvm::Constant *>(nullptr);

								// les tableaux fixes ont une initialisation nulle
								if (tableau_valeur[i] == nullptr) {
									auto type_llvm_valeur = converti_type_llvm(type->membres[i].type);
									valeur = llvm::ConstantAggregateZero::get(type_llvm_valeur);
								}
								else {
									valeur = llvm::cast<llvm::Constant>(genere_code_pour_atome(tableau_valeur[i], pour_globale));
								}

								tableau_membre.push_back(valeur);
							}

							return llvm::ConstantStruct::get(
										llvm::cast<llvm::StructType>(converti_type_llvm(type)),
										tableau_membre);
						}
						case AtomeValeurConstante::Valeur::Genre::TABLEAU_FIXE:
						{
							// À FAIRE : tableaux fixes
							return nullptr;
						}
						case AtomeValeurConstante::Valeur::Genre::TABLEAU_DONNEES_CONSTANTES:
						{
							return nullptr;
						}
					}
				}
			}

			return nullptr;
		}
		case Atome::Genre::INSTRUCTION:
		{
			auto inst = static_cast<Instruction const *>(atome);
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
		case Instruction::Genre::ENREGISTRE_LOCALES:
		case Instruction::Genre::RESTAURE_LOCALES:
		{
			break;
		}
		case Instruction::Genre::ALLOCATION:
		{
			auto type_pointeur = static_cast<TypePointeur *>(inst->type);
			auto type_llvm = converti_type_llvm(type_pointeur->type_pointe);
			auto alloca = m_builder.CreateAlloca(type_llvm, 0u);
			alloca->setAlignment(type_pointeur->type_pointe->alignement);
			table_valeurs[inst] = alloca;
			break;
		}
		case Instruction::Genre::APPEL:
		{
			auto inst_appel = static_cast<InstructionAppel const *>(inst);

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
			auto inst_branche = static_cast<InstructionBranche const *>(inst);
			m_builder.CreateBr(table_blocs[inst_branche->label]);
			break;
		}
		case Instruction::Genre::BRANCHE_CONDITION:
		{
			auto inst_branche = static_cast<InstructionBrancheCondition const *>(inst);
			auto condition = genere_code_pour_atome(inst_branche->condition, false);
			auto bloc_si_vrai = table_blocs[inst_branche->label_si_vrai];
			auto bloc_si_faux = table_blocs[inst_branche->label_si_faux];
			m_builder.CreateCondBr(condition, bloc_si_vrai, bloc_si_faux);
			break;
		}
		case Instruction::Genre::CHARGE_MEMOIRE:
		{
			auto inst_charge = static_cast<InstructionChargeMem const *>(inst);
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
			auto inst_stocke = static_cast<InstructionStockeMem const *>(inst);
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
			auto inst_label = static_cast<InstructionLabel const *>(inst);
			auto bloc = llvm::BasicBlock::Create(m_contexte_llvm, "", m_fonction_courante);
			table_blocs[inst_label] = bloc;
			m_builder.SetInsertPoint(bloc);
			break;
		}
		case Instruction::Genre::OPERATION_UNAIRE:
		{
			auto inst_un = static_cast<InstructionOpUnaire const *>(inst);
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
			auto inst_bin = static_cast<InstructionOpBinaire const *>(inst);
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
			auto inst_retour = static_cast<InstructionRetour const *>(inst);
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
			auto inst_acces = static_cast<InstructionAccedeIndex const *>(inst);
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
			auto inst_acces = static_cast<InstructionAccedeMembre const *>(inst);

			auto accede = inst_acces->accede;
			auto valeur_accede = static_cast<llvm::Value *>(nullptr);

			if (accede->genre_atome == Atome::Genre::INSTRUCTION) {
				valeur_accede = table_valeurs[accede];
			}
			else {
				valeur_accede = table_globales[accede];
			}

			// À FAIRE : type union

			auto index_membre = static_cast<AtomeValeurConstante *>(inst_acces->index)->valeur.valeur_entiere;

			auto index = std::vector<llvm::Value *>(2);
			index[0] = m_builder.getInt64(0);
			index[1] = m_builder.getInt64(index_membre);

			table_valeurs[inst] = m_builder.CreateGEP(valeur_accede, index);
			break;
		}
		case Instruction::Genre::TRANSTYPE:
		{
			using CastOps = llvm::Instruction::CastOps;

			auto inst_transtype = static_cast<InstructionTranstype const *>(inst);
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

		auto valeur_initialisateur = static_cast<llvm::Constant *>(nullptr);

		if (valeur_globale->initialisateur) {
			valeur_initialisateur = static_cast<llvm::Constant *>(genere_code_pour_atome(valeur_globale->initialisateur, true));
		}

		auto type = static_cast<TypePointeur *>(valeur_globale->type)->type_pointe;
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
					valeur_initialisateur,
					nom_globale);

		table_globales[valeur_globale] = globale;
	}

	POUR_TABLEAU_PAGE (m_espace.fonctions) {
		auto atome_fonc = &it;

		auto type_fonction = static_cast<TypeFonction *>(atome_fonc->type);
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

		//std::cerr << "Génère code pour : " << atome_fonc->nom << '\n';

		auto fonction = m_module->getFunction(atome_fonc->nom.c_str());

		m_fonction_courante = fonction;

		auto valeurs_args = fonction->arg_begin();

		for (auto &param : atome_fonc->params_entrees) {
			auto const &nom_argument = param->ident->nom;

			auto valeur = &(*valeurs_args++);
			valeur->setName(dls::chaine(nom_argument).c_str());

			auto type = static_cast<TypePointeur *>(param->type)->type_pointe;
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

		/* génère d'abord tous les blocs depuis les labels */
		for (auto inst : atome_fonc->instructions) {
			if (inst->genre != Instruction::Genre::LABEL) {
				continue;
			}

			genere_code_pour_instruction(inst);
		}

		for (auto inst : atome_fonc->instructions) {
			if (inst->genre == Instruction::Genre::LABEL) {
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
