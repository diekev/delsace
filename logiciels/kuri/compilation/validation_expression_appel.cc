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

#include "validation_expression_appel.hh"

#include "biblinternes/outils/conditions.h"

#include "assembleuse_arbre.h"
#include "arbre_syntactic.h"
#include "erreur.h"
#include "modules.hh"
#include "validation_semantique.hh"

static auto trouve_declarations_dans_bloc(
		dls::tableau<NoeudDeclaration *> &declarations,
		NoeudBloc *bloc,
		IdentifiantCode *ident)
{
	auto bloc_courant = bloc;

	while (bloc_courant != nullptr) {
		POUR (bloc_courant->membres) {
			if (it->ident == ident) {
				declarations.pousse(it);
			}
		}

		bloc_courant = bloc_courant->parent;
	}
}

static auto trouve_declarations_dans_bloc_ou_module(
		ContexteGenerationCode const &contexte,
		dls::tableau<NoeudDeclaration *> &declarations,
		NoeudBloc *bloc,
		IdentifiantCode *ident,
		Fichier *fichier)
{
	trouve_declarations_dans_bloc(declarations, bloc, ident);

	/* cherche dans les modules importés */
	dls::pour_chaque_element(fichier->modules_importes, [&](auto& nom_module)
	{
		auto module = contexte.module(nom_module);
		trouve_declarations_dans_bloc(declarations, module->bloc, ident);
		return dls::DecisionIteration::Continue;
	});
}

enum {
	CANDIDATE_EST_DECLARATION,
	CANDIDATE_EST_ACCES,
	CANDIDATE_EST_APPEL_UNIFORME
};

struct CandidateExpressionAppel {
	int quoi = 0;
	NoeudExpression *decl = nullptr;
};

static auto trouve_candidates_pour_fonction_appelee(
		ContexteGenerationCode &contexte,
		NoeudExpression *appelee)
{
	auto candidates = dls::tableau<CandidateExpressionAppel>();

	auto fichier = contexte.fichier(static_cast<size_t>(appelee->lexeme->fichier));

	if (appelee->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
		auto declarations = dls::tableau<NoeudDeclaration *>();
		trouve_declarations_dans_bloc_ou_module(contexte, declarations, appelee->bloc_parent, appelee->ident, fichier);

		POUR (declarations) {
			// À FAIRE : on peut avoir des expressions du genre invere := inverse(matrice),
			// auquel cas il faut exclure la déclaration de la variable, mais le test si
			//dessous exclus également les variables globales
			if (it->genre == GenreNoeud::DECLARATION_VARIABLE) {
				if (it->lexeme->fichier == appelee->lexeme->fichier && it->lexeme->ligne >= appelee->lexeme->ligne) {
					continue;
				}
			}

			candidates.pousse({ CANDIDATE_EST_DECLARATION, it });
		}
	}
	else if (appelee->genre == GenreNoeud::EXPRESSION_REFERENCE_MEMBRE) {
		auto acces = static_cast<NoeudExpressionBinaire *>(appelee);

		auto accede = acces->expr1;
		auto membre = acces->expr2;

		if (accede->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION && fichier->importe_module(accede->ident->nom)) {
			auto module = contexte.module(accede->ident->nom);
			auto declarations = dls::tableau<NoeudDeclaration *>();
			trouve_declarations_dans_bloc(declarations, module->bloc, membre->ident);

			POUR (declarations) {
				candidates.pousse({ CANDIDATE_EST_DECLARATION, it });
			}
		}
		else {
			noeud::performe_validation_semantique(accede, contexte, false);

			auto type_accede = accede->type;

			while (type_accede->genre == GenreType::POINTEUR || type_accede->genre == GenreType::REFERENCE) {
				type_accede = contexte.typeuse.type_dereference_pour(type_accede);
			}

			if (type_accede->genre == GenreType::STRUCTURE) {
				auto type_struct = static_cast<TypeStructure *>(type_accede);
				auto membre_trouve = false;

				POUR (type_struct->membres) {
					if (it.nom == membre->ident->nom) {
						acces->type = it.type;
						membre_trouve = true;
						break;
					}
				}

				if (membre_trouve != false) {
					candidates.pousse({ CANDIDATE_EST_ACCES, acces });
					return candidates;
				}
			}

			candidates.pousse({ CANDIDATE_EST_APPEL_UNIFORME, acces });
		}
	}

	return candidates;
}

