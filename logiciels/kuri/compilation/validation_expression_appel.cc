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
#include "arbre_syntaxique.hh"
#include "erreur.h"
#include "modules.hh"
#include "portee.hh"
#include "profilage.hh"
#include "validation_semantique.hh"

enum {
	CANDIDATE_EST_DECLARATION,
	CANDIDATE_EST_ACCES,
	CANDIDATE_EST_APPEL_UNIFORME,
	CANDIDATE_EST_INIT_DE,
	CANDIDATE_EST_EXPRESSION_QUELCONQUE,
};

static constexpr auto TAILLE_CANDIDATES_DEFAUT = 10;

struct CandidateExpressionAppel {
	int quoi = 0;
	NoeudExpression *decl = nullptr;
};

static auto trouve_candidates_pour_fonction_appelee(
		ContexteValidationCode &contexte,
		EspaceDeTravail &espace,
		NoeudExpression *appelee,
		dls::tablet<CandidateExpressionAppel, TAILLE_CANDIDATES_DEFAUT> &candidates)
{
	Prof(trouve_candidates_pour_fonction_appelee);

	auto fichier = espace.fichier(appelee->lexeme->fichier);

	if (appelee->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
		auto declarations = dls::tablet<NoeudDeclaration *, 10>();
		trouve_declarations_dans_bloc_ou_module(espace, declarations, appelee->bloc_parent, appelee->ident, fichier);

		POUR (declarations) {
			// on peut avoir des expressions du genre inverse := inverse(matrice),
			// À FAIRE : si nous enlevons la vérification du drapeau EST_GLOBALE, la compilation est bloquée dans une boucle infinie, il nous faudra un état pour dire qu'aucune candidate n'a été trouvée
			if (it->genre == GenreNoeud::DECLARATION_VARIABLE) {
				if (it->lexeme->fichier == appelee->lexeme->fichier && it->lexeme->ligne >= appelee->lexeme->ligne && ((it->drapeaux & EST_GLOBALE) == 0)) {
					continue;
				}
			}

			candidates.pousse({ CANDIDATE_EST_DECLARATION, it });
		}
	}
	else if (appelee->genre == GenreNoeud::EXPRESSION_REFERENCE_MEMBRE) {
		auto acces = static_cast<NoeudExpressionMembre *>(appelee);

		auto accede = acces->accede;
		auto membre = acces->membre;

		if (accede->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION && fichier->importe_module(accede->ident->nom)) {
			auto module = espace.module(accede->ident->nom);
			auto declarations = dls::tablet<NoeudDeclaration *, 10>();
			trouve_declarations_dans_bloc(declarations, module->bloc, membre->ident);

			POUR (declarations) {
				candidates.pousse({ CANDIDATE_EST_DECLARATION, it });
			}
		}
		else {
			auto type_accede = accede->type;

			while (type_accede->genre == GenreType::POINTEUR || type_accede->genre == GenreType::REFERENCE) {
				type_accede = type_dereference_pour(type_accede);
			}

			if (type_accede->genre == GenreType::STRUCTURE) {
				if ((type_accede->drapeaux & TYPE_FUT_VALIDE) == 0) {
					contexte.unite->attend_sur_type(type_accede);
					return true;
				}

				auto type_struct = type_accede->comme_structure();
				auto membre_trouve = false;
				auto index_membre = 0;

				POUR (type_struct->membres) {
					if (it.nom == membre->ident->nom) {
						acces->type = it.type;
						membre_trouve = true;
						break;
					}

					index_membre += 1;
				}

				if (membre_trouve != false) {
					candidates.pousse({ CANDIDATE_EST_ACCES, acces });
					acces->index_membre = index_membre;
					return false;
				}
			}

			candidates.pousse({ CANDIDATE_EST_APPEL_UNIFORME, acces });
		}
	}
	else if (appelee->genre == GenreNoeud::EXPRESSION_INIT_DE) {
		candidates.pousse({ CANDIDATE_EST_INIT_DE, appelee });
	}
	else {
		if (appelee->type->genre == GenreType::FONCTION) {
			candidates.pousse({ CANDIDATE_EST_EXPRESSION_QUELCONQUE, appelee });
		}
		else {
			contexte.rapporte_erreur("L'expression n'est pas de type fonction", appelee);
			return true;
		}
	}

	return false;
}

