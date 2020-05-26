﻿/*
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

#include "validation_semantique.hh"

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/outils/chaine.hh"
#include "biblinternes/outils/conditions.h"
#include "biblinternes/structures/dico_fixe.hh"
#include "biblinternes/structures/ensemblon.hh"
#include "biblinternes/structures/flux_chaine.hh"
#include "biblinternes/structures/magasin.hh"

#include "arbre_syntactic.h"
#include "assembleuse_arbre.h"
#include "broyage.hh"
#include "contexte_generation_code.h"
#include "erreur.h"
#include "expression.h"
#include "outils_lexemes.hh"
#include "portee.hh"
#include "typage.hh"
#include "validation_expression_appel.hh"

namespace noeud {

/* ************************************************************************** */

static Type *resoud_type_final(ContexteGenerationCode &contexte, NoeudExpression *expression_type)
{
	if (expression_type == nullptr) {
		return nullptr;
	}

	performe_validation_semantique(expression_type, contexte, true);

	auto type_var = expression_type->type;

	if (type_var->genre != GenreType::TYPE_DE_DONNEES) {
		erreur::lance_erreur("attendu un type de données", contexte, expression_type->lexeme);
	}

	auto type_de_donnees = static_cast<TypeTypeDeDonnees *>(type_var);

	if (type_de_donnees->type_connu == nullptr) {
		erreur::lance_erreur("impossible de définir le type selon l'expression", contexte, expression_type->lexeme);
	}

	return type_de_donnees->type_connu;
}

/* ************************************************************************** */

static NoeudBase *derniere_instruction(NoeudBloc *b)
{
	if (b->expressions.taille == 0) {
		return static_cast<NoeudBase *>(nullptr);
	}

	auto di = b->expressions[b->expressions.taille - 1];

	if (est_instruction_retour(di->genre) || (di->genre == GenreNoeud::INSTRUCTION_CONTINUE_ARRETE)) {
		return di;
	}

	if (di->genre == GenreNoeud::INSTRUCTION_SI) {
		auto inst = static_cast<NoeudSi *>(di);

		if (inst->bloc_si_faux == nullptr) {
			return static_cast<NoeudBase *>(nullptr);
		}

		return derniere_instruction(inst->bloc_si_faux);
	}

	if (di->genre == GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE) {
		auto inst = static_cast<NoeudPousseContexte *>(di);
		return derniere_instruction(inst->bloc);
	}

	return static_cast<NoeudBase *>(nullptr);
}

/* ************************************************************************** */

static void valide_acces_membre(
		ContexteGenerationCode &contexte,
		NoeudExpressionMembre *b,
		NoeudExpression *structure,
		NoeudExpression *membre,
		bool expr_gauche)
{
	structure->aide_generation_code = EST_NOEUD_ACCES;
	performe_validation_semantique(structure, contexte, expr_gauche);

	auto type = structure->type;

	/* nous pouvons avoir une référence d'un pointeur, donc déréférence au plus */
	while (type->genre == GenreType::POINTEUR || type->genre == GenreType::REFERENCE) {
		type = static_cast<TypePointeur *>(type)->type_pointe;
	}

	if (est_type_compose(type)) {
		auto type_compose = static_cast<TypeCompose *>(type);

		auto membre_trouve = false;
		auto index_membre = 0;

		POUR (type_compose->membres) {
			if (it.nom == membre->ident->nom) {
				b->type = it.type;
				membre_trouve = true;
				break;
			}

			index_membre += 1;
		}

		if (membre_trouve == false) {
			erreur::membre_inconnu(contexte, b, structure, membre, type_compose);
		}

		b->index_membre = index_membre;

		if (type->genre == GenreType::ENUM || type->genre == GenreType::ERREUR) {
			b->genre_valeur = GenreValeur::DROITE;
		}
		else if (type->genre == GenreType::UNION) {
			auto noeud_struct = static_cast<TypeUnion *>(type)->decl;
			if (!noeud_struct->est_nonsure) {
				b->genre = GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION;

				if (expr_gauche) {
					contexte.renseigne_membre_actif(structure->ident->nom, membre->ident->nom);
				}
				else {
					auto membre_actif = contexte.trouve_membre_actif(structure->ident->nom);

					contexte.donnees_dependance.fonctions_utilisees.insere(contexte.interface_kuri.decl_panique_membre_union);

					/* si l'union vient d'un retour ou d'un paramètre, le membre actif sera inconnu */
					if (membre_actif != "") {
						if (membre_actif != membre->ident->nom) {
							erreur::membre_inactif(contexte, b, structure, membre);
						}

						/* nous savons que nous avons le bon membre actif */
						b->aide_generation_code = IGNORE_VERIFICATION;
					}
				}
			}
		}

		return;
	}

	auto flux = dls::flux_chaine();
	flux << "Impossible d'accéder au membre d'un objet n'étant pas une structure";
	flux << ", le type est ";
	flux << chaine_type(type);

	erreur::lance_erreur(
				flux.chn(),
				contexte,
				structure->lexeme,
				erreur::type_erreur::TYPE_DIFFERENTS);
}

void valide_type_fonction(NoeudExpression *b, ContexteGenerationCode &contexte)
{
	// certaines fonctions sont validées 2 fois...
	if (b->type != nullptr) {
		return;
	}

	using dls::outils::possede_drapeau;

	auto decl = static_cast<NoeudDeclarationFonction *>(b);

	if (decl->est_coroutine) {
		decl->genre = GenreNoeud::DECLARATION_COROUTINE;
	}

	// -----------------------------------
	if (!decl->est_instantiation_gabarit) {
		auto noms = dls::ensemblon<IdentifiantCode *, 16>();
		auto dernier_est_variadic = false;

		POUR (decl->params) {
			auto param = static_cast<NoeudDeclarationVariable *>(it);
			auto variable = param->valeur;
			auto expression = param->expression;

			if (noms.possede(variable->ident)) {
				erreur::lance_erreur(
							"Redéfinition de l'argument",
							contexte,
							variable->lexeme,
							erreur::type_erreur::ARGUMENT_REDEFINI);
			}

			if (dernier_est_variadic) {
				erreur::lance_erreur(
							"Argument déclaré après un argument variadic",
							contexte,
							variable->lexeme,
							erreur::type_erreur::NORMAL);
			}

			if (variable->expression_type != nullptr) {
				variable->type = resoud_type_final(contexte, variable->expression_type);
			}

			if (variable->type && variable->type->drapeaux & TYPE_EST_POLYMORPHIQUE) {
				auto nom = nom_type_polymorphique(variable->type);
				decl->noms_types_gabarits.pousse(nom);
				decl->est_gabarit = true;
			}
			else {
				if (expression != nullptr) {
					if (decl->genre == GenreNoeud::DECLARATION_OPERATEUR) {
						erreur::lance_erreur("Un paramètre d'une surcharge d'opérateur ne peut avoir de valeur par défaut",
											 contexte,
											 param->lexeme);
					}

					performe_validation_semantique(expression, contexte, false);

					if (variable->type == nullptr) {
						if (expression->type->genre == GenreType::ENTIER_CONSTANT) {
							variable->type = contexte.typeuse[TypeBase::Z32];
							expression->transformation = { TypeTransformation::CONVERTI_ENTIER_CONSTANT, variable->type };
						}
						else {
							variable->type = expression->type;
						}
					}
					else if (variable->type != expression->type) {
						auto transformation = cherche_transformation(contexte, expression->type, variable->type);

						if (transformation.type == TypeTransformation::IMPOSSIBLE) {
							erreur::lance_erreur_type_arguments(
										expression->type,
										variable->type,
										contexte,
										variable->lexeme,
										expression->lexeme);
						}

						expression->transformation = transformation;
					}
				}

				if (variable->type == nullptr) {
					erreur::lance_erreur("paramètre déclaré sans type", contexte, variable->lexeme);
				}
			}

			it->type = variable->type;

			noms.insere(variable->ident);

			/* doit être vrai uniquement pour le dernier argument */
			if (it->type->genre == GenreType::VARIADIQUE) {
				it->drapeaux |= EST_VARIADIQUE;
				decl->est_variadique = true;

				if (it == decl->params[decl->params.taille - 1]) {
					auto type_var = static_cast<TypeVariadique *>(it->type);

					if (!decl->est_externe && type_var->type_pointe == nullptr) {
						erreur::lance_erreur(
									"La déclaration de fonction variadique sans type n'est"
									" implémentée que pour les fonctions externes",
									contexte,
									it->lexeme);
					}
				}
			}
		}

		if (decl->est_gabarit) {
			return;
		}
	}
	else {
		POUR (decl->params) {
			auto variable = static_cast<NoeudDeclarationVariable *>(it)->valeur;
			variable->type = resoud_type_final(contexte, variable->expression_type);
			it->type = variable->type;
		}
	}

	// -----------------------------------

	kuri::tableau<Type *> types_entrees;
	auto possede_contexte = !decl->est_externe && !possede_drapeau(b->drapeaux, FORCE_NULCTX);
	types_entrees.reserve(decl->params.taille + possede_contexte);

	if (possede_contexte) {
		types_entrees.pousse(contexte.type_contexte);
	}

	POUR (decl->params) {
		types_entrees.pousse(it->type);
		contexte.donnees_dependance.types_utilises.insere(it->type);
	}

	kuri::tableau<Type *> types_sorties;
	types_sorties.reserve(decl->params_sorties.taille);

	for (auto &type_declare : decl->params_sorties) {
		auto type_sortie = resoud_type_final(contexte, type_declare);
		types_sorties.pousse(type_sortie);
		contexte.donnees_dependance.types_utilises.insere(type_sortie);
	}

	auto type_fonc = contexte.typeuse.type_fonction(std::move(types_entrees), std::move(types_sorties));
	decl->type = type_fonc;
	contexte.donnees_dependance.types_utilises.insere(b->type);

	if (decl->genre == GenreNoeud::DECLARATION_OPERATEUR) {
		auto type_resultat = type_fonc->types_sorties[0];

		if (type_resultat == contexte.typeuse[TypeBase::RIEN]) {
			erreur::lance_erreur("Un opérateur ne peut retourner 'rien'",
								 contexte,
								 decl->lexeme);
		}

		if (est_operateur_bool(decl->lexeme->genre) && type_resultat != contexte.typeuse[TypeBase::BOOL]) {
			erreur::lance_erreur("Un opérateur de comparaison doit retourner 'bool'",
								 contexte,
								 decl->lexeme);
		}

		auto fichier = contexte.fichier(static_cast<size_t>(decl->lexeme->fichier));
		decl->nom_broye = broye_nom_fonction(decl, fichier->module->nom);

		if (decl->params.taille == 1) {
			auto &iter_op = contexte.operateurs.trouve_unaire(decl->lexeme->genre);
			auto type1 = type_fonc->types_entrees[0 + possede_contexte];

			for (auto i = 0; i < iter_op.taille(); ++i) {
				auto op = &iter_op[i];

				if (op->type_operande == type1) {
					// À FAIRE : stocke le noeud de déclaration, quid des opérateurs basique ?
					erreur::lance_erreur("redéfinition de l'opérateur", contexte, decl->lexeme);
				}
			}

			contexte.operateurs.ajoute_perso_unaire(
						decl->lexeme->genre,
						type1,
						type_resultat,
						decl);
		}
		else if (decl->params.taille == 2) {
			auto &iter_op = contexte.operateurs.trouve_binaire(decl->lexeme->genre);
			auto type1 = type_fonc->types_entrees[0 + possede_contexte];
			auto type2 = type_fonc->types_entrees[1 + possede_contexte];

			for (auto i = 0; i < iter_op.taille(); ++i) {
				auto op = &iter_op[i];

				if (op->type1 == type1 && op->type2 == type2) {
					// À FAIRE : stocke le noeud de déclaration, quid des opérateurs basique ?
					erreur::lance_erreur("redéfinition de l'opérateur", contexte, decl->lexeme);
				}
			}

			contexte.operateurs.ajoute_perso(
						decl->lexeme->genre,
						type1,
						type2,
						type_resultat,
						decl);
		}
	}
	else {
		POUR (decl->bloc_parent->membres) {
			if (it == decl) {
				continue;
			}

			if (it->genre != GenreNoeud::DECLARATION_FONCTION && it->genre != GenreNoeud::DECLARATION_COROUTINE) {
				continue;
			}

			if (it->ident != decl->ident) {
				continue;
			}

			if (it->type == decl->type) {
				erreur::redefinition_fonction(
							contexte,
							it->lexeme,
							decl->lexeme);
			}
		}

		/* nous devons attendre d'avoir les types des arguments avant de
		 * pouvoir broyer le nom de la fonction */
		if (decl->lexeme->chaine != "principale" && !possede_drapeau(b->drapeaux, EST_EXTERNE)) {
			auto fichier = contexte.fichier(static_cast<size_t>(decl->lexeme->fichier));
			decl->nom_broye = broye_nom_fonction(decl, fichier->module->nom);
		}
		else {
			decl->nom_broye = decl->lexeme->chaine;
		}
	}

	contexte.graphe_dependance.cree_noeud_fonction(decl);
}

