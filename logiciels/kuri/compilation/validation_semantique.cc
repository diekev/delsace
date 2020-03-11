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
#include "biblinternes/langage/nombres.hh"
#include "biblinternes/outils/chaine.hh"
#include "biblinternes/outils/conditions.h"
#include "biblinternes/structures/dico_fixe.hh"
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

using denombreuse = lng::decoupeuse_nombre<GenreLexeme>;

namespace noeud {

/* ************************************************************************** */

static void performe_validation_semantique(
		NoeudExpression *b,
		ContexteGenerationCode &contexte,
		bool expr_gauche);

static Type *resoud_type_final(
		ContexteGenerationCode &contexte,
		DonneesTypeDeclare &type_declare,
		NoeudBloc *bloc,
		DonneesLexeme const *lexeme,
		bool evalue_expr = true)
{
	if (type_declare.taille() == 0) {
		return nullptr;
	}

	auto type_final = static_cast<Type *>(nullptr);
	auto &typeuse = contexte.typeuse;
	auto idx_expr = 0;

	for (auto i = type_declare.taille() - 1; i >= 0; --i) {
		auto type = type_declare[i];

		if (type == GenreLexeme::TYPE_DE) {
			auto expr = type_declare.expressions[idx_expr++];
			assert(expr != nullptr);

			performe_validation_semantique(expr, contexte, false);

			if (expr->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				auto decl = trouve_dans_bloc(expr->bloc_parent, expr->ident);
				// À FAIRE : decl peut être nulle
				expr->type = decl->type;
			}

			type_final = expr->type;
		}
		else if (type == GenreLexeme::TROIS_POINTS) {
			type_final = typeuse.type_variadique(type_final);
		}
		else if (type == GenreLexeme::TABLEAU) {
			auto expr = type_declare.expressions[idx_expr++];

			if (expr != nullptr && evalue_expr) {
				performe_validation_semantique(expr, contexte, false);

				auto res = evalue_expression(contexte, expr->bloc_parent, expr);

				if (res.est_errone) {
					erreur::lance_erreur(
								res.message_erreur,
								contexte,
								expr->lexeme);
				}

				if (res.type != type_expression::ENTIER) {
					erreur::lance_erreur(
								"Attendu un type entier pour l'expression du tableau",
								contexte,
								expr->lexeme);
				}

				if (res.entier == 0) {
					erreur::lance_erreur(
								"L'expression évalue à zéro",
								contexte,
								expr->lexeme);
				}

				type_final = typeuse.type_tableau_fixe(type_final, res.entier);
			}
			else {
				type_final = typeuse.type_tableau_dynamique(type_final);
			}
		}
		else if (type == GenreLexeme::DOLLAR) {
			for (auto &paire : contexte.paires_expansion_gabarit) {
				if (paire.first == type_declare.nom_gabarit) {
					type_final = paire.second;
				}
			}
		}
		else if (type == GenreLexeme::CHAINE_CARACTERE) {
			auto ident = contexte.table_identifiants.identifiant_pour_chaine(type_declare.nom_struct);
			auto fichier =  contexte.fichier(static_cast<size_t>(lexeme->fichier));
			auto decl = trouve_type_dans_bloc_ou_module(contexte, bloc, ident, fichier);

			if (decl == nullptr) {
				erreur::lance_erreur("Impossible de définir le type selon le nom", contexte, lexeme);
			}

			if (!est_declaration(decl->genre)) {
				erreur::lance_erreur("Le symbole n'est pas celui d'une déclaration", contexte, lexeme);
			}

			type_final = decl->type;
		}
		else if (type == GenreLexeme::POINTEUR) {
			type_final = typeuse.type_pointeur_pour(type_final);
		}
		else if (type == GenreLexeme::REFERENCE) {
			type_final = typeuse.type_reference_pour(type_final);
		}
		else if (type == GenreLexeme::FONC) {
			auto types_entrees = kuri::tableau<Type *>();

			for (auto t = 0; t < type_declare.types_entrees.taille(); ++t) {
				auto td = type_declare.types_entrees[t];
				auto type_entree = resoud_type_final(contexte, td, bloc, lexeme);

				types_entrees.pousse(type_entree);
			}

			auto types_sorties = kuri::tableau<Type *>();

			for (auto t = 0; t < type_declare.types_sorties.taille(); ++t) {
				auto td = type_declare.types_sorties[t];
				auto type_sortie = resoud_type_final(contexte, td, bloc, lexeme);

				types_sorties.pousse(type_sortie);
			}

			type_final = typeuse.type_fonction(types_entrees, types_sorties);
		}
		else {
			type_final = typeuse.type_pour_lexeme(type);
		}
	}

	assert(type_final != nullptr);

	return type_final;
}

/* ************************************************************************** */

static NoeudBase *derniere_instruction(NoeudBloc *b)
{
	if (b->expressions.taille == 0) {
		return static_cast<NoeudBase *>(nullptr);
	}

	auto di = b->expressions[b->expressions.taille - 1];

	if (est_instruction_retour(di->genre)) {
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

static auto valide_appel_pointeur_fonction(
		NoeudExpressionAppel *b,
		ContexteGenerationCode &contexte,
		kuri::tableau<IdentifiantCode *> const &noms_arguments,
		kuri::tableau<NoeudExpression *> const &args)
{
	for (auto i = 0; i < noms_arguments.taille; ++i) {
		if (noms_arguments[i] == nullptr) {
			continue;
		}

		erreur::lance_erreur(
					"Les arguments d'un pointeur fonction ne peuvent être nommés",
					contexte,
					args[i]->lexeme,
					erreur::type_erreur::ARGUMENT_INCONNU);
	}

	if (b->type->genre != GenreType::FONCTION) {
		erreur::lance_erreur(
					"La variable doit être un pointeur vers une fonction",
					contexte,
					b->lexeme,
					erreur::type_erreur::FONCTION_INCONNUE);
	}

	/* vérifie la compatibilité des arguments pour déterminer
	 * s'il y aura besoin d'une transformation. */
	auto type_fonction = static_cast<TypeFonction *>(b->type);

	auto debut_params = 0l;

	if (type_fonction->types_entrees.taille != 0 && type_fonction->types_entrees[0] == contexte.type_contexte) {
		debut_params = 1;

		auto fonc_courante = contexte.donnees_fonction;

		if (fonc_courante != nullptr && dls::outils::possede_drapeau(fonc_courante->drapeaux, FORCE_NULCTX)) {
			erreur::lance_erreur_fonction_nulctx(contexte, b, b, fonc_courante);
		}
	}
	else {
		b->drapeaux |= FORCE_NULCTX;
	}

	/* Validation des types passés en paramètre. */
	for (auto i = debut_params; i < type_fonction->types_entrees.taille; ++i) {
		auto arg = args[i - debut_params];
		auto type_prm = type_fonction->types_entrees[i];
		auto type_enf = arg->type;

		if (type_prm->genre == GenreType::VARIADIQUE) {
			type_prm = contexte.typeuse.type_dereference_pour(type_prm);
		}

		auto transformation = cherche_transformation(type_enf, type_prm);

		if (transformation.type == TypeTransformation::IMPOSSIBLE) {
			erreur::lance_erreur_type_arguments(
						type_prm,
						type_enf,
						contexte,
						arg->lexeme,
						b->lexeme);
		}

		arg->transformation = transformation;

		b->exprs.pousse(arg);
	}

	b->nom_fonction_appel = b->ident->nom;
	/* À FAIRE : multiples types retours */
	b->type = type_fonction->types_sorties[0];
	b->type_fonc = type_fonction;
	b->aide_generation_code = APPEL_POINTEUR_FONCTION;
}

static void valide_acces_membre(
		ContexteGenerationCode &contexte,
		NoeudExpression *b,
		NoeudExpression *structure,
		NoeudExpression *membre,
		bool expr_gauche)
{
	performe_validation_semantique(structure, contexte, expr_gauche);

	auto type = structure->type;

	/* nous pouvons avoir une référence d'un pointeur, donc déréférence au plus */
	while (type->genre == GenreType::POINTEUR || type->genre == GenreType::REFERENCE) {
		type = static_cast<TypePointeur *>(type)->type_pointe;
	}

	auto est_structure = type->genre == GenreType::STRUCTURE;

	if (membre->genre == GenreNoeud::EXPRESSION_APPEL_FONCTION) {
		/* si nous avons une structure, vérifie si nous avons un appel vers un
		 * pointeur de fonction */
		if (est_structure) {
			auto noeud_struct = static_cast<TypeStructure *>(type)->decl;

			if (!noeud_struct->est_union) {
				POUR (noeud_struct->desc.membres) {
					if (it.nom == membre->ident->nom) {
						b->type = it.type;
						membre->type = it.type;
						membre->aide_generation_code = GENERE_CODE_PTR_FONC_MEMBRE;

						performe_validation_semantique(membre, contexte, expr_gauche);

						/* le type de l'accès est celui du retour de la fonction */
						b->type = membre->type;
						b->genre_valeur = GenreValeur::DROITE;
						return;
					}
				}
			}
		}

		auto expr_appel = static_cast<NoeudExpressionAppel *>(membre);
		expr_appel->params.pousse_front(structure);

		performe_validation_semantique(membre, contexte, expr_gauche);

//		/* transforme le noeud, À FAIRE (réusinage arbre) */
//		b->genre = GenreNoeud::EXPRESSION_APPEL_FONCTION;
//		b->module_appel = membre->module_appel;
		b->type = membre->type;
		b->drapeaux |= EST_APPEL_SYNTAXE_UNIFORME;
//		b->nom_fonction_appel = membre->nom_fonction_appel;
//		b->df = membre->df;
//		b->enfants = membre->enfants;
//		b->genre_valeur = GenreValeur::DROITE;

		return;
	}

	if (type->genre == GenreType::CHAINE) {
		if (membre->ident->nom == "taille") {
			b->type = contexte.typeuse[TypeBase::Z64];
			return;
		}

		if (membre->ident->nom == "pointeur") {
			b->type = contexte.typeuse[TypeBase::PTR_Z8];
			return;
		}

		erreur::membre_inconnu_chaine(contexte, b, structure, membre);
	}

	if (type->genre == GenreType::EINI) {
		if (membre->ident->nom == "info") {
			auto type_info_type = contexte.typeuse.type_pour_nom("InfoType");
			assert(type_info_type != nullptr);
			b->type = contexte.typeuse.type_pointeur_pour(type_info_type);
			return;
		}

		if (membre->ident->nom == "pointeur") {
			b->type = contexte.typeuse[TypeBase::PTR_Z8];
			return;
		}

		erreur::membre_inconnu_eini(contexte, b, structure, membre);
	}

	if (type->genre == GenreType::TABLEAU_DYNAMIQUE || type->genre == GenreType::VARIADIQUE) {
#ifdef NONSUR
		if (!contexte.non_sur() && expr_gauche) {
			erreur::lance_erreur(
						"Modification des membres du tableau hors d'un bloc 'nonsûr' interdite",
						contexte,
						b->lexeme,
						erreur::type_erreur::ASSIGNATION_INVALIDE);
		}
#endif
		if (membre->ident->nom == "pointeur") {
			b->type = contexte.typeuse.type_pointeur_pour(contexte.typeuse.type_dereference_pour(type));
			return;
		}

		if (membre->ident->nom == "taille") {
			b->type = contexte.typeuse[TypeBase::Z64];
			return;
		}

		erreur::membre_inconnu_tableau(contexte, b, structure, membre);
	}

	if (type->genre == GenreType::TABLEAU_FIXE) {
		if (membre->ident->nom == "pointeur") {
			b->type = contexte.typeuse.type_pointeur_pour(contexte.typeuse.type_dereference_pour(type));
			return;
		}

		if (membre->ident->nom == "taille") {
			b->type = contexte.typeuse[TypeBase::Z64];
			return;
		}

		erreur::membre_inconnu_tableau(contexte, b, structure, membre);
	}

	if (est_structure) {
		auto noeud_struct = static_cast<TypeStructure *>(type)->decl;
		auto membre_trouve = false;

		POUR (noeud_struct->desc.membres) {
			if (it.nom == membre->ident->nom) {
				membre_trouve = true;
				b->type = it.type;
				return;
			}
		}

		if (membre_trouve == false) {
			erreur::membre_inconnu(contexte, noeud_struct->bloc, b, structure, membre, type);
		}

		if (noeud_struct->est_union && !noeud_struct->est_nonsure) {
			b->genre = GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION;

			// À FAIRE (réusinage arbre)
			//b->valeur_calculee = noeud_membre->index_membre;

			if (expr_gauche) {
				contexte.renseigne_membre_actif(structure->ident->nom, membre->ident->nom);
			}
			else {
				auto membre_actif = contexte.trouve_membre_actif(structure->ident->nom);

				/* si l'union vient d'un retour ou d'un paramètre, le membre actif sera inconnu */
				if (membre_actif != "" && membre_actif != membre->ident->nom) {
					erreur::membre_inactif(contexte, b, structure, membre);
				}

				/* nous savons que nous avons le bon membre actif */
				b->aide_generation_code = IGNORE_VERIFICATION;
			}
		}

		return;
	}

	if (type->genre == GenreType::ENUM) {
		auto noeud_enum = static_cast<TypeEnum *>(type)->decl;
		auto noeud_membre = trouve_dans_bloc_seul(noeud_enum->bloc, membre);

		if (noeud_membre == nullptr) {
			erreur::membre_inconnu(contexte, noeud_enum->bloc, b, structure, membre, type);
		}

		b->type = noeud_enum->type;
		b->genre_valeur = GenreValeur::DROITE;
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

static void valide_type_fonction(NoeudExpression *b, ContexteGenerationCode &contexte)
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
	if (!contexte.pour_gabarit) {
		auto noms = dls::ensemble<IdentifiantCode *>();
		auto dernier_est_variadic = false;

		POUR (decl->params) {
			auto param = static_cast<NoeudDeclarationVariable *>(it);
			auto variable = param->valeur;
			auto expression = param->expression;

			if (expression != nullptr) {
				if (decl->genre == GenreNoeud::DECLARATION_OPERATEUR) {
					erreur::lance_erreur("Un paramètre d'une surcharge d'opérateur ne peut avoir de valeur par défaut",
										 contexte,
										 param->lexeme);
				}

				performe_validation_semantique(expression, contexte, false);

				/* À FAIRE: vérifie que le type de l'argument correspond à celui de l'expression */
				if (variable->type == nullptr) {
					variable->type = expression->type;
				}
			}

			if (noms.trouve(variable->ident) != noms.fin()) {
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

			auto &type_declare = variable->type_declare;

			if (type_declare.est_gabarit) {
				decl->noms_types_gabarits.pousse(type_declare.nom_gabarit);
				decl->est_gabarit = true;
			}

			if (!type_declare.est_gabarit) {
				if (!est_invalide(type_declare.plage())) {
					variable->type = resoud_type_final(contexte, type_declare, variable->bloc_parent, variable->lexeme);
				}
			}

			it->type = variable->type;

			noms.insere(variable->ident);

			/* doit être vrai uniquement pour le dernier argument */
			if (type_declare.type_base() == GenreLexeme::TROIS_POINTS) {
				it->drapeaux |= EST_VARIADIQUE;
				decl->est_variadique = true;

				if (it == decl->params[decl->params.taille - 1]) {
					if (!decl->est_externe && est_invalide(type_declare.dereference())) {
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

			variable->type = resoud_type_final(contexte, variable->type_declare, variable->bloc_parent, variable->lexeme);
			variable->type_declare.est_gabarit = false;

			it->type = variable->type;
		}
	}

	// -----------------------------------

	kuri::tableau<Type *> types_entrees;
	auto possede_contexte = !decl->est_externe && !possede_drapeau(b->drapeaux, FORCE_NULCTX);

	if (possede_contexte) {
		types_entrees.pousse(contexte.type_contexte);
	}

	POUR (decl->params) {
		types_entrees.pousse(it->type);
		contexte.donnees_dependance.types_utilises.insere(it->type);
	}

	kuri::tableau<Type *> types_sorties;

	for (auto &type_declare : decl->type_declare.types_sorties) {
		auto type_sortie = resoud_type_final(contexte, type_declare, b->bloc_parent, decl->lexeme);
		types_sorties.pousse(type_sortie);
		contexte.donnees_dependance.types_utilises.insere(type_sortie);
	}

	decl->type_fonc = contexte.typeuse.type_fonction(types_entrees, types_sorties);
	decl->type = decl->type_fonc;
	contexte.donnees_dependance.types_utilises.insere(b->type);

	if (decl->genre == GenreNoeud::DECLARATION_OPERATEUR) {
		auto &iter_op = contexte.operateurs.trouve(decl->lexeme->genre);

		auto type_resultat = types_sorties[0];

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
			auto type1 = types_entrees[0 + possede_contexte];

			for (auto i = 0; i < iter_op.taille(); ++i) {
				auto op = &iter_op[i];

				if (op->type1 == type1) {
					// À FAIRE : stocke le noeud de déclaration, quid des opérateurs basique ?
					erreur::lance_erreur("redéfinition de l'opérateur", contexte, decl->lexeme);
				}
			}

			contexte.operateurs.ajoute_perso_unaire(
						decl->lexeme->genre,
						type1,
						type_resultat,
						decl->nom_broye);
		}
		else if (decl->params.taille == 2) {
			auto type1 = types_entrees[0 + possede_contexte];
			auto type2 = types_entrees[1 + possede_contexte];

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
						types_sorties[0],
						decl->nom_broye);
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

	contexte.graphe_dependance.cree_noeud_fonction(decl->nom_broye, decl);
}

static void performe_validation_semantique(
		NoeudExpression *b,
		ContexteGenerationCode &contexte,
		bool expr_gauche)
{
	auto &graphe = contexte.graphe_dependance;
	auto &donnees_dependance = contexte.donnees_dependance;
	auto fonction_courante = contexte.donnees_fonction;

	switch (b->genre) {
		case GenreNoeud::INSTRUCTION_SINON:
		case GenreNoeud::RACINE:
		case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:
		case GenreNoeud::INSTRUCTION_RETOUR_SIMPLE:
		case GenreNoeud::DECLARATION_COROUTINE:
		case GenreNoeud::EXPRESSION_INDICE:
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			break;
		}
		case GenreNoeud::DECLARATION_FONCTION:
		{
			auto decl = static_cast<NoeudDeclarationFonction *>(b);
			using dls::outils::possede_drapeau;

			/* Il est possible que certaines fonctions ne soient pas connectées
			 * dans le graphe de symboles alors que nous avons besoin d'elles,
			 * voir dans la fonction plus bas. */
			if (decl->type == nullptr && !decl->est_gabarit) {
				valide_type_fonction(decl, contexte);
			}

			if (decl->est_gabarit && !contexte.pour_gabarit) {
				// nous ferons l'analyse sémantique plus tard
				return;
			}

			if (decl->est_externe) {
				POUR (decl->params) {
					auto variable = static_cast<NoeudDeclarationVariable *>(it)->valeur;

					variable->type = resoud_type_final(contexte, variable->type_declare, decl->bloc_parent, variable->lexeme);
					it->type = variable->type;
					donnees_dependance.types_utilises.insere(variable->type);
				}

				auto noeud_dep = graphe.cree_noeud_fonction(decl->ident->nom, decl);
				graphe.ajoute_dependances(*noeud_dep, donnees_dependance);

				return;
			}

			contexte.commence_fonction(decl);

			if (!possede_drapeau(decl->drapeaux, FORCE_NULCTX)) {
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
		//				type_var = contexte.typeuse.type_dereference_pour(type_var);
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

			auto noeud_dep = graphe.cree_noeud_fonction(decl->nom_broye, decl);

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

			if (!possede_drapeau(decl->drapeaux, FORCE_NULCTX)) {
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
		//				type_var = contexte.typeuse.type_dereference_pour(type_var);
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

			auto noeud_dep = graphe.cree_noeud_fonction(decl->nom_broye, decl);

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

			kuri::tableau<IdentifiantCode *> noms_args;
			kuri::tableau<NoeudExpression *> args;

			/* Commence par valider les enfants puisqu'il nous faudra leurs
			 * types pour déterminer la fonction à appeler. */
			for (auto f : expr->params) {
				// l'argument est nommé
				if (f->genre == GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE) {
					auto assign = static_cast<NoeudExpressionBinaire *>(f);
					auto nom_arg = assign->expr1;
					auto arg = assign->expr2;

					performe_validation_semantique(arg, contexte, false);

					noms_args.pousse(nom_arg->ident);
					args.pousse(arg);
				}
				else {
					performe_validation_semantique(f, contexte, false);

					noms_args.pousse(nullptr);
					args.pousse(f);
				}
			}

			// ------------
			// trouve la fonction, pour savoir ce que l'on a

			auto const nom_fonction = dls::chaine(b->lexeme->chaine);

			// À FAIRE(réusinage arbre): syntaxe uniforme
//			if (b->nom_fonction_appel != "") {
//				/* Nous avons déjà validé ce noeud, sans doute via une syntaxe
//				 * d'appel uniforme. */
//				return;
//			}

			/* Nous avons un pointeur vers une fonction. */
			if (b->aide_generation_code == GENERE_CODE_PTR_FONC_MEMBRE) {
				valide_appel_pointeur_fonction(expr, contexte, noms_args, args);
				return;
			}

			auto decl = trouve_dans_bloc(expr->bloc_parent, expr->ident);

			/* Nous avons un pointeur vers une fonction.
			 * À FAIRE: vérifie que la declaration sont antérieure, la condition 'decl->type' sers à ça.
			 * Peut que l'on pourrait fusionner la logique avec celle de la recherche de fonctions, et mettre les pointeurs dans les candidats. */
			if (decl != nullptr && decl->genre == GenreNoeud::DECLARATION_VARIABLE && decl->type && decl->type->genre == GenreType::FONCTION) {
				b->type = decl->type;
				valide_appel_pointeur_fonction(expr, contexte, noms_args, args);
				return;
			}

			auto res = cherche_donnees_fonction(
						contexte,
						expr->ident,
						noms_args,
						args,
						static_cast<size_t>(b->lexeme->fichier),
						static_cast<size_t>(b->module_appel));

			auto candidate = static_cast<DonneesCandidate *>(nullptr);

			auto poids = 0.0;

			for (auto &dc : res.candidates) {
				if (dc.etat == FONCTION_TROUVEE) {
					if (dc.poids_args > poids) {
						candidate = &dc;
						poids = dc.poids_args;
					}
				}
			}

			if (candidate == nullptr || candidate->decl_fonc == nullptr) {
				erreur::lance_erreur_fonction_inconnue(
							contexte,
							b,
							res.candidates);
			}

			auto decl_fonction_appelee = candidate->decl_fonc;

			/* pour les directives d'exécution, la fonction courante est nulle */
			if (fonction_courante != nullptr) {
				using dls::outils::possede_drapeau;
				auto decl_fonc = fonction_courante;

				if (possede_drapeau(decl_fonc->drapeaux, FORCE_NULCTX)) {
					auto decl_appel = decl_fonction_appelee;

					if (!decl_appel->est_externe && !possede_drapeau(decl_appel->drapeaux, FORCE_NULCTX)) {
						erreur::lance_erreur_fonction_nulctx(
									contexte,
									b,
									decl_fonc,
									decl_appel);
					}
				}
			}

			/* ---------------------- */

			if (!candidate->paires_expansion_gabarit.est_vide()) {
				auto noeud_decl = copie_noeud(contexte.assembleuse, decl_fonction_appelee, decl_fonction_appelee->bloc_parent);
				decl_fonction_appelee = static_cast<NoeudDeclarationFonction *>(noeud_decl);

				contexte.pour_gabarit = true;
				contexte.paires_expansion_gabarit = candidate->paires_expansion_gabarit;

				valide_type_fonction(noeud_decl, contexte);

				performe_validation_semantique(noeud_decl, contexte, expr_gauche);

				contexte.donnees_fonction = fonction_courante;

				contexte.pour_gabarit = false;
			}

			/* met en place les drapeaux sur les enfants */

			auto nombre_args_simples = candidate->exprs.taille();
			auto nombre_args_variadics = nombre_args_simples;

			if (!candidate->exprs.est_vide() && candidate->exprs.back()->genre == GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES) {
				/* ne compte pas le tableau */
				nombre_args_simples -= 1;
				nombre_args_variadics = candidate->transformations.taille();

				/* ajoute le type du tableau */
				auto noeud_tabl = static_cast<NoeudTableauArgsVariadiques *>(candidate->exprs.back());
				auto taille_tableau = noeud_tabl->exprs.taille;
				auto &type_tabl = noeud_tabl->type;

				auto type_tfixe = contexte.typeuse.type_tableau_fixe(type_tabl, taille_tableau);
				donnees_dependance.types_utilises.insere(type_tfixe);
			}

			auto i = 0l;
			/* les drapeaux pour les arguments simples */
			for (; i < nombre_args_simples; ++i) {
				auto enfant = candidate->exprs[i];
				enfant->transformation = candidate->transformations[i];
			}

			/* les drapeaux pour les arguments variadics */
			if (!candidate->exprs.est_vide() && candidate->exprs.back()->genre == GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES) {
				auto noeud_tableau = static_cast<NoeudTableauArgsVariadiques *>(candidate->exprs.back());
				auto enfant_tabl = noeud_tableau->exprs.begin();

				for (; i < nombre_args_variadics; ++i) {
					auto enfant = *enfant_tabl++;
					enfant->transformation = candidate->transformations[i];
				}
			}

			expr->noeud_fonction_appelee = decl_fonction_appelee;
			expr->nom_fonction_appel = decl_fonction_appelee->nom_broye;

			if (b->type == nullptr) {
				/* À FAIRE: multiple type retour */
				b->type = decl_fonction_appelee->type_fonc->types_sorties[0];
			}

			for (auto enfant : candidate->exprs) {
				expr->exprs.pousse(enfant);
			}

			donnees_dependance.fonctions_utilisees.insere(decl_fonction_appelee->nom_broye);

			for (auto te : decl_fonction_appelee->type_fonc->types_entrees) {
				donnees_dependance.types_utilises.insere(te);
			}

			for (auto ts : decl_fonction_appelee->type_fonc->types_sorties) {
				donnees_dependance.types_utilises.insere(ts);
			}

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

			/* À FAIRE: vérifie que la déclaration est antérieure à la référence (pour une variable) */

			expr->type = decl->type;
			assert(expr->type);

			if (decl->drapeaux & EST_VAR_BOUCLE) {
				expr->drapeaux |= EST_VAR_BOUCLE;
			}

			donnees_dependance.types_utilises.insere(expr->type);

			if (decl->genre == GenreNoeud::DECLARATION_FONCTION) {
				auto decl_fonc = static_cast<NoeudDeclarationFonction *>(decl);
				b->nom_fonction_appel = decl_fonc->nom_broye;
				donnees_dependance.fonctions_utilisees.insere(b->nom_fonction_appel);
			}
			else if (decl->genre == GenreNoeud::DECLARATION_VARIABLE) {
				if (decl->drapeaux & EST_GLOBALE) {
					donnees_dependance.globales_utilisees.insere(b->lexeme->chaine);
				}
			}

			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		{
			auto inst = static_cast<NoeudExpressionBinaire *>(b);
			auto enfant1 = inst->expr1;
			auto enfant2 = inst->expr2;
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

			performe_validation_semantique(expression, contexte, false);

			if (expression->type == nullptr) {
				erreur::lance_erreur(
							"Impossible de définir le type de la variable !",
							contexte,
							b->lexeme,
							erreur::type_erreur::TYPE_INCONNU);
			}

			/* NOTE : l'appel à performe_validation_semantique plus bas peut
			 * changer le vecteur et invalider une référence ou un pointeur,
			 * donc nous faisons une copie... */
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

				dls::tableau<NoeudExpression *> feuilles;
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
			decl->ident = variable->ident;

			auto decl_prec = trouve_dans_bloc(variable->bloc_parent, decl);

			if (decl_prec != nullptr && decl_prec->genre == decl->genre) {
				auto fichier = contexte.fichier(static_cast<size_t>(decl->lexeme->fichier));
				auto pos_decl = trouve_position(*decl->lexeme, fichier);
				auto pos_decl_prec = trouve_position(*decl_prec->lexeme, fichier);

				if (pos_decl.numero_ligne > pos_decl_prec.numero_ligne) {
					erreur::redefinition_symbole(contexte, variable->lexeme, decl_prec->lexeme);
				}
			}

			variable->type = resoud_type_final(contexte, variable->type_declare, decl->bloc_parent, variable->lexeme);

			if (expression != nullptr) {
				performe_validation_semantique(expression, contexte, false);

				if (expression->type == nullptr) {
					erreur::lance_erreur("impossible de définir le type de l'expression", contexte, expression->lexeme);
				}
				else if (variable->type == nullptr) {
					if (expression->type->genre == GenreType::ENTIER_CONSTANT) {
						variable->type = contexte.typeuse[TypeBase::Z32];
					}
					else {
						variable->type = expression->type;
					}
				}
				else {
					auto transformation = cherche_transformation(
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
				graphe.cree_noeud_globale(variable->ident->nom, b);
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

			/* À FAIRE : transformation automatique */

			performe_validation_semantique(enfant1, contexte, expr_gauche);
			performe_validation_semantique(enfant2, contexte, expr_gauche);

			auto type1 = enfant1->type;
			auto type2 = enfant2->type;

			/* détecte a comp b comp c */
			if (est_operateur_comp(expr->lexeme->genre) && est_operateur_comp(enfant1->lexeme->genre)) {
				expr->genre = GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE;
				expr->type = contexte.typeuse[TypeBase::BOOL];

				auto type_op = expr->lexeme->genre;

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

				if (!expr->op->est_basique) {
					donnees_dependance.fonctions_utilisees.insere(expr->op->nom_fonction);
				}
			}
			else if (b->lexeme->genre == GenreLexeme::CROCHET_OUVRANT) {
				expr->genre = GenreNoeud::EXPRESSION_INDICE;
				expr->genre_valeur = GenreValeur::TRANSCENDANTALE;

				if (type1->genre == GenreType::REFERENCE) {
					enfant1->transformation = TypeTransformation::DEREFERENCE;
					type1 = contexte.typeuse.type_dereference_pour(type1);
				}

				switch (type1->genre) {
					case GenreType::VARIADIQUE:
					case GenreType::TABLEAU_DYNAMIQUE:
					{
						expr->type = contexte.typeuse.type_dereference_pour(type1);
						break;
					}
					case GenreType::TABLEAU_FIXE:
					{
						auto type_tabl = static_cast<TypeTableauFixe *>(type1);
						expr->type = contexte.typeuse.type_dereference_pour(type1);

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

						break;
					}
					case GenreType::POINTEUR:
					{
						expr->type = contexte.typeuse.type_dereference_pour(type1);
						break;
					}
					case GenreType::CHAINE:
					{
						expr->type = contexte.typeuse[TypeBase::Z8];
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
			}
			else {
				auto type_op = b->lexeme->genre;

				if (est_assignation_operee(type_op)) {
					type_op = operateur_pour_assignation_operee(type_op);
					expr->drapeaux |= EST_ASSIGNATION_OPEREE;
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
					donnees_dependance.fonctions_utilisees.insere(expr->op->nom_fonction);
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

			if (type->genre == GenreType::REFERENCE) {
				enfant->transformation = TypeTransformation::DEREFERENCE;
				type = contexte.typeuse.type_dereference_pour(type);
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
				else {
					if (type->genre == GenreType::ENTIER_CONSTANT) {
						type = contexte.typeuse[TypeBase::Z32];
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
			auto nombre_retour = fonction_courante->type_fonc->types_sorties.taille;

			if (nombre_retour > 1) {
				if (enfant->lexeme->genre == GenreLexeme::VIRGULE) {
					dls::tableau<NoeudExpression *> feuilles;
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
			/* fais en sorte que les caractères échappés ne soient pas comptés
			 * comme deux caractères distincts, ce qui ne peut se faire avec la
			 * dls::vue_chaine */
			dls::chaine corrigee;
			corrigee.reserve(b->lexeme->chaine.taille());

			for (auto i = 0l; i < b->lexeme->chaine.taille(); ++i) {
				auto c = b->lexeme->chaine[i];

				if (c == '\\') {
					c = dls::caractere_echappe(&b->lexeme->chaine[i]);
					++i;
				}

				corrigee.pousse(c);
			}

			/* À FAIRE : ceci ne fonctionne pas dans le cas des noeuds différés
			 * où la valeur calculee est redéfinie. */
			b->valeur_calculee = corrigee;
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

			if (type_condition->genre != GenreType::BOOL) {
				erreur::lance_erreur("Attendu un type booléen pour l'expression 'si'",
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
#ifdef AVEC_LLVM
			/* Évite les crash lors de l'estimation du bloc suivant les
			 * contrôles de flux. */
			b->valeur_calculee = static_cast<llvm::BasicBlock *>(nullptr);
#endif

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

			auto feuilles = dls::tableau<NoeudExpression *>{};
			rassemble_feuilles(enfant1, feuilles);

			for (auto f : feuilles) {
				auto decl_f = trouve_dans_bloc(b->bloc_parent, f->ident);

				if (decl_f != nullptr) {
					auto fichier = contexte.fichier(static_cast<size_t>(f->lexeme->fichier));
					auto pos_decl = trouve_position(*f->lexeme, fichier);
					auto pos_decl_prec = trouve_position(*decl_f->lexeme, fichier);

					if (pos_decl.numero_ligne > pos_decl_prec.numero_ligne) {
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
					type = contexte.typeuse.type_dereference_pour(type);

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
				decl_idx->type = contexte.typeuse[TypeBase::Z32];
				decl_idx->ident = idx->ident;
				decl_idx->lexeme = idx->lexeme;

				enfant3->membres.pousse(decl_idx);
			}

			/* À FAIRE : ceci duplique logique coulisse. */
			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b);
			auto goto_brise = "__boucle_pour_brise" + dls::vers_chaine(b);

			contexte.empile_goto_continue(variable->ident->nom, goto_continue);
			contexte.empile_goto_arrete(variable->ident->nom, (enfant4 != nullptr) ? goto_brise : goto_apres);

			performe_validation_semantique(enfant3, contexte, true);

			if (enfant4 != nullptr) {
				performe_validation_semantique(enfant4, contexte, true);
			}

			if (enfant5 != nullptr) {
				performe_validation_semantique(enfant5, contexte, true);
			}

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			break;
		}
		case GenreNoeud::EXPRESSION_TRANSTYPE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(b);
			expr->genre_valeur = GenreValeur::DROITE;
			expr->type = resoud_type_final(contexte, b->type_declare, b->bloc_parent, b->lexeme);

			/* À FAIRE : vérifie compatibilité */

			if (b->type == nullptr) {
				erreur::lance_erreur(
							"Ne peut transtyper vers un type invalide",
							contexte,
							expr->lexeme,
							erreur::type_erreur::TYPE_INCONNU);
			}

			donnees_dependance.types_utilises.insere(b->type);

			auto enfant = expr->expr;
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
			auto expr = b;
			expr->genre_valeur = GenreValeur::DROITE;
			expr->type = contexte.typeuse[TypeBase::N32];

			auto type_declare = std::any_cast<DonneesTypeDeclare>(b->valeur_calculee);
			b->valeur_calculee = resoud_type_final(contexte, type_declare, b->bloc_parent, b->lexeme);

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
				}
				else if (type_fin->genre == GenreType::ENTIER_CONSTANT && est_type_entier(type_debut)) {
					type_fin = type_debut;
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

			auto chaine_var = inst->expr == nullptr ? dls::vue_chaine_compacte{""} : inst->expr->ident->nom;

			auto label_goto = (b->lexeme->genre == GenreLexeme::CONTINUE)
					? contexte.goto_continue(chaine_var)
					: contexte.goto_arrete(chaine_var);

			if (label_goto.est_vide()) {
				if (chaine_var.est_vide()) {
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

			/* À FAIRE : ceci duplique logique coulisse */
			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b);

			contexte.empile_goto_continue("", goto_continue);
			contexte.empile_goto_arrete("", goto_apres);

			performe_validation_semantique(inst->bloc, contexte, true);

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			break;
		}
		case GenreNoeud::INSTRUCTION_REPETE:
		{
			auto inst = static_cast<NoeudBoucle *>(b);

			/* À FAIRE : ceci duplique logique coulisse */
			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b);

			contexte.empile_goto_continue("", goto_continue);
			contexte.empile_goto_arrete("", goto_apres);

			performe_validation_semantique(inst->bloc, contexte, true);
			performe_validation_semantique(inst->condition, contexte, false);

			if (inst->condition->type == nullptr && !est_operateur_bool(inst->condition->lexeme->genre)) {
				erreur::lance_erreur("Attendu un opérateur booléen pour la condition", contexte, inst->condition->lexeme);
			}

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

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

			/* À FAIRE : ceci duplique logique coulisse */
			auto goto_continue = "__continue_boucle_pour" + dls::vers_chaine(b);
			auto goto_apres = "__boucle_pour_post" + dls::vers_chaine(b);

			contexte.empile_goto_continue("", goto_continue);
			contexte.empile_goto_arrete("", goto_apres);

			performe_validation_semantique(inst->bloc, contexte, true);

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			/* À FAIRE : tests */
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

			dls::tableau<NoeudExpression *> feuilles;
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
			}

			for (auto f : feuilles) {
				/* À FAIRE : test */
				if (f->type != type_feuille && f->type->genre != GenreType::ENTIER_CONSTANT) {
					erreur::lance_erreur_assignation_type_differents(
								f->type,
								type_feuille,
								contexte,
								f->lexeme);
				}
			}

			b->type = contexte.typeuse.type_tableau_fixe(type_feuille, feuilles.taille());

			/* ajoute également le type de pointeur pour la génération de code C */
			auto type_ptr = contexte.typeuse.type_pointeur_pour(type_feuille);

			donnees_dependance.types_utilises.insere(b->type);
			donnees_dependance.types_utilises.insere(type_ptr);
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		{
			auto expr = static_cast<NoeudExpressionAppel *>(b);
			expr->genre_valeur = GenreValeur::DROITE;

			auto fichier = contexte.fichier(static_cast<size_t>(expr->lexeme->fichier));
			auto decl = trouve_dans_bloc_ou_module(contexte, expr->bloc_parent, expr->ident, fichier);

			if (decl == nullptr) {
				erreur::lance_erreur(
							"Structure inconnue",
							contexte,
							b->lexeme,
							erreur::type_erreur::STRUCTURE_INCONNUE);
			}

			if (decl->genre != GenreNoeud::DECLARATION_STRUCTURE) {
				erreur::lance_erreur(
							"Le symbole n'est pas une structure",
							contexte,
							b->lexeme,
							erreur::type_erreur::STRUCTURE_INCONNUE);
			}

			expr->type = decl->type;

			auto decl_struct = static_cast<NoeudStruct *>(decl);

			auto noms_rencontres = dls::ensemble<dls::vue_chaine_compacte>();

			if (decl_struct->est_union) {
				if (expr->params.taille > 1) {
					erreur::lance_erreur(
								"On ne peut initialiser qu'un seul membre d'une union à la fois",
								contexte,
								b->lexeme);
				}
				else if (expr->params.taille == 0) {
					erreur::lance_erreur(
								"On doit initialiser au moins un membre de l'union",
								contexte,
								b->lexeme);
				}
			}

			/* À FAIRE : il nous faut les noms -> il nous faut les assignations
			 * dans l'arbre */
			for (auto param : expr->params) {
				if (param->genre != GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE) {
					erreur::lance_erreur(
								"expression invalide dans la construction de structure, requiers « nom = expr »",
								contexte,
								param->lexeme);
				}

				auto expr_assign = static_cast<NoeudExpressionBinaire *>(param);

				performe_validation_semantique(expr_assign->expr2, contexte, false);

				auto nom = expr_assign->expr1->lexeme->chaine;

				if (noms_rencontres.trouve(nom) != noms_rencontres.fin()) {
					erreur::lance_erreur(
								"Redéfinition de l'initialisation du membre",
								contexte,
								b->lexeme);
				}

				noms_rencontres.insere(nom);
			}

			break;
		}
		case GenreNoeud::EXPRESSION_INFO_DE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(b);

			performe_validation_semantique(expr->expr, contexte, false);

			if (expr->expr->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
				auto decl = trouve_dans_bloc(expr->bloc_parent, expr->expr->ident);
				expr->expr->type = decl->type;
			}

			auto type = expr->expr->type;

			auto nom_struct = "InfoType";

			switch (type->genre) {
				case GenreType::INVALIDE:
				{
					break;
				}
				case GenreType::EINI:
				case GenreType::CHAINE:
				case GenreType::RIEN:
				case GenreType::BOOL:
				case GenreType::OCTET:
				{
					nom_struct = "InfoType";
					break;
				}
				case GenreType::ENTIER_CONSTANT:
				case GenreType::ENTIER_NATUREL:
				case GenreType::ENTIER_RELATIF:
				{
					nom_struct = "InfoTypeEntier";
					break;
				}
				case GenreType::REEL:
				{
					nom_struct = "InfoTypeRéel";
					break;
				}
				case GenreType::REFERENCE:
				case GenreType::POINTEUR:
				{
					nom_struct = "InfoTypePointeur";
					break;
				}
				case GenreType::UNION:
				case GenreType::STRUCTURE:
				{
					nom_struct = "InfoTypeStructure";
					break;
				}
				case GenreType::VARIADIQUE:
				case GenreType::TABLEAU_DYNAMIQUE:
				case GenreType::TABLEAU_FIXE:
				{
					nom_struct = "InfoTypeTableau";
					break;
				}
				case GenreType::FONCTION:
				{
					nom_struct = "InfoTypeFonction";
					break;
				}
				case GenreType::ENUM:
				{
					nom_struct = "InfoTypeÉnum";
					break;
				}
			}

			b->genre_valeur = GenreValeur::DROITE;
			b->type = contexte.typeuse.type_pointeur_pour(contexte.typeuse.type_pour_nom(nom_struct));

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
			expr_loge->type = resoud_type_final(contexte, b->type_declare,  b->bloc_parent, b->lexeme, false);

			if (expr_loge->type->genre == GenreType::TABLEAU_DYNAMIQUE) {
				auto expr = b->type_declare.expressions[0];
				performe_validation_semantique(expr, contexte, false);

				auto idx_type_deref = contexte.typeuse.type_dereference_pour(b->type);

				// pour la coulisse C, ajout d'une dépendance vers le type du pointeur du tableau
				auto idx_type_pointeur = contexte.typeuse.type_pointeur_pour(idx_type_deref);
				donnees_dependance.types_utilises.insere(idx_type_pointeur);
			}
			else if (expr_loge->type->genre == GenreType::CHAINE) {
				performe_validation_semantique(expr_loge->expr_chaine, contexte, false);
			}
			else {
				expr_loge->type = contexte.typeuse.type_pointeur_pour(expr_loge->type);
			}

			if (expr_loge->bloc != nullptr) {
				performe_validation_semantique(expr_loge->bloc, contexte, true);
			}

			donnees_dependance.types_utilises.insere(expr_loge->type);

			break;
		}
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			auto expr_loge = static_cast<NoeudExpressionLogement *>(b);
			expr_loge->type = resoud_type_final(contexte, b->type_declare, b->bloc_parent, b->lexeme, false);

			performe_validation_semantique(expr_loge->expr, contexte, true);

			if (expr_loge->type->genre == GenreType::TABLEAU_DYNAMIQUE) {
				auto expr = b->type_declare.expressions[0];
				performe_validation_semantique(expr, contexte, false);

				// pour la coulisse C, ajout d'une dépendance vers le type du pointeur du tableau
				auto idx_type_deref = contexte.typeuse.type_dereference_pour(b->type);
				auto idx_type_pointeur = contexte.typeuse.type_pointeur_pour(idx_type_deref);
				donnees_dependance.types_utilises.insere(idx_type_pointeur);
			}
			else if (expr_loge->type->genre == GenreType::CHAINE) {
				performe_validation_semantique(expr_loge->expr_chaine, contexte, false);
			}
			else {
				expr_loge->type = contexte.typeuse.type_pointeur_pour(expr_loge->type);
			}

			/* pour les références */
			auto transformation = cherche_transformation(expr_loge->expr->type, expr_loge->type);

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

			auto decl_precedente = trouve_dans_bloc(decl->bloc_parent, decl);

			// la bibliothèque C a des symboles qui peuvent être les mêmes pour les fonctions et les structres (p.e. stat)
			if (decl_precedente != nullptr && decl_precedente->genre == decl->genre) {
				erreur::redefinition_symbole(contexte, decl->lexeme, decl_precedente->lexeme);
			}

			auto noeud_dependance = graphe.cree_noeud_type(decl->type);
			noeud_dependance->noeud_syntactique = decl;

			auto type_struct = static_cast<TypeStructure *>(decl->type);

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
						auto type_deref = contexte.typeuse.type_dereference_pour(type_base);

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

			auto ajoute_donnees_membre = [&decalage, &decl, &max_alignement, &donnees_dependance](NoeudBase *enfant)
			{
				auto type_membre = enfant->type;
				auto align_type = type_membre->alignement;
				max_alignement = std::max(align_type, max_alignement);
				auto padding = (align_type - (decalage % align_type)) % align_type;
				decalage += padding;

				donnees_dependance.types_utilises.insere(type_membre);

				auto desc_membre = MembreStructure();
				desc_membre.nom = enfant->ident->nom;
				desc_membre.decalage = decalage;
				desc_membre.type = type_membre;

				decalage += type_membre->taille_octet;

				decl->desc.membres.pousse(desc_membre);
			};

			if (decl->est_union) {
				POUR (decl->bloc->membres) {
					auto decl_var = static_cast<NoeudDeclarationVariable *>(it);
					auto decl_membre = decl_var->valeur;
					decl_membre->type = resoud_type_final(contexte, decl_membre->type_declare, decl_membre->bloc_parent, decl_membre->lexeme);
					decl_var->type = decl_membre->type;

					verifie_redefinition_membre(decl_var);
					verifie_inclusion_valeur(decl_var);

					ajoute_donnees_membre(decl_membre);
				}

				auto taille_union = 0u;

				POUR (decl->bloc->membres) {
					auto type_membre = it->type;
					auto taille = type_membre->taille_octet;

					taille_union = std::max(taille_union, taille);
				}

				/* Pour les unions sûres, il nous faut prendre en compte le
				 * membre supplémentaire. */
				if (!decl->est_nonsure) {
					/* ajoute une marge d'alignement */
					auto padding = (max_alignement - (taille_union % max_alignement)) % max_alignement;
					taille_union += padding;

					/* ajoute la taille du membre actif */
					taille_union += static_cast<unsigned>(sizeof(int));

					/* ajoute une marge d'alignement finale */
					padding = (max_alignement - (taille_union % max_alignement)) % max_alignement;
					taille_union += padding;
				}

				decl->type->taille_octet = taille_union;

				graphe.ajoute_dependances(*noeud_dependance, donnees_dependance);
				return;
			}

			POUR (decl->bloc->membres) {
				auto decl_var = static_cast<NoeudDeclarationVariable *>(it);
				auto decl_membre = decl_var->valeur;
				auto decl_expr = decl_var->expression;

				it->ident = decl_membre->ident;

				decl_membre->type = resoud_type_final(contexte, decl_membre->type_declare, decl_membre->bloc_parent, decl_membre->lexeme);

				verifie_redefinition_membre(decl_var);

				if (decl_expr != nullptr) {
					performe_validation_semantique(decl_expr, contexte, false);

					if (decl_membre->type != decl_expr->type) {
						if (decl_membre->type == nullptr) {
							if (decl_expr->type->genre == GenreType::ENTIER_CONSTANT) {
								decl_membre->type = contexte.typeuse[TypeBase::Z32];
							}
							else {
								decl_membre->type = decl_expr->type;
							}
						}
						else {
							auto transformation = cherche_transformation(
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

				it->type = decl_membre->type;

				verifie_inclusion_valeur(decl_membre);

				// À FAIRE : emploi de structures
				// - validation que nous avons une structure
				// - prend note de la hierarchie, notamment pour valider les transtypages
				// - préserve l'emploi dans les données types
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

					for (auto it_empl : decl_struct_empl->bloc->membres) {
						ajoute_donnees_membre(it_empl);
					}
				}
				else {
					ajoute_donnees_membre(decl_membre);
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
			auto &desc = decl->desc;

			auto type_enum = static_cast<TypeEnum *>(decl->type);
			type_enum->type_donnees = resoud_type_final(contexte, decl->type_declare, decl->bloc_parent, decl->lexeme);
			type_enum->taille_octet = type_enum->type_donnees->taille_octet;
			type_enum->alignement = type_enum->type_donnees->alignement;

			contexte.operateurs.ajoute_operateur_basique_enum(decl->type);

			/* À FAIRE : tests */

			auto noms_presents = dls::ensemble<dls::vue_chaine_compacte>();

			auto dernier_res = ResultatExpression();
			/* utilise est_errone pour indiquer que nous sommes à la première valeur */
			dernier_res.est_errone = true;

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
				auto expr = decl_expr->expression;

				auto decl_anterieure = trouve_dans_bloc_seul(decl->bloc, decl_expr);

				if (decl_anterieure != nullptr) {
					erreur::lance_erreur(
								"Rédéfinition de la valeur de l'énum",
								contexte,
								var->lexeme,
								erreur::type_erreur::VARIABLE_REDEFINIE);
				}

				decl->bloc->membres.pousse(static_cast<NoeudDeclaration *>(it));

				desc.noms.pousse(kuri::chaine(var->ident->nom));

				auto res = ResultatExpression();

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

						if (desc.est_drapeau) {
							res.type = type_expression::ENTIER;
							res.entier = 1;
						}
					}
					else {
						if (dernier_res.type == type_expression::ENTIER) {
							if (desc.est_drapeau) {
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

				desc.valeurs.pousse(static_cast<int>(res.entier));

				dernier_res = res;
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_DISCR:
		{
			/* TESTS : si énum -> vérifie que toutes les valeurs soient prises
			 * en compte, sauf s'il y a un bloc sinon après. */

			auto inst = static_cast<NoeudDiscr *>(b);

			auto expression = inst->expr;

			performe_validation_semantique(expression, contexte, false);
			auto type = expression->type;

			if (type->genre == GenreType::REFERENCE) {
				type = static_cast<TypeReference *>(type)->type_pointe;
				b->transformation = TypeTransformation::DEREFERENCE;
			}

			if (type->genre == GenreType::UNION) {
				auto decl = static_cast<TypeStructure *>(type)->decl;

				if (decl->est_nonsure) {
					erreur::lance_erreur(
								"« discr » ne peut prendre une union nonsûre",
								contexte,
								expression->lexeme);
				}

				auto membres_rencontres = dls::ensemble<IdentifiantCode *>();

				auto valide_presence_membres = [&membres_rencontres, &decl, &contexte, &expression]() {
					auto valeurs_manquantes = dls::ensemble<dls::vue_chaine_compacte>();

					// À FAIRE : réusinage arbre, ceci ne semble pas correcte
					POUR (decl->bloc->membres) {
						if (membres_rencontres.trouve(it->ident) == membres_rencontres.fin()) {
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

					if (membres_rencontres.trouve(expr_paire->ident) != membres_rencontres.fin()) {
						erreur::lance_erreur(
									"Redéfinition de l'expression",
									contexte,
									expr_paire->lexeme);
					}

					membres_rencontres.insere(expr_paire->ident);

					auto decl_var = trouve_dans_bloc_seul(decl->bloc, expr_paire);

					if (decl_var == nullptr) {
						erreur::membre_inconnu(contexte, decl->bloc, b, expression, expr_paire, type);
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
					// À FAIRE : est_dynamique

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
			else if (type->genre == GenreType::ENUM) {
				auto decl = static_cast<TypeEnum *>(type)->decl;

				auto membres_rencontres = dls::ensemble<dls::vue_chaine_compacte>();
				b->genre = GenreNoeud::INSTRUCTION_DISCR_ENUM;

				for (int i = 0; i < inst->paires_discr.taille; ++i) {
					auto expr_paire = inst->paires_discr[i].first;
					auto bloc_paire = inst->paires_discr[i].second;

					auto feuilles = dls::tableau<NoeudExpression *>();
					rassemble_feuilles(expr_paire, feuilles);

					for (auto f : feuilles) {
						auto nom_membre = f->ident->nom;

						auto nom_trouve = false;

						POUR (decl->desc.noms) {
							if (dls::vue_chaine_compacte(it.pointeur, it.taille) == nom_membre) {
								nom_trouve = true;
								break;
							}
						}

						if (!nom_trouve) {
							erreur::membre_inconnu(contexte, decl->bloc, b, expression, expr_paire, type);
						}

						if (membres_rencontres.trouve(nom_membre) != membres_rencontres.fin()) {
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

					POUR (decl->desc.noms) {
						if (membres_rencontres.trouve({ it.pointeur, it.taille }) == membres_rencontres.fin()) {
							valeurs_manquantes.insere({ it.pointeur, it.taille });
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
					donnees_dependance.fonctions_utilisees.insere(inst->op->nom_fonction);
				}

				for (int i = 0; i < inst->paires_discr.taille; ++i) {
					auto expr_paire = inst->paires_discr[i].first;
					auto bloc_paire = inst->paires_discr[i].second;

					performe_validation_semantique(bloc_paire, contexte, true);

					auto feuilles = dls::tableau<NoeudExpression *>();
					rassemble_feuilles(expr_paire, feuilles);

					for (auto f : feuilles) {
						performe_validation_semantique(f, contexte, true);

						if (f->type != expression->type && f->type->genre != GenreType::ENTIER_CONSTANT) {
							erreur::lance_erreur_type_arguments(
										expression->type,
									f->type,
									contexte,
									f->lexeme,
									expression->lexeme);
						}
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
			auto transformation = cherche_transformation(inst->type, type_retour);

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
			performe_validation_semantique(expr->expr, contexte, expr_gauche);
			expr->type = expr->expr->type;
			break;
		}
	}
}

void performe_validation_semantique(ContexteGenerationCode &contexte)
{
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

	POUR (contexte.file_typage) {
		performe_validation_semantique(it, contexte, true);
	}

	contexte.temps_validation = debut_validation.temps();
}

}  /* namespace noeud */