static std::pair<bool, double> verifie_compatibilite(
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		Type *type_arg,
		Type *type_enf,
		NoeudExpression *enfant,
		TransformationType &transformation)
{
	Prof(verifie_compatibilite_appel);

	if (cherche_transformation(espace, contexte, type_enf, type_arg, transformation)) {
		return { true, 0.0 };
	}

	if (transformation.type == TypeTransformation::INUTILE) {
		return { false, 1.0 };
	}

	if (transformation.type == TypeTransformation::IMPOSSIBLE) {
		return { false, 0.0 };
	}

	if (transformation.type == TypeTransformation::PREND_REFERENCE) {
		return { false, est_valeur_gauche(enfant->genre_valeur) ? 1.0 : 0.0 };
	}

	/* nous savons que nous devons transformer la valeur (par ex. eini), donc
	 * donne un mi-poids à l'argument */
	return { false, 0.5 };
}

static auto apparie_appel_pointeur(
		NoeudExpressionAppel const *b,
		Type *type,
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		kuri::tableau<IdentifiantEtExpression> const &args,
		DonneesCandidate &resultat)
{
	Prof(apparie_appel_pointeur);

	if (type->genre != GenreType::FONCTION) {
		resultat.etat = FONCTION_INTROUVEE;
		resultat.raison = TYPE_N_EST_PAS_FONCTION;
		return false;
	}

	POUR (args) {
		if (it.ident == nullptr) {
			continue;
		}

		resultat.etat = FONCTION_INTROUVEE;
		resultat.raison = NOMMAGE_ARG_POINTEUR_FONCTION;
		resultat.noeud_erreur = it.expr;
		return false;
	}

	/* vérifie la compatibilité des arguments pour déterminer
	 * s'il y aura besoin d'une transformation. */
	auto type_fonction = type->comme_fonction();

	auto debut_params = 0l;

	if (type_fonction->types_entrees.taille != 0 && type_fonction->types_entrees[0] == espace.typeuse.type_contexte) {
		debut_params = 1;

		auto fonc_courante = contexte.fonction_courante;

		if (fonc_courante != nullptr && dls::outils::possede_drapeau(fonc_courante->drapeaux, FORCE_NULCTX)) {
			resultat.noeud_erreur = b;
			resultat.etat = FONCTION_INTROUVEE;
			resultat.raison = CONTEXTE_MANQUANT;
			return false;
		}
	}
	else {
		resultat.requiers_contexte = false;
	}

	if (type_fonction->types_entrees.taille - debut_params != args.taille) {
		resultat.noeud_erreur = b;
		resultat.type = type;
		resultat.etat = FONCTION_INTROUVEE;
		resultat.raison = MECOMPTAGE_ARGS;
		return false;
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
			type_prm = type_dereference_pour(type_prm);
		}

		auto transformation = TransformationType();
		auto [erreur_dep, poids_pour_enfant] = verifie_compatibilite(espace, contexte, type_prm, type_enf, arg, transformation);

		if (erreur_dep) {
			return true;
		}

		poids_args *= poids_pour_enfant;

		if (poids_args == 0.0) {
			poids_args = 0.0;
			resultat.raison = METYPAGE_ARG;
			resultat.type_attendu = type_dereference_pour(type_prm);
			resultat.type_obtenu = type_enf;
			resultat.noeud_erreur = arg;
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

	return false;
}

static auto apparie_appel_init_de(
		NoeudExpression *expr,
		kuri::tableau<IdentifiantEtExpression> const &args)
{
	Prof(apparie_appel_init_de);

	auto resultat = DonneesCandidate{};

	if (args.taille > 1) {
		resultat.etat = FONCTION_INTROUVEE;
		resultat.raison = MECOMPTAGE_ARGS;
		return resultat;
	}

	auto type_fonction = expr->type->comme_fonction();
	auto type_pointeur = type_fonction->types_entrees[1];

	if (type_pointeur != args[0].expr->type) {
		resultat.etat = FONCTION_INTROUVEE;
		resultat.raison = METYPAGE_ARG;
		resultat.type_attendu = type_pointeur;
		resultat.type_obtenu = args[0].expr->type;
		return resultat;
	}

	auto exprs = dls::tablet<NoeudExpression *, 10>();
	exprs.pousse(args[0].expr);

	auto transformations = dls::tableau<TransformationType>(1);
	transformations[0] = { TypeTransformation::INUTILE };

	resultat.etat = FONCTION_TROUVEE;
	resultat.note = CANDIDATE_EST_APPEL_INIT_DE;
	resultat.type = expr->type;
	resultat.poids_args = 1.0;
	resultat.exprs = exprs;
	resultat.transformations = transformations;

	return resultat;
}

/* ************************************************************************** */

static auto apparie_appel_fonction(
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		NoeudDeclarationEnteteFonction *decl,
		kuri::tableau<IdentifiantEtExpression> const &args,
		DonneesCandidate &res)
{
	Prof(apparie_appel_fonction);

	res.note = CANDIDATE_EST_APPEL_FONCTION;
	res.noeud_decl = decl;
	res.type = decl->type;

	auto const nombre_args = decl->params.taille;

	if (!decl->est_variadique && (args.taille > nombre_args)) {
		res.etat = FONCTION_INTROUVEE;
		res.raison = MECOMPTAGE_ARGS;
		return false;
	}

	if (nombre_args == 0 && args.taille == 0) {
		res.poids_args = 1.0;
		res.etat = FONCTION_TROUVEE;
		res.raison = AUCUNE_RAISON;
		return false;
	}

	dls::tablet<NoeudExpression *, 10> slots;
	slots.redimensionne(nombre_args - decl->est_variadique);

	for (auto i = 0; i < slots.taille(); ++i) {
		auto param = decl->parametre_entree(i);
		slots[i] = param->expression;
	}

	auto index = 0l;
	auto arguments_nommes = false;
	auto dernier_arg_variadique = false;
	dls::ensemble<IdentifiantCode *> args_rencontres;

	POUR (args) {
		if (it.ident != nullptr) {
			arguments_nommes = true;

			auto param = NoeudDeclarationVariable::nul();
			auto index_param = 0l;

			for (auto i = 0; i < decl->params.taille; ++i) {
				auto dp = decl->parametre_entree(i);

				if (dp->ident == it.ident) {
					param = dp;
					index_param = i;
					break;
				}
			}

			if (param == nullptr) {
				res.etat = FONCTION_INTROUVEE;
				res.raison = MENOMMAGE_ARG;
				res.nom_arg = it.ident->nom;
				return false;
			}

			if ((args_rencontres.trouve(it.ident) != args_rencontres.fin()) && (param->drapeaux & EST_VARIADIQUE) == 0) {
				res.etat = FONCTION_INTROUVEE;
				res.raison = RENOMMAGE_ARG;
				res.nom_arg = it.ident->nom;
				return false;
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
					return false;
				}

				slots[index_param] = it.expr;
			}
		}
		else {
			if (arguments_nommes == true && dernier_arg_variadique == false) {
				res.etat = FONCTION_INTROUVEE;
				res.raison = MANQUE_NOM_APRES_VARIADIC;
				return false;
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

	POUR (slots) {
		if (it == nullptr) {
			// À FAIRE : on pourrait donner les noms des arguments manquants
			res.etat = FONCTION_INTROUVEE;
			res.raison = MECOMPTAGE_ARGS;
			return false;
		}
	}

	auto paires_expansion_gabarit = dls::tableau<std::pair<dls::vue_chaine_compacte, Type *>>();

	auto poids_args = 1.0;
	auto fonction_variadique_interne = decl->est_variadique && !decl->est_externe;
	auto expansion_rencontree = false;

	auto transformations = dls::tableau<TransformationType>(slots.taille());

	auto nombre_arg_variadiques_rencontres = 0;

	// utilisé pour déterminer le type des données des arguments variadiques
	// pour la création des tableaux ; nécessaire au cas où nous avons une
	// fonction polymorphique, au quel cas le type serait un type polymorphique
	auto dernier_type_parametre = decl->params[decl->params.taille - 1]->type;

	if (dernier_type_parametre->genre == GenreType::VARIADIQUE) {
		dernier_type_parametre = type_dereference_pour(dernier_type_parametre);
	}

	auto type_donnees_argument_variadique = dernier_type_parametre;

	for (auto i = 0l; i < slots.taille(); ++i) {
		auto index_arg = std::min(i, decl->params.taille - 1);
		auto param = decl->parametre_entree(index_arg);
		auto arg = param->valeur;
		auto slot = slots[i];

		if (slot == param->expression) {
			continue;
		}

		auto type_de_l_expression = slot->type;
		auto type_du_parametre = arg->type;

		if (arg->type->drapeaux & TYPE_EST_POLYMORPHIQUE) {
			// trouve l'argument
			kuri::tableau<dls::vue_chaine_compacte> noms_polymorphiqes;
			rassemble_noms_type_polymorphique(arg->type, noms_polymorphiqes);

			kuri::tableau<std::pair<dls::vue_chaine_compacte, Type *>> paires_appariement_gabarit;
			paires_appariement_gabarit.reserve(noms_polymorphiqes.taille);

			for (auto &nom_polymorphique : noms_polymorphiqes) {
				auto paire_appariement = std::pair<dls::vue_chaine_compacte, Type *>(nom_polymorphique, nullptr);

				for (auto &paire : paires_expansion_gabarit) {
					if (paire.first == nom_polymorphique) {
						paire_appariement.second = paire.second;
						break;
					}
				}

				paires_appariement_gabarit.pousse(paire_appariement);
			}

			// À FAIRE : gère les cas où nous avons plus d'un argument polymorphiques
			auto type_gabarit = paires_appariement_gabarit[0].second;
			auto nom_gabarit = paires_appariement_gabarit[0].first;

			if (!type_gabarit) {
				type_gabarit = apparie_type_gabarit(type_de_l_expression, arg->type);

				if (type_gabarit == nullptr) {
					poids_args = 0.0;
					res.raison = METYPAGE_ARG;
					//res.type_attendu = type_de_l_expression;
					res.type_obtenu = type_de_l_expression;
					res.noeud_erreur = slot;
					return false;
				}

				paires_expansion_gabarit.pousse({ nom_gabarit, type_gabarit });
			}

			type_du_parametre = resoud_type_polymorphique(espace.typeuse, type_du_parametre, type_gabarit);
		}

		if ((param->drapeaux & EST_VARIADIQUE) != 0) {
			if (type_dereference_pour(type_du_parametre) != nullptr) {
				auto transformation = TransformationType();
				auto type_deref = type_dereference_pour(type_du_parametre);
				type_donnees_argument_variadique = type_deref;
				auto poids_pour_enfant = 0.0;

				if (slot->genre == GenreNoeud::EXPANSION_VARIADIQUE) {
					if (!fonction_variadique_interne) {
						res.etat = FONCTION_INTROUVEE;
						res.raison = EXPANSION_VARIADIQUE_FONCTION_EXTERNE;
						return false;
					}

					if (expansion_rencontree) {
						res.etat = FONCTION_INTROUVEE;
						res.raison = MULTIPLE_EXPANSIONS_VARIADIQUES;
						return false;
					}

					auto type_deref_enf = type_dereference_pour(type_de_l_expression);

					auto [erreur_dep, poids_pour_enfant_] = verifie_compatibilite(espace, contexte, type_deref, type_deref_enf, slot, transformation);

					if (erreur_dep) {
						return true;
					}

					poids_pour_enfant = poids_pour_enfant_;

					// aucune transformation acceptée sauf si nous avons un tableau fixe qu'il faudra convertir en un tableau dynamique
					if (poids_pour_enfant != 1.0) {
						poids_pour_enfant = 0.0;
					}

					expansion_rencontree = true;
				}
				else {
					auto [erreur_dep, poids_pour_enfant_] = verifie_compatibilite(espace, contexte, type_deref, type_de_l_expression, slot, transformation);

					if (erreur_dep) {
						return true;
					}

					poids_pour_enfant = poids_pour_enfant_;
				}

				// allège les instantiations pour que les versions déjà instantiées soient préférées pour la selection de la meilleure candidate
				if (arg->type->drapeaux & TYPE_EST_POLYMORPHIQUE) {
					poids_pour_enfant *= 0.95;
				}

				poids_args *= poids_pour_enfant;

				if (poids_args == 0.0) {
					poids_args = 0.0;
					res.raison = METYPAGE_ARG;
					res.type_attendu = type_dereference_pour(type_du_parametre);
					res.type_obtenu = type_de_l_expression;
					res.noeud_erreur = slot;
					break;
				}

				if (fonction_variadique_interne) {
					if (expansion_rencontree && nombre_arg_variadiques_rencontres != 0) {
						if (slot->genre == GenreNoeud::EXPANSION_VARIADIQUE) {
							res.raison = EXPANSION_VARIADIQUE_APRES_ARGUMENTS_VARIADIQUES;
						}
						else {
							res.raison = ARGUMENTS_VARIADIQEUS_APRES_EXPANSION_VARIAQUES;
						}

						res.etat = FONCTION_INTROUVEE;
						return false;
					}
				}

				transformations[i] = transformation;
			}
			else {
				if (slot->genre == GenreNoeud::EXPANSION_VARIADIQUE) {
					if (!fonction_variadique_interne) {
						res.etat = FONCTION_INTROUVEE;
						res.raison = EXPANSION_VARIADIQUE_FONCTION_EXTERNE;
						return false;
					}
				}

				transformations[i] = TransformationType();
			}

			nombre_arg_variadiques_rencontres += 1;
		}
		else {
			auto transformation = TransformationType();
			auto [erreur_dep, poids_pour_enfant] = verifie_compatibilite(espace, contexte, type_du_parametre, type_de_l_expression, slot, transformation);

			if (erreur_dep) {
				return true;
			}

			// allège les instantiations pour que les versions déjà instantiées soient préférées pour la selection de la meilleure candidate
			if (arg->type->drapeaux & TYPE_EST_POLYMORPHIQUE) {
				poids_pour_enfant *= 0.95;
			}

			poids_args *= poids_pour_enfant;

			if (poids_args == 0.0) {
				poids_args = 0.0;
				res.raison = METYPAGE_ARG;
				res.type_attendu = type_du_parametre;
				res.type_obtenu = type_de_l_expression;
				res.noeud_erreur = slot;
				break;
			}

			transformations[i] = transformation;
		}
	}

	if (fonction_variadique_interne) {
		/* Il y a des collisions entre les fonctions variadiques et les fonctions non-variadiques quand le nombre d'arguments correspond pour tous les cas.
		 *
		 * Par exemple :
		 *
		 * passe_une_chaine       :: fonc (a : chaine)
		 * passe_plusieurs_chaine :: fonc (args : ...chaine)
		 * sont ambigües si la variadique n'est appelée qu'avec un seul élément
		 *
		 * et
		 *
		 * ne_passe_rien           :: fonc ()
		 * ne_passe_peut_être_rien :: fonc (args: ...z32)
		 * sont ambigües si la variadique est appelée sans arguments
		 *
		 * Donc diminue le poids pour les fonctions variadiques.
		 */
		poids_args *= 0.95;

		auto index_premier_var_arg = nombre_args - 1;

		if (slots.taille() != nombre_args || slots[index_premier_var_arg]->genre != GenreNoeud::EXPANSION_VARIADIQUE) {
			/* Pour les fonctions variadiques interne, nous créons un tableau
			 * correspondant au types des arguments. */
			static Lexeme lexeme_tableau = { "", {}, GenreLexeme::CHAINE_CARACTERE, 0, 0, 0 };
			auto noeud_tableau = static_cast<NoeudTableauArgsVariadiques *>(espace.assembleuse->cree_noeud(
						GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES, &lexeme_tableau));

			noeud_tableau->type = type_donnees_argument_variadique;
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

	res.poids_args = poids_args;
	res.exprs = slots;
	res.etat = FONCTION_TROUVEE;
	res.transformations = transformations;
	res.paires_expansion_gabarit = paires_expansion_gabarit;

	return false;
}

/* ************************************************************************** */

static auto apparie_appel_structure(
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		NoeudExpressionAppel const *expr,
		NoeudStruct *decl_struct,
		kuri::tableau<IdentifiantEtExpression> const &arguments,
		DonneesCandidate &resultat)
{
	Prof(apparie_appel_structure);

	auto type_compose = decl_struct->type->comme_compose();

	if (decl_struct->est_union) {
		if (expr->params.taille > 1) {
			resultat.etat = FONCTION_TROUVEE;
			resultat.raison = TROP_D_EXPRESSION_POUR_UNION;
			resultat.poids_args = 0.0;
			return false;
		}

		if (expr->params.taille == 0) {
			resultat.etat = FONCTION_TROUVEE;
			resultat.raison = EXPRESSION_MANQUANTE_POUR_UNION;
			resultat.poids_args = 0.0;
			return false;
		}
	}

	auto slots = dls::tablet<NoeudExpression *, 10>();
	slots.redimensionne(type_compose->membres.taille);
	auto transformations = dls::tableau<TransformationType>(slots.taille());

	auto index_membre = 0;
	POUR (type_compose->membres) {
		slots[index_membre] = it.expression_valeur_defaut;

		// dans le cas où l'expression par défaut n'est pas remplacée par une
		// expression il faut préserver la transformation originale
		if (it.expression_valeur_defaut) {
			transformations[index_membre] = it.expression_valeur_defaut->transformation;
		}

		index_membre += 1;
	}

	auto noms_rencontres = dls::ensemble<IdentifiantCode *>();
	auto poids_appariement = 1.0;

	POUR (arguments) {
		if (it.ident == nullptr) {
			resultat.etat = FONCTION_TROUVEE;
			resultat.raison = NOM_ARGUMENT_REQUIS;
			resultat.poids_args = 0.0;
			resultat.noeud_erreur = it.expr;
			return false;
		}

		if (noms_rencontres.trouve(it.ident) != noms_rencontres.fin()) {
			resultat.etat = FONCTION_TROUVEE;
			resultat.raison = RENOMMAGE_ARG;
			resultat.poids_args = 0.0;
			resultat.noeud_erreur = it.expr;
			return false;
		}

		auto type_membre = Type::nul();
		auto decl_membre = NoeudDeclaration::nul();
		index_membre = 0;

		for (auto &membre : *decl_struct->bloc->membres.verrou_lecture()) {
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
			resultat.noeud_erreur = it.expr;
			resultat.noeud_decl = decl_struct;
			return false;
		}

		auto transformation = TransformationType{};
		auto [erreur_dep, poids_pour_enfant] = verifie_compatibilite(espace, contexte, type_membre, it.expr->type, it.expr, transformation);

		if (erreur_dep) {
			return true;
		}

		poids_appariement *= poids_pour_enfant;

		if (poids_appariement == 0.0) {
			resultat.etat = FONCTION_TROUVEE;
			resultat.raison = METYPAGE_ARG;
			resultat.poids_args = 0.0;
			resultat.noeud_erreur = it.expr;
			resultat.type_attendu = type_membre;
			resultat.type_obtenu = it.expr->type;
			return false;
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

	return false;
}

/* ************************************************************************** */

static auto trouve_candidates_pour_appel(
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		NoeudExpressionAppel *expr,
		kuri::tableau<IdentifiantEtExpression> &args,
		dls::tablet<DonneesCandidate, 10> &resultat)
{
	Prof(trouve_candidates_pour_appel);

	auto candidates_appel = dls::tablet<CandidateExpressionAppel, TAILLE_CANDIDATES_DEFAUT>();
	if (trouve_candidates_pour_fonction_appelee(contexte, espace, expr->appelee, candidates_appel)) {
		return true;
	}

	if (candidates_appel.taille() == 0) {
		return true;
	}

	auto nouvelles_candidates = dls::tablet<CandidateExpressionAppel, TAILLE_CANDIDATES_DEFAUT>();

	POUR (candidates_appel) {
		if (it.quoi == CANDIDATE_EST_APPEL_UNIFORME) {
			auto acces = static_cast<NoeudExpressionBinaire *>(it.decl);
			auto candidates = dls::tablet<CandidateExpressionAppel, TAILLE_CANDIDATES_DEFAUT>();
			if (trouve_candidates_pour_fonction_appelee(contexte, espace, acces->expr2, candidates)) {
				return true;
			}

			if (candidates.taille() == 0) {
				contexte.unite->attend_sur_symbole(acces->expr2->lexeme);
				return true;
			}

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

	POUR (candidates_appel) {
		if (it.quoi == CANDIDATE_EST_ACCES) {
			auto dc = DonneesCandidate();
			if (apparie_appel_pointeur(expr, it.decl->type, espace, contexte, args, dc)) {
				return true;
			}
			resultat.pousse(dc);
		}
		else if (it.quoi == CANDIDATE_EST_DECLARATION) {
			auto decl = it.decl;

			if (decl->genre == GenreNoeud::DECLARATION_STRUCTURE) {
				auto decl_struct = static_cast<NoeudStruct *>(decl);

				if ((decl->type->drapeaux & TYPE_FUT_VALIDE) == 0) {
					contexte.unite->attend_sur_type(decl->type);
					return true;
				}

				auto dc = DonneesCandidate();
				if (apparie_appel_structure(espace, contexte, expr, decl_struct, args, dc)) {
					return true;
				}
				resultat.pousse(dc);
			}
			else if (decl->est_entete_fonction()) {
				auto decl_fonc = decl->comme_entete_fonction();

				if ((decl_fonc->drapeaux & DECLARATION_FUT_VALIDEE) == 0) {
					contexte.unite->attend_sur_declaration(decl_fonc);
					return true;
				}

				auto dc = DonneesCandidate();
				if (apparie_appel_fonction(espace, contexte, decl_fonc, args, dc)) {
					return true;
				}
				resultat.pousse(dc);
			}
			else if (decl->genre == GenreNoeud::DECLARATION_VARIABLE) {
				auto dc = DonneesCandidate();
				if (apparie_appel_pointeur(expr, decl->type, espace, contexte, args, dc)) {
					return true;
				}
				resultat.pousse(dc);
			}
		}
		else if (it.quoi == CANDIDATE_EST_INIT_DE) {
			// ici nous pourrions directement retourner si le type est correcte...
			auto dc = apparie_appel_init_de(it.decl, args);
			resultat.pousse(dc);
		}
		else if (it.quoi == CANDIDATE_EST_EXPRESSION_QUELCONQUE) {
			auto dc = DonneesCandidate();
			if (apparie_appel_pointeur(expr, it.decl->type, espace, contexte, args, dc)) {
				return true;
			}
			resultat.pousse(dc);
		}
	}

	return false;
}

/* ************************************************************************** */

static std::pair<NoeudDeclarationEnteteFonction *, bool> trouve_fonction_epandue_ou_crees_en_une(
		Compilatrice &compilatrice,
		EspaceDeTravail &espace,
		NoeudDeclarationEnteteFonction *decl,
		NoeudDeclarationEnteteFonction::tableau_paire_expansion const &paires)
{
	POUR (decl->epandu_pour) {
		if (it.first.taille() != paires.taille()) {
			continue;
		}

		auto trouve = true;

		for (auto i = 0; i < paires.taille(); ++i) {
			if (it.first[i] != paires[i]) {
				trouve = false;
				break;
			}
		}

		if (!trouve) {
			continue;
		}

		return { it.second, false };
	}

	auto noeud_decl = static_cast<NoeudDeclarationEnteteFonction *>(copie_noeud(espace.assembleuse, decl, decl->bloc_parent));
	noeud_decl->est_instantiation_gabarit = true;
	noeud_decl->paires_expansion_gabarit = paires;

	decl->epandu_pour.pousse({ paires, noeud_decl });

	compilatrice.ordonnanceuse->cree_tache_pour_typage(&espace, noeud_decl);
	compilatrice.ordonnanceuse->cree_tache_pour_typage(&espace, noeud_decl->corps);

	return { noeud_decl, true };
}

/* ************************************************************************** */

// À FAIRE : ajout d'un état de résolution des appels afin de savoir à quelle étape nous nous arrêté en cas d'erreur recouvrable (typage fait, tri des arguments fait, etc.)
bool valide_appel_fonction(
		Compilatrice &compilatrice,
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		NoeudExpressionAppel *expr)
{
	Prof(valide_appel_fonction);

	auto fonction_courante = contexte.fonction_courante;
	auto &donnees_dependance = contexte.donnees_dependance;

	// ------------
	// valide d'abord les expressions, leurs types sont nécessaire pour trouver les candidates

	kuri::tableau<IdentifiantEtExpression> args;
	args.reserve(expr->params.taille);

	POUR (expr->params) {
		// l'argument est nommé
		if (it->genre == GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE) {
			auto assign = static_cast<NoeudExpressionBinaire *>(it);
			auto nom_arg = assign->expr1;
			auto arg = assign->expr2;

			args.pousse({ nom_arg->ident, arg });
		}
		else {
			args.pousse({ nullptr, it });
		}
	}

	// ------------
	// trouve la fonction, pour savoir ce que l'on a

	auto candidates = dls::tablet<DonneesCandidate, 10>();

	if (trouve_candidates_pour_appel(espace, contexte, expr, args, candidates)) {
		return true;
	}

	auto candidate = DonneesCandidate::nul();
	auto poids = 0.0;

	POUR (candidates) {
		if (it.etat == FONCTION_TROUVEE) {
			if (it.poids_args > poids) {
				candidate = &it;
				poids = it.poids_args;
			}
		}
	}

	if (candidate == nullptr) {
		contexte.rapporte_erreur_fonction_inconnue(expr, candidates);
		return true;
	}

	POUR (candidates) {
		// À FAIRE : nous avons plusieurs fois les mêmes fonctions ?
		if (it.noeud_decl == candidate->noeud_decl) {
			continue;
		}

		if (it.poids_args == poids) {
			rapporte_erreur(&espace, expr, "Je ne peux pas déterminer quelle fonction appeler car plusieurs fonctions correspondent à l'expression d'appel.")
					.ajoute_message("Candidate possible :\n")
					.ajoute_site(it.noeud_decl)
					.ajoute_message("Candidate possible :\n")
					.ajoute_site(candidate->noeud_decl);
			return true;
		}
	}

	// ------------
	// copie les données

	expr->exprs.reserve(candidate->exprs.taille());

	for (auto enfant : candidate->exprs) {
		expr->exprs.pousse(enfant);
	}

	if (candidate->note == CANDIDATE_EST_APPEL_FONCTION) {
		// @vérifie si utile
		if (candidate->noeud_decl == nullptr) {
			contexte.rapporte_erreur_fonction_inconnue(expr, candidates);
			return true;
		}

		auto decl_fonction_appelee = candidate->noeud_decl->comme_entete_fonction();

		/* pour les directives d'exécution, la fonction courante est nulle */
		if (fonction_courante != nullptr) {
			using dls::outils::possede_drapeau;
			auto decl_fonc = fonction_courante;

			if (possede_drapeau(decl_fonc->drapeaux, FORCE_NULCTX)) {
				auto decl_appel = decl_fonction_appelee;

				if (!decl_appel->est_externe && !possede_drapeau(decl_appel->drapeaux, FORCE_NULCTX)) {
					contexte.rapporte_erreur_fonction_nulctx(expr, decl_fonc, decl_appel);
					return false;
				}
			}
		}

		/* ---------------------- */

		if (!candidate->paires_expansion_gabarit.est_vide()) {
			auto [noeud_decl, doit_epandre] = trouve_fonction_epandue_ou_crees_en_une(compilatrice, espace, decl_fonction_appelee, std::move(candidate->paires_expansion_gabarit));

			if (doit_epandre || (noeud_decl->drapeaux & DECLARATION_FUT_VALIDEE) == 0) {
				contexte.unite->attend_sur_declaration(noeud_decl);
				return true;
			}

			decl_fonction_appelee = noeud_decl;
		}

		// nous devons instantier les gabarits (ou avoir leurs types) avant de pouvoir faire ça
		auto type_fonc = decl_fonction_appelee->type->comme_fonction();
		auto type_sortie = type_fonc->types_sorties[0];

		auto expr_gauche = (expr->drapeaux & DROITE_ASSIGNATION) == 0;
		if (type_sortie->genre != GenreType::RIEN && expr_gauche) {
			rapporte_erreur(&espace, expr, "La valeur de retour de la fonction n'est pas utilisée. Il est important de toujours utiliser les valeurs retournées par les fonctions, par exemple pour ne pas oublier de vérifier si une erreur existe.")
					.ajoute_message("La fonction a été déclarée comme retournant une valeur :\n")
					.ajoute_site(decl_fonction_appelee)
					.ajoute_conseil("si vous ne voulez pas utiliser la valeur de retour, vous pouvez utiliser « _ » comme identifiant pour la capturer et l'ignorer :\n")
					.ajoute_message("\t_ := appel_mais_ignore_le_retour()\n");
			return true;
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

			auto type_tfixe = espace.typeuse.type_tableau_fixe(type_tabl, taille_tableau);
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

		if (decl_fonction_appelee->est_externe || dls::outils::possede_drapeau(decl_fonction_appelee->drapeaux, FORCE_NULCTX)) {
			expr->drapeaux |= FORCE_NULCTX;
		}

		if (expr->type == nullptr) {
			/* À FAIRE: multiple type retour */
			expr->type = type_sortie;
		}

		donnees_dependance.fonctions_utilisees.insere(decl_fonction_appelee);
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
			expr->type = candidate->type->comme_fonction()->types_sorties[0];
		}

		for (auto i = 0; i < expr->exprs.taille; ++i) {
			expr->exprs[i]->transformation = candidate->transformations[i];
		}
	}
	else if (candidate->note == CANDIDATE_EST_APPEL_INIT_DE) {
		// le type du retour
		expr->type = espace.typeuse[TypeBase::RIEN];
	}

	assert(expr->type);
	return false;
}