void performe_validation_semantique(
		NoeudExpression *b,
		ContexteGenerationCode &contexte,
		bool expr_gauche)
{
	auto &graphe = contexte.graphe_dependance;
	auto &donnees_dependance = contexte.donnees_dependance;
	auto fonction_courante = contexte.donnees_fonction;

	switch (b->genre) {
		case GenreNoeud::INSTRUCTION_SINON:
		case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:
		case GenreNoeud::INSTRUCTION_RETOUR_SIMPLE:
		case GenreNoeud::DECLARATION_COROUTINE:
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		{
			break;
		}
		case GenreNoeud::DECLARATION_FONCTION:
		{
			auto decl = static_cast<NoeudDeclarationFonction *>(b);
			using dls::outils::possede_drapeau;

			if (decl->est_declaration_type) {
				auto requiers_contexte = !possede_drapeau(decl->drapeaux, FORCE_NULCTX);
				auto types_entrees = kuri::tableau<Type *>(decl->params.taille + requiers_contexte);

				if (requiers_contexte) {
					types_entrees[0] = contexte.type_contexte;
				}

				for (auto i = 0; i < decl->params.taille; ++i) {
					// le syntaxage des expressions des entrées des fonctions est commune
					// aux déclarations des fonctions et des types de fonctions faisant
					// qu'une chaine de caractère seule (référence d'un type) est considérée
					// comme étant une déclaration de variable, il nous faut donc extraire
					// le noeud de référence à la variable afin de valider correctement le type
					NoeudExpression *type_entree = decl->params[i];

					if (type_entree->genre == GenreNoeud::DECLARATION_VARIABLE) {
						type_entree = static_cast<NoeudDeclarationVariable *>(type_entree)->valeur;
					}

					types_entrees[i + requiers_contexte] = resoud_type_final(contexte, type_entree);
				}

				auto types_sorties = kuri::tableau<Type *>(decl->params_sorties.taille);

				for (auto i = 0; i < decl->params_sorties.taille; ++i) {
					types_sorties[i] = resoud_type_final(contexte, decl->params_sorties[i]);
				}

				auto type_fonction = contexte.typeuse.type_fonction(std::move(types_entrees), std::move(types_sorties));
				decl->type = contexte.typeuse.type_type_de_donnees(type_fonction);
				return;
			}

			/* Il est possible que certaines fonctions ne soient pas connectées
			 * dans le graphe de symboles alors que nous avons besoin d'elles,
			 * voir dans la fonction plus bas. */
			if (decl->type == nullptr && !decl->est_gabarit) {
				valide_type_fonction(decl, contexte);
			}

			if (decl->est_gabarit && !decl->est_instantiation_gabarit) {
				// nous ferons l'analyse sémantique plus tard
				return;
			}

			if (decl->est_externe) {
				POUR (decl->params) {
					auto variable = static_cast<NoeudDeclarationVariable *>(it)->valeur;

					variable->type = resoud_type_final(contexte, variable->expression_type);
					it->type = variable->type;
					donnees_dependance.types_utilises.insere(variable->type);
				}

				auto noeud_dep = graphe.cree_noeud_fonction(decl);
				graphe.ajoute_dependances(*noeud_dep, donnees_dependance);

				return;
			}

			contexte.commence_fonction(decl);

			auto requiers_contexte = !possede_drapeau(decl->drapeaux, FORCE_NULCTX);

			decl->bloc->membres.reserve(decl->params.taille + requiers_contexte);

			if (requiers_contexte) {
				auto val_ctx = static_cast<NoeudExpressionReference *>(contexte.assembleuse->cree_noeud(GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, b->lexeme));
				val_ctx->type = contexte.type_contexte;
				val_ctx->bloc_parent = b->bloc_parent;
				val_ctx->ident = contexte.table_identifiants.identifiant_pour_chaine("contexte");

				auto decl_ctx = static_cast<NoeudDeclarationVariable *>(contexte.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, b->lexeme));
				decl_ctx->bloc_parent = b->bloc_parent;
				decl_ctx->valeur = val_ctx;
				decl_ctx->type = val_ctx->type;
				decl_ctx->ident = val_ctx->ident;

				decl->bloc->membres.pousse(decl_ctx);

				donnees_dependance.types_utilises.insere(contexte.type_contexte);
			}

			/* Pousse les paramètres sur la pile. */
			POUR (decl->params) {
				auto argument = static_cast<NoeudDeclarationVariable *>(it);

				auto variable = argument->valeur;
				argument->ident = variable->ident;
				argument->type = variable->type;

				donnees_dependance.types_utilises.insere(argument->type);

				decl->bloc->membres.pousse(argument);

				/* À FAIRE(réusinage arbre) */
		//		if (argument.est_employe) {
		//			auto type_var = argument.type;
		//			auto nom_structure = dls::vue_chaine_compacte("");

		//			if (type_var->genre == GenreType::POINTEUR || type_var->genre == GenreType::REFERENCE) {
		//				type_var = type_dereference_pour(type_var);
		//				nom_structure = static_cast<TypeStructure *>(type_var)->nom;
		//			}
		//			else {
		//				nom_structure = static_cast<TypeStructure *>(type_var)->nom;
		//			}

		//			auto &ds = contexte.donnees_structure(nom_structure);

		//			/* pousse chaque membre de la structure sur la pile */

		//			for (auto &dm : ds.donnees_membres) {
		//				auto type_membre = ds.types[dm.second.index_membre];

		//				donnees_var.type = type_membre;
		//				donnees_var.est_argument = true;
		//				donnees_var.est_membre_emploie = true;

		//				contexte.pousse_locale(dm.first, donnees_var);
		//			}
		//		}
			}

			/* vérifie le type du bloc */
			auto bloc = decl->bloc;

			auto noeud_dep = graphe.cree_noeud_fonction(decl);

			performe_validation_semantique(bloc, contexte, true);
			auto inst_ret = derniere_instruction(bloc);

			/* si aucune instruction de retour -> vérifie qu'aucun type n'a été spécifié */
			if (inst_ret == nullptr) {
				assert(decl->type->genre == GenreType::FONCTION);
				auto type_fonc = static_cast<TypeFonction *>(decl->type);

				if (type_fonc->types_sorties[0]->genre != GenreType::RIEN && !decl->est_coroutine) {
					erreur::lance_erreur(
								"Instruction de retour manquante",
								contexte,
								decl->lexeme,
								erreur::type_erreur::TYPE_DIFFERENTS);
				}

				decl->aide_generation_code = REQUIERS_CODE_EXTRA_RETOUR;
			}

			graphe.ajoute_dependances(*noeud_dep, donnees_dependance);

			contexte.termine_fonction();
			break;
		}
		case GenreNoeud::DECLARATION_OPERATEUR:
		{
			auto decl = static_cast<NoeudDeclarationFonction *>(b);
			using dls::outils::possede_drapeau;

			valide_type_fonction(decl, contexte);

			contexte.commence_fonction(decl);

			auto requiers_contexte = !possede_drapeau(decl->drapeaux, FORCE_NULCTX);

			decl->bloc->membres.reserve(decl->params.taille + requiers_contexte);

			if (requiers_contexte) {
				auto val_ctx = static_cast<NoeudExpressionReference *>(contexte.assembleuse->cree_noeud(GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, b->lexeme));
				val_ctx->type = contexte.type_contexte;
				val_ctx->bloc_parent = b->bloc_parent;
				val_ctx->ident = contexte.table_identifiants.identifiant_pour_chaine("contexte");

				auto decl_ctx = static_cast<NoeudDeclarationVariable *>(contexte.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, b->lexeme));
				decl_ctx->bloc_parent = b->bloc_parent;
				decl_ctx->valeur = val_ctx;
				decl_ctx->type = val_ctx->type;
				decl_ctx->ident = val_ctx->ident;

				decl->bloc->membres.pousse(decl_ctx);

				donnees_dependance.types_utilises.insere(contexte.type_contexte);
			}

			/* Pousse les paramètres sur la pile. */
			POUR (decl->params) {
				auto argument = static_cast<NoeudDeclarationVariable *>(it);

				auto variable = argument->valeur;
				argument->ident = variable->ident;
				argument->type = variable->type;

				donnees_dependance.types_utilises.insere(argument->type);

				decl->bloc->membres.pousse(argument);

				/* À FAIRE(réusinage arbre) */
		//		if (argument.est_employe) {
		//			auto type_var = argument.type;
		//			auto nom_structure = dls::vue_chaine_compacte("");

		//			if (type_var->genre == GenreType::POINTEUR || type_var->genre == GenreType::REFERENCE) {
		//				type_var = type_dereference_pour(type_var);
		//				nom_structure = static_cast<TypeStructure *>(type_var)->nom;
		//			}
		//			else {
		//				nom_structure = static_cast<TypeStructure *>(type_var)->nom;
		//			}

		//			auto &ds = contexte.donnees_structure(nom_structure);

		//			/* pousse chaque membre de la structure sur la pile */

		//			for (auto &dm : ds.donnees_membres) {
		//				auto type_membre = ds.types[dm.second.index_membre];

		//				donnees_var.type = type_membre;
		//				donnees_var.est_argument = true;
		//				donnees_var.est_membre_emploie = true;

		//				contexte.pousse_locale(dm.first, donnees_var);
		//			}
		//		}
			}

			/* vérifie le type du bloc */
			auto bloc = decl->bloc;

			auto noeud_dep = graphe.cree_noeud_fonction(decl);

			performe_validation_semantique(bloc, contexte, true);
			auto inst_ret = derniere_instruction(bloc);

			/* si aucune instruction de retour -> vérifie qu'aucun type n'a été spécifié */
			if (inst_ret == nullptr) {
				erreur::lance_erreur(
							"Instruction de retour manquante",
							contexte,
							decl->lexeme,
							erreur::type_erreur::TYPE_DIFFERENTS);
			}

			graphe.ajoute_dependances(*noeud_dep, donnees_dependance);

			contexte.termine_fonction();

			break;
		}
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			auto expr = static_cast<NoeudExpressionAppel *>(b);
			expr->genre_valeur = GenreValeur::DROITE;
			valide_appel_fonction(contexte, expr, expr_gauche);
			break;
		}
		case GenreNoeud::DIRECTIVE_EXECUTION:
		{
			auto dir = static_cast<NoeudExpressionUnaire *>(b);
			performe_validation_semantique(dir->expr, contexte, true);
			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		{
			auto expr = static_cast<NoeudExpressionReference *>(b);

			if (expr->drapeaux & DECLARATION_TYPE_POLYMORPHIQUE) {
				expr->genre_valeur = GenreValeur::DROITE;

				if (contexte.donnees_fonction && contexte.donnees_fonction->est_instantiation_gabarit) {
					auto type_instantie = static_cast<Type *>(nullptr);

					for (auto &paire : contexte.paires_expansion_gabarit) {
						if (paire.first == expr->ident->nom) {
							type_instantie = paire.second;
						}
					}

					if (type_instantie == nullptr) {
						erreur::lance_erreur("impossible de définir le type de l'instantiation polymorphique", contexte, expr->lexeme);
					}

					expr->type = contexte.typeuse.type_type_de_donnees(type_instantie);
					return;
				}

				auto type_poly = contexte.typeuse.cree_polymorphique(expr->ident);
				expr->type = contexte.typeuse.type_type_de_donnees(type_poly);
				return;
			}

			expr->genre_valeur = GenreValeur::TRANSCENDANTALE;

			auto bloc = expr->bloc_parent;
			assert(bloc != nullptr);

			/* À FAIRE : pour une fonction, trouve la selon le type */
			auto fichier = contexte.fichier(static_cast<size_t>(expr->lexeme->fichier));
			auto decl = trouve_dans_bloc_ou_module(contexte, bloc, expr->ident, fichier);

			if (decl == nullptr) {
				erreur::lance_erreur(
							"Variable inconnue",
							contexte,
							b->lexeme,
							erreur::type_erreur::VARIABLE_INCONNUE);
			}

			if (decl->lexeme->fichier == expr->lexeme->fichier && decl->genre == GenreNoeud::DECLARATION_VARIABLE && ((decl->drapeaux_decl & EST_GLOBALE) == 0)) {
				if (decl->lexeme->ligne > expr->lexeme->ligne) {
					erreur::lance_erreur("Utilisation d'une variable avant sa déclaration", contexte, expr->lexeme);
				}
			}

			assert(decl->type);
			expr->decl = decl;

			if (dls::outils::est_element(decl->genre, GenreNoeud::DECLARATION_ENUM, GenreNoeud::DECLARATION_STRUCTURE) && expr->aide_generation_code != EST_NOEUD_ACCES) {
				expr->type = contexte.typeuse.type_type_de_donnees(decl->type);
			}
			else {
				expr->type = decl->type;
			}

			if (decl->drapeaux & EST_VAR_BOUCLE) {
				expr->drapeaux |= EST_VAR_BOUCLE;
			}
			else if (decl->drapeaux & EST_CONSTANTE) {
				expr->genre_valeur = GenreValeur::DROITE;
			}

			donnees_dependance.types_utilises.insere(expr->type);

			if (decl->genre == GenreNoeud::DECLARATION_FONCTION) {
				auto decl_fonc = static_cast<NoeudDeclarationFonction *>(decl);
				donnees_dependance.fonctions_utilisees.insere(decl_fonc);
			}
			else if (decl->genre == GenreNoeud::DECLARATION_VARIABLE) {
				if (decl->drapeaux & EST_GLOBALE) {
					donnees_dependance.globales_utilisees.insere(static_cast<NoeudDeclarationVariable *>(decl));
				}
			}

			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
		{
			auto type_connu = contexte.typeuse.type_pour_lexeme(b->lexeme->genre);
			auto type_type = contexte.typeuse.type_type_de_donnees(type_connu);

			b->type = type_type;

			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		{
			auto inst = static_cast<NoeudExpressionMembre *>(b);
			auto enfant1 = inst->accede;
			auto enfant2 = inst->membre;
			b->genre_valeur = GenreValeur::TRANSCENDANTALE;

			if (enfant1->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				auto fichier = contexte.fichier(static_cast<size_t>(b->lexeme->fichier));

				auto const nom_symbole = enfant1->ident->nom;
				if (fichier->importe_module(nom_symbole)) {
					/* À FAIRE(réusinage arbre) */
//					auto module_importe = contexte.module(nom_symbole);

//					if (module_importe == nullptr) {
//						erreur::lance_erreur(
//									"module inconnu",
//									contexte,
//									enfant1->lexeme,
//									erreur::type_erreur::MODULE_INCONNU);
//					}

//					auto const nom_fonction = enfant2->ident->nom;

//					if (!module_importe->possede_fonction(nom_fonction)) {
//						erreur::lance_erreur(
//									"Le module ne possède pas la fonction",
//									contexte,
//									enfant2->lexeme,
//									erreur::type_erreur::FONCTION_INCONNUE);
//					}

//					enfant2->module_appel = static_cast<int>(module_importe->id);

//					performe_validation_semantique(enfant2, contexte, expr_gauche);

//					b->type = enfant2->type;
//					b->aide_generation_code = ACCEDE_MODULE;

					return;
				}
			}

			valide_acces_membre(contexte, inst, enfant1, enfant2, expr_gauche);

			donnees_dependance.types_utilises.insere(b->type);

			break;
		}
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		{
			auto inst = static_cast<NoeudExpressionBinaire *>(b);
			auto variable = inst->expr1;
			auto expression = inst->expr2;

			if (expression->genre == GenreNoeud::INSTRUCTION_NON_INITIALISATION) {
				erreur::lance_erreur("Impossible d'utiliser '---' dans une expression d'assignation", contexte, expression->lexeme);
			}

			performe_validation_semantique(expression, contexte, false);

			if (expression->type == nullptr) {
				erreur::lance_erreur(
							"Impossible de définir le type de la variable !",
							contexte,
							b->lexeme,
							erreur::type_erreur::TYPE_INCONNU);
			}

			if (expression->type->genre == GenreType::RIEN) {
				erreur::lance_erreur(
							"Impossible d'assigner une expression de type 'rien' à une variable !",
							contexte,
							b->lexeme,
							erreur::type_erreur::ASSIGNATION_RIEN);
			}

			/* a, b = foo() */
			if (variable->lexeme->genre == GenreLexeme::VIRGULE) {
				if (expression->genre != GenreNoeud::EXPRESSION_APPEL_FONCTION) {
					erreur::lance_erreur(
								"Une virgule ne peut se trouver qu'à gauche d'un appel de fonction.",
								contexte,
								variable->lexeme,
								erreur::type_erreur::NORMAL);
				}

				dls::tablet<NoeudExpression *, 10> feuilles;
				rassemble_feuilles(variable, feuilles);

				/* Utilisation du type de la fonction et non
				 * DonneesFonction::idx_types_retour car les pointeurs de
				 * fonctions n'ont pas de DonneesFonction. */
				auto type_fonc = static_cast<TypeFonction *>(expression->type);

				if (feuilles.taille() != type_fonc->types_sorties.taille) {
					erreur::lance_erreur(
								"L'ignorance d'une valeur de retour non implémentée.",
								contexte,
								variable->lexeme,
								erreur::type_erreur::NORMAL);
				}

				for (auto i = 0l; i < feuilles.taille(); ++i) {
					auto &f = feuilles[i];

					if (f->type == nullptr) {
						f->type = type_fonc->types_sorties[i];
					}

					performe_validation_semantique(f, contexte, true);
				}

				return;
			}

			performe_validation_semantique(variable, contexte, true);

			if (!est_valeur_gauche(variable->genre_valeur)) {
				erreur::lance_erreur(
							"Impossible d'assigner une expression à une valeur-droite !",
							contexte,
							b->lexeme,
							erreur::type_erreur::ASSIGNATION_INVALIDE);
			}

			auto transformation = cherche_transformation(
						contexte,
						expression->type,
						variable->type);

			if (transformation.type == TypeTransformation::IMPOSSIBLE) {
				erreur::lance_erreur_assignation_type_differents(
							variable->type,
							expression->type,
							contexte,
							b->lexeme);
			}

			expression->transformation = transformation;

			break;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			auto decl = static_cast<NoeudDeclarationVariable *>(b);
			auto variable = decl->valeur;
			auto expression = decl->expression;

			// À FAIRE : cas où nous avons plusieurs variables déclarées
			if (variable->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				erreur::lance_erreur("Expression inattendue à gauche de la déclaration",
									 contexte,
									 variable->lexeme);
			}

			decl->ident = variable->ident;

			auto decl_prec = trouve_dans_bloc(variable->bloc_parent, decl);

			if (decl_prec != nullptr && decl_prec->genre == decl->genre) {
				if (decl->lexeme->ligne > decl_prec->lexeme->ligne) {
					erreur::redefinition_symbole(contexte, variable->lexeme, decl_prec->lexeme);
				}
			}

			variable->type = resoud_type_final(contexte, variable->expression_type);

			if ((decl->drapeaux & EST_CONSTANTE) && expression != nullptr && expression->genre == GenreNoeud::INSTRUCTION_NON_INITIALISATION) {
				erreur::lance_erreur("Impossible de ne pas initialiser une constante", contexte, expression->lexeme);
			}

			if (expression != nullptr && expression->genre != GenreNoeud::INSTRUCTION_NON_INITIALISATION) {
				performe_validation_semantique(expression, contexte, false);

				if (expression->type == nullptr) {
					erreur::lance_erreur("impossible de définir le type de l'expression", contexte, expression->lexeme);
				}
				else if (variable->type == nullptr) {
					if (expression->type->genre == GenreType::ENTIER_CONSTANT) {
						variable->type = contexte.typeuse[TypeBase::Z32];
						expression->transformation = { TypeTransformation::CONVERTI_ENTIER_CONSTANT, variable->type };
					}
					else if (expression->type->genre == GenreType::RIEN) {
						erreur::lance_erreur(
									"impossible d'assigner une expression de type « rien » à une variable",
									contexte,
									expression->lexeme,
									erreur::type_erreur::ASSIGNATION_RIEN);
					}
					else {
						variable->type = expression->type;
					}
				}
				else {
					auto transformation = cherche_transformation(
								contexte,
							expression->type,
							variable->type);

					if (transformation.type == TypeTransformation::IMPOSSIBLE) {
						erreur::lance_erreur_assignation_type_differents(
									variable->type,
									expression->type,
									contexte,
									b->lexeme);
					}

					expression->transformation = transformation;
				}

				if (decl->drapeaux & EST_CONSTANTE) {
					auto res_exec = evalue_expression(contexte, decl->bloc_parent, expression);

					if (res_exec.est_errone) {
						erreur::lance_erreur("Impossible d'évaluer l'expression de la constante", contexte, expression->lexeme);
					}

					decl->valeur_expression = res_exec;
				}
			}
			else {
				if (variable->type == nullptr) {
					erreur::lance_erreur("variable déclarée sans type", contexte, variable->lexeme);
				}
			}

			decl->type = variable->type;

			if (decl->drapeaux_decl & EST_EXTERNE) {
				erreur::lance_erreur(
							"Ne peut pas assigner une variable globale externe dans sa déclaration",
							contexte,
							b->lexeme);
			}

			if (decl->drapeaux & EST_GLOBALE) {
				graphe.cree_noeud_globale(decl);
			}

			donnees_dependance.types_utilises.insere(decl->type);

			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		{
			b->genre_valeur = GenreValeur::DROITE;
			b->type = contexte.typeuse[TypeBase::R32];

			donnees_dependance.types_utilises.insere(b->type);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		{
			b->genre_valeur = GenreValeur::DROITE;
			b->type = contexte.typeuse[TypeBase::ENTIER_CONSTANT];

			donnees_dependance.types_utilises.insere(b->type);
			break;
		}
		case GenreNoeud::OPERATEUR_BINAIRE:
		{
			auto expr = static_cast<NoeudExpressionBinaire *>(b);
			expr->genre_valeur = GenreValeur::DROITE;

			auto enfant1 = expr->expr1;
			auto enfant2 = expr->expr2;

			if (expr->lexeme->genre == GenreLexeme::TABLEAU) {
				auto expression_taille = enfant1;
				auto expression_type = enfant2;

				performe_validation_semantique(expression_type, contexte, false);

				auto type2 = expression_type->type;

				if (type2->genre != GenreType::TYPE_DE_DONNEES) {
					erreur::lance_erreur("Attendu une expression de type après la déclaration de type tableau", contexte, enfant2->lexeme);
				}

				auto type_de_donnees = static_cast<TypeTypeDeDonnees *>(type2);
				auto type_connu = type_de_donnees->type_connu ? type_de_donnees->type_connu : type_de_donnees;

				auto taille_tableau = 0l;

				if (expression_taille) {
					performe_validation_semantique(expression_taille, contexte, false);

					auto res = evalue_expression(contexte, expression_taille->bloc_parent, expression_taille);

					if (res.est_errone) {
						erreur::lance_erreur("Impossible d'évaluer la taille du tableau", contexte, expression_taille->lexeme);
					}

					if (res.type != type_expression::ENTIER) {
						erreur::lance_erreur("L'expression n'est pas de type entier", contexte, expression_taille->lexeme);
					}

					taille_tableau = res.entier;
				}

				if (taille_tableau != 0) {
					auto type_tableau = contexte.typeuse.type_tableau_fixe(type_connu, taille_tableau);
					expr->type = contexte.typeuse.type_type_de_donnees(type_tableau);
					donnees_dependance.types_utilises.insere(type_tableau);
				}
				else {
					auto type_tableau = contexte.typeuse.type_tableau_dynamique(type_connu);
					expr->type = contexte.typeuse.type_type_de_donnees(type_tableau);
					donnees_dependance.types_utilises.insere(type_tableau);
				}

				return;
			}

			auto type_op = expr->lexeme->genre;

			auto assignation_composee = est_assignation_composee(type_op);

			performe_validation_semantique(enfant1, contexte, expr_gauche);
			performe_validation_semantique(enfant2, contexte, assignation_composee ? false : expr_gauche);

			auto type1 = enfant1->type;
			auto type2 = enfant2->type;

			if (type1->genre == GenreType::TYPE_DE_DONNEES) {
				if (type2->genre != GenreType::TYPE_DE_DONNEES) {
					erreur::lance_erreur("Opération impossible entre un type et autre chose", contexte, expr->lexeme);
				}

				auto type_type1 = static_cast<TypeTypeDeDonnees *>(type1);
				auto type_type2 = static_cast<TypeTypeDeDonnees *>(type2);

				if (type_type1->type_connu == nullptr) {
					erreur::lance_erreur("Opération impossible car le type n'est pas connu", contexte, enfant1->lexeme);
				}

				if (type_type2->type_connu == nullptr) {
					erreur::lance_erreur("Opération impossible car le type n'est pas connu", contexte, enfant2->lexeme);
				}

				switch (expr->lexeme->genre) {
					default:
					{
						erreur::lance_erreur("Opérateur inapplicable sur des types", contexte, expr->lexeme);
					}
					case GenreLexeme::BARRE:
					{
						if (type_type1->type_connu == type_type2->type_connu) {
							erreur::lance_erreur("Impossible de créer une union depuis des types similaires\n", contexte, expr->lexeme);
						}

						auto membres = kuri::tableau<TypeCompose::Membre>(2);
						membres[0] = { type_type1->type_connu, "0" };
						membres[1] = { type_type2->type_connu, "1" };

						auto type_union = contexte.typeuse.union_anonyme(std::move(membres));
						expr->type = contexte.typeuse.type_type_de_donnees(type_union);
						donnees_dependance.types_utilises.insere(type_union);
						return;
					}
					case GenreLexeme::EGALITE:
					{
						auto op = contexte.operateurs.op_comp_egal_types;
						expr->type = op->type_resultat;
						expr->op = op;
						donnees_dependance.types_utilises.insere(expr->type);
						return;
					}
					case GenreLexeme::DIFFERENCE:
					{
						auto op = contexte.operateurs.op_comp_diff_types;
						expr->type = op->type_resultat;
						expr->op = op;
						donnees_dependance.types_utilises.insere(expr->type);
						return;
					}
				}
			}

			/* détecte a comp b comp c */
			if (est_operateur_comp(type_op) && est_operateur_comp(enfant1->lexeme->genre)) {
				expr->genre = GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE;
				expr->type = contexte.typeuse[TypeBase::BOOL];

				auto enfant_expr = static_cast<NoeudExpressionBinaire *>(enfant1);
				type1 = enfant_expr->expr2->type;

				auto candidats = cherche_candidats_operateurs(contexte, type1, type2, type_op);
				auto meilleur_candidat = static_cast<OperateurCandidat const *>(nullptr);
				auto poids = 0.0;

				for (auto const &candidat : candidats) {
					if (candidat.poids > poids) {
						poids = candidat.poids;
						meilleur_candidat = &candidat;
					}
				}

				if (meilleur_candidat == nullptr) {
					erreur::lance_erreur_type_operation(contexte, b);
				}

				expr->op = meilleur_candidat->op;
				enfant1->transformation = meilleur_candidat->transformation_type1;
				enfant2->transformation = meilleur_candidat->transformation_type2;

				if (!expr->op->est_basique) {
					donnees_dependance.fonctions_utilisees.insere(expr->op->decl);
				}
			}
			else {
				if (assignation_composee) {
					type_op = operateur_pour_assignation_composee(type_op);
					expr->drapeaux |= EST_ASSIGNATION_COMPOSEE;

					// exclue les arithmétiques de pointeur
					if (!(type1->genre == GenreType::POINTEUR && (est_type_entier(type2) || type2->genre == GenreType::ENTIER_CONSTANT))) {
						auto transformation = cherche_transformation(contexte, type2, type1);

						if (transformation.type == TypeTransformation::IMPOSSIBLE) {
							erreur::lance_erreur_assignation_type_differents(
										type1,
										type2,
										contexte,
										enfant2->lexeme);
						}
					}
				}

				auto candidats = cherche_candidats_operateurs(contexte, type1, type2, type_op);
				auto meilleur_candidat = static_cast<OperateurCandidat const *>(nullptr);
				auto poids = 0.0;

				for (auto const &candidat : candidats) {
					if (candidat.poids > poids) {
						poids = candidat.poids;
						meilleur_candidat = &candidat;
					}
				}

				if (meilleur_candidat == nullptr) {
					erreur::lance_erreur_type_operation(contexte, b);
				}

				expr->type = meilleur_candidat->op->type_resultat;
				expr->op = meilleur_candidat->op;
				enfant1->transformation = meilleur_candidat->transformation_type1;
				enfant2->transformation = meilleur_candidat->transformation_type2;

				if (!expr->op->est_basique) {
					donnees_dependance.fonctions_utilisees.insere(expr->op->decl);
				}
			}

			donnees_dependance.types_utilises.insere(expr->type);
			break;
		}
		case GenreNoeud::OPERATEUR_UNAIRE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(b);
			expr->genre_valeur = GenreValeur::DROITE;

			auto enfant = expr->expr;
			performe_validation_semantique(enfant, contexte, expr_gauche);
			auto type = enfant->type;

			if (dls::outils::est_element(expr->lexeme->genre, GenreLexeme::FOIS_UNAIRE, GenreLexeme::ESP_UNAIRE)) {
				if (type->genre != GenreType::TYPE_DE_DONNEES) {
					erreur::lance_erreur("attendu l'expression d'un type", contexte, enfant->lexeme);
				}

				auto type_de_donnees = static_cast<TypeTypeDeDonnees *>(type);
				auto type_connu = type_de_donnees->type_connu;

				if (type_connu == nullptr) {
					type_connu = type_de_donnees;
				}

				if (expr->lexeme->genre == GenreLexeme::FOIS_UNAIRE) {
					type_connu = contexte.typeuse.type_pointeur_pour(type_connu);
				}
				else if (expr->lexeme->genre == GenreLexeme::ESP_UNAIRE) {
					type_connu = contexte.typeuse.type_reference_pour(type_connu);
				}

				b->type = contexte.typeuse.type_type_de_donnees(type_connu);
				break;
			}

			if (type->genre == GenreType::REFERENCE) {
				enfant->transformation = TypeTransformation::DEREFERENCE;
				type = type_dereference_pour(type);
			}

			if (expr->type == nullptr) {
				if (expr->lexeme->genre == GenreLexeme::AROBASE) {
					if (!est_valeur_gauche(enfant->genre_valeur)) {
						erreur::lance_erreur(
									"Ne peut pas prendre l'adresse d'une valeur-droite.",
									contexte,
									enfant->lexeme);
					}

					expr->type = contexte.typeuse.type_pointeur_pour(type);
				}
				else if (expr->lexeme->genre == GenreLexeme::EXCLAMATION) {
					if (!est_type_conditionnable(enfant->type)) {
						erreur::lance_erreur(
									"Ne peut pas appliquer l'opérateur « ! » au type de l'expression",
									contexte,
									enfant->lexeme);
					}

					expr->type = contexte.typeuse[TypeBase::BOOL];
				}
				else {
					if (type->genre == GenreType::ENTIER_CONSTANT) {
						type = contexte.typeuse[TypeBase::Z32];
						enfant->transformation = { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type };
					}

					auto op = cherche_operateur_unaire(contexte.operateurs, type, expr->lexeme->genre);

					if (op == nullptr) {
						erreur::lance_erreur_type_operation_unaire(contexte, expr);
					}

					expr->type = op->type_resultat;
					expr->op = op;
				}
			}

			donnees_dependance.types_utilises.insere(expr->type);
			break;
		}
		case GenreNoeud::EXPRESSION_INDEXAGE:
		{
			auto expr = static_cast<NoeudExpressionBinaire *>(b);
			expr->genre_valeur = GenreValeur::TRANSCENDANTALE;

			auto enfant1 = expr->expr1;
			auto enfant2 = expr->expr2;

			performe_validation_semantique(enfant1, contexte, expr_gauche);
			performe_validation_semantique(enfant2, contexte, expr_gauche);

			auto type1 = enfant1->type;

			if (type1->genre == GenreType::REFERENCE) {
				enfant1->transformation = TypeTransformation::DEREFERENCE;
				type1 = type_dereference_pour(type1);
			}

			switch (type1->genre) {
				case GenreType::VARIADIQUE:
				case GenreType::TABLEAU_DYNAMIQUE:
				{
					expr->type = type_dereference_pour(type1);
					donnees_dependance.fonctions_utilisees.insere(contexte.interface_kuri.decl_panique_tableau);
					break;
				}
				case GenreType::TABLEAU_FIXE:
				{
					auto type_tabl = static_cast<TypeTableauFixe *>(type1);
					expr->type = type_dereference_pour(type1);

					auto res = evalue_expression(contexte, enfant2->bloc_parent, enfant2);

					if (!res.est_errone) {
						if (res.entier >= type_tabl->taille) {
							erreur::lance_erreur_acces_hors_limites(
										contexte,
										enfant2,
										type_tabl->taille,
										type1,
										res.entier);
						}

						/* nous savons que l'accès est dans les limites,
								 * évite d'émettre le code de vérification */
						expr->aide_generation_code = IGNORE_VERIFICATION;
					}

					donnees_dependance.fonctions_utilisees.insere(contexte.interface_kuri.decl_panique_tableau);
					break;
				}
				case GenreType::POINTEUR:
				{
					expr->type = type_dereference_pour(type1);
					break;
				}
				case GenreType::CHAINE:
				{
					expr->type = contexte.typeuse[TypeBase::Z8];
					donnees_dependance.fonctions_utilisees.insere(contexte.interface_kuri.decl_panique_chaine);
					break;
				}
				default:
				{
					dls::flux_chaine ss;
					ss << "Le type '" << chaine_type(type1)
					   << "' ne peut être déréférencé par opérateur[] !";

					erreur::lance_erreur(
								ss.chn(),
								contexte,
								b->lexeme,
								erreur::type_erreur::TYPE_DIFFERENTS);
				}
			}

			donnees_dependance.types_utilises.insere(expr->type);
			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(b);
			b->genre_valeur = GenreValeur::DROITE;

			auto type_fonc = static_cast<TypeFonction *>(fonction_courante->type);

			if (inst->expr == nullptr) {
				b->type = contexte.typeuse[TypeBase::RIEN];

				if (!fonction_courante->est_coroutine && (type_fonc->types_sorties[0] != b->type)) {
					erreur::lance_erreur(
								"Expression de retour manquante",
								contexte,
								b->lexeme);
				}

				donnees_dependance.types_utilises.insere(b->type);
				return;
			}

			auto enfant = inst->expr;
			auto nombre_retour = type_fonc->types_sorties.taille;

			if (nombre_retour > 1) {
				if (enfant->lexeme->genre == GenreLexeme::VIRGULE) {
					dls::tablet<NoeudExpression *, 10> feuilles;
					rassemble_feuilles(enfant, feuilles);

					if (feuilles.taille() != nombre_retour) {
						erreur::lance_erreur(
									"Le compte d'expression de retour est invalide",
									contexte,
									b->lexeme);
					}

					for (auto i = 0l; i < feuilles.taille(); ++i) {
						auto f = feuilles[i];
						performe_validation_semantique(f, contexte, false);

						auto transformation = cherche_transformation(
									contexte,
									f->type,
									type_fonc->types_sorties[i]);

						if (transformation.type == TypeTransformation::IMPOSSIBLE) {
							erreur::lance_erreur_type_retour(
										type_fonc->types_sorties[i],
										f->type,
										contexte,
										b);
						}

						f->transformation = transformation;

						donnees_dependance.types_utilises.insere(f->type);
					}

					/* À FAIRE : multiples types de retour */
					b->type = feuilles[0]->type;
					b->genre = GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE;
				}
				else if (enfant->genre == GenreNoeud::EXPRESSION_APPEL_FONCTION) {
					performe_validation_semantique(enfant, contexte, false);

					/* À FAIRE : multiples types de retour, confirmation typage */
					b->type = enfant->type;
					b->genre = GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE;
				}
				else {
					erreur::lance_erreur(
								"Le compte d'expression de retour est invalide",
								contexte,
								b->lexeme);
				}
			}
			else {
				performe_validation_semantique(enfant, contexte, false);
				b->type = type_fonc->types_sorties[0];
				b->genre = GenreNoeud::INSTRUCTION_RETOUR_SIMPLE;

				auto transformation = cherche_transformation(
							contexte,
							enfant->type,
							b->type);

				if (transformation.type == TypeTransformation::IMPOSSIBLE) {
					erreur::lance_erreur_type_retour(
								b->type,
								enfant->type,
								contexte,
								b);
				}

				enfant->transformation = transformation;
			}

			donnees_dependance.types_utilises.insere(b->type);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		{
			b->type = contexte.typeuse[TypeBase::CHAINE];
			b->genre_valeur = GenreValeur::DROITE;
			donnees_dependance.types_utilises.insere(b->type);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		{
			b->genre_valeur = GenreValeur::DROITE;
			b->type = contexte.typeuse[TypeBase::BOOL];

			donnees_dependance.types_utilises.insere(b->type);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		{
			b->genre_valeur = GenreValeur::DROITE;
			b->type = contexte.typeuse[TypeBase::Z8];
			donnees_dependance.types_utilises.insere(b->type);
			break;
		}
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		{
			auto inst = static_cast<NoeudSi *>(b);

			performe_validation_semantique(inst->condition, contexte, false);
			auto type_condition = inst->condition->type;

			if (type_condition == nullptr && !est_operateur_bool(inst->condition->lexeme->genre)) {
				erreur::lance_erreur("Attendu un opérateur booléen pour la condition", contexte, inst->condition->lexeme);
			}

			if (!est_type_conditionnable(type_condition)) {
				erreur::lance_erreur("Impossible de conditionner le type de l'expression 'si'",
									 contexte,
									 inst->condition->lexeme,
									 erreur::type_erreur::TYPE_DIFFERENTS);
			}

			performe_validation_semantique(inst->bloc_si_vrai, contexte, true);

			/* noeud 3 : sinon (optionel) */
			if (inst->bloc_si_faux != nullptr) {
				performe_validation_semantique(inst->bloc_si_faux, contexte, true);
			}

			/* pour les expressions x = si y { z } sinon { w } */
			inst->type = inst->bloc_si_vrai->type;

			break;
		}
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			auto inst = static_cast<NoeudBloc *>(b);

			if (inst->expressions.est_vide()) {
				b->type = contexte.typeuse[TypeBase::RIEN];
			}
			else {
				POUR (inst->expressions) {
					performe_validation_semantique(it, contexte, true);
				}

				b->type = inst->expressions[inst->expressions.taille - 1]->type;
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_POUR:
		{
			auto inst = static_cast<NoeudPour *>(b);

			/* on génère d'abord le type de la variable */
			auto enfant1 = inst->variable;
			auto enfant2 = inst->expression;
			auto enfant3 = inst->bloc;
			auto enfant4 = inst->bloc_sansarret;
			auto enfant5 = inst->bloc_sinon;

			performe_validation_semantique(enfant2, contexte, true);

			/* À FAIRE : utilisation du type */
//			auto df = static_cast<DonneesFonction *>(nullptr);

			auto feuilles = dls::tablet<NoeudExpression *, 10>{};
			rassemble_feuilles(enfant1, feuilles);

			for (auto f : feuilles) {
				auto decl_f = trouve_dans_bloc(b->bloc_parent, f->ident);

				if (decl_f != nullptr) {
					if (f->lexeme->ligne > decl_f->lexeme->ligne) {
						erreur::lance_erreur(
									"Redéfinition de la variable",
									contexte,
									f->lexeme,
									erreur::type_erreur::VARIABLE_REDEFINIE);
					}
				}
			}

			auto variable = feuilles[0];

			auto requiers_index = feuilles.taille() == 2;

			auto type = enfant2->type;

			/* NOTE : nous testons le type des noeuds d'abord pour ne pas que le
			 * type de retour d'une coroutine n'interfère avec le type d'une
			 * variable (par exemple quand nous retournons une chaine). */
			if (enfant2->genre == GenreNoeud::EXPRESSION_PLAGE) {
				if (requiers_index) {
					b->aide_generation_code = GENERE_BOUCLE_PLAGE_INDEX;
				}
				else {
					b->aide_generation_code = GENERE_BOUCLE_PLAGE;
				}
			}
			// À FAIRE (réusinage arbre)
//			else if (enfant2->genre == GenreNoeud::EXPRESSION_APPEL_FONCTION && enfant2->df->est_coroutine) {
//				enfant1->type = enfant2->type;

//				df = enfant2->df;
//				auto nombre_vars_ret = df->idx_types_retours.taille();

//				if (feuilles.taille() == nombre_vars_ret) {
//					requiers_index = false;
//					b->aide_generation_code = GENERE_BOUCLE_COROUTINE;
//				}
//				else if (feuilles.taille() == nombre_vars_ret + 1) {
//					requiers_index = true;
//					b->aide_generation_code = GENERE_BOUCLE_COROUTINE_INDEX;
//				}
//				else {
//					erreur::lance_erreur(
//								"Mauvais compte d'arguments à déployer",
//								contexte,
//								*enfant1->lexeme);
//				}
//			}
			else {
				if (type->genre == GenreType::TABLEAU_DYNAMIQUE || type->genre == GenreType::TABLEAU_FIXE || type->genre == GenreType::VARIADIQUE) {
					type = type_dereference_pour(type);

					if (requiers_index) {
						b->aide_generation_code = GENERE_BOUCLE_TABLEAU_INDEX;
					}
					else {
						b->aide_generation_code = GENERE_BOUCLE_TABLEAU;
					}
				}
				else if (type->genre == GenreType::CHAINE) {
					type = contexte.typeuse[TypeBase::Z8];
					enfant1->type = type;

					if (requiers_index) {
						b->aide_generation_code = GENERE_BOUCLE_TABLEAU_INDEX;
					}
					else {
						b->aide_generation_code = GENERE_BOUCLE_TABLEAU;
					}
				}
				else {
					erreur::lance_erreur(
								"La variable n'est ni un argument variadic, ni un tableau, ni une chaine",
								contexte,
								enfant2->lexeme);
				}
			}

			donnees_dependance.types_utilises.insere(type);
			enfant3->membres.reserve(feuilles.taille());

			auto nombre_feuilles = feuilles.taille() - requiers_index;

			for (auto i = 0l; i < nombre_feuilles; ++i) {
				auto f = feuilles[i];

				auto decl_f = static_cast<NoeudDeclarationVariable *>(contexte.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, b->lexeme));
				decl_f->bloc_parent = b->bloc_parent;
				decl_f->valeur = f;
				decl_f->type = type;
				decl_f->ident = f->ident;
				decl_f->lexeme = f->lexeme;

				if (enfant2->genre != GenreNoeud::EXPRESSION_PLAGE) {
					decl_f->drapeaux |= EST_VAR_BOUCLE;
					f->drapeaux |= EST_VAR_BOUCLE;
				}

				enfant3->membres.pousse(decl_f);
			}

			if (requiers_index) {
				auto idx = feuilles.back();

				auto decl_idx = static_cast<NoeudDeclarationVariable *>(contexte.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, b->lexeme));
				decl_idx->bloc_parent = b->bloc_parent;
				decl_idx->valeur = idx;

				if (b->aide_generation_code == GENERE_BOUCLE_PLAGE_INDEX) {
					decl_idx->type = contexte.typeuse[TypeBase::Z32];
				}
				else {
					decl_idx->type = contexte.typeuse[TypeBase::Z64];
				}

				decl_idx->ident = idx->ident;
				decl_idx->lexeme = idx->lexeme;

				enfant3->membres.pousse(decl_idx);
			}

			contexte.empile_controle_boucle(variable->ident);
			performe_validation_semantique(enfant3, contexte, true);
			contexte.depile_controle_boucle();

			if (enfant4 != nullptr) {
				performe_validation_semantique(enfant4, contexte, true);
			}

			if (enfant5 != nullptr) {
				performe_validation_semantique(enfant5, contexte, true);
			}

			break;
		}
		case GenreNoeud::EXPRESSION_COMME:
		{
			auto expr = static_cast<NoeudExpressionBinaire *>(b);
			expr->genre_valeur = GenreValeur::DROITE;
			expr->type = resoud_type_final(contexte, expr->expr2);

			/* À FAIRE : vérifie compatibilité */

			if (b->type == nullptr) {
				erreur::lance_erreur(
							"Ne peut transtyper vers un type invalide",
							contexte,
							expr->lexeme,
							erreur::type_erreur::TYPE_INCONNU);
			}

			donnees_dependance.types_utilises.insere(b->type);

			auto enfant = expr->expr1;
			performe_validation_semantique(enfant, contexte, false);

			if (enfant->type == nullptr) {
				erreur::lance_erreur(
							"Ne peut calculer le type d'origine",
							contexte,
							enfant->lexeme,
							erreur::type_erreur::TYPE_INCONNU);
			}

			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		{
			b->genre_valeur = GenreValeur::DROITE;
			b->type = contexte.typeuse[TypeBase::PTR_NUL];

			donnees_dependance.types_utilises.insere(b->type);
			break;
		}
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(b);
			expr->genre_valeur = GenreValeur::DROITE;
			expr->type = contexte.typeuse[TypeBase::N32];

			auto expr_type = expr->expr;
			expr_type->type = resoud_type_final(contexte, expr_type->expression_type);

			break;
		}
		case GenreNoeud::EXPRESSION_PLAGE:
		{
			auto inst = static_cast<NoeudExpressionBinaire *>(b);
			auto enfant1 = inst->expr1;
			auto enfant2 = inst->expr2;

			performe_validation_semantique(enfant1, contexte, false);
			performe_validation_semantique(enfant2, contexte, false);

			auto type_debut = enfant1->type;
			auto type_fin   = enfant2->type;

			if (type_debut == nullptr || type_fin == nullptr) {
				erreur::lance_erreur(
							"Les types de l'expression sont invalides !",
							contexte,
							b->lexeme,
							erreur::type_erreur::TYPE_INCONNU);
			}

			if (type_debut != type_fin) {
				if (type_debut->genre == GenreType::ENTIER_CONSTANT && est_type_entier(type_fin)) {
					type_debut = type_fin;
					enfant1->type = type_debut;
					enfant1->transformation = { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_debut };
				}
				else if (type_fin->genre == GenreType::ENTIER_CONSTANT && est_type_entier(type_debut)) {
					type_fin = type_debut;
					enfant2->type = type_fin;
					enfant2->transformation = { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_fin };
				}
				else {
					erreur::lance_erreur_type_operation(
								type_debut,
								type_fin,
								contexte,
								b->lexeme);
				}
			}
			else if (type_debut->genre == GenreType::ENTIER_CONSTANT) {
				type_debut = contexte.typeuse[TypeBase::Z32];
				enfant1->transformation = { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_debut };
				enfant2->transformation = { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_debut };
			}

			if (type_debut->genre != GenreType::ENTIER_NATUREL && type_debut->genre != GenreType::ENTIER_RELATIF && type_debut->genre != GenreType::REEL) {
				erreur::lance_erreur(
							"Attendu des types réguliers dans la plage de la boucle 'pour'",
							contexte,
							b->lexeme,
							erreur::type_erreur::TYPE_DIFFERENTS);
			}

			b->type = type_debut;

			break;
		}
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(b);
			auto chaine_var = inst->expr == nullptr ? nullptr : inst->expr->ident;
			auto ok = contexte.possede_controle_boucle(chaine_var);

			if (ok == false) {
				if (chaine_var == nullptr) {
					erreur::lance_erreur(
								"'continue' ou 'arrête' en dehors d'une boucle",
								contexte,
								b->lexeme,
								erreur::type_erreur::CONTROLE_INVALIDE);
				}
				else {
					erreur::lance_erreur(
								"Variable inconnue",
								contexte,
								inst->expr->lexeme,
								erreur::type_erreur::VARIABLE_INCONNUE);
				}
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_BOUCLE:
		{
			auto inst = static_cast<NoeudBoucle *>(b);
			contexte.empile_controle_boucle(nullptr);
			performe_validation_semantique(inst->bloc, contexte, true);
			contexte.depile_controle_boucle();
			break;
		}
		case GenreNoeud::INSTRUCTION_REPETE:
		{
			auto inst = static_cast<NoeudBoucle *>(b);
			contexte.empile_controle_boucle(nullptr);
			performe_validation_semantique(inst->bloc, contexte, true);
			contexte.depile_controle_boucle();
			performe_validation_semantique(inst->condition, contexte, false);

			if (inst->condition->type == nullptr && !est_operateur_bool(inst->condition->lexeme->genre)) {
				erreur::lance_erreur("Attendu un opérateur booléen pour la condition", contexte, inst->condition->lexeme);
			}

			break;
		}
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		{
			// RÀF
			break;
		}
		case GenreNoeud::INSTRUCTION_TANTQUE:
		{
			auto inst = static_cast<NoeudBoucle *>(b);

			performe_validation_semantique(inst->condition, contexte, false);

			if (inst->condition->type == nullptr && !est_operateur_bool(inst->condition->lexeme->genre)) {
				erreur::lance_erreur("Attendu un opérateur booléen pour la condition", contexte, inst->condition->lexeme);
			}

			contexte.empile_controle_boucle(nullptr);
			performe_validation_semantique(inst->bloc, contexte, true);
			contexte.depile_controle_boucle();

			if (inst->condition->type->genre != GenreType::BOOL) {
				erreur::lance_erreur(
							"Une expression booléenne est requise pour la boucle 'tantque'",
							contexte,
							inst->condition->lexeme,
							erreur::type_erreur::TYPE_ARGUMENT);
			}

			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(b);
			b->genre_valeur = GenreValeur::DROITE;

			dls::tablet<NoeudExpression *, 10> feuilles;
			rassemble_feuilles(expr->expr, feuilles);

			for (auto f : feuilles) {
				performe_validation_semantique(f, contexte, false);
			}

			if (feuilles.est_vide()) {
				return;
			}

			auto premiere_feuille = feuilles.front();

			auto type_feuille = premiere_feuille->type;

			if (type_feuille->genre == GenreType::ENTIER_CONSTANT) {
				type_feuille = contexte.typeuse[TypeBase::Z32];
				premiere_feuille->transformation = { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_feuille };
			}

			for (auto f : feuilles) {
				auto transformation = cherche_transformation(contexte, f->type, type_feuille);

				if (transformation.type == TypeTransformation::IMPOSSIBLE) {
					erreur::lance_erreur_assignation_type_differents(
								f->type,
								type_feuille,
								contexte,
								f->lexeme);
				}

				f->transformation = transformation;
			}

			b->type = contexte.typeuse.type_tableau_fixe(type_feuille, feuilles.taille());

			/* ajoute également le type de pointeur pour la génération de code C */
			auto type_ptr = contexte.typeuse.type_pointeur_pour(type_feuille);

			donnees_dependance.types_utilises.insere(b->type);
			donnees_dependance.types_utilises.insere(type_ptr);
			break;
		}
		case GenreNoeud::EXPRESSION_INFO_DE:
		{
			auto noeud_expr = static_cast<NoeudExpressionUnaire *>(b);
			auto expr = noeud_expr->expr;
			expr->type = resoud_type_final(contexte, expr->expression_type);

			auto type_info_type = static_cast<Type *>(nullptr);

			switch (expr->type->genre) {
				case GenreType::INVALIDE:
				case GenreType::POLYMORPHIQUE:
				{
					break;
				}
				case GenreType::EINI:
				case GenreType::CHAINE:
				case GenreType::RIEN:
				case GenreType::BOOL:
				case GenreType::OCTET:
				case GenreType::TYPE_DE_DONNEES:
				case GenreType::REEL:
				{
					type_info_type = contexte.typeuse.type_info_type_;
					break;
				}
				case GenreType::ENTIER_CONSTANT:
				case GenreType::ENTIER_NATUREL:
				case GenreType::ENTIER_RELATIF:
				{
					type_info_type = contexte.typeuse.type_info_type_entier;
					break;
				}
				case GenreType::REFERENCE:
				case GenreType::POINTEUR:
				{
					type_info_type = contexte.typeuse.type_info_type_pointeur;
					break;
				}
				case GenreType::STRUCTURE:
				{
					type_info_type = contexte.typeuse.type_info_type_structure;
					break;
				}
				case GenreType::UNION:
				{
					type_info_type = contexte.typeuse.type_info_type_union;
					break;
				}
				case GenreType::VARIADIQUE:
				case GenreType::TABLEAU_DYNAMIQUE:
				case GenreType::TABLEAU_FIXE:
				{
					type_info_type = contexte.typeuse.type_info_type_tableau;
					break;
				}
				case GenreType::FONCTION:
				{
					type_info_type = contexte.typeuse.type_info_type_fonction;
					break;
				}
				case GenreType::ENUM:
				case GenreType::ERREUR:
				{
					type_info_type = contexte.typeuse.type_info_type_enum;
					break;
				}
			}

			b->genre_valeur = GenreValeur::DROITE;
			b->type = contexte.typeuse.type_pointeur_pour(type_info_type);

			break;
		}
		case GenreNoeud::EXPRESSION_INIT_DE:
		{
			auto type = resoud_type_final(contexte, b->expression_type);

			if (type == nullptr) {
				erreur::lance_erreur("impossible de définir le type de init_de", contexte, b->lexeme);
			}

			if (type->genre != GenreType::STRUCTURE && type->genre != GenreType::UNION) {
				erreur::lance_erreur("init_de doit prendre le type d'une structure ou d'une union", contexte, b->lexeme);
			}

			auto types_entrees = kuri::tableau<Type *>(2);
			types_entrees[0] = contexte.type_contexte;
			types_entrees[1] = contexte.typeuse.type_pointeur_pour(type);

			auto types_sorties = kuri::tableau<Type *>(1);
			types_sorties[0] = contexte.typeuse[TypeBase::RIEN];

			auto type_fonction = contexte.typeuse.type_fonction(std::move(types_entrees), std::move(types_sorties));
			b->type = type_fonction;

			donnees_dependance.types_utilises.insere(b->type);

			break;
		}
		case GenreNoeud::EXPRESSION_TYPE_DE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(b);
			auto expr_type = expr->expr;
			performe_validation_semantique(expr_type, contexte, false);

			if (expr_type->type == nullptr) {
				erreur::lance_erreur("impossible de définir le type de l'expression de type_de", contexte, expr_type->lexeme);
			}

			if (expr_type->type->genre == GenreType::TYPE_DE_DONNEES) {
				b->type = expr_type->type;
			}
			else {
				b->type = contexte.typeuse.type_type_de_donnees(expr_type->type);
			}

			break;
		}
		case GenreNoeud::EXPRESSION_MEMOIRE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(b);
			performe_validation_semantique(expr->expr, contexte, false);

			auto type = expr->expr->type;

			if (type->genre != GenreType::POINTEUR) {
				erreur::lance_erreur(
							"Un pointeur est requis pour le déréférencement via 'mémoire'",
							contexte,
							expr->expr->lexeme,
							erreur::type_erreur::TYPE_DIFFERENTS);
			}

			auto type_pointeur = static_cast<TypePointeur *>(type);
			b->genre_valeur = GenreValeur::TRANSCENDANTALE;
			b->type = type_pointeur->type_pointe;

			break;
		}
		case GenreNoeud::EXPRESSION_LOGE:
		{
			auto expr_loge = static_cast<NoeudExpressionLogement *>(b);
			expr_loge->genre_valeur = GenreValeur::DROITE;
			expr_loge->type = resoud_type_final(contexte, expr_loge->expression_type);

			if (dls::outils::est_element(expr_loge->type->genre, GenreType::CHAINE, GenreType::TABLEAU_DYNAMIQUE)) {
				if (expr_loge->expr_taille == nullptr) {
					erreur::lance_erreur("Attendu une expression pour définir la taille du tableau à loger", contexte, b->lexeme);
				}

				performe_validation_semantique(expr_loge->expr_taille, contexte, false);

				auto type_cible = contexte.typeuse[TypeBase::Z64];
				auto transformation = cherche_transformation(contexte, expr_loge->expr_taille->type, type_cible);

				if (transformation.type == TypeTransformation::IMPOSSIBLE) {
					erreur::lance_erreur_assignation_type_differents(
								expr_loge->expr_taille->type,
								type_cible,
								contexte,
								expr_loge->expr_taille->lexeme);
				}

				expr_loge->expr_taille->transformation = transformation;
			}
			else {
				expr_loge->type = contexte.typeuse.type_pointeur_pour(expr_loge->type);
			}

			if (expr_loge->bloc != nullptr) {
				performe_validation_semantique(expr_loge->bloc, contexte, true);

				auto di = derniere_instruction(expr_loge->bloc);
				if (di == nullptr || !dls::outils::est_element(di->genre, GenreNoeud::INSTRUCTION_RETOUR, GenreNoeud::INSTRUCTION_RETOUR_SIMPLE, GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE, GenreNoeud::INSTRUCTION_CONTINUE_ARRETE)) {
					erreur::lance_erreur("Le bloc sinon d'une instruction « loge » doit obligatoirement retourner, ou si dans une boucle, la continuer ou l'arrêter",
										 contexte,
										 expr_loge->lexeme);
				}
			}
			else {
				donnees_dependance.fonctions_utilisees.insere(contexte.interface_kuri.decl_panique_memoire);
			}

			donnees_dependance.types_utilises.insere(expr_loge->type);

			break;
		}
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			auto expr_loge = static_cast<NoeudExpressionLogement *>(b);
			expr_loge->type = resoud_type_final(contexte, expr_loge->expression_type);

			performe_validation_semantique(expr_loge->expr, contexte, true);

			if (dls::outils::est_element(expr_loge->type->genre, GenreType::CHAINE, GenreType::TABLEAU_DYNAMIQUE)) {
				if (expr_loge->expr_taille == nullptr) {
					erreur::lance_erreur("Attendu une expression pour définir la taille à reloger", contexte, b->lexeme);
				}

				performe_validation_semantique(expr_loge->expr_taille, contexte, false);

				auto type_cible = contexte.typeuse[TypeBase::Z64];
				auto transformation = cherche_transformation(contexte, expr_loge->expr_taille->type, type_cible);

				if (transformation.type == TypeTransformation::IMPOSSIBLE) {
					erreur::lance_erreur_assignation_type_differents(
								expr_loge->expr_taille->type,
								type_cible,
								contexte,
								expr_loge->expr_taille->lexeme);
				}

				expr_loge->expr_taille->transformation = transformation;
			}
			else {
				expr_loge->type = contexte.typeuse.type_pointeur_pour(expr_loge->type);
			}

			/* pour les références */
			auto transformation = cherche_transformation(contexte, expr_loge->expr->type, expr_loge->type);

			if (transformation.type == TypeTransformation::IMPOSSIBLE) {
				erreur::lance_erreur_type_arguments(
							expr_loge->type,
							expr_loge->expr->type,
							contexte,
							expr_loge->lexeme,
							expr_loge->expr->lexeme);
			}

			expr_loge->expr->transformation = transformation;

			if (expr_loge->bloc != nullptr) {
				performe_validation_semantique(expr_loge->bloc, contexte, true);

				auto di = derniere_instruction(expr_loge->bloc);
				if (di == nullptr || !dls::outils::est_element(di->genre, GenreNoeud::INSTRUCTION_RETOUR, GenreNoeud::INSTRUCTION_RETOUR_SIMPLE, GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE, GenreNoeud::INSTRUCTION_CONTINUE_ARRETE)) {
					erreur::lance_erreur("Le bloc sinon d'une instruction « reloge » doit obligatoirement retourner, ou si dans une boucle, la continuer ou l'arrêter",
										 contexte,
										 expr_loge->lexeme);
				}
			}
			else {
				donnees_dependance.fonctions_utilisees.insere(contexte.interface_kuri.decl_panique_memoire);
			}

			donnees_dependance.types_utilises.insere(expr_loge->type);

			break;
		}
		case GenreNoeud::EXPRESSION_DELOGE:
		{
			auto expr_loge = static_cast<NoeudExpressionLogement *>(b);
			performe_validation_semantique(expr_loge->expr, contexte, true);

			auto type = expr_loge->expr->type;

			if (type->genre == GenreType::REFERENCE) {
				expr_loge->expr->transformation = TypeTransformation::DEREFERENCE;
				type = static_cast<TypeReference *>(type)->type_pointe;
			}

			if (!dls::outils::est_element(type->genre, GenreType::POINTEUR, GenreType::TABLEAU_DYNAMIQUE, GenreType::CHAINE)) {
				erreur::lance_erreur("Le type n'est pas délogeable", contexte, b->lexeme);
			}

			break;
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			auto decl = static_cast<NoeudStruct *>(b);

			if (decl->est_externe && decl->bloc == nullptr) {
				return;
			}

			if (decl->bloc->membres.est_vide()) {
				erreur::lance_erreur("Bloc vide pour la déclaration de structure", contexte, decl->lexeme);
			}

			auto decl_precedente = trouve_dans_bloc(decl->bloc_parent, decl);

			// la bibliothèque C a des symboles qui peuvent être les mêmes pour les fonctions et les structres (p.e. stat)
			if (decl_precedente != nullptr && decl_precedente->genre == decl->genre) {
				erreur::redefinition_symbole(contexte, decl->lexeme, decl_precedente->lexeme);
			}

			auto noeud_dependance = graphe.cree_noeud_type(decl->type);
			noeud_dependance->noeud_syntactique = decl;

			auto type_struct = static_cast<TypeStructure *>(decl->type);
			type_struct->membres.reserve(decl->bloc->membres.taille);

			auto verifie_inclusion_valeur = [&decl, &contexte](NoeudBase *enf)
			{
				if (enf->type == decl->type) {
					erreur::lance_erreur(
								"Ne peut inclure la structure dans elle-même par valeur",
								contexte,
								enf->lexeme,
								erreur::type_erreur::TYPE_ARGUMENT);
				}
				else {
					auto type_base = enf->type;

					if (type_base->genre == GenreType::TABLEAU_FIXE) {
						auto type_deref = type_dereference_pour(type_base);

						if (type_deref == decl->type) {
							erreur::lance_erreur(
										"Ne peut inclure la structure dans elle-même par valeur",
										contexte,
										enf->lexeme,
										erreur::type_erreur::TYPE_ARGUMENT);
						}
					}
				}
			};

			auto verifie_redefinition_membre = [&decl, &contexte](NoeudBase *enf)
			{
				if (trouve_dans_bloc_seul(decl->bloc, enf) != nullptr) {
					erreur::lance_erreur(
								"Redéfinition du membre",
								contexte,
								enf->lexeme,
								erreur::type_erreur::MEMBRE_REDEFINI);
				}
			};

			auto decalage = 0u;
			auto max_alignement = 0u;

			auto ajoute_donnees_membre = [&](NoeudBase *enfant, NoeudExpression *expr_valeur)
			{
				auto type_membre = enfant->type;
				auto align_type = type_membre->alignement;

				if (align_type == 0) {
					erreur::lance_erreur(
								"impossible de définir l'alignement du type",
								contexte,
								enfant->lexeme);
				}

				if (type_membre->taille_octet == 0) {
					erreur::lance_erreur(
								"impossible de définir la taille du type",
								contexte,
								enfant->lexeme);
				}

				max_alignement = std::max(align_type, max_alignement);
				auto padding = (align_type - (decalage % align_type)) % align_type;
				decalage += padding;

				type_struct->membres.pousse({ enfant->type, enfant->ident->nom, decalage, 0, expr_valeur });

				donnees_dependance.types_utilises.insere(type_membre);

				decalage += type_membre->taille_octet;
			};

			if (decl->est_union) {
				auto type_union = static_cast<TypeUnion *>(decl->type);
				type_union->est_nonsure = decl->est_nonsure;

				POUR (decl->bloc->membres) {
					auto decl_var = static_cast<NoeudDeclarationVariable *>(it);
					auto decl_membre = decl_var->valeur;

					if (decl_membre->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
						erreur::lance_erreur("Expression invalide dans la déclaration du membre de l'union", contexte, decl_membre->lexeme);
					}

					decl_membre->type = resoud_type_final(contexte, decl_membre->expression_type);

					if (decl_membre->type->genre == GenreType::RIEN) {
						erreur::lance_erreur("Ne peut avoir un type « rien » dans une union", contexte, decl_membre->lexeme, erreur::type_erreur::TYPE_DIFFERENTS);
					}

					decl_var->type = decl_membre->type;

					verifie_redefinition_membre(decl_var);
					verifie_inclusion_valeur(decl_var);

					ajoute_donnees_membre(decl_membre, decl_var->expression);
				}

				auto taille_union = 0u;

				POUR (decl->bloc->membres) {
					auto type_membre = it->type;
					auto taille = type_membre->taille_octet;

					if (taille > taille_union) {
						type_union->type_le_plus_grand = type_membre;
						taille_union = taille;
					}
				}

				/* Pour les unions sûres, il nous faut prendre en compte le
				 * membre supplémentaire. */
				if (!decl->est_nonsure) {
					/* ajoute une marge d'alignement */
					auto padding = (max_alignement - (taille_union % max_alignement)) % max_alignement;
					taille_union += padding;

					type_union->decalage_index = taille_union;

					/* ajoute la taille du membre actif */
					taille_union += static_cast<unsigned>(sizeof(int));

					/* ajoute une marge d'alignement finale */
					padding = (max_alignement - (taille_union % max_alignement)) % max_alignement;
					taille_union += padding;
				}

				decl->type->taille_octet = taille_union;
				decl->type->alignement = max_alignement;

				graphe.ajoute_dependances(*noeud_dependance, donnees_dependance);
				return;
			}

			POUR (decl->bloc->membres) {
				if (it->genre == GenreNoeud::DECLARATION_STRUCTURE) {
					performe_validation_semantique(it, contexte, true);
					continue;
				}

				if (it->genre != GenreNoeud::DECLARATION_VARIABLE) {
					erreur::lance_erreur("Déclaration inattendu dans le bloc de la structure", contexte, it->lexeme);
				}

				auto decl_var = static_cast<NoeudDeclarationVariable *>(it);
				auto decl_membre = decl_var->valeur;

				if (decl_membre->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
					erreur::lance_erreur("Expression invalide dans la déclaration du membre de la structure", contexte, decl_membre->lexeme);
				}

				auto decl_expr = decl_var->expression;

				it->ident = decl_membre->ident;

				decl_membre->type = resoud_type_final(contexte, decl_membre->expression_type);

				verifie_redefinition_membre(decl_var);

				if (decl_expr != nullptr) {
					performe_validation_semantique(decl_expr, contexte, false);

					if (decl_membre->type != decl_expr->type) {
						if (decl_membre->type == nullptr) {
							if (decl_expr->type->genre == GenreType::ENTIER_CONSTANT) {
								decl_membre->type = contexte.typeuse[TypeBase::Z32];
								decl_expr->transformation = { TypeTransformation::CONVERTI_ENTIER_CONSTANT, decl_membre->type };
							}
							else {
								decl_membre->type = decl_expr->type;
							}
						}
						else {
							auto transformation = cherche_transformation(
										contexte,
										decl_expr->type,
										decl_membre->type);

							if (transformation.type == TypeTransformation::IMPOSSIBLE) {
								erreur::lance_erreur_type_arguments(
											decl_membre->type,
										decl_expr->type,
										contexte,
										decl_membre->lexeme,
										decl_expr->lexeme);
							}

							decl_expr->transformation = transformation;
						}
					}
				}

				if (decl_membre->type == nullptr) {
					erreur::lance_erreur("Membre déclaré sans type", contexte, decl_membre->lexeme);
				}

				if (decl_membre->type->genre == GenreType::RIEN) {
					erreur::lance_erreur("Ne peut avoir un type « rien » dans une structure", contexte, decl_membre->lexeme, erreur::type_erreur::TYPE_DIFFERENTS);
				}

				it->type = decl_membre->type;

				verifie_inclusion_valeur(decl_membre);

				// À FAIRE : préserve l'emploi dans les données types
				if (decl_membre->drapeaux & EMPLOYE) {
					if (decl_membre->type->genre != GenreType::STRUCTURE) {
						erreur::lance_erreur("Ne peut employer un type n'étant pas une structure",
											 contexte,
											 decl_membre->lexeme);
					}

					for (auto it_type : type_struct->types_employes) {
						if (decl_membre->type == it_type) {
							erreur::lance_erreur("Ne peut employer plusieurs fois le même type",
												 contexte,
												 decl_membre->lexeme);
						}
					}

					auto type_struct_empl = static_cast<TypeStructure *>(decl_membre->type);
					type_struct->types_employes.pousse(type_struct_empl);

					auto decl_struct_empl = type_struct_empl->decl;

					type_struct->membres.reserve(type_struct->membres.taille + decl_struct_empl->bloc->membres.taille);

					for (auto decl_it_empl : decl_struct_empl->bloc->membres) {
						auto it_empl = static_cast<NoeudDeclarationVariable *>(decl_it_empl);
						ajoute_donnees_membre(it_empl->valeur, it_empl->expression);
					}
				}
				else {
					ajoute_donnees_membre(decl_membre, decl_expr);
				}
			}

			auto padding = (max_alignement - (decalage % max_alignement)) % max_alignement;
			decalage += padding;
			decl->type->taille_octet = decalage;
			decl->type->alignement = max_alignement;

			graphe.ajoute_dependances(*noeud_dependance, donnees_dependance);
			break;
		}
		case GenreNoeud::DECLARATION_ENUM:
		{
			auto decl = static_cast<NoeudEnum *>(b);

			auto type_enum = static_cast<TypeEnum *>(decl->type);
			auto &membres = type_enum->membres;

			if (type_enum->est_erreur) {
				type_enum->type_donnees = contexte.typeuse[TypeBase::Z32];
			}
			else if (decl->expression_type != nullptr) {
				type_enum->type_donnees = resoud_type_final(contexte, decl->expression_type);
			}
			else {
				type_enum->type_donnees = contexte.typeuse[TypeBase::Z32];
			}

			type_enum->taille_octet = type_enum->type_donnees->taille_octet;
			type_enum->alignement = type_enum->type_donnees->alignement;

			contexte.operateurs.ajoute_operateur_basique_enum(decl->type);

			auto noms_rencontres = dls::ensemblon<IdentifiantCode *, 32>();

			auto dernier_res = ResultatExpression();
			/* utilise est_errone pour indiquer que nous sommes à la première valeur */
			dernier_res.est_errone = true;

			membres.reserve(decl->bloc->expressions.taille);

			POUR (decl->bloc->expressions) {
				if (it->genre != GenreNoeud::DECLARATION_VARIABLE) {
					erreur::lance_erreur(
								"Type d'expression inattendu dans l'énum",
								contexte,
								it->lexeme);
				}

				auto decl_expr = static_cast<NoeudDeclarationVariable *>(it);
				decl_expr->type = type_enum->type_donnees;

				auto var = decl_expr->valeur;

				if (var->expression_type != nullptr) {
					erreur::lance_erreur(
								"Expression d'énumération déclarée avec un type",
								contexte,
								it->lexeme);
				}

				if (var->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
					erreur::lance_erreur("Expression invalide dans la déclaration du membre de l'énumération", contexte, var->lexeme);
				}

				if (noms_rencontres.possede(var->ident)) {
					erreur::lance_erreur("Redéfinition du membre", contexte, var->lexeme);
				}

				noms_rencontres.insere(var->ident);

				auto expr = decl_expr->expression;

				it->ident = var->ident;

				auto res = ResultatExpression();

				// À FAIRE(erreur) : vérifie qu'aucune expression s'évalue à zéro
				if (expr != nullptr) {
					performe_validation_semantique(expr, contexte, false);

					res = evalue_expression(contexte, decl->bloc, expr);

					if (res.est_errone) {
						erreur::lance_erreur(
									res.message_erreur,
									contexte,
									res.noeud_erreur->lexeme,
									erreur::type_erreur::VARIABLE_REDEFINIE);
					}
				}
				else {
					if (dernier_res.est_errone) {
						/* première valeur, laisse à zéro si énum normal */
						dernier_res.est_errone = false;

						if (type_enum->est_drapeau || type_enum->est_erreur) {
							res.type = type_expression::ENTIER;
							res.entier = 1;
						}
					}
					else {
						if (dernier_res.type == type_expression::ENTIER) {
							if (type_enum->est_drapeau) {
								res.entier = dernier_res.entier * 2;
							}
							else {
								res.entier = dernier_res.entier + 1;
							}
						}
						else {
							res.reel = dernier_res.reel + 1;
						}
					}
				}

				membres.pousse({ type_enum, var->ident->nom, 0, static_cast<int>(res.entier) });

				dernier_res = res;
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_DISCR:
		{
			auto inst = static_cast<NoeudDiscr *>(b);

			auto expression = inst->expr;

			performe_validation_semantique(expression, contexte, false);
			auto type = expression->type;

			if (type->genre == GenreType::REFERENCE) {
				type = static_cast<TypeReference *>(type)->type_pointe;
				b->transformation = TypeTransformation::DEREFERENCE;
			}

			if (type->genre == GenreType::UNION) {
				auto type_union = static_cast<TypeUnion *>(type);
				auto decl = type_union->decl;

				if (decl->est_nonsure) {
					erreur::lance_erreur(
								"« discr » ne peut prendre une union nonsûre",
								contexte,
								expression->lexeme);
				}

				auto membres_rencontres = dls::ensemblon<IdentifiantCode *, 16>();

				auto valide_presence_membres = [&membres_rencontres, &decl, &contexte, &expression]() {
					auto valeurs_manquantes = dls::ensemble<dls::vue_chaine_compacte>();

					POUR (decl->bloc->membres) {
						if (!membres_rencontres.possede(it->ident)) {
							valeurs_manquantes.insere(it->lexeme->chaine);
						}
					}

					if (valeurs_manquantes.taille() != 0) {
						erreur::valeur_manquante_discr(contexte, expression, valeurs_manquantes);
					}
				};

				b->genre = GenreNoeud::INSTRUCTION_DISCR_UNION;

				for (int i = 0; i < inst->paires_discr.taille; ++i) {
					auto expr_paire = inst->paires_discr[i].first;
					auto bloc_paire = inst->paires_discr[i].second;

					/* vérifie que toutes les expressions des paires sont bel et
					 * bien des membres */
					if (expr_paire->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
						erreur::lance_erreur(
									"Attendu une variable membre de l'union nonsûre",
									contexte,
									expr_paire->lexeme);
					}

					auto nom_membre = expr_paire->ident->nom;

					if (membres_rencontres.possede(expr_paire->ident)) {
						erreur::lance_erreur(
									"Redéfinition de l'expression",
									contexte,
									expr_paire->lexeme);
					}

					membres_rencontres.insere(expr_paire->ident);

					auto decl_var = trouve_dans_bloc_seul(decl->bloc, expr_paire);

					if (decl_var == nullptr) {
						erreur::membre_inconnu(contexte, b, expression, expr_paire, type_union);
					}

					contexte.renseigne_membre_actif(expression->ident->nom, nom_membre);

					auto decl_prec = trouve_dans_bloc(inst->bloc_parent, expression->ident);

					/* Pousse la variable comme étant employée, puisque nous savons ce qu'elle est */
					if (decl_prec != nullptr) {
						erreur::lance_erreur(
									"Ne peut pas utiliser implicitement le membre car une variable de ce nom existe déjà",
									contexte,
									expr_paire->lexeme);
					}

					/* pousse la variable dans le bloc suivant */
					auto decl_expr = static_cast<NoeudDeclaration *>(nullptr);
					decl_expr->ident = expression->ident;
					decl_expr->lexeme = expression->lexeme;
					decl_expr->bloc_parent = bloc_paire;
					decl_expr->drapeaux_decl |= EMPLOYE;
					decl_expr->type = expression->type;
					// À FAIRE: mise en place des informations d'emploie

					bloc_paire->membres.pousse(decl_expr);

					performe_validation_semantique(bloc_paire, contexte, true);
				}

				if (inst->bloc_sinon == nullptr) {
					valide_presence_membres();
				}
				else {
					performe_validation_semantique(inst->bloc_sinon, contexte, true);
				}
			}
			else if (type->genre == GenreType::ENUM || type->genre == GenreType::ERREUR) {
				auto type_enum = static_cast<TypeEnum *>(type);

				auto membres_rencontres = dls::ensemblon<dls::vue_chaine_compacte, 16>();
				b->genre = GenreNoeud::INSTRUCTION_DISCR_ENUM;

				for (int i = 0; i < inst->paires_discr.taille; ++i) {
					auto expr_paire = inst->paires_discr[i].first;
					auto bloc_paire = inst->paires_discr[i].second;

					auto feuilles = dls::tablet<NoeudExpression *, 10>();
					rassemble_feuilles(expr_paire, feuilles);

					for (auto f : feuilles) {
						if (f->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
							erreur::lance_erreur(
										"expression inattendue dans la discrimination, seules les références de déclarations sont supportées pour le moment",
										contexte,
										f->lexeme);
						}

						auto nom_membre = f->ident->nom;

						auto nom_trouve = false;

						POUR (type_enum->membres) {
							if (it.nom == nom_membre) {
								nom_trouve = true;
								break;
							}
						}

						if (!nom_trouve) {
							erreur::membre_inconnu(contexte, b, expression, expr_paire, type_enum);
						}

						if (membres_rencontres.possede(nom_membre)) {
							erreur::lance_erreur(
										"Redéfinition de l'expression",
										contexte,
										f->lexeme);
						}

						membres_rencontres.insere(nom_membre);
					}

					performe_validation_semantique(bloc_paire, contexte, true);
				}

				if (inst->bloc_sinon == nullptr) {
					auto valeurs_manquantes = dls::ensemble<dls::vue_chaine_compacte>();

					POUR (type_enum->membres) {
						if (!membres_rencontres.possede(it.nom)) {
							valeurs_manquantes.insere(it.nom);
						}
					}

					if (valeurs_manquantes.taille() != 0) {
						erreur::valeur_manquante_discr(contexte, expression, valeurs_manquantes);
					}
				}
				else {
					performe_validation_semantique(inst->bloc_sinon, contexte, true);
				}
			}
			else {
				auto candidats = cherche_candidats_operateurs(contexte, type, type, GenreLexeme::EGALITE);
				auto meilleur_candidat = static_cast<OperateurCandidat const *>(nullptr);
				auto poids = 0.0;

				for (auto const &candidat : candidats) {
					if (candidat.poids > poids) {
						poids = candidat.poids;
						meilleur_candidat = &candidat;
					}
				}

				if (meilleur_candidat == nullptr) {
					erreur::lance_erreur_type_operation(contexte, b);
				}

				inst->op = meilleur_candidat->op;

				if (!inst->op->est_basique) {
					donnees_dependance.fonctions_utilisees.insere(inst->op->decl);
				}

				for (int i = 0; i < inst->paires_discr.taille; ++i) {
					auto expr_paire = inst->paires_discr[i].first;
					auto bloc_paire = inst->paires_discr[i].second;

					performe_validation_semantique(bloc_paire, contexte, true);

					auto feuilles = dls::tablet<NoeudExpression *, 10>();
					rassemble_feuilles(expr_paire, feuilles);

					for (auto f : feuilles) {
						performe_validation_semantique(f, contexte, true);

						auto transformation = cherche_transformation(contexte, f->type, expression->type);

						if (transformation.type == TypeTransformation::IMPOSSIBLE) {
							erreur::lance_erreur_type_arguments(
										expression->type,
										f->type,
										contexte,
										f->lexeme,
										expression->lexeme);
						}

						f->transformation = transformation;
					}
				}

				if (inst->bloc_sinon != nullptr) {
					performe_validation_semantique(inst->bloc_sinon, contexte, true);
				}
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_RETIENS:
		{
			if (!fonction_courante->est_coroutine) {
				erreur::lance_erreur(
							"'retiens' hors d'une coroutine",
							contexte,
							b->lexeme);
			}

			auto type_fonc = static_cast<TypeFonction *>(fonction_courante->type);

			auto inst = static_cast<NoeudExpressionUnaire *>(b);
			performe_validation_semantique(inst->expr, contexte, false);

			/* À FAIRE : multiple types retours. */
			auto type_retour = type_fonc->types_sorties[0];
			auto transformation = cherche_transformation(contexte, inst->type, type_retour);

			if (transformation.type == TypeTransformation::IMPOSSIBLE) {
				erreur::lance_erreur_type_retour(
							type_retour,
							inst->type,
							contexte,
							b);
			}

			inst->transformation = transformation;

			break;
		}
		case GenreNoeud::EXPRESSION_PARENTHESE:
		{
			auto expr = static_cast<NoeudExpressionParenthese *>(b);
			performe_validation_semantique(expr->expr, contexte, expr_gauche);
			b->type = expr->expr->type;
			b->genre_valeur = expr->expr->genre_valeur;
			break;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			auto inst = static_cast<NoeudPousseContexte *>(b);
			auto variable = inst->expr;

			auto decl = trouve_dans_bloc(inst->bloc_parent, variable->ident);

			if (decl == nullptr) {
				erreur::lance_erreur("variable inconnu", contexte, variable->lexeme);
			}

			performe_validation_semantique(inst->bloc, contexte, true);

			break;
		}
		case GenreNoeud::EXPANSION_VARIADIQUE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(b);

			if (expr->expr == nullptr) {
				// nous avons un type variadique
				auto type_var = contexte.typeuse.type_variadique(nullptr);
				expr->type = contexte.typeuse.type_type_de_donnees(type_var);
				return;
			}

			performe_validation_semantique(expr->expr, contexte, expr_gauche);

			auto type_expr = expr->expr->type;

			if (type_expr->genre == GenreType::TYPE_DE_DONNEES) {
				auto type_de_donnees = static_cast<TypeTypeDeDonnees *>(type_expr);
				auto type_var = contexte.typeuse.type_variadique(type_de_donnees->type_connu);
				expr->type = contexte.typeuse.type_type_de_donnees(type_var);
			}
			else {
				expr->type = type_expr;
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_TENTE:
		{
			auto inst = static_cast<NoeudTente *>(b);

			// À FAIRE :  défini correctement si on peut ignorer les valeurs de retours
			if (inst->expr_piege != nullptr) {
				expr_gauche = false;
			}

			performe_validation_semantique(inst->expr_appel, contexte, expr_gauche);
			inst->type = inst->expr_appel->type;
			inst->genre_valeur = GenreValeur::DROITE;

			auto type_de_l_erreur = static_cast<Type *>(nullptr);

			// voir ce que l'on retourne
			// - si aucun type erreur -> erreur ?
			// - si erreur seule -> il faudra vérifier l'erreur
			// - si union -> voir si l'union est sûre et contient une erreur, dépaquete celle-ci dans le génération de code

			if (inst->type->genre == GenreType::ERREUR) {
				type_de_l_erreur = inst->type;
			}
			else if (inst->type->genre == GenreType::UNION) {
				auto type_union = static_cast<TypeUnion *>(inst->type);
				auto possede_type_erreur = false;

				POUR (type_union->membres) {
					if (it.type->genre == GenreType::ERREUR) {
						possede_type_erreur = true;
					}
				}

				if (!possede_type_erreur) {
					erreur::lance_erreur("Utilisation de « tente » sur une fonction qui ne retourne pas d'erreur",
										 contexte,
										 inst->lexeme);
				}

				if (type_union->membres.taille == 2) {
					if (type_union->membres[0].type->genre == GenreType::ERREUR) {
						type_de_l_erreur = type_union->membres[0].type;
						inst->type = type_union->membres[1].type;
					}
					else {
						inst->type = type_union->membres[0].type;
						type_de_l_erreur = type_union->membres[1].type;
					}
				}
			}
			else {
				erreur::lance_erreur("Utilisation de « tente » sur une fonction qui ne retourne pas d'erreur",
									 contexte,
									 inst->lexeme);
			}

			if (inst->expr_piege) {
				if (inst->expr_piege->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
					erreur::lance_erreur("Expression inattendu dans l'expression de piège, nous devons avoir une référence à une variable", contexte, inst->expr_piege->lexeme);
				}

				auto var_piege = static_cast<NoeudExpressionReference *>(inst->expr_piege);

				auto decl = trouve_dans_bloc(var_piege->bloc_parent, var_piege->ident);

				if (decl != nullptr) {
					erreur::redefinition_symbole(contexte, var_piege->lexeme, decl->lexeme);
				}

				var_piege->type = type_de_l_erreur;

				auto decl_var_piege = static_cast<NoeudDeclarationVariable *>(contexte.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, var_piege->lexeme));
				decl_var_piege->bloc_parent = inst->bloc;
				decl_var_piege->valeur = var_piege;
				decl_var_piege->type = var_piege->type;
				decl_var_piege->ident = var_piege->ident;

				// ne l'ajoute pas aux expressions, car nous devons l'initialiser manuellement
				inst->bloc->membres.pousse_front(decl_var_piege);

				performe_validation_semantique(inst->bloc, contexte, false);

				auto di = derniere_instruction(inst->bloc);

				if (di == nullptr || !dls::outils::est_element(di->genre, GenreNoeud::INSTRUCTION_RETOUR, GenreNoeud::INSTRUCTION_RETOUR_SIMPLE, GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE, GenreNoeud::INSTRUCTION_CONTINUE_ARRETE)) {
					erreur::lance_erreur("Un bloc de piège doit obligatoirement retourner, ou si dans une boucle, la continuer ou l'arrêter",
										 contexte,
										 inst->lexeme);
				}
			}
			else {
				donnees_dependance.fonctions_utilisees.insere(contexte.interface_kuri.decl_panique_erreur);
			}

			break;
		}
	}
}

void performe_validation_semantique(ContexteGenerationCode &contexte)
{
	//auto nombre_allocations = memoire::nombre_allocations();

	auto debut_validation = dls::chrono::compte_seconde();

	/* valide d'abord les types de fonctions afin de résoudre les fonctions
	 * appelées dans le cas de fonctions mutuellement récursives */
	POUR (contexte.file_typage) {
		if (it->genre != GenreNoeud::DECLARATION_FONCTION && it->genre != GenreNoeud::DECLARATION_COROUTINE) {
			continue;
		}

		auto decl = static_cast<NoeudDeclarationFonction *>(it);

		valide_type_fonction(decl, contexte);
	}

	//std::cout << "Nombre allocations typage fonctions = " << memoire::nombre_allocations() - nombre_allocations << '\n';

	POUR (contexte.file_typage) {
		performe_validation_semantique(it, contexte, true);
	}

	contexte.temps_validation = debut_validation.temps();

	//std::cout << "Nombre allocations validations sémantique = " << memoire::nombre_allocations() - nombre_allocations << '\n';
}

}  /* namespace noeud */
