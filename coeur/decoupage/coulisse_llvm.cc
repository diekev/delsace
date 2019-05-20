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

#include <delsace/chrono/chronometrage.hh>

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
#include "contexte_generation_code.h"
#include "erreur.h"
#include "info_type.hh"
#include "modules.hh"
#include "nombres.h"
#include "validation_semantique.hh"

#undef NOMME_IR

/* ************************************************************************** */

/* À FAIRE (coulisse LLVM)
 * - type 'chaine'
 * - noeud 'mémoire', 'déclaration structure/énum'
 * - infos types
 * - loge, déloge, reloge
 * - opérateurs : +=, -=, etc..
 * - ajourne opérateur [] pour les chaines
 * - converti paramètres fonction principale en un tableau
 * - boucle 'tantque'
 * - raccourci opérateurs comparaisons (a <= b <= c au lieu de a <= b && b <= c)
 * - prend en compte la portée des blocs pour générer le code des noeuds différés
 * - conversion tableau octet
 * - union accès membre par '.' et par 'de'
 * - déclaration structure/énum
 * - associe
 * - ajourne la génération de code pour les boucles 'pour'
 * - saufsi
 * - trace de la mémoire utilisée
 * - retiens
 */

/* ************************************************************************** */

static void genere_code_extra_pre_retour(ContexteGenerationCode &contexte)
{
	/* génère le code pour les blocs déférés */
	auto pile_noeud = contexte.noeuds_differes();

	while (!pile_noeud.empty()) {
		auto noeud = pile_noeud.back();
		genere_code_llvm(noeud, contexte, true);
		pile_noeud.pop_back();
	}
}

/* ************************************************************************** */

static size_t taille_de(id_morceau type)
{
	switch (type) {
		case id_morceau::BOOL:
			return 1;
		case id_morceau::N8:
		case id_morceau::Z8:
			return 8;
		case id_morceau::N16:
		case id_morceau::R16:
		case id_morceau::Z16:
			return 16;
		case id_morceau::N32:
		case id_morceau::R32:
		case id_morceau::Z32:
			return 32;
		case id_morceau::N64:
		case id_morceau::R64:
		case id_morceau::Z64:
		case id_morceau::POINTEUR:
			return 64;
		default:
			return 0ul;
	}
}