static double verifie_compatibilite(
		Type *type_arg,
		Type *type_enf,
		NoeudBase *enfant,
		TransformationType &transformation)
{
	transformation = cherche_transformation(type_enf, type_arg);

	if (transformation.type == TypeTransformation::INUTILE) {
		return 1.0;
	}

	if (transformation.type == TypeTransformation::IMPOSSIBLE) {
		return 0.0;
	}

	if (transformation.type == TypeTransformation::PREND_REFERENCE) {
		return est_valeur_gauche(enfant->genre_valeur) ? 1.0 : 0.0;
	}

	/* nous savons que nous devons transformer la valeur (par ex. eini), donc
	 * donne un mi-poids à l'argument */
	return 0.5;
}

static auto apparie_appel_pointeur(
		NoeudExpressionAppel const *b,
		Type *type,
		ContexteGenerationCode &contexte,
		kuri::tableau<IdentifiantEtExpression> const &args)
{
	auto resultat = DonneesCandidate{};

	if (type->genre != GenreType::FONCTION) {
		resultat.etat = FONCTION_INTROUVEE;
		resultat.poids_args = 0.0;
		resultat.raison = TYPE_N_EST_PAS_FONCTION;
		return resultat;
	}

	POUR (args) {
		if (it.ident == nullptr) {
			continue;
		}

		resultat.etat = FONCTION_INTROUVEE;
		resultat.raison = NOMMAGE_ARG_POINTEUR_FONCTION;
		resultat.poids_args = 0.0;
		resultat.noeud_decl = it.expr;
		return resultat;
	}

	/* vérifie la compatibilité des arguments pour déterminer
	 * s'il y aura besoin d'une transformation. */
	auto type_fonction = static_cast<TypeFonction *>(type);

	auto debut_params = 0l;

	if (type_fonction->types_entrees.taille != 0 && type_fonction->types_entrees[0] == contexte.type_contexte) {
		debut_params = 1;

		auto fonc_courante = contexte.donnees_fonction;

		if (fonc_courante != nullptr && dls::outils::possede_drapeau(fonc_courante->drapeaux, FORCE_NULCTX)) {
			erreur::lance_erreur_fonction_nulctx(contexte, b, b, fonc_courante);
		}
	}
	else {
		resultat.requiers_contexte = false;
	}

	auto exprs = dls::tablet<NoeudExpression *, 10>();
	exprs.reserve(type_fonction->types_entrees.taille - debut_params);

	auto transformations = dls::tableau<TransformationType>(type_fonction->types_entrees.taille - debut_params);

	auto poids_args = 1.0;

	/* Validation des types passés en paramètre. */
	for (auto i = debut_params; i < type_fonction->types_entrees.taille; ++i) {
		auto arg = args[i - debut_params].expr;
		auto type_prm = type_fonction->types_entrees[i];
		auto type_enf = arg->type;

		if (type_prm->genre == GenreType::VARIADIQUE) {
			type_prm = contexte.typeuse.type_dereference_pour(type_prm);
		}

		auto transformation = TransformationType();
		auto poids_pour_enfant = verifie_compatibilite(type_prm, type_enf, arg, transformation);

		poids_args *= poids_pour_enfant;

		if (poids_args == 0.0) {
			poids_args = 0.0;
			resultat.raison = METYPAGE_ARG;
			resultat.type1 = contexte.typeuse.type_dereference_pour(type_prm);
			resultat.type2 = type_enf;
			resultat.noeud_decl = arg;
			break;
		}

		transformations[i - debut_params] = transformation;

		exprs.pousse(arg);
	}

	resultat.note = CANDIDATE_EST_APPEL_POINTEUR;
	resultat.type = type_fonction;
	resultat.etat = FONCTION_TROUVEE;
	resultat.poids_args = poids_args;
	resultat.exprs = exprs;
	resultat.transformations = transformations;

	return resultat;
}

