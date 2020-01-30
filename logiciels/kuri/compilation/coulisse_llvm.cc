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
#include "info_type.hh"
#include "modules.hh"
#include "outils_lexemes.hh"
#include "validation_semantique.hh"

using denombreuse = lng::decoupeuse_nombre<TypeLexeme>;

#undef NOMME_IR

/* ************************************************************************** */

/* À FAIRE (coulisse LLVM)
 * - infos types
 * - loge, déloge, reloge
 * - opérateurs : +=, -=, etc..
 * - raccourci opérateurs comparaisons (a <= b <= c au lieu de a <= b && b <= c)
 * - prend en compte la portée des blocs pour générer le code des noeuds différés
 * - conversion tableau octet
 * - discr
 * - trace de la mémoire utilisée
 * - coroutine, retiens
 * - erreur en cas de débordement des limites, où d'accès à un membre non-actif d'une union
 */

/* ************************************************************************** */

namespace noeud {
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

static size_t taille_de(TypeLexeme type)
{
	switch (type) {
		case TypeLexeme::BOOL:
			return 1;
		case TypeLexeme::N8:
		case TypeLexeme::Z8:
			return 8;
		case TypeLexeme::N16:
		case TypeLexeme::R16:
		case TypeLexeme::Z16:
			return 16;
		case TypeLexeme::N32:
		case TypeLexeme::R32:
		case TypeLexeme::Z32:
			return 32;
		case TypeLexeme::N64:
		case TypeLexeme::R64:
		case TypeLexeme::Z64:
		case TypeLexeme::POINTEUR:
			return 64;
		default:
			return 0ul;
	}
}

static bool est_plus_petit(TypeLexeme type1, TypeLexeme type2)
{
	return taille_de(type1) < taille_de(type2);
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
		DonneesTypeFinal &donnees_retour,
		bool est_variadique)
{
	std::vector<llvm::Type *> parametres;
	parametres.reserve(static_cast<size_t>(donnees_fonction.args.taille()));

	if (!donnees_fonction.est_externe) {
		parametres.push_back(converti_type_llvm(contexte, contexte.index_type_contexte));
	}

	for (auto const &argument : donnees_fonction.args) {
		if (argument.est_variadic) {
			/* les arguments variadics sont transformés en un tableau */
			if (!donnees_fonction.est_externe) {
				auto idx_dt_tabl = contexte.typeuse.type_tableau_pour(argument.index_type);
				parametres.push_back(converti_type_llvm(contexte, contexte.typeuse[idx_dt_tabl]));
			}

			break;
		}

		parametres.push_back(converti_type_llvm(contexte, contexte.typeuse[argument.index_type]));
	}

	return llvm::FunctionType::get(
				converti_type_llvm(contexte, donnees_retour),
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
		DonneesTypeFinal const &donnees_type,
		bool charge_ = true)
{
	/* trouve le type de la structure tableau */
	auto deref = donnees_type.dereference();
	auto dt = DonneesTypeFinal{};
	dt.pousse(TypeLexeme::TABLEAU);
	dt.pousse(deref);

	auto type_llvm = converti_type_llvm(contexte, dt);

	/* alloue de l'espace pour ce type */
	auto alloc = new llvm::AllocaInst(type_llvm, 0, "", contexte.bloc_courant());
	alloc->setAlignment(8);

	/* copie le pointeur du début du tableau */
	auto ptr_valeur = accede_membre_structure(contexte, alloc, POINTEUR_TABLEAU);

	/* charge le premier élément du tableau */
	auto premier_elem = accede_element_tableau(
							contexte,
							tableau,
							donnees_type.type_llvm(),
							0ul);

	auto charge = new llvm::LoadInst(premier_elem, "", contexte.bloc_courant());
	charge->setAlignment(8);
	auto addresse = charge->getPointerOperand();

	auto stocke = new llvm::StoreInst(addresse, ptr_valeur, contexte.bloc_courant());
	stocke->setAlignment(8);

	/* copie la taille du tableau */
	auto ptr_taille = accede_membre_structure(contexte, alloc, TAILLE_TABLEAU);

	auto taille_tableau = donnees_type.type_base() >> 8;
	auto constante = llvm::ConstantInt::get(
						 llvm::Type::getInt64Ty(contexte.contexte),
						 uint64_t(taille_tableau),
						 false);

	stocke = new llvm::StoreInst(constante, ptr_taille, contexte.bloc_courant());
	stocke->setAlignment(8);

	if (charge_) {
		charge = new llvm::LoadInst(alloc, "", contexte.bloc_courant());
		charge->setAlignment(8);
		return charge;
	}

	return alloc;
}

[[nodiscard]] static llvm::Value *converti_vers_eini(
		ContexteGenerationCode &contexte,
		llvm::Value *valeur,
		DonneesTypeFinal &donnees_type,
		bool charge_)
{
	/* alloue de l'espace pour un eini */
	auto dt = DonneesTypeFinal{};
	dt.pousse(TypeLexeme::EINI);

	auto type_eini_llvm = converti_type_llvm(contexte, dt);

	auto alloc_eini = new llvm::AllocaInst(type_eini_llvm, 0, "", contexte.bloc_courant());
	alloc_eini->setAlignment(8);

	/* copie le pointeur de la valeur vers le type eini */
	auto ptr_eini = accede_membre_structure(contexte, alloc_eini, POINTEUR_EINI);

	/* Dans le cas des constantes, nous pouvons directement utiliser la valeur,
	 * car il s'agit de l'adresse où elle est stockée. */
	if (llvm::isa<llvm::Constant>(valeur) == false) {
		auto charge_valeur = new llvm::LoadInst(valeur, "", contexte.bloc_courant());
		valeur = charge_valeur->getPointerOperand();
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

	auto ptr_info_type = cree_info_type(contexte, donnees_type);

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

[[nodiscard]] static llvm::Value *genere_code_enfant(
		ContexteGenerationCode &contexte,
		base *enfant,
		bool expr_gauche)
{
	// À FAIRE(transformation)
	//auto conversion = (enfant->drapeaux & MASQUE_CONVERSION) != 0;
	auto valeur_enfant = genere_code_llvm(enfant, contexte, expr_gauche);

//	if (conversion) {
//		auto &dt = contexte.typeuse[enfant->index_type];

//		if ((enfant->drapeaux & CONVERTI_TABLEAU) != 0) {
//			valeur_enfant = converti_vers_tableau_dyn(contexte, valeur_enfant, dt, false);
//		}

//		if ((enfant->drapeaux & CONVERTI_EINI) != 0) {
//			valeur_enfant = converti_vers_eini(contexte, valeur_enfant, dt, false);
//		}

//		if ((enfant->drapeaux & EXTRAIT_EINI) != 0) {
//			auto ptr_valeur = accede_membre_structure(contexte, valeur_enfant, POINTEUR_EINI);

//			/* déréfence ce pointeur */
//			auto charge = new llvm::LoadInst(ptr_valeur, "", false, contexte.bloc_courant());
//			charge->setAlignment(8);

//			/* transtype le pointeur vers le bon type */
//			auto dt_vers = DonneesTypeFinal{};
//			dt_vers.pousse(id_morceau::POINTEUR);
//			dt_vers.pousse(contexte.typeuse[enfant->index_type]);

//			auto type_llvm = converti_type_llvm(contexte, dt_vers);
//			valeur_enfant = new llvm::BitCastInst(charge, type_llvm, "", contexte.bloc_courant());

//			/* déréfence ce pointeur : après la portée */
//		}

//		auto charge = new llvm::LoadInst(valeur_enfant, "", contexte.bloc_courant());
//		//charge->setAlignment(8);
//		valeur_enfant = charge;
//	}

	return valeur_enfant;
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
		return genere_code_enfant(contexte, noeud_enfant, false);
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
		const TypeLexeme &type,
		llvm::PHINode *noeud_phi,
		llvm::Value *valeur_fin,
		llvm::BasicBlock *bloc_courant)
{
	if (est_type_entier_naturel(type)) {
		return llvm::ICmpInst::Create(
					llvm::Instruction::ICmp,
					llvm::CmpInst::Predicate::ICMP_ULE,
					noeud_phi,
					valeur_fin,
					"",
					bloc_courant);
	}

	if (est_type_entier_relatif(type)) {
		return llvm::ICmpInst::Create(
					llvm::Instruction::ICmp,
					llvm::CmpInst::Predicate::ICMP_SLE,
					noeud_phi,
					valeur_fin,
					"",
					bloc_courant);
	}

	if (est_type_reel(type)) {
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
		const TypeLexeme &type,
		ContexteGenerationCode &contexte,
		llvm::PHINode *noeud_phi,
		llvm::BasicBlock *bloc_courant)
{
	auto type_llvm = converti_type_simple_llvm(contexte, type, nullptr);

	if (est_type_entier(type)) {
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

	if (est_type_reel(type)) {
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

static llvm::Value *genere_code_llvm(
		base *b,
		ContexteGenerationCode &contexte,
		const bool expr_gauche)
{
	switch (b->type) {
		case type_noeud::DECLARATION_ENUM:
		case type_noeud::LISTE_PARAMETRES_FONCTION:
		case type_noeud::PAIRE_DISCR:
		case type_noeud::RACINE:
		case type_noeud::RETOUR:
		{
			/* RÀF pour ces types de noeuds */
			return nullptr;
		}
		case type_noeud::DECLARATION_COROUTINE:
		case type_noeud::SINON:
		case type_noeud::OPERATION_COMP_CHAINEE:
		case type_noeud::ACCES_MEMBRE_UNION:
		{
			/* À FAIRE */
			return nullptr;
		}
		case type_noeud::EXPRESSION_PARENTHESE:
		{
			return genere_code_llvm(b->enfants.front(), contexte, expr_gauche);
		}
		case type_noeud::DECLARATION_FONCTION:
		{
			auto fichier = contexte.fichier(static_cast<size_t>(b->morceau.fichier));
			auto module = fichier->module;
			auto &vdf = module->donnees_fonction(b->morceau.chaine);
			auto donnees_fonction = static_cast<DonneesFonction *>(nullptr);

			for (auto &df : vdf) {
				if (df.noeud_decl == b) {
					donnees_fonction = &df;
				}
			}

			/* Pour les fonctions variadiques nous transformons la liste d'argument en
			 * un tableau dynamique transmis à la fonction. La raison étant que les
			 * instruction de LLVM pour les arguments variadiques ne fonctionnent
			 * vraiment que pour les types simples et les pointeurs. Pour pouvoir passer
			 * des structures, il faudrait manuellement gérer les instructions
			 * d'incrémentation et d'extraction des arguments pour chaque plateforme.
			 * Nos tableaux, quant à eux, sont portables.
			 */

			/* Crée le type de la fonction */
			auto &this_dt = contexte.typeuse[b->index_type];
			auto type_fonction = obtiens_type_fonction(
									 contexte,
									 *donnees_fonction,
									 this_dt,
									 donnees_fonction->est_variadique);

			contexte.typeuse[donnees_fonction->index_type].type_llvm(type_fonction);

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

			if (!donnees_fonction->est_externe) {
				auto valeur = &(*valeurs_args++);
				valeur->setName("contexte");

				auto type = converti_type_llvm(contexte, contexte.index_type_contexte);

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
				donnees_var.est_dynamique = true;
				donnees_var.est_variadic = false;
				donnees_var.index_type = contexte.index_type_contexte;

				contexte.pousse_locale("contexte", donnees_var);
			}

			for (auto const &argument : donnees_fonction->args) {
				auto index_type = argument.index_type;
				auto align = unsigned{0};
				auto type = static_cast<llvm::Type *>(nullptr);

				if (argument.est_variadic) {
					align = 8;

					auto idx_tabl = contexte.typeuse.type_tableau_pour(argument.index_type);
					auto &dt = contexte.typeuse[idx_tabl];

					type = converti_type_llvm(contexte, dt);
				}
				else {
					auto dt = contexte.typeuse[argument.index_type];
					align = alignement(contexte, dt);
					type = converti_type_llvm(contexte, dt);
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
				donnees_var.est_dynamique = argument.est_dynamic;
				donnees_var.est_variadic = argument.est_variadic;
				donnees_var.index_type = index_type;

				contexte.pousse_locale(argument.nom, donnees_var);
			}

			/* Crée code pour le bloc. */
			auto bloc = b->enfants.back();
			bloc->valeur_calculee = static_cast<llvm::BasicBlock *>(nullptr);
			auto ret = genere_code_llvm(bloc, contexte, true);

			/* Ajoute une instruction de retour si la dernière n'en est pas une. */
			if ((ret != nullptr) && !llvm::isa<llvm::ReturnInst>(*ret)) {
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
		case type_noeud::APPEL_FONCTION:
		{
			auto est_pointeur_fonction = contexte.locale_existe(b->morceau.chaine);

			if (est_pointeur_fonction) {
				auto valeur = contexte.valeur_locale(b->morceau.chaine);

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
		case type_noeud::VARIABLE:
		{
			if (b->aide_generation_code == GENERE_CODE_DECL_VAR || b->aide_generation_code == GENERE_CODE_DECL_VAR_GLOBALE) {
				auto &type = contexte.typeuse[b->index_type];
				auto type_llvm = converti_type_llvm(contexte, type);

				if (contexte.fonction == nullptr) {
					auto valeur = new llvm::GlobalVariable(
									  *contexte.module_llvm,
									  type_llvm,
									  !possede_drapeau(b->drapeaux, DYNAMIC),
									  llvm::GlobalValue::InternalLinkage,
									  nullptr);

					valeur->setConstant((b->drapeaux & DYNAMIC) == 0);
					valeur->setAlignment(alignement(contexte, type));

					auto donnees_var = DonneesVariable{};
					donnees_var.valeur = valeur;
					donnees_var.est_dynamique = (b->drapeaux & DYNAMIC) != 0;
					donnees_var.index_type = b->index_type;

					contexte.pousse_globale(b->chaine(), donnees_var);
					return valeur;
				}

				auto alloc = new llvm::AllocaInst(
								 type_llvm,
								 0,
			#ifdef NOMME_IR
								 std::string(b->morceau.chaine),
			#else
								 "",
			#endif
								 contexte.bloc_courant());

				alloc->setAlignment(alignement(contexte, type));

				/* Mets à zéro les valeurs des tableaux dynamics. */
				if (type.type_base() == TypeLexeme::TABLEAU) {
					auto pointeur = accede_membre_structure(contexte, alloc, POINTEUR_TABLEAU);

					auto stocke = new llvm::StoreInst(
									  llvm::ConstantInt::get(
										  llvm::Type::getInt64Ty(contexte.contexte),
										  static_cast<uint64_t>(0),
										  false),
									  pointeur,
									  contexte.bloc_courant());
					stocke->setAlignment(8);

					auto taille = accede_membre_structure(contexte, alloc, TAILLE_TABLEAU);
					stocke = new llvm::StoreInst(
								 llvm::ConstantInt::get(
									 llvm::Type::getInt64Ty(contexte.contexte),
									 static_cast<uint64_t>(0),
									 false),
								 taille,
								 contexte.bloc_courant());
					stocke->setAlignment(8);
				}

				auto donnees_var = DonneesVariable{};
				donnees_var.valeur = alloc;
				donnees_var.est_dynamique = (b->drapeaux & DYNAMIC) != 0;
				donnees_var.index_type = b->index_type;

				contexte.pousse_locale(b->morceau.chaine, donnees_var);

				return alloc;
			}

			auto valeur = contexte.valeur_locale(b->morceau.chaine);

			if (valeur == nullptr) {
				valeur = contexte.valeur_globale(b->morceau.chaine);

				if (valeur == nullptr && b->nom_fonction_appel != "") {
					valeur = contexte.module_llvm->getFunction(b->nom_fonction_appel.c_str());
					return valeur;
				}
			}

			if (expr_gauche || llvm::dyn_cast<llvm::PHINode>(valeur)) {
				return valeur;
			}

			auto const &index_type = b->index_type;
			auto &type = contexte.typeuse[index_type];

			auto charge = new llvm::LoadInst(valeur, "", false, contexte.bloc_courant());
			charge->setAlignment(alignement(contexte, type));

			return charge;
		}
		case type_noeud::ACCES_MEMBRE_POINT:
		{
			auto structure = b->enfants.front();
			auto membre = b->enfants.back();

			auto const &index_type = structure->index_type;
			auto type_structure = contexte.typeuse[index_type];

			auto est_pointeur = type_structure.type_base() == TypeLexeme::POINTEUR;

			if (est_pointeur) {
				type_structure = type_structure.dereference();
			}

			if (type_structure.type_base() == TypeLexeme::EINI) {
				auto valeur = genere_code_llvm(structure, contexte, true);

				if (est_pointeur) {
					valeur = new llvm::LoadInst(valeur, "", contexte.bloc_courant());
				}

				if (membre->chaine() == "pointeur") {
					return accede_membre_structure(contexte, valeur, POINTEUR_EINI, true);
				}

				return accede_membre_structure(contexte, valeur, TYPE_EINI, true);
			}

			if (type_structure.type_base() == TypeLexeme::CHAINE) {
				auto valeur = genere_code_llvm(structure, contexte, true);

				if (est_pointeur) {
					valeur = new llvm::LoadInst(valeur, "", contexte.bloc_courant());
				}

				if (membre->chaine() == "pointeur") {
					return accede_membre_structure(contexte, valeur, POINTEUR_CHAINE, true);
				}

				return accede_membre_structure(contexte, valeur, TYPE_CHAINE, true);
			}

			if ((type_structure.type_base() & 0xff) == TypeLexeme::TABLEAU) {
				auto taille = static_cast<size_t>(type_structure.type_base() >> 8);

				if (taille != 0) {
					return llvm::ConstantInt::get(
								converti_type_llvm(contexte, b->index_type),
								taille);
				}

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

			auto index_structure = long(type_structure.type_base() >> 8);

			auto const &nom_membre = membre->chaine();

			auto &donnees_structure = contexte.donnees_structure(index_structure);

			auto const iter = donnees_structure.donnees_membres.trouve(nom_membre);

			auto const &donnees_membres = iter->second;
			auto const index_membre = donnees_membres.index_membre;

			auto valeur = genere_code_llvm(structure, contexte, true);

			llvm::Value *ret;

			/* À FAIRE : le pointeur vers l'InfoType (index = 0) d'un eini n'est pas
			 * vraiment un pointeur, donc on ne devrait pas le déréférencé. */
			if (est_pointeur && index_structure != 0) {
				/* déréférence le pointeur en le chargeant */
				auto charge = new llvm::LoadInst(valeur, "", contexte.bloc_courant());
				charge->setAlignment(8);
				valeur = charge;
			}

			ret = accede_membre_structure(contexte, valeur, static_cast<size_t>(index_membre));

			if (!expr_gauche) {
				auto charge = new llvm::LoadInst(ret, "", contexte.bloc_courant());
				auto const &dt = contexte.typeuse[donnees_structure.index_types[index_membre]];
				charge->setAlignment(alignement(contexte, dt));
				ret = charge;
			}

			return ret;
		}
		case type_noeud::ASSIGNATION_VARIABLE:
		{
			assert(b->enfants.taille() == 2);

			auto variable = b->enfants.front();
			auto expression = b->enfants.back();

			// À FAIRE(transformation)

//			auto compatibilite = std::any_cast<niveau_compat>(b->valeur_calculee);

//			if ((compatibilite & niveau_compat::converti_tableau) != niveau_compat::aucune) {
//				expression->drapeaux |= CONVERTI_TABLEAU;
//			}

//			if ((compatibilite & niveau_compat::converti_eini) != niveau_compat::aucune) {
//				expression->drapeaux |= CONVERTI_EINI;
//			}

//			if ((compatibilite & niveau_compat::extrait_eini) != niveau_compat::aucune) {
//				expression->drapeaux |= EXTRAIT_EINI;
//				expression->index_type = variable->index_type;
//			}

			/* Génère d'abord le code de l'enfant afin que l'instruction d'allocation de
			 * la variable sur la pile et celle de stockage de la valeur soit côte à
			 * côte. */
			auto valeur = genere_code_enfant(contexte, expression, false);

			auto alloc = genere_code_llvm(variable, contexte, true);

			if (variable->aide_generation_code == GENERE_CODE_DECL_VAR && contexte.fonction == nullptr) {
				//assert(est_constant(expression));
				auto vg = llvm::dyn_cast<llvm::GlobalVariable>(alloc);
				vg->setInitializer(llvm::dyn_cast<llvm::Constant>(valeur));
				return vg;
			}

			auto store = new llvm::StoreInst(valeur, alloc, false, contexte.bloc_courant());

			auto const &dt = contexte.typeuse[expression->index_type];
			store->setAlignment(alignement(contexte, dt));

			return store;
		}
		case type_noeud::NOMBRE_REEL:
		{
			auto const est_calcule = possede_drapeau(b->drapeaux, EST_CALCULE);
			auto const valeur = est_calcule ? std::any_cast<double>(b->valeur_calculee) :
												denombreuse::converti_chaine_nombre_reel(
													b->morceau.chaine,
													b->morceau.identifiant);

			return llvm::ConstantFP::get(
						llvm::Type::getDoubleTy(contexte.contexte),
						valeur);
		}
		case type_noeud::NOMBRE_ENTIER:
		{
			auto const est_calcule = possede_drapeau(b->drapeaux, EST_CALCULE);
			auto const valeur = est_calcule ? std::any_cast<long>(b->valeur_calculee) :
												denombreuse::converti_chaine_nombre_entier(
													b->morceau.chaine,
													b->morceau.identifiant);

			return llvm::ConstantInt::get(
						llvm::Type::getInt32Ty(contexte.contexte),
						static_cast<uint64_t>(valeur),
						false);
		}
		case type_noeud::OPERATION_BINAIRE:
		{
			auto instr = llvm::Instruction::Add;
			auto predicat = llvm::CmpInst::Predicate::FCMP_FALSE;
			auto est_comp_entier = false;
			auto est_comp_reel = false;

			auto enfant1 = b->enfants.front();
			auto enfant2 = b->enfants.back();

			auto const index_type1 = enfant1->index_type;
			auto const index_type2 = enfant2->index_type;

			auto const &type1 = contexte.typeuse[index_type1];
			auto &type2 = contexte.typeuse[index_type2];

			/* À FAIRE : typage */

			/* Ne crée pas d'instruction de chargement si nous avons un tableau. */
			auto const valeur2_brut = ((type2.type_base() & 0xff) == TypeLexeme::TABLEAU);

			auto valeur1 = genere_code_llvm(enfant1, contexte, false);
			auto valeur2 = genere_code_llvm(enfant2, contexte, valeur2_brut);

			switch (b->morceau.identifiant) {
				case TypeLexeme::PLUS:
					if (est_type_entier(type1.type_base())) {
						instr = llvm::Instruction::Add;
					}
					else {
						instr = llvm::Instruction::FAdd;
					}

					break;
				case TypeLexeme::MOINS:
					if (est_type_entier(type1.type_base())) {
						instr = llvm::Instruction::Sub;
					}
					else {
						instr = llvm::Instruction::FSub;
					}

					break;
				case TypeLexeme::FOIS:
					if (est_type_entier(type1.type_base())) {
						instr = llvm::Instruction::Mul;
					}
					else {
						instr = llvm::Instruction::FMul;
					}

					break;
				case TypeLexeme::DIVISE:
					if (est_type_entier_naturel(type1.type_base())) {
						instr = llvm::Instruction::UDiv;
					}
					else if (est_type_entier_relatif(type1.type_base())) {
						instr = llvm::Instruction::SDiv;
					}
					else {
						instr = llvm::Instruction::FDiv;
					}

					break;
				case TypeLexeme::POURCENT:
					if (est_type_entier_naturel(type1.type_base())) {
						instr = llvm::Instruction::URem;
					}
					else if (est_type_entier_relatif(type1.type_base())) {
						instr = llvm::Instruction::SRem;
					}
					else {
						instr = llvm::Instruction::FRem;
					}

					break;
				case TypeLexeme::DECALAGE_DROITE:
					if (est_type_entier_naturel(type1.type_base())) {
						instr = llvm::Instruction::LShr;
					}
					else if (est_type_entier_relatif(type1.type_base())) {
						instr = llvm::Instruction::AShr;
					}
					break;
				case TypeLexeme::DECALAGE_GAUCHE:
					instr = llvm::Instruction::Shl;
					break;
				case TypeLexeme::ESPERLUETTE:
				case TypeLexeme::ESP_ESP:
					instr = llvm::Instruction::And;
					break;
				case TypeLexeme::BARRE:
				case TypeLexeme::BARRE_BARRE:
					instr = llvm::Instruction::Or;
					break;
				case TypeLexeme::CHAPEAU:
					instr = llvm::Instruction::Xor;
					break;
					/* À FAIRE. */
				case TypeLexeme::INFERIEUR:
					if (est_type_entier_naturel(type1.type_base())) {
						est_comp_entier = true;
						predicat = llvm::CmpInst::Predicate::ICMP_ULT;
					}
					else if (est_type_entier_relatif(type1.type_base())) {
						est_comp_entier = true;
						predicat = llvm::CmpInst::Predicate::ICMP_SLT;
					}
					else {
						est_comp_reel = true;
						predicat = llvm::CmpInst::Predicate::FCMP_OLT;
					}

					break;
				case TypeLexeme::INFERIEUR_EGAL:
					if (est_type_entier_naturel(type1.type_base())) {
						est_comp_entier = true;
						predicat = llvm::CmpInst::Predicate::ICMP_ULE;
					}
					else if (est_type_entier_relatif(type1.type_base())) {
						est_comp_entier = true;
						predicat = llvm::CmpInst::Predicate::ICMP_SLE;
					}
					else {
						est_comp_reel = true;
						predicat = llvm::CmpInst::Predicate::FCMP_OLE;
					}

					break;
				case TypeLexeme::SUPERIEUR:
					if (est_type_entier_naturel(type1.type_base())) {
						est_comp_entier = true;
						predicat = llvm::CmpInst::Predicate::ICMP_UGT;
					}
					else if (est_type_entier_relatif(type1.type_base())) {
						est_comp_entier = true;
						predicat = llvm::CmpInst::Predicate::ICMP_SGT;
					}
					else {
						est_comp_reel = true;
						predicat = llvm::CmpInst::Predicate::FCMP_OGT;
					}

					break;
				case TypeLexeme::SUPERIEUR_EGAL:
					if (est_type_entier_naturel(type1.type_base())) {
						est_comp_entier = true;
						predicat = llvm::CmpInst::Predicate::ICMP_UGE;
					}
					else if (est_type_entier_relatif(type1.type_base())) {
						est_comp_entier = true;
						predicat = llvm::CmpInst::Predicate::ICMP_SGE;
					}
					else {
						est_comp_reel = true;
						predicat = llvm::CmpInst::Predicate::FCMP_OGE;
					}

					break;
				case TypeLexeme::EGALITE:
					if (est_type_entier(type1.type_base())) {
						est_comp_entier = true;
						predicat = llvm::CmpInst::Predicate::ICMP_EQ;
					}
					else {
						est_comp_reel = true;
						predicat = llvm::CmpInst::Predicate::FCMP_OEQ;
					}

					break;
				case TypeLexeme::DIFFERENCE:
					if (est_type_entier(type1.type_base())) {
						est_comp_entier = true;
						predicat = llvm::CmpInst::Predicate::ICMP_NE;
					}
					else {
						est_comp_reel = true;
						predicat = llvm::CmpInst::Predicate::FCMP_ONE;
					}

					break;
				default:
					return nullptr;
			}

			if (est_comp_entier) {
				return llvm::ICmpInst::Create(llvm::Instruction::ICmp, predicat, valeur1, valeur2, "", contexte.bloc_courant());
			}

			if (est_comp_reel) {
				return llvm::FCmpInst::Create(llvm::Instruction::FCmp, predicat, valeur1, valeur2, "", contexte.bloc_courant());
			}

			return llvm::BinaryOperator::Create(instr, valeur1, valeur2, "", contexte.bloc_courant());
		}
		case type_noeud::OPERATION_UNAIRE:
		{
			llvm::Instruction::BinaryOps instr;
			auto enfant = b->enfants.front();
			auto index_type1 = enfant->index_type;
			auto const &type1 = contexte.typeuse[index_type1];
			auto valeur1 = genere_code_llvm(enfant, contexte, false);
			auto valeur2 = static_cast<llvm::Value *>(nullptr);

			switch (b->morceau.identifiant) {
				case TypeLexeme::EXCLAMATION:
				{
					instr = llvm::Instruction::Xor;
					valeur2 = valeur1;
					break;
				}
				case TypeLexeme::TILDE:
				{
					instr = llvm::Instruction::Xor;
					valeur2 = llvm::ConstantInt::get(
								  llvm::Type::getInt32Ty(contexte.contexte),
								  static_cast<uint64_t>(0),
								  false);
					break;
				}
				case TypeLexeme::AROBASE:
				{
					auto inst_load = llvm::dyn_cast<llvm::LoadInst>(valeur1);

					if (inst_load == nullptr) {
						/* Ne devrais pas arriver. */
						return nullptr;
					}

					return inst_load->getPointerOperand();
				}
				case TypeLexeme::PLUS_UNAIRE:
				{
					return valeur1;
				}
				case TypeLexeme::MOINS_UNAIRE:
				{
					valeur2 = valeur1;

					if (est_type_entier(type1.type_base())) {
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
		case type_noeud::ACCES_TABLEAU:
		{
			auto enfant1 = b->enfants.front();
			auto enfant2 = b->enfants.back();

			auto const index_type1 = enfant1->index_type;
			auto &type1 = contexte.typeuse[index_type1];

			auto valeur1 = genere_code_llvm(enfant1, contexte, true);
			auto valeur2 = genere_code_llvm(enfant2, contexte, false);

			llvm::Value *valeur;

			if (type1.type_base() == TypeLexeme::POINTEUR) {
				valeur1 = new llvm::LoadInst(valeur1, "", false, contexte.bloc_courant());
				valeur = llvm::GetElementPtrInst::CreateInBounds(
							 valeur1,
							 valeur2,
							 "",
							 contexte.bloc_courant());
			}
			else {
				if (est_type_tableau_fixe(type1)) {
					valeur = accede_element_tableau(
								 contexte,
								 valeur1,
								 converti_type_llvm(contexte, index_type1),
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
			charge->setAlignment(alignement(contexte, type1));
			return charge;
		}
		case type_noeud::RETOUR_MULTIPLE:
		{
			return nullptr;
		}
		case type_noeud::RETOUR_SIMPLE:
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
		case type_noeud::CHAINE_LITTERALE:
		{
			auto builder = llvm::IRBuilder<>(contexte.contexte);
			builder.SetInsertPoint(contexte.bloc_courant());

			auto chaine = std::any_cast<dls::chaine>(b->valeur_calculee);

			auto pointeur_chaine_c = builder.CreateGlobalStringPtr(chaine.c_str());

			/* crée la constante pour la taille */
			auto valeur_taille = builder.getInt64(static_cast<uint64_t>(chaine.taille()));

			/* crée la chaine */
			auto type = converti_type_llvm(contexte, b->index_type);

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
		case type_noeud::BOOLEEN:
		{
			auto const est_calcule = possede_drapeau(b->drapeaux, EST_CALCULE);
			auto const valeur = est_calcule ? std::any_cast<bool>(b->valeur_calculee)
											  : (b->chaine() == "vrai");
			return llvm::ConstantInt::get(
						llvm::Type::getInt1Ty(contexte.contexte),
						static_cast<uint64_t>(valeur),
						false);
		}
		case type_noeud::CARACTERE:
		{
			auto valeur = dls::caractere_echappe(&b->morceau.chaine[0]);

			return llvm::ConstantInt::get(
						llvm::Type::getInt8Ty(contexte.contexte),
						static_cast<uint64_t>(valeur),
						false);
		}
		case type_noeud::SAUFSI:
		case type_noeud::SI:
		{
			auto const nombre_enfants = b->enfants.taille();
			auto iter_enfant = b->enfants.debut();

			/* noeud 1 : condition */
			auto enfant1 = *iter_enfant++;

			auto condition = genere_code_llvm(enfant1, contexte, false);

			if (b->type == type_noeud::SAUFSI) {
				condition = llvm::BinaryOperator::Create(llvm::Instruction::Xor, condition, condition, "", contexte.bloc_courant());
			}

			auto bloc_alors = cree_bloc(contexte, "alors");

			auto bloc_sinon = (nombre_enfants == 3)
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
			auto enfant2 = *iter_enfant++;
			enfant2->valeur_calculee = bloc_fusion;
			auto ret = genere_code_llvm(enfant2, contexte, false);

			/* noeud 3 : sinon (optionel) */
			if (nombre_enfants == 3) {
				contexte.bloc_courant(bloc_sinon);

				auto enfant3 = *iter_enfant++;
				enfant3->valeur_calculee = bloc_fusion;
				ret = genere_code_llvm(enfant3, contexte, false);
			}

			contexte.bloc_courant(bloc_fusion);

			return ret;
		}
		case type_noeud::BLOC:
		{
			llvm::Value *valeur = nullptr;

			auto bloc_entree = contexte.bloc_courant();

			contexte.empile_nombre_locales();

			for (auto enfant : b->enfants) {
				valeur = genere_code_llvm(enfant, contexte, true);

				/* nul besoin de continuer à générer du code pour des expressions qui ne
				 * seront jamais executées. À FAIRE : erreur de compilation ? */
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

			contexte.depile_nombre_locales();

			return valeur;
		}
		case type_noeud::POUR:
		{
			/* Arbre :
			 * pour
			 * - enfant 1 : déclaration variable
			 * - enfant 2 : expr
			 * - enfant 3 : bloc
			 * - enfant 4 : bloc sansarrêt ou sinon (optionel)
			 * - enfant 5 : bloc sinon (optionel)
			 *
			 * boucle:
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

			auto nombre_enfants = b->enfants.taille();
			auto iter = b->enfants.debut();

			/* on génère d'abord le type de la variable */
			auto enfant1 = *iter++;
			auto enfant2 = *iter++;
			auto enfant3 = *iter++;
			auto enfant4 = (nombre_enfants >= 4) ? *iter++ : nullptr;
			auto enfant5 = (nombre_enfants == 5) ? *iter++ : nullptr;

			auto enfant_sans_arret = enfant4;
			auto enfant_sinon = (nombre_enfants == 5) ? enfant5 : enfant4;

			auto index_type = enfant2->index_type;
			auto const &type_debut = contexte.typeuse[index_type];
			auto const type = type_debut.type_base();

			enfant1->index_type = index_type;

			/* création des blocs */
			auto bloc_boucle = cree_bloc(contexte, "boucle");
			auto bloc_corps = cree_bloc(contexte, "corps_boucle");
			auto bloc_inc = cree_bloc(contexte, "inc_boucle");

			auto bloc_sansarret = static_cast<llvm::BasicBlock *>(nullptr);
			auto bloc_sinon = static_cast<llvm::BasicBlock *>(nullptr);

			if (nombre_enfants == 4) {
				if (enfant4->identifiant() == TypeLexeme::SINON) {
					bloc_sinon = cree_bloc(contexte, "sinon_boucle");
				}
				else {
					bloc_sansarret = cree_bloc(contexte, "sansarret_boucle");
				}
			}
			else if (nombre_enfants == 5) {
				bloc_sansarret = cree_bloc(contexte, "sansarret_boucle");
				bloc_sinon = cree_bloc(contexte, "sinon_boucle");
			}

			auto bloc_apres = cree_bloc(contexte, "apres_boucle");

			contexte.empile_bloc_continue(enfant1->chaine(), bloc_inc);
			contexte.empile_bloc_arrete(enfant1->chaine(), (bloc_sinon != nullptr) ? bloc_sinon : bloc_apres);

			auto bloc_pre = contexte.bloc_courant();

			contexte.empile_nombre_locales();

			auto noeud_phi = static_cast<llvm::PHINode *>(nullptr);

			/* bloc_boucle */
			/* on crée une branche explicite dans le bloc */
			llvm::BranchInst::Create(bloc_boucle, contexte.bloc_courant());

			contexte.bloc_courant(bloc_boucle);

			const auto taille_tableau = static_cast<uint64_t>(type >> 8);
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

					if (enfant1->morceau.identifiant == TypeLexeme::VIRGULE) {
						var = enfant1->enfants.front();
						idx = enfant1->enfants.back();
					}

					if (idx != nullptr) {
						valeur_idx = builder.CreatePHI(builder.getInt32Ty(), 2, "");
						auto init_zero = builder.getInt32(0);
						valeur_idx->addIncoming(init_zero, bloc_pre);
					}

					/* création du bloc de condition */

					noeud_phi = builder.CreatePHI(converti_type_llvm(contexte, index_type), 2, dls::chaine(var->chaine()).c_str());

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

					if (b->aide_generation_code == GENERE_BOUCLE_PLAGE_INDEX) {
						auto donnees_var = DonneesVariable{};

						donnees_var.valeur = noeud_phi;
						donnees_var.index_type = var->index_type;
						contexte.pousse_locale(var->chaine(), donnees_var);

						donnees_var.valeur = valeur_idx;
						donnees_var.index_type = idx->index_type;
						contexte.pousse_locale(idx->chaine(), donnees_var);
					}
					else {
						auto donnees_var = DonneesVariable{};
						donnees_var.valeur = noeud_phi;
						donnees_var.index_type = index_type;
						contexte.pousse_locale(enfant1->chaine(), donnees_var);
					}

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
										 taille_tableau,
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
									 converti_type_llvm(contexte, index_type),
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

					if (b->aide_generation_code == GENERE_BOUCLE_TABLEAU_INDEX) {
						auto var = enfant1->enfants.front();
						auto idx = enfant1->enfants.back();

						auto donnees_var = DonneesVariable{};

						donnees_var.valeur = valeur_arg;
						donnees_var.index_type = index_type;
						contexte.pousse_locale(var->chaine(), donnees_var);

						donnees_var.valeur = noeud_phi;
						donnees_var.index_type = idx->index_type;
						contexte.pousse_locale(idx->chaine(), donnees_var);
					}
					else {
						auto donnees_var = DonneesVariable{};
						donnees_var.valeur = valeur_arg;
						donnees_var.index_type = index_type;
						contexte.pousse_locale(enfant1->chaine(), donnees_var);
					}

					/* création du bloc de corps */
					builder.SetInsertPoint(contexte.bloc_courant());

					enfant3->valeur_calculee = bloc_inc;
					ret = genere_code_llvm(enfant3, contexte, false);

					/* bloc_inc */
					contexte.bloc_courant(bloc_inc);
					builder.SetInsertPoint(contexte.bloc_courant());

					auto inc = incremente_pour_type(
								   TypeLexeme::N64,
								   contexte,
								   noeud_phi,
								   contexte.bloc_courant());

					noeud_phi->addIncoming(inc, contexte.bloc_courant());

					break;
				}
				case GENERE_BOUCLE_COROUTINE:
				case GENERE_BOUCLE_COROUTINE_INDEX:
				{
					/* À FAIRE */
					break;
				}
			}

			ret = llvm::BranchInst::Create(bloc_boucle, contexte.bloc_courant());

			/* 'continue'/'arrête' dans les blocs 'sinon'/'sansarrêt' n'a aucun sens */
			contexte.depile_bloc_continue();
			contexte.depile_bloc_arrete();

			if (bloc_sansarret != nullptr) {
				contexte.bloc_courant(bloc_sansarret);
				enfant_sans_arret->valeur_calculee = bloc_apres;
				ret = genere_code_llvm(enfant_sans_arret, contexte, false);
			}

			if (bloc_sinon != nullptr) {
				contexte.bloc_courant(bloc_sinon);
				enfant_sinon->valeur_calculee = bloc_apres;
				ret = genere_code_llvm(enfant_sinon, contexte, false);
			}

			contexte.depile_nombre_locales();
			contexte.bloc_courant(bloc_apres);

			return ret;
		}
		case type_noeud::CONTINUE_ARRETE:
		{
			auto chaine_var = b->enfants.est_vide() ? dls::vue_chaine_compacte{""} : b->enfants.front()->chaine();

			auto bloc = (b->morceau.identifiant == TypeLexeme::CONTINUE)
						? contexte.bloc_continue(chaine_var)
						: contexte.bloc_arrete(chaine_var);

			return llvm::BranchInst::Create(bloc, contexte.bloc_courant());
		}
		case type_noeud::BOUCLE:
		{
			/* boucle:
			 *	corps
			 *  br boucle
			 *
			 * apres_boucle:
			 *	...
			 */

			auto enfant = b->enfants.front();

			auto bloc_boucle = cree_bloc(contexte, "boucle");
			auto bloc_apres = cree_bloc(contexte, "apres_boucle");

			contexte.empile_bloc_continue("", bloc_boucle);
			contexte.empile_bloc_arrete("", bloc_apres);

			/* on crée une branche explicite dans le bloc */
			llvm::BranchInst::Create(bloc_boucle, contexte.bloc_courant());

			contexte.bloc_courant(bloc_boucle);

			enfant->valeur_calculee = bloc_boucle;
			auto ret = genere_code_llvm(enfant, contexte, false);

			contexte.depile_bloc_continue();
			contexte.depile_bloc_arrete();
			contexte.bloc_courant(bloc_apres);

			return ret;
		}
		case type_noeud::REPETE:
		{
			auto enfant = b->enfants.front();

			auto bloc_boucle = cree_bloc(contexte, "repete");
			auto bloc_tantque = cree_bloc(contexte, "tantque_boucle");
			auto bloc_apres = cree_bloc(contexte, "apres_repete");

			contexte.empile_bloc_continue("", bloc_boucle);
			contexte.empile_bloc_arrete("", bloc_apres);

			/* on crée une branche explicite dans le bloc */
			llvm::BranchInst::Create(bloc_boucle, contexte.bloc_courant());

			contexte.bloc_courant(bloc_boucle);

			enfant->valeur_calculee = bloc_tantque;
			auto ret = genere_code_llvm(enfant, contexte, false);

			contexte.bloc_courant(bloc_tantque);

			auto condition = genere_code_llvm(b->enfants.back(), contexte, false);

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
		case type_noeud::TANTQUE:
		{
			auto enfant_condition = b->enfants.front();
			auto enfant_bloc = b->enfants.back();

			auto bloc_tantque_cond = cree_bloc(contexte, "tantque_cond");
			auto bloc_tantque_corps = cree_bloc(contexte, "tantque_corps");
			auto bloc_apres = cree_bloc(contexte, "apres_tantque");

			contexte.empile_bloc_continue("", bloc_tantque_cond);
			contexte.empile_bloc_arrete("", bloc_apres);

			/* on crée une branche explicite dans le bloc */
			llvm::BranchInst::Create(bloc_tantque_cond, contexte.bloc_courant());

			contexte.bloc_courant(bloc_tantque_cond);

			auto condition = genere_code_llvm(enfant_condition, contexte, false);

			llvm::BranchInst::Create(
						bloc_tantque_corps,
						bloc_apres,
						condition,
						contexte.bloc_courant());

			contexte.bloc_courant(bloc_tantque_corps);

			enfant_bloc->valeur_calculee = bloc_tantque_cond;
			genere_code_llvm(enfant_bloc, contexte, false);

			contexte.depile_bloc_continue();
			contexte.depile_bloc_arrete();
			contexte.bloc_courant(bloc_apres);
			return nullptr;
		}
		case type_noeud::TRANSTYPE:
		{
			auto enfant = b->enfants.front();
			auto valeur = genere_code_llvm(enfant, contexte, false);
			auto const &index_type_de = enfant->index_type;

			if (index_type_de == b->index_type) {
				return valeur;
			}

			auto const &donnees_type_de = contexte.typeuse[index_type_de];

			using CastOps = llvm::Instruction::CastOps;
			auto &dt = contexte.typeuse[b->index_type];

			auto type = converti_type_llvm(contexte, dt);
			auto bloc = contexte.bloc_courant();
			auto type_de = donnees_type_de.type_base();
			auto type_vers = dt.type_base();

			if (est_type_entier(type_de)) {
				/* un nombre entier peut être converti en l'adresse d'un pointeur */
				if (type_vers == TypeLexeme::POINTEUR) {
					return cree_instruction<CastOps::IntToPtr>(valeur, type, bloc);
				}

				if (est_type_reel(type_vers)) {
					if (est_type_entier_naturel(type_de)) {
						return cree_instruction<CastOps::UIToFP>(valeur, type, bloc);
					}

					return cree_instruction<CastOps::SIToFP>(valeur, type, bloc);
				}

				if (est_type_entier(type_vers)) {
					if (est_plus_petit(type_vers, type_de)) {
						return cree_instruction<CastOps::Trunc>(valeur, type, bloc);
					}

					if (est_type_entier_naturel(type_de)) {
						return cree_instruction<CastOps::ZExt>(valeur, type, bloc);
					}

					return cree_instruction<CastOps::SExt>(valeur, type, bloc);
				}
			}

			if (est_type_reel(type_de)) {
				if (est_type_entier_naturel(type_vers)) {
					return cree_instruction<CastOps::FPToUI>(valeur, type, bloc);
				}

				if (est_type_entier_relatif(type_vers)) {
					return cree_instruction<CastOps::FPToSI>(valeur, type, bloc);
				}

				if (est_type_reel(type_de)) {
					if (est_plus_petit(type_vers, type_de)) {
						return cree_instruction<CastOps::FPTrunc>(valeur, type, bloc);
					}

					return cree_instruction<CastOps::FPExt>(valeur, type, bloc);
				}
			}

			if (type_de == TypeLexeme::POINTEUR && est_type_entier(type_vers)) {
				return cree_instruction<CastOps::PtrToInt>(valeur, type, bloc);
			}

			/* À FAIRE : BitCast (Type Cast) */
			return nullptr;
		}
		case type_noeud::NUL:
		{
			return llvm::ConstantInt::get(
						llvm::Type::getInt32Ty(contexte.contexte),
						static_cast<uint64_t>(0),
						false);
		}
		case type_noeud::TAILLE_DE:
		{
			auto dl = llvm::DataLayout(contexte.module_llvm);
			auto index_dt = std::any_cast<long>(b->valeur_calculee);
			auto &donnees = contexte.typeuse[index_dt];
			auto type = converti_type_llvm(contexte, donnees);
			auto taille = dl.getTypeAllocSize(type);

			return llvm::ConstantInt::get(
						llvm::Type::getInt32Ty(contexte.contexte),
						taille,
						false);
		}
		case type_noeud::PLAGE:
		{
			auto iter = b->enfants.debut();

			auto enfant1 = *iter++;
			auto enfant2 = *iter++;

			auto valeur_debut = genere_code_llvm(enfant1, contexte, false);
			auto valeur_fin = genere_code_llvm(enfant2, contexte, false);

			auto donnees_var = DonneesVariable{};
			donnees_var.valeur = valeur_debut;
			donnees_var.index_type = b->index_type;

			contexte.pousse_locale("__debut", donnees_var);

			donnees_var.valeur = valeur_fin;
			contexte.pousse_locale("__fin", donnees_var);

			return valeur_fin;
		}
		case type_noeud::DIFFERE:
		{
			auto noeud = b->enfants.front();

			/* La valeur_calculee d'un bloc est son bloc suivant, qui dans le cas d'un
			 * bloc déféré n'en est aucun. */
			noeud->valeur_calculee = static_cast<llvm::BasicBlock *>(nullptr);

			contexte.differe_noeud(noeud);

			return nullptr;
		}
		case type_noeud::NONSUR:
		{
			contexte.non_sur(true);
			genere_code_llvm(b->enfants.front(), contexte, false);
			contexte.non_sur(false);
			return nullptr;
		}
		case type_noeud::TABLEAU:
		{
			auto taille_tableau = b->enfants.taille();

			auto const est_calcule = possede_drapeau(b->drapeaux, EST_CALCULE);
			if (est_calcule) {
				assert(taille_tableau == std::any_cast<long>(b->valeur_calculee));
			}

			auto &type = contexte.typeuse[b->index_type];

			/* alloue un tableau fixe */
			auto dt_tfixe = DonneesTypeFinal{};
			dt_tfixe.pousse(TypeLexeme::TABLEAU | static_cast<int>(taille_tableau << 8));
			dt_tfixe.pousse(type);

			auto type_llvm = converti_type_llvm(contexte, dt_tfixe);

			auto pointeur_tableau = new llvm::AllocaInst(
										type_llvm,
										0,
										nullptr,
										"",
										contexte.bloc_courant());

			/* copie les valeurs dans le tableau fixe */
			auto index = 0ul;

			for (auto enfant : b->enfants) {
				auto valeur_enfant = genere_code_enfant(contexte, enfant, false);

				auto index_tableau = accede_element_tableau(
										 contexte,
										 pointeur_tableau,
										 type_llvm,
										 index++);

				new llvm::StoreInst(valeur_enfant, index_tableau, contexte.bloc_courant());
			}

			/* alloue un tableau dynamique */
			return converti_vers_tableau_dyn(contexte, pointeur_tableau, dt_tfixe);
		}
		case type_noeud::CONSTRUIT_TABLEAU:
		{
			/* À FAIRE : le stockage n'a pas l'air de fonctionner. */
			dls::tableau<base *> feuilles;
			rassemble_feuilles(b, feuilles);

			/* alloue de la place pour le tableau */
			auto &dt = contexte.typeuse[b->index_type];
			auto type_llvm = converti_type_llvm(contexte, dt);

			auto pointeur_tableau = new llvm::AllocaInst(
										type_llvm,
										0,
										nullptr,
										"",
										contexte.bloc_courant());
			pointeur_tableau->setAlignment(alignement(contexte, dt));

			/* stocke les valeurs des feuilles */
			auto index = 0ul;

			for (auto f : feuilles) {
				auto valeur = genere_code_enfant(contexte, f, false);

				auto index_tableau = accede_element_tableau(
										 contexte,
										 pointeur_tableau,
										 type_llvm,
										 index++);

				auto stocke = new llvm::StoreInst(valeur, index_tableau, contexte.bloc_courant());
				stocke->setAlignment(alignement(contexte, dt));
			}

			return pointeur_tableau;
		}
		case type_noeud::CONSTRUIT_STRUCTURE:
		{
			/* À FAIRE */
			return nullptr;
		}
		case type_noeud::INFO_DE:
		{
			/* À FAIRE */
			return nullptr;
		}
		case type_noeud::MEMOIRE:
		{
			auto valeur = genere_code_llvm(b->enfants.front(), contexte, expr_gauche);
			return new llvm::LoadInst(valeur, "", contexte.bloc_courant());
		}
		case type_noeud::LOGE:
		{
			/* À FAIRE */
			return nullptr;
		}
		case type_noeud::DELOGE:
		{
			/* À FAIRE */
			return nullptr;
		}
		case type_noeud::RELOGE:
		{
			/* À FAIRE */
			return nullptr;
		}
		case type_noeud::DECLARATION_STRUCTURE:
		{
			auto donnees_structure = contexte.donnees_structure(b->chaine());
			converti_type_llvm(contexte, donnees_structure.index_type);
			return nullptr;
		}
		case type_noeud::DISCR:
		{
			/* À FAIRE */
			return nullptr;
		}
		case type_noeud::DISCR_ENUM:
		{
			/* À FAIRE */
			return nullptr;
		}
		case type_noeud::DISCR_UNION:
		{
			/* À FAIRE */
			return nullptr;
		}
		case type_noeud::RETIENS:
		{
			/* À FAIRE */
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

		/* À FAIRE : dépendances cycliques :
		 * - types qui s'incluent indirectement (listes chainées intrusives)
		 * - fonctions recursives
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
	type_tabl = contexte.typeuse.type_tableau_pour(type_tabl);

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
	auto type_contexte = converti_type_llvm(contexte, contexte.index_type_contexte);
	type_contexte = type_contexte->getPointerElementType();

	auto alloc_contexte = builder.CreateAlloca(type_contexte, 0u, nullptr, "contexte");
	alloc_contexte->setAlignment(8);

	// appel notre fonction principale en passant le contexte et le tableau
	auto fonc_princ = contexte.module_llvm->getFunction("principale");

	std::vector<llvm::Value *> parametres_appel;
	parametres_appel.push_back(alloc_contexte);

	auto charge_tabl = builder.CreateLoad(alloc_tabl, "");
	charge_tabl->setAlignment(8);

	parametres_appel.push_back(charge_tabl);

	llvm::ArrayRef<llvm::Value *> args(parametres_appel);

	auto valeur_princ = builder.CreateCall(fonc_princ, args);

	// return
	builder.CreateRet(valeur_princ);
}

void genere_code_llvm(
		assembleuse_arbre const &arbre,
		ContexteGenerationCode &contexte)
{
	auto racine = arbre.racine();

	if (racine == nullptr) {
		return;
	}

	if (racine->type != type_noeud::RACINE) {
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
		"InfoTypeRéel",
		"InfoTypePointeur",
		"InfoTypeÉnum",
		"InfoTypeStructure",
		"InfoTypeTableau",
		"InfoTypeFonction",
		"InfoTypeMembreStructure",
	};

	for (auto nom_struct : noms_structs_infos_types) {
		auto const &ds = contexte.donnees_structure(nom_struct);
		auto noeud = graphe_dependance.cherche_noeud_type(ds.index_type);

		traverse_graphe_pour_generation_code(contexte, noeud);
	}

	traverse_graphe_pour_generation_code(contexte, noeud_fonction_principale);

	genere_fonction_vraie_principale(contexte);

	contexte.temps_generation = temps_generation.temps();
}

}  /* namespace noeud */