static bool est_plus_petit(id_morceau type1, id_morceau type2)
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
		DonneesType &donnees_retour,
		bool est_variadique)
{
	std::vector<llvm::Type *> parametres;
	parametres.reserve(donnees_fonction.nom_args.size());

	for (auto const &nom : donnees_fonction.nom_args) {
		auto const &argument = donnees_fonction.args.find(nom);

		if (argument->second.est_variadic) {
			/* les arguments variadics sont transformés en un tableau */
			if (!donnees_fonction.est_externe) {
				auto dt = DonneesType{};
				dt.pousse(id_morceau::TABLEAU);
				dt.pousse(contexte.magasin_types.donnees_types[argument->second.donnees_type]);

				parametres.push_back(contexte.magasin_types.converti_type(contexte, dt));
			}

			break;
		}

		parametres.push_back(contexte.magasin_types.converti_type(contexte, argument->second.donnees_type));
	}

	return llvm::FunctionType::get(
				contexte.magasin_types.converti_type(contexte, donnees_retour),
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
		DonneesType const &donnees_type,
		bool charge_ = true)
{
	/* trouve le type de la structure tableau */
	auto deref = donnees_type.derefence();
	auto dt = DonneesType{};
	dt.pousse(id_morceau::TABLEAU);
	dt.pousse(deref);

	auto type_llvm = contexte.magasin_types.converti_type(contexte, dt);

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
		DonneesType &donnees_type,
		bool charge_)
{
	/* alloue de l'espace pour un eini */
	auto dt = DonneesType{};
	dt.pousse(id_morceau::EINI);

	auto type_eini_llvm = contexte.magasin_types.converti_type(contexte, dt);

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
		base *enfant)
{
	auto conversion = (enfant->drapeaux & MASQUE_CONVERSION) != 0;
	auto valeur_enfant = genere_code_llvm(enfant, contexte, conversion);

	if (conversion) {
		auto &dt = contexte.magasin_types.donnees_types[enfant->index_type];

		if ((enfant->drapeaux & CONVERTI_TABLEAU) != 0) {
			valeur_enfant = converti_vers_tableau_dyn(contexte, valeur_enfant, dt, false);
		}

		if ((enfant->drapeaux & CONVERTI_EINI) != 0) {
			valeur_enfant = converti_vers_eini(contexte, valeur_enfant, dt, false);
		}

		if ((enfant->drapeaux & EXTRAIT_EINI) != 0) {
			auto ptr_valeur = accede_membre_structure(contexte, valeur_enfant, POINTEUR_EINI);

			/* déréfence ce pointeur */
			auto charge = new llvm::LoadInst(ptr_valeur, "", false, contexte.bloc_courant());
			charge->setAlignment(8);

			/* transtype le pointeur vers le bon type */
			auto dt_vers = DonneesType{};
			dt_vers.pousse(id_morceau::POINTEUR);
			dt_vers.pousse(contexte.magasin_types.donnees_types[enfant->index_type]);

			auto type_llvm = contexte.magasin_types.converti_type(contexte, dt_vers);
			valeur_enfant = new llvm::BitCastInst(charge, type_llvm, "", contexte.bloc_courant());

			/* déréfence ce pointeur : après la portée */
		}

		auto charge = new llvm::LoadInst(valeur_enfant, "", contexte.bloc_courant());
		//charge->setAlignment(8);
		valeur_enfant = charge;
	}

	return valeur_enfant;
}

template <typename Conteneur>
llvm::Value *cree_appel(
		ContexteGenerationCode &contexte,
		llvm::Value *fonction,
		Conteneur const &conteneur)
{
	std::vector<llvm::Value *> parametres(conteneur.size());

	std::transform(conteneur.begin(), conteneur.end(), parametres.begin(),
				   [&](base *noeud_enfant)
	{
		return genere_code_enfant(contexte, noeud_enfant);
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
		const id_morceau &type,
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
		const id_morceau &type,
		ContexteGenerationCode &contexte,
		llvm::PHINode *noeud_phi,
		llvm::BasicBlock *bloc_courant)
{
	auto type_llvm = converti_type_simple(contexte, type, nullptr);

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

llvm::Value *genere_code_llvm(
		base *b,
		ContexteGenerationCode &contexte,
		const bool expr_gauche)
{
	switch (b->type) {
		case type_noeud::RACINE:
		{
			auto temps_validation = 0.0;
			auto temps_generation = 0.0;

			for (auto noeud : b->enfants) {
				auto debut_validation = dls::chrono::maintenant();
				performe_validation_semantique(noeud, contexte);
				temps_validation += dls::chrono::delta(debut_validation);

				auto debut_generation = dls::chrono::maintenant();
				genere_code_llvm(noeud, contexte, true);
				temps_generation += dls::chrono::delta(debut_generation);
			}

			contexte.temps_generation = temps_generation;
			contexte.temps_validation = temps_validation;

			return nullptr;
		}
		case type_noeud::DECLARATION_FONCTION:
		{
			auto module = contexte.module(static_cast<size_t>(b->morceau.module));
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
			auto &this_dt = contexte.magasin_types.donnees_types[b->index_type];
			auto type_fonction = obtiens_type_fonction(
									 contexte,
									 *donnees_fonction,
									 this_dt,
									 (b->drapeaux & VARIADIC) != 0);

			contexte.magasin_types.donnees_types[donnees_fonction->index_type].type_llvm(type_fonction);

			auto const est_externe = possede_drapeau(b->drapeaux, EST_EXTERNE);

			/* Crée fonction */
			auto fonction = llvm::Function::Create(
								type_fonction,
								llvm::Function::ExternalLinkage,
								donnees_fonction->nom_broye,
								contexte.module_llvm);

			if (est_externe) {
				return fonction;
			}

			contexte.commence_fonction(fonction, donnees_fonction);

			auto block = cree_bloc(contexte, "entree");

			contexte.bloc_courant(block);

			/* Crée code pour les arguments */
			auto valeurs_args = fonction->arg_begin();

			for (auto const &nom : donnees_fonction->nom_args) {
				auto &argument = donnees_fonction->args[nom];
				auto index_type = argument.donnees_type;
				auto align = unsigned{0};
				auto type = static_cast<llvm::Type *>(nullptr);

				if (argument.est_variadic) {
					align = 8;

					auto dt = DonneesType{};
					dt.pousse(id_morceau::TABLEAU);
					dt.pousse(contexte.magasin_types.donnees_types[argument.donnees_type]);

					index_type = contexte.magasin_types.ajoute_type(dt);

					type = contexte.magasin_types.converti_type(contexte, index_type);
				}
				else {
					auto dt = contexte.magasin_types.donnees_types[argument.donnees_type];
					align = alignement(contexte, dt);
					type = contexte.magasin_types.converti_type(contexte, argument.donnees_type);
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
				donnees_var.donnees_type = index_type;

				contexte.pousse_locale(nom, donnees_var);
			}

			/* Crée code pour le bloc. */
			auto bloc = b->enfants.front();
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

				return cree_appel(contexte, charge, b->enfants);
			}

			auto fonction = contexte.module_llvm->getFunction(b->nom_fonction_appel);
			return cree_appel(contexte, fonction, b->enfants);
		}
		case type_noeud::VARIABLE:
		{
			if (b->aide_generation_code == GENERE_CODE_DECL_VAR) {
				auto const &type = contexte.magasin_types.donnees_types[b->index_type];
				auto type_llvm = contexte.magasin_types.converti_type(contexte, b->index_type);

				if (contexte.fonction == nullptr) {
					auto valeur = new llvm::GlobalVariable(
									  *contexte.module_llvm,
									  type_llvm,
									  true,
									  llvm::GlobalValue::InternalLinkage,
									  nullptr);

					valeur->setConstant((b->drapeaux & DYNAMIC) == 0);
					valeur->setAlignment(alignement(contexte, type));

					auto donnees_var = DonneesVariable{};
					donnees_var.valeur = valeur;
					donnees_var.est_dynamique = (b->drapeaux & DYNAMIC) != 0;
					donnees_var.donnees_type = b->index_type;

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
				if (type.type_base() == id_morceau::TABLEAU) {
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
				donnees_var.donnees_type = b->index_type;

				contexte.pousse_locale(b->morceau.chaine, donnees_var);

				return alloc;
			}

			auto valeur = contexte.valeur_locale(b->morceau.chaine);

			if (valeur == nullptr) {
				valeur = contexte.valeur_globale(b->morceau.chaine);

				if (valeur == nullptr && b->nom_fonction_appel != "") {
					valeur = contexte.module_llvm->getFunction(b->nom_fonction_appel);
					return valeur;
				}
			}

			if (expr_gauche || llvm::dyn_cast<llvm::PHINode>(valeur)) {
				return valeur;
			}

			auto const &index_type = b->index_type;
			auto &type = contexte.magasin_types.donnees_types[index_type];

			auto charge = new llvm::LoadInst(valeur, "", false, contexte.bloc_courant());
			charge->setAlignment(alignement(contexte, type));

			return charge;
		}
		case type_noeud::ACCES_MEMBRE:
		{
			auto structure = b->enfants.back();
			auto membre = b->enfants.front();

			auto const &index_type = structure->index_type;
			auto type_structure = contexte.magasin_types.donnees_types[index_type];

			auto est_pointeur = type_structure.type_base() == id_morceau::POINTEUR;

			if (est_pointeur) {
				type_structure = type_structure.derefence();
			}

			if (type_structure.type_base() == id_morceau::EINI) {
				auto valeur = genere_code_llvm(structure, contexte, true);

				if (est_pointeur) {
					valeur = new llvm::LoadInst(valeur, "", contexte.bloc_courant());
				}

				if (membre->chaine() == "pointeur") {
					return accede_membre_structure(contexte, valeur, POINTEUR_EINI, true);
				}

				return accede_membre_structure(contexte, valeur, TYPE_EINI, true);
			}

			/* À FAIRE : chaine */

			if ((type_structure.type_base() & 0xff) == id_morceau::TABLEAU) {
				if (!contexte.non_sur() && expr_gauche) {
					erreur::lance_erreur(
								"Modification des membres du tableau hors d'un bloc 'nonsûr' interdite",
								contexte,
								b->morceau,
								erreur::type_erreur::ASSIGNATION_INVALIDE);
				}

				auto taille = static_cast<size_t>(type_structure.type_base() >> 8);

				if (taille != 0) {
					return llvm::ConstantInt::get(
								contexte.magasin_types.converti_type(contexte, b->index_type),
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

				if (b->morceau.chaine == "pointeur") {
					return accede_membre_structure(contexte, valeur, POINTEUR_TABLEAU);
				}

				return accede_membre_structure(contexte, valeur, true, TAILLE_TABLEAU);
			}

			auto index_structure = size_t(type_structure.type_base() >> 8);

			auto const &nom_membre = membre->chaine();

			auto &donnees_structure = contexte.donnees_structure(index_structure);

			auto const iter = donnees_structure.donnees_membres.find(nom_membre);

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

			ret = accede_membre_structure(contexte, valeur, index_membre);

			if (!expr_gauche) {
				auto charge = new llvm::LoadInst(ret, "", contexte.bloc_courant());
				auto const &dt = contexte.magasin_types.donnees_types[donnees_structure.donnees_types[index_membre]];
				charge->setAlignment(alignement(contexte, dt));
				ret = charge;
			}

			return ret;
		}
		case type_noeud::ACCES_MEMBRE_POINT:
		{
			return genere_code_llvm(b->enfants.back(), contexte, true);
		}
		case type_noeud::ASSIGNATION_VARIABLE:
		{
			assert(b->enfants.size() == 2);

			auto variable = b->enfants.front();
			auto expression = b->enfants.back();

			auto compatibilite = std::any_cast<niveau_compat>(b->valeur_calculee);

			if ((compatibilite & niveau_compat::converti_tableau) != niveau_compat::aucune) {
				expression->drapeaux |= CONVERTI_TABLEAU;
			}

			if ((compatibilite & niveau_compat::converti_eini) != niveau_compat::aucune) {
				expression->drapeaux |= CONVERTI_EINI;
			}

			if ((compatibilite & niveau_compat::extrait_eini) != niveau_compat::aucune) {
				expression->drapeaux |= EXTRAIT_EINI;
				expression->index_type = variable->index_type;
			}

			/* Génère d'abord le code de l'enfant afin que l'instruction d'allocation de
			 * la variable sur la pile et celle de stockage de la valeur soit côte à
			 * côte. */
			auto valeur = genere_code_enfant(contexte, expression);

			auto alloc = genere_code_llvm(variable, contexte, true);

			if (variable->aide_generation_code == GENERE_CODE_DECL_VAR && contexte.fonction == nullptr) {
				assert(est_constant(expression));
				auto vg = llvm::dyn_cast<llvm::GlobalVariable>(alloc);
				vg->setInitializer(llvm::dyn_cast<llvm::Constant>(valeur));
				return vg;
			}

			auto store = new llvm::StoreInst(valeur, alloc, false, contexte.bloc_courant());

			auto const &dt = contexte.magasin_types.donnees_types[expression->index_type];
			store->setAlignment(alignement(contexte, dt));

			return store;
		}
		case type_noeud::NOMBRE_REEL:
		{
			auto const est_calcule = possede_drapeau(b->drapeaux, EST_CALCULE);
			auto const valeur = est_calcule ? std::any_cast<double>(b->valeur_calculee) :
												converti_chaine_nombre_reel(
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
												converti_chaine_nombre_entier(
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

			auto const &type1 = contexte.magasin_types.donnees_types[index_type1];
			auto &type2 = contexte.magasin_types.donnees_types[index_type2];

			if ((b->morceau.identifiant != id_morceau::CROCHET_OUVRANT)) {
				if (!peut_operer(type1, type2, enfant1->type, enfant2->type)) {
					erreur::lance_erreur_type_operation(
								type1,
								type2,
								contexte,
								b->morceau);
				}
			}

			/* À FAIRE : typage */

			/* Ne crée pas d'instruction de chargement si nous avons un tableau. */
			auto const valeur2_brut = ((type2.type_base() & 0xff) == id_morceau::TABLEAU);

			auto valeur1 = genere_code_llvm(enfant1, contexte, false);
			auto valeur2 = genere_code_llvm(enfant2, contexte, valeur2_brut);

			switch (b->morceau.identifiant) {
				case id_morceau::PLUS:
					if (est_type_entier(type1.type_base())) {
						instr = llvm::Instruction::Add;
					}
					else {
						instr = llvm::Instruction::FAdd;
					}

					break;
				case id_morceau::MOINS:
					if (est_type_entier(type1.type_base())) {
						instr = llvm::Instruction::Sub;
					}
					else {
						instr = llvm::Instruction::FSub;
					}

					break;
				case id_morceau::FOIS:
					if (est_type_entier(type1.type_base())) {
						instr = llvm::Instruction::Mul;
					}
					else {
						instr = llvm::Instruction::FMul;
					}

					break;
				case id_morceau::DIVISE:
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
				case id_morceau::POURCENT:
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
				case id_morceau::DECALAGE_DROITE:
					if (est_type_entier_naturel(type1.type_base())) {
						instr = llvm::Instruction::LShr;
					}
					else if (est_type_entier_relatif(type1.type_base())) {
						instr = llvm::Instruction::AShr;
					}
					else {
						erreur::lance_erreur(
									"Besoin d'un type entier pour le décalage !",
									contexte,
									b->morceau,
									erreur::type_erreur::TYPE_DIFFERENTS);
					}
					break;
				case id_morceau::DECALAGE_GAUCHE:
					if (!est_type_entier(type1.type_base())) {
						erreur::lance_erreur(
									"Besoin d'un type entier pour le décalage !",
									contexte,
									b->morceau,
									erreur::type_erreur::TYPE_DIFFERENTS);
					}

					instr = llvm::Instruction::Shl;
					break;
				case id_morceau::ESPERLUETTE:
				case id_morceau::ESP_ESP:
					if (!est_type_entier(type1.type_base())) {
						erreur::lance_erreur(
									"Besoin d'un type entier pour l'opération binaire !",
									contexte,
									b->morceau,
									erreur::type_erreur::TYPE_DIFFERENTS);
					}
					instr = llvm::Instruction::And;
					break;
				case id_morceau::BARRE:
				case id_morceau::BARRE_BARRE:
					if (!est_type_entier(type1.type_base())) {
						erreur::lance_erreur(
									"Besoin d'un type entier pour l'opération binaire !",
									contexte,
									b->morceau,
									erreur::type_erreur::TYPE_DIFFERENTS);
					}
					instr = llvm::Instruction::Or;
					break;
				case id_morceau::CHAPEAU:
					if (!est_type_entier(type1.type_base())) {
						erreur::lance_erreur(
									"Besoin d'un type entier pour l'opération binaire !",
									contexte,
									b->morceau,
									erreur::type_erreur::TYPE_DIFFERENTS);
					}
					instr = llvm::Instruction::Xor;
					break;
					/* À FAIRE. */
				case id_morceau::INFERIEUR:
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
				case id_morceau::INFERIEUR_EGAL:
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
				case id_morceau::SUPERIEUR:
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
				case id_morceau::SUPERIEUR_EGAL:
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
				case id_morceau::EGALITE:
					if (est_type_entier(type1.type_base())) {
						est_comp_entier = true;
						predicat = llvm::CmpInst::Predicate::ICMP_EQ;
					}
					else {
						est_comp_reel = true;
						predicat = llvm::CmpInst::Predicate::FCMP_OEQ;
					}

					break;
				case id_morceau::DIFFERENCE:
					if (est_type_entier(type1.type_base())) {
						est_comp_entier = true;
						predicat = llvm::CmpInst::Predicate::ICMP_NE;
					}
					else {
						est_comp_reel = true;
						predicat = llvm::CmpInst::Predicate::FCMP_ONE;
					}

					break;
				case id_morceau::CROCHET_OUVRANT:
				{
					/* À FAIRE : la compilation de l'opérateur du crochet
					 * ouvrant a été changé, vérifié si c'est toujours correcte
					 */
					if (type1.type_base() != id_morceau::POINTEUR && (type1.type_base() & 0xff) != id_morceau::TABLEAU) {
						erreur::lance_erreur(
									"Le type ne peut être déréférencé !",
									contexte,
									b->morceau,
									erreur::type_erreur::TYPE_DIFFERENTS);
					}

					llvm::Value *valeur;

					if (type2.type_base() == id_morceau::POINTEUR) {
						valeur = llvm::GetElementPtrInst::CreateInBounds(
									 valeur1,
									 valeur2,
									 "",
									 contexte.bloc_courant());
					}
					else {
						valeur = accede_element_tableau(
									 contexte,
									 valeur1,
									 contexte.magasin_types.converti_type(contexte, index_type1),
									 valeur2);
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
					charge->setAlignment(alignement(contexte, type2));
					return charge;
				}
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
			auto const &type1 = contexte.magasin_types.donnees_types[index_type1];
			auto valeur1 = genere_code_llvm(enfant, contexte, false);
			auto valeur2 = static_cast<llvm::Value *>(nullptr);

			switch (b->morceau.identifiant) {
				case id_morceau::EXCLAMATION:
				{
					instr = llvm::Instruction::Xor;
					valeur2 = valeur1;
					break;
				}
				case id_morceau::TILDE:
				{
					instr = llvm::Instruction::Xor;
					valeur2 = llvm::ConstantInt::get(
								  llvm::Type::getInt32Ty(contexte.contexte),
								  static_cast<uint64_t>(0),
								  false);
					break;
				}
				case id_morceau::AROBASE:
				{
					auto inst_load = llvm::dyn_cast<llvm::LoadInst>(valeur1);

					if (inst_load == nullptr) {
						/* Ne devrais pas arriver. */
						return nullptr;
					}

					return inst_load->getPointerOperand();
				}
				case id_morceau::PLUS_UNAIRE:
				{
					return valeur1;
				}
				case id_morceau::MOINS_UNAIRE:
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
		case type_noeud::RETOUR:
		{
			llvm::Value *valeur = nullptr;

			if (!b->enfants.empty()) {
				assert(b->enfants.size() == 1);
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
			auto chaine = std::any_cast<std::string>(b->valeur_calculee);

			auto constante = llvm::ConstantDataArray::getString(
								 contexte.contexte,
								 chaine);

			auto type = contexte.magasin_types.converti_type(contexte, b->index_type);

			auto globale = new llvm::GlobalVariable(
							   *contexte.module_llvm,
							   type,
							   true,
							   llvm::GlobalValue::PrivateLinkage,
							   constante,
							   ".chn");

			globale->setAlignment(1);
			globale->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);

			return accede_membre_structure(contexte, globale, 0);
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
			auto valeur = caractere_echape(&b->morceau.chaine[0]);

			return llvm::ConstantInt::get(
						llvm::Type::getInt8Ty(contexte.contexte),
						static_cast<uint64_t>(valeur),
						false);
		}
		case type_noeud::SAUFSI:
		case type_noeud::SI:
		{
			auto const nombre_enfants = b->enfants.size();
			auto iter_enfant = b->enfants.begin();

			/* noeud 1 : condition */
			auto enfant1 = *iter_enfant++;

			auto condition = genere_code_llvm(enfant1, contexte, false);

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

			auto nombre_enfants = b->enfants.size();
			auto iter = b->enfants.begin();

			/* on génère d'abord le type de la variable */
			auto enfant1 = *iter++;
			auto enfant2 = *iter++;
			auto enfant3 = *iter++;
			auto enfant4 = (nombre_enfants >= 4) ? *iter++ : nullptr;
			auto enfant5 = (nombre_enfants == 5) ? *iter++ : nullptr;

			auto enfant_sans_arret = enfant4;
			auto enfant_sinon = (nombre_enfants == 5) ? enfant5 : enfant4;

			auto index_type = enfant2->index_type;
			auto const &type_debut = contexte.magasin_types.donnees_types[index_type];
			auto const type = type_debut.type_base();

			enfant1->index_type = index_type;

			/* création des blocs */
			auto bloc_boucle = cree_bloc(contexte, "boucle");
			auto bloc_corps = cree_bloc(contexte, "corps_boucle");
			auto bloc_inc = cree_bloc(contexte, "inc_boucle");

			auto bloc_sansarret = static_cast<llvm::BasicBlock *>(nullptr);
			auto bloc_sinon = static_cast<llvm::BasicBlock *>(nullptr);

			if (nombre_enfants == 4) {
				if (enfant4->identifiant() == id_morceau::SINON) {
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

			const auto tableau = (type & 0xff) == id_morceau::TABLEAU;
			const auto taille_tableau = static_cast<uint64_t>(type >> 8);
			auto pointeur_tableau = static_cast<llvm::Value *>(nullptr);

			if (enfant2->type == type_noeud::PLAGE) {
				noeud_phi = llvm::PHINode::Create(
								contexte.magasin_types.converti_type(contexte, index_type),
								2,
								std::string(enfant1->chaine()),
								contexte.bloc_courant());
			}
			else if (enfant2->type == type_noeud::VARIABLE) {
				noeud_phi = llvm::PHINode::Create(
								tableau ? llvm::Type::getInt64Ty(contexte.contexte)
										: llvm::Type::getInt32Ty(contexte.contexte),
								2,
								std::string(enfant1->chaine()),
								contexte.bloc_courant());
			}

			if (enfant2->type == type_noeud::PLAGE) {
				genere_code_llvm(enfant2, contexte, false);

				auto valeur_debut = contexte.valeur_locale("__debut");
				auto valeur_fin = contexte.valeur_locale("__fin");

				noeud_phi->addIncoming(valeur_debut, bloc_pre);

				auto condition = comparaison_pour_type(
									 type,
									 noeud_phi,
									 valeur_fin,
									 contexte.bloc_courant());

				llvm::BranchInst::Create(
							bloc_corps,
							(bloc_sansarret != nullptr) ? bloc_sansarret : bloc_apres,
							condition,
							contexte.bloc_courant());

				auto donnees_var = DonneesVariable{};
				donnees_var.valeur = noeud_phi;
				donnees_var.donnees_type = index_type;

				contexte.pousse_locale(enfant1->chaine(), donnees_var);
			}
			else if (enfant2->type == type_noeud::VARIABLE) {
				if (tableau) {
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
				}
			}

			/* bloc_corps */
			contexte.bloc_courant(bloc_corps);

			if (tableau) {
				auto valeur_arg = static_cast<llvm::Value *>(nullptr);

				if (taille_tableau != 0) {
					auto valeur_tableau = genere_code_llvm(enfant2, contexte, true);

					valeur_arg = accede_element_tableau(
								 contexte,
								 valeur_tableau,
								 contexte.magasin_types.converti_type(contexte, index_type),
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

				auto donnees_var = DonneesVariable{};
				donnees_var.valeur = valeur_arg;
				donnees_var.donnees_type = index_type;

				contexte.pousse_locale(enfant1->chaine(), donnees_var);
			}

			enfant3->valeur_calculee = bloc_inc;
			auto ret = genere_code_llvm(enfant3, contexte, false);

			/* bloc_inc */
			contexte.bloc_courant(bloc_inc);

			if (enfant2->type == type_noeud::PLAGE) {
				auto inc = incremente_pour_type(
							   type,
							   contexte,
							   noeud_phi,
							   contexte.bloc_courant());

				noeud_phi->addIncoming(inc, contexte.bloc_courant());
			}
			else if (enfant2->type == type_noeud::VARIABLE) {
				auto inc = incremente_pour_type(
							   tableau ? id_morceau::N64 : id_morceau::Z32,
							   contexte,
							   noeud_phi,
							   contexte.bloc_courant());

				noeud_phi->addIncoming(inc, contexte.bloc_courant());
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
			auto chaine_var = b->enfants.empty() ? std::string_view{""} : b->enfants.front()->chaine();

			auto bloc = (b->morceau.identifiant == id_morceau::CONTINUE)
						? contexte.bloc_continue(chaine_var)
						: contexte.bloc_arrete(chaine_var);

			if (bloc == nullptr) {
				if (chaine_var.empty()) {
					erreur::lance_erreur(
								"'continue' ou 'arrête' en dehors d'une boucle",
								contexte,
								b->morceau,
								erreur::type_erreur::CONTROLE_INVALIDE);
				}
				else {
					erreur::lance_erreur(
								"Variable inconnue",
								contexte,
								b->enfants.front()->donnees_morceau(),
								erreur::type_erreur::VARIABLE_INCONNUE);
				}
			}

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

			/* création des blocs */
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
		case type_noeud::TANTQUE:
		{
			/* À FAIRE */
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

			auto const &donnees_type_de = contexte.magasin_types.donnees_types[index_type_de];

			using CastOps = llvm::Instruction::CastOps;
			auto const &dt = contexte.magasin_types.donnees_types[b->index_type];

			auto type = contexte.magasin_types.converti_type(contexte, b->index_type);
			auto bloc = contexte.bloc_courant();
			auto type_de = donnees_type_de.type_base();
			auto type_vers = dt.type_base();

			if (est_type_entier(type_de)) {
				/* un nombre entier peut être converti en l'adresse d'un pointeur */
				if (type_vers == id_morceau::POINTEUR) {
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

			if (type_de == id_morceau::POINTEUR && est_type_entier(type_vers)) {
				return cree_instruction<CastOps::PtrToInt>(valeur, type, bloc);
			}

			/* À FAIRE : BitCast (Type Cast) */
			erreur::lance_erreur_type_operation(
						donnees_type_de,
						dt,
						contexte,
						b->donnees_morceau());
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
			auto donnees = std::any_cast<size_t>(b->valeur_calculee);
			auto type = contexte.magasin_types.converti_type(contexte, donnees);
			auto taille = dl.getTypeAllocSize(type);

			return llvm::ConstantInt::get(
						llvm::Type::getInt32Ty(contexte.contexte),
						taille,
						false);
		}
		case type_noeud::PLAGE:
		{
			auto iter = b->enfants.begin();

			auto enfant1 = *iter++;
			auto enfant2 = *iter++;

			auto valeur_debut = genere_code_llvm(enfant1, contexte, false);
			auto valeur_fin = genere_code_llvm(enfant2, contexte, false);

			auto donnees_var = DonneesVariable{};
			donnees_var.valeur = valeur_debut;
			donnees_var.donnees_type = b->index_type;

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
			auto taille_tableau = b->enfants.size();

			auto const est_calcule = possede_drapeau(b->drapeaux, EST_CALCULE);
			if (est_calcule) {
				assert(static_cast<long>(taille_tableau) == std::any_cast<long>(b->valeur_calculee));
			}

			auto &type = contexte.magasin_types.donnees_types[b->index_type];

			/* alloue un tableau fixe */
			auto dt_tfixe = DonneesType{};
			dt_tfixe.pousse(id_morceau::TABLEAU | static_cast<int>(taille_tableau << 8));
			dt_tfixe.pousse(type);

			auto type_llvm = contexte.magasin_types.converti_type(contexte, dt_tfixe);

			auto pointeur_tableau = new llvm::AllocaInst(
										type_llvm,
										0,
										nullptr,
										"",
										contexte.bloc_courant());

			/* copie les valeurs dans le tableau fixe */
			auto index = 0ul;

			for (auto enfant : b->enfants) {
				auto valeur_enfant = genere_code_enfant(contexte, enfant);

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
			std::vector<base *> feuilles;
			rassemble_feuilles(b, feuilles);

			/* alloue de la place pour le tableau */
			auto dt = contexte.magasin_types.donnees_types[b->index_type];
			auto type_llvm = contexte.magasin_types.converti_type(contexte, dt);

			auto pointeur_tableau = new llvm::AllocaInst(
										type_llvm,
										0,
										nullptr,
										"",
										contexte.bloc_courant());
			pointeur_tableau->setAlignment(4); /* À FAIRE : nombre magic pour les z32 */

			/* stocke les valeurs des feuilles */
			auto index = 0ul;

			for (auto f : feuilles) {
				auto valeur = genere_code_enfant(contexte, f);

				auto index_tableau = accede_element_tableau(
										 contexte,
										 pointeur_tableau,
										 type_llvm,
										 index++);

				auto stocke = new llvm::StoreInst(valeur, index_tableau, contexte.bloc_courant());
				stocke->setAlignment(4); /* À FAIRE : nombre magic pour les z32 */
			}

			return pointeur_tableau;
		}
		case type_noeud::CONSTRUIT_STRUCTURE:
		{
			/* À FAIRE */
			return nullptr;
		}
		case type_noeud::TYPE_DE:
		{
			auto enfant = b->enfants.front();

			genere_code_llvm(enfant, contexte, false);

			auto valeur = llvm::ConstantInt::get(
							  llvm::Type::getInt64Ty(contexte.contexte),
							  enfant->index_type,
							  false);

			return valeur;
		}
		case type_noeud::MEMOIRE:
		{
			/* À FAIRE */
			return nullptr;
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
			/* À FAIRE */
			return nullptr;
		}
		case type_noeud::DECLARATION_ENUM:
		{
			/* À FAIRE */
			return nullptr;
		}
		case type_noeud::ASSOCIE:
		{
			/* À FAIRE */
			return nullptr;
		}
		case type_noeud::PAIRE_ASSOCIATION:
		{
			/* RAF : pris en charge dans type_noeud::ASSOCIE, ce noeud n'est que
			 * pour ajouter un niveau d'indirection et faciliter la compilation
			 * des associations. */
			assert(false);
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

}  /* namespace noeud */