/* ************************************************************************** */

static Type *apparie_type_gabarit(Type *type, DonneesTypeDeclare const &type_declare)
{
	auto type_courant = type;

	for (auto i = 0; i < type_declare.taille(); ++i) {
		auto lexeme = type_declare[i];

		if (lexeme == GenreLexeme::DOLLAR) {
			break;
		}

		if (lexeme == GenreLexeme::POINTEUR) {
			if (type_courant->genre != GenreType::POINTEUR) {
				return nullptr;
			}

			type_courant = static_cast<TypePointeur *>(type_courant)->type_pointe;
		}
		else if (lexeme == GenreLexeme::REFERENCE) {
			if (type_courant->genre != GenreType::REFERENCE) {
				return nullptr;
			}

			type_courant = static_cast<TypeReference *>(type_courant)->type_pointe;
		}
		else if (lexeme == GenreLexeme::TABLEAU) {
			if (type_courant->genre != GenreType::TABLEAU_DYNAMIQUE) {
				return nullptr;
			}

			type_courant = static_cast<TypeTableauDynamique *>(type_courant)->type_pointe;
		}
		// À FAIRE : type tableau fixe
		else {
			return nullptr;
		}
	}

	return type_courant;
}

/* ************************************************************************** */

static DonneesCandidate apparie_appel_fonction(
		ContexteGenerationCode &contexte,
		NoeudDeclarationFonction const *decl,
		kuri::tableau<IdentifiantEtExpression> const &args)
{
	auto res = DonneesCandidate{};
	res.note = CANDIDATE_EST_APPEL_FONCTION;

	auto const nombre_args = decl->params.taille;

	if (!decl->est_variadique && (args.taille > nombre_args)) {
		res.etat = FONCTION_INTROUVEE;
		res.raison = MECOMPTAGE_ARGS;
		res.decl_fonc = decl;
		return res;
	}

	if (nombre_args == 0 && args.taille == 0) {
		res.poids_args = 1.0;
		res.etat = FONCTION_TROUVEE;
		res.raison = AUCUNE_RAISON;
		res.decl_fonc = decl;
		return res;
	}

	dls::tablet<NoeudExpression *, 10> slots;
	slots.redimensionne(nombre_args - decl->est_variadique);

	for (auto i = 0; i < slots.taille(); ++i) {
		auto param = static_cast<NoeudDeclarationVariable *>(decl->params[i]);
		slots[i] = param->expression;
	}

	auto index = 0l;
	auto arguments_nommes = false;
	auto dernier_arg_variadique = false;
	dls::ensemble<IdentifiantCode *> args_rencontres;

	POUR (args) {
		if (it.ident != nullptr) {
			arguments_nommes = true;

			auto param = static_cast<NoeudDeclarationVariable *>(nullptr);
			auto index_param = 0l;

			for (auto i = 0; i < decl->params.taille; ++i) {
				auto dp = decl->params[i];

				if (dp->ident == it.ident) {
					param = static_cast<NoeudDeclarationVariable *>(dp);
					index_param = i;
					break;
				}
			}

			if (param == nullptr) {
				res.etat = FONCTION_INTROUVEE;
				res.raison = MENOMMAGE_ARG;
				res.nom_arg = it.ident->nom;
				res.decl_fonc = decl;
				return res;
			}

			if ((args_rencontres.trouve(it.ident) != args_rencontres.fin()) && (param->drapeaux & EST_VARIADIQUE) == 0) {
				res.etat = FONCTION_INTROUVEE;
				res.raison = RENOMMAGE_ARG;
				res.nom_arg = it.ident->nom;
				res.decl_fonc = decl;
				return res;
			}

			dernier_arg_variadique = (param->drapeaux & EST_VARIADIQUE) != 0;

			args_rencontres.insere(it.ident);

			if (dernier_arg_variadique || index_param >= slots.taille()) {
				slots.pousse(it.expr);
			}
			else {
				if (slots[index_param] != param->expression) {
					res.etat = FONCTION_INTROUVEE;
					res.raison = RENOMMAGE_ARG;
					res.nom_arg = it.ident->nom;
					res.decl_fonc = decl;
					return res;
				}

				slots[index_param] = it.expr;
			}
		}
		else {
			if (arguments_nommes == true && dernier_arg_variadique == false) {
				res.etat = FONCTION_INTROUVEE;
				res.raison = MANQUE_NOM_APRES_VARIADIC;
				res.decl_fonc = decl;
				return res;
			}

			if (dernier_arg_variadique || index >= slots.taille()) {
				slots.pousse(it.expr);
				index++;
			}
			else {
				slots[index++] = it.expr;
			}
		}
	}

	for (auto slot : slots) {
		if (slot == nullptr) {
			// À FAIRE : on pourrait donner les noms des arguments manquants
			res.etat = FONCTION_INTROUVEE;
			res.raison = MECOMPTAGE_ARGS;
			res.decl_fonc = decl;
			return res;
		}
	}

	auto paires_expansion_gabarit = dls::tableau<std::pair<dls::vue_chaine_compacte, Type *>>();

	auto poids_args = 1.0;
	auto fonction_variadique_interne = decl->est_variadique && !decl->est_externe;
	auto expansion_rencontree = false;

	auto transformations = dls::tableau<TransformationType>(slots.taille());

	auto nombre_arg_variadiques_rencontres = 0;

	for (auto i = 0l; i < slots.taille(); ++i) {
		auto index_arg = std::min(i, decl->params.taille - 1);
		auto param = static_cast<NoeudDeclarationVariable *>(decl->params[index_arg]);
		auto arg = param->valeur;
		auto slot = slots[i];

		if (slot == param->expression) {
			continue;
		}

		auto type_enf = slot->type;
		auto type_arg = arg->type;

		if (arg->type_declare.est_gabarit) {
			// trouve l'argument
			auto type_trouve = false;
			auto type_errone = false;
			for (auto &paire : paires_expansion_gabarit) {
				if (paire.first == arg->type_declare.nom_gabarit) {
					type_trouve = true;
					break;
				}
			}

			if (type_errone) {
				break;
			}

			if (!type_trouve) {
				auto type_gabarit = apparie_type_gabarit(type_enf, arg->type_declare);

				if (type_gabarit == nullptr) {
					poids_args = 0.0;
					res.raison = METYPAGE_ARG;
					//res.type1 = type_enf;
					res.type2 = type_enf;
					res.noeud_decl = slot;
					res.decl_fonc = decl;
					return res;
				}

				paires_expansion_gabarit.pousse({ arg->type_declare.nom_gabarit, type_gabarit });
			}

			type_arg = type_enf;
		}

		if ((param->drapeaux & EST_VARIADIQUE) != 0) {
			if (contexte.typeuse.type_dereference_pour(type_arg) != nullptr) {
				auto transformation = TransformationType();
				auto type_deref = contexte.typeuse.type_dereference_pour(type_arg);
				auto poids_pour_enfant = 0.0;

				if (slot->genre == GenreNoeud::EXPANSION_VARIADIQUE) {
					if (!fonction_variadique_interne) {
						erreur::lance_erreur("Impossible d'utiliser une expansion variadique dans une fonction variadique externe", contexte, slot->lexeme);
					}

					if (expansion_rencontree) {
						erreur::lance_erreur("Ne peut utiliser qu'une seule expansion d'argument variadique", contexte, slot->lexeme);
					}

					auto type_deref_enf = contexte.typeuse.type_dereference_pour(type_enf);

					poids_pour_enfant = verifie_compatibilite(type_deref, type_deref_enf, slot, transformation);

					// aucune transformation acceptée sauf si nous avons un tableau fixe qu'il faudra convertir en un tableau dynamique
					if (poids_pour_enfant != 1.0) {
						poids_pour_enfant = 0.0;
					}
					else {
						if (type_enf->genre == GenreType::TABLEAU_FIXE) {
							transformation = TypeTransformation::CONVERTI_TABLEAU;
						}
					}

					expansion_rencontree = true;
				}
				else {
					poids_pour_enfant = verifie_compatibilite(type_deref, type_enf, slot, transformation);
				}

				// À FAIRE: trouve une manière de trouver les fonctions gabarits déjà instantiées
				if (arg->type_declare.est_gabarit) {
					poids_pour_enfant *= 0.95;
				}

				poids_args *= poids_pour_enfant;

				if (poids_args == 0.0) {
					poids_args = 0.0;
					res.raison = METYPAGE_ARG;
					res.type1 = contexte.typeuse.type_dereference_pour(type_arg);
					res.type2 = type_enf;
					res.noeud_decl = slot;
					break;
				}

				if (fonction_variadique_interne) {
					if (expansion_rencontree && nombre_arg_variadiques_rencontres != 0) {
						if (slot->genre == GenreNoeud::EXPANSION_VARIADIQUE) {
							erreur::lance_erreur("Tentative d'utiliser une expansion d'arguments variadiques alors que d'autres arguments ont déjà été précisés", contexte, slot->lexeme);
						}
						else {
							erreur::lance_erreur("Tentative d'ajouter des arguments variadiques supplémentaire alors qu'une expansion est également utilisée", contexte, slot->lexeme);
						}
					}
				}

				transformations[i] = transformation;
			}
			else {
				if (slot->genre == GenreNoeud::EXPANSION_VARIADIQUE) {
					if (!fonction_variadique_interne) {
						erreur::lance_erreur("Impossible d'utiliser une expansion variadique dans une fonction variadique externe", contexte, slot->lexeme);
					}
				}

				transformations[i] = TransformationType();
			}

			nombre_arg_variadiques_rencontres += 1;
		}
		else {
			auto transformation = TransformationType();
			auto poids_pour_enfant = verifie_compatibilite(type_arg, type_enf, slot, transformation);

			// À FAIRE: trouve une manière de trouver les fonctions gabarits déjà instantiées
			if (arg->type_declare.est_gabarit) {
				poids_pour_enfant *= 0.95;
			}

			poids_args *= poids_pour_enfant;

			if (poids_args == 0.0) {
				poids_args = 0.0;
				res.raison = METYPAGE_ARG;
				res.type1 = type_arg;
				res.type2 = type_enf;
				res.noeud_decl = slot;
				break;
			}

			transformations[i] = transformation;
		}
	}

	if (fonction_variadique_interne) {
		auto index_premier_var_arg = nombre_args - 1;

		if (slots.taille() != nombre_args || slots[index_premier_var_arg]->genre != GenreNoeud::EXPANSION_VARIADIQUE) {
			/* Pour les fonctions variadiques interne, nous créons un tableau
			 * correspondant au types des arguments. */
			auto noeud_tableau = static_cast<NoeudTableauArgsVariadiques *>(contexte.assembleuse->cree_noeud(
						GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES, args[0].expr->lexeme));
			noeud_tableau->drapeaux |= EST_CALCULE;

			auto type_var = decl->params[decl->params.taille - 1]->type;
			noeud_tableau->type = contexte.typeuse.type_dereference_pour(type_var);
			noeud_tableau->exprs.reserve(slots.taille() - index_premier_var_arg);

			for (auto i = index_premier_var_arg; i < slots.taille(); ++i) {
				noeud_tableau->exprs.pousse(slots[i]);
			}

			if (index_premier_var_arg >= slots.taille()) {
				slots.pousse(noeud_tableau);
			}
			else {
				slots[index_premier_var_arg] = noeud_tableau;
			}

			slots.redimensionne(nombre_args);
		}
	}

	res.decl_fonc = decl;
	res.poids_args = poids_args;
	res.exprs = slots;
	res.etat = FONCTION_TROUVEE;
	res.transformations = transformations;
	res.paires_expansion_gabarit = paires_expansion_gabarit;

	return res;
}

/* ************************************************************************** */

static auto apparie_appel_structure(
		NoeudExpressionAppel const *expr,
		NoeudStruct *decl_struct,
		kuri::tableau<IdentifiantEtExpression> const &arguments)
{
	auto resultat = DonneesCandidate{};
	auto type_struct = static_cast<TypeStructure *>(decl_struct->type);

	if (decl_struct->est_union) {
		if (expr->params.taille > 1) {
			resultat.etat = FONCTION_TROUVEE;
			resultat.raison = TROP_D_EXPRESSION_POUR_UNION;
			resultat.poids_args = 0.0;
			return resultat;
		}

		if (expr->params.taille == 0) {
			resultat.etat = FONCTION_TROUVEE;
			resultat.raison = EXPRESSION_MANQUANTE_POUR_UNION;
			resultat.poids_args = 0.0;
			return resultat;
		}
	}

	auto slots = dls::tablet<NoeudExpression *, 10>();
	slots.redimensionne(type_struct->membres.taille);

	auto index_membre = 0;
	POUR (type_struct->membres) {
		slots[index_membre++] = it.expression_valeur_defaut;
	}

	auto noms_rencontres = dls::ensemble<IdentifiantCode *>();
	auto poids_appariement = 1.0;
	auto transformations = dls::tableau<TransformationType>(slots.taille());

	POUR (arguments) {
		if (it.ident == nullptr) {
			resultat.etat = FONCTION_TROUVEE;
			resultat.raison = NOM_ARGUMENT_REQUIS;
			resultat.poids_args = 0.0;
			resultat.noeud_decl = it.expr;
			return resultat;
		}

		if (noms_rencontres.trouve(it.ident) != noms_rencontres.fin()) {
			resultat.etat = FONCTION_TROUVEE;
			resultat.raison = RENOMMAGE_ARG;
			resultat.poids_args = 0.0;
			resultat.noeud_decl = it.expr;
			return resultat;
		}

		auto type_membre = static_cast<Type *>(nullptr);
		auto decl_membre = static_cast<NoeudDeclaration *>(nullptr);
		index_membre = 0;

		for (auto &membre : decl_struct->bloc->membres) {
			if (membre->ident == it.ident) {
				type_membre = membre->type;
				decl_membre = membre;
				break;
			}

			index_membre += 1;
		}

		noms_rencontres.insere(it.ident);

		if (decl_membre == nullptr) {
			resultat.etat = FONCTION_TROUVEE;
			resultat.raison = MENOMMAGE_ARG;
			resultat.poids_args = 0.0;
			resultat.noeud_decl = it.expr;
			resultat.decl_fonc = decl_struct;
			return resultat;
		}

		auto transformation = TransformationType{};
		auto poids_pour_enfant = verifie_compatibilite(type_membre, it.expr->type, it.expr, transformation);

		poids_appariement *= poids_pour_enfant;

		if (poids_appariement == 0.0) {
			resultat.etat = FONCTION_TROUVEE;
			resultat.raison = METYPAGE_ARG;
			resultat.poids_args = 0.0;
			resultat.noeud_decl = it.expr;
			resultat.type1 = type_membre;
			resultat.type2 = it.expr->type;
			return resultat;
		}

		slots[index_membre] = it.expr;
		transformations[index_membre] = transformation;
	}

	resultat.type = decl_struct->type;
	resultat.note = CANDIDATE_EST_INITIALISATION_STRUCTURE;
	resultat.etat = FONCTION_TROUVEE;
	resultat.raison = AUCUNE_RAISON;
	resultat.poids_args = poids_appariement;
	resultat.exprs = slots;
	resultat.transformations = transformations;

	return resultat;
}

/* ************************************************************************** */

ResultatRecherche trouve_candidates_pour_appel(
		ContexteGenerationCode &contexte,
		NoeudExpressionAppel *expr,
		kuri::tableau<IdentifiantEtExpression> &args)
{
	auto candidates_appel = trouve_candidates_pour_fonction_appelee(contexte, expr->appelee);

	if (candidates_appel.taille() == 0) {
		erreur::lance_erreur("fonction inconnue", contexte, expr->appelee->lexeme);
	}

	auto nouvelles_candidates = dls::tableau<CandidateExpressionAppel>();

	POUR (candidates_appel) {
		if (it.quoi == CANDIDATE_EST_APPEL_UNIFORME) {
			auto acces = static_cast<NoeudExpressionBinaire *>(it.decl);
			auto candidates = trouve_candidates_pour_fonction_appelee(contexte, acces->expr2);

			args.pousse_front({ nullptr, acces->expr1 });

			for (auto c : candidates) {
				nouvelles_candidates.pousse(c);
			}
		}
		else {
			nouvelles_candidates.pousse(it);
		}
	}

	candidates_appel = nouvelles_candidates;

	auto resultat = ResultatRecherche{};

	POUR (candidates_appel) {
		if (it.quoi == CANDIDATE_EST_ACCES) {
			auto dc = apparie_appel_pointeur(expr, it.decl->type, contexte, args);
			resultat.candidates.pousse(dc);
		}
		else if (it.quoi == CANDIDATE_EST_DECLARATION) {
			auto decl = it.decl;

			if (decl->genre == GenreNoeud::DECLARATION_STRUCTURE) {
				auto decl_struct = static_cast<NoeudStruct *>(decl);
				auto dc = apparie_appel_structure(expr, decl_struct, args);
				resultat.candidates.pousse(dc);
			}
			else if (decl->genre == GenreNoeud::DECLARATION_FONCTION) {
				auto decl_fonc = static_cast<NoeudDeclarationFonction *>(decl);
				auto dc = apparie_appel_fonction(contexte, decl_fonc, args);
				resultat.candidates.pousse(dc);
			}
			else if (decl->genre == GenreNoeud::DECLARATION_VARIABLE) {
				auto dc = apparie_appel_pointeur(expr, decl->type, contexte, args);
				resultat.candidates.pousse(dc);
			}
		}
	}

	return resultat;
}

/* ************************************************************************** */

void valide_appel_fonction(
		ContexteGenerationCode &contexte,
		NoeudExpressionAppel *expr,
		bool expr_gauche)
{
	auto fonction_courante = contexte.donnees_fonction;
	auto &donnees_dependance = contexte.donnees_dependance;

	// ------------
	// valide d'abord les expressions, leurs types sont nécessaire pour trouver les candidates

	kuri::tableau<IdentifiantEtExpression> args;
	args.reserve(expr->params.taille);

	/* Commence par valider les enfants puisqu'il nous faudra leurs
	 * types pour déterminer la fonction à appeler. */
	for (auto f : expr->params) {
		// l'argument est nommé
		if (f->genre == GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE) {
			auto assign = static_cast<NoeudExpressionBinaire *>(f);
			auto nom_arg = assign->expr1;
			auto arg = assign->expr2;

			noeud::performe_validation_semantique(arg, contexte, false);

			args.pousse({ nom_arg->ident, arg });
		}
		else {
			noeud::performe_validation_semantique(f, contexte, false);

			args.pousse({ nullptr, f });
		}
	}

	// ------------
	// trouve la fonction, pour savoir ce que l'on a

	auto res = trouve_candidates_pour_appel(contexte, expr, args);
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

	if (candidate == nullptr) {
		erreur::lance_erreur_fonction_inconnue(
					contexte,
					expr,
					res.candidates);
	}

	// ------------
	// copie les données

	expr->exprs.reserve(candidate->exprs.taille());

	for (auto enfant : candidate->exprs) {
		expr->exprs.pousse(enfant);
	}

	if (candidate->note == CANDIDATE_EST_APPEL_FONCTION) {
		if (candidate->decl_fonc == nullptr) {
			erreur::lance_erreur_fonction_inconnue(
						contexte,
						expr,
						res.candidates);
		}

		auto decl_fonction_appelee = static_cast<NoeudDeclarationFonction const *>(candidate->decl_fonc);

		/* pour les directives d'exécution, la fonction courante est nulle */
		if (fonction_courante != nullptr) {
			using dls::outils::possede_drapeau;
			auto decl_fonc = fonction_courante;

			if (possede_drapeau(decl_fonc->drapeaux, FORCE_NULCTX)) {
				auto decl_appel = decl_fonction_appelee;

				if (!decl_appel->est_externe && !possede_drapeau(decl_appel->drapeaux, FORCE_NULCTX)) {
					erreur::lance_erreur_fonction_nulctx(
								contexte,
								expr,
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

			noeud::valide_type_fonction(noeud_decl, contexte);

			noeud::performe_validation_semantique(noeud_decl, contexte, expr_gauche);

			contexte.donnees_fonction = fonction_courante;

			contexte.pour_gabarit = false;
		}

		// nous devons instantier les gabarits (ou avoir leurs types) avant de pouvoir faire ça
		auto type_fonc = static_cast<TypeFonction *>(decl_fonction_appelee->type);
		auto type_sortie = type_fonc->types_sorties[0];

		// À FAIRE : ceci ne prend pas en compte les appels de syntaxe uniforme
		if (type_sortie->genre != GenreType::RIEN && expr_gauche) {
			erreur::lance_erreur(
						"Inutilisation du retour de la fonction",
						contexte,
						expr->lexeme);
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

		if (expr->type == nullptr) {
			/* À FAIRE: multiple type retour */
			expr->type = decl_fonction_appelee->type_fonc->types_sorties[0];
		}

		donnees_dependance.fonctions_utilisees.insere(decl_fonction_appelee->nom_broye);

		for (auto te : decl_fonction_appelee->type_fonc->types_entrees) {
			donnees_dependance.types_utilises.insere(te);
		}

		for (auto ts : decl_fonction_appelee->type_fonc->types_sorties) {
			donnees_dependance.types_utilises.insere(ts);
		}
	}
	else if (candidate->note == CANDIDATE_EST_INITIALISATION_STRUCTURE) {
		expr->genre = GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE;
		expr->type = candidate->type;

		for (auto i = 0; i < expr->exprs.taille; ++i) {
			if (expr->exprs[i] != nullptr) {
				expr->exprs[i]->transformation = candidate->transformations[i];
			}
		}
	}
	else if (candidate->note == CANDIDATE_EST_APPEL_POINTEUR) {
		expr->aide_generation_code = APPEL_POINTEUR_FONCTION;

		if (!candidate->requiers_contexte) {
			expr->drapeaux |= FORCE_NULCTX;
		}

		if (expr->type == nullptr) {
			/* À FAIRE: multiple type retour */
			expr->type = static_cast<TypeFonction *>(candidate->type)->types_sorties[0];
		}

		for (auto i = 0; i < expr->exprs.taille; ++i) {
			expr->exprs[i]->transformation = candidate->transformations[i];
		}
	}

	assert(expr->type);
}
