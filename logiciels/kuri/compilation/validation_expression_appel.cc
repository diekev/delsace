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

#include "biblinternes/chrono/outils.hh"
#include "biblinternes/outils/assert.hh"

#include "assembleuse_arbre.h"
#include "compilatrice.hh"
#include "erreur.h"
#include "portee.hh"
#include "profilage.hh"
#include "validation_semantique.hh"

struct Monomorpheuse {
	using paire_item = dls::paire<IdentifiantCode *, Type *>;
	dls::tablet<paire_item, 6> items{};

	using paire_type = dls::paire<Type *, Type *>;
	dls::tablet<paire_type, 6> paires_types{};

	// mise en cache des types structures polymorphiques et de leurs monomorphisations passées
	dls::tablet<paire_type, 6> table_structures{};

	void ajoute_item(IdentifiantCode *ident)
	{
		items.pousse({ident, nullptr});
	}

	bool ajoute_paire_types(Type *type_poly, Type *type_cible)
	{
		// enlève les références
		if (type_poly->est_reference()) {
			type_poly = type_poly->comme_reference()->type_pointe;
		}

		if (type_cible->est_reference()) {
			type_cible = type_cible->comme_reference()->type_pointe;
		}

		// si nous avons des fonctions, ajoute ici les paires pour chaque type polymorphique
		if (type_poly->est_fonction() && type_cible->est_fonction()) {
			auto type_poly_fonction = type_poly->comme_fonction();
			auto type_cible_fonction = type_cible->comme_fonction();

			if (type_poly_fonction->types_entrees.taille != type_cible_fonction->types_entrees.taille) {
				return false;
			}

			if (type_poly_fonction->types_sorties.taille != type_cible_fonction->types_sorties.taille) {
				return false;
			}

			for (auto i = 0; i < type_poly_fonction->types_entrees.taille; ++i) {
				if (type_poly_fonction->types_entrees[i]->drapeaux & TYPE_EST_POLYMORPHIQUE) {
					if (!ajoute_paire_types(type_poly_fonction->types_entrees[i], type_cible_fonction->types_entrees[i])) {
						return false;
					}
				}
			}

			for (auto i = 0; i < type_poly_fonction->types_sorties.taille; ++i) {
				if (type_poly_fonction->types_sorties[i]->drapeaux & TYPE_EST_POLYMORPHIQUE) {
					if (!ajoute_paire_types(type_poly_fonction->types_sorties[i], type_cible_fonction->types_sorties[i])) {
						return false;
					}
				}
			}

			return true;
		}

		// À FAIRE(poly) : comment détecter ces cas ? x: fonc()() = nul
		if (type_poly->est_fonction() && type_cible->est_pointeur() && type_cible->comme_pointeur()->type_pointe == nullptr) {
			return true;
		}

		if (type_poly->est_variadique()) {
			type_poly = type_poly->comme_variadique()->type_pointe;
		}

		// vérifie si nous avons des structures polymorphiques, sinon ajoute les types à la liste
		return apparie_structure(type_poly, type_cible);
	}

	bool apparie_structure(Type *type_polymorphique, Type *type_cible)
	{
		auto type_courant = type_cible;
		auto type_courant_poly = type_polymorphique;

		while (true) {
			if (type_courant_poly->genre == GenreType::POLYMORPHIQUE) {
				auto type = type_courant_poly->comme_polymorphique();

				// le type peut-être celui d'une structure (Polymorphe(T = $T)
				if (type->est_structure_poly) {
					if (!type_courant->est_structure()) {
						return false;
					}

					auto decl_struct = type_courant->comme_structure()->decl;

					// À FAIRE : que faire ici?
					if (!decl_struct->est_monomorphisation) {
						return false;
					}

					if (decl_struct->polymorphe_de_base != type->structure) {
						return false;
					}

					for (auto i = 0; i < type->types_constants_structure.taille(); ++i) {
						auto type1 = type->types_constants_structure[i]->comme_polymorphique();
						auto type2 = Type::nul();

						POUR (type_courant->comme_structure()->membres) {
							if (it.nom == type1->ident->nom) {
								type2 = it.type->comme_type_de_donnees()->type_connu;
								break;
							}
						}

						paires_types.pousse({ type1, type2 });
					}

					table_structures.pousse({type_polymorphique, type_cible});
					return true;
				}

				break;
			}

			if (type_courant->genre != type_courant_poly->genre) {
				return false;
			}

			// À FAIRE : type tableau fixe
			type_courant = type_dereference_pour(type_courant);
			type_courant_poly = type_dereference_pour(type_courant_poly);
		}

		paires_types.pousse({type_polymorphique, type_cible});
		return true;
	}

	bool resoud_polymorphes(Typeuse &typeuse)
	{
		POUR (paires_types) {
			IdentifiantCode *ident = nullptr;
			auto type_polymorphique = it.premier;
			auto type_cible = it.second;
			auto type = apparie_type(typeuse, type_polymorphique, type_cible, ident);

			if (!type) {
				return false;
			}

			for (auto &item : items) {
				if (item.premier == ident) {
					if (item.second == nullptr) {
						item.second = type;
					}
					break;
				}
			}
		}

		POUR (items) {
			if (it.second == nullptr) {
				return false;
			}
		}

		return true;
	}

	Type *apparie_type(Typeuse &typeuse, Type *type_polymorphique, Type *type_cible, IdentifiantCode *&ident)
	{
		auto type_courant = type_cible;
		auto type_courant_poly = type_polymorphique;

		while (true) {
			if (type_courant_poly->genre == GenreType::POLYMORPHIQUE) {
				ident = type_courant_poly->comme_polymorphique()->ident;
				break;
			}

			if (type_courant->genre != type_courant_poly->genre) {
				return nullptr;
			}

			// À FAIRE : type tableau fixe
			type_courant = type_dereference_pour(type_courant);
			type_courant_poly = type_dereference_pour(type_courant_poly);
		}

		if (type_courant && type_courant->est_entier_constant()) {
			return typeuse[TypeBase::Z32];
		}

		return type_courant;
	}

	Type *resoud_type_final(Typeuse &typeuse, Type *type_polymorphique)
	{
		auto resultat = Type::nul();

		POUR (table_structures) {
			if (it.premier == type_polymorphique) {
				return it.second;
			}
		}

		if (type_polymorphique->genre == GenreType::POINTEUR) {
			auto type_pointe = type_polymorphique->comme_pointeur()->type_pointe;
			auto type_pointe_pour_type = resoud_type_final(typeuse, type_pointe);
			resultat = typeuse.type_pointeur_pour(type_pointe_pour_type);
		}
		else if (type_polymorphique->genre == GenreType::REFERENCE) {
			auto type_pointe = type_polymorphique->comme_reference()->type_pointe;
			auto type_pointe_pour_type = resoud_type_final(typeuse, type_pointe);
			resultat = typeuse.type_reference_pour(type_pointe_pour_type);
		}
		else if (type_polymorphique->genre == GenreType::TABLEAU_DYNAMIQUE) {
			auto type_pointe = type_polymorphique->comme_tableau_dynamique()->type_pointe;
			auto type_pointe_pour_type = resoud_type_final(typeuse, type_pointe);
			resultat = typeuse.type_tableau_dynamique(type_pointe_pour_type);
		}
		else if (type_polymorphique->genre == GenreType::TABLEAU_FIXE) {
			auto type_tableau_fixe = type_polymorphique->comme_tableau_fixe();
			auto type_pointe = type_tableau_fixe->type_pointe;
			auto type_pointe_pour_type = resoud_type_final(typeuse, type_pointe);
			resultat = typeuse.type_tableau_fixe(type_pointe_pour_type, type_tableau_fixe->taille);
		}
		else if (type_polymorphique->genre == GenreType::VARIADIQUE) {
			auto type_pointe = type_polymorphique->comme_variadique()->type_pointe;
			auto type_pointe_pour_type = resoud_type_final(typeuse, type_pointe);
			resultat = typeuse.type_variadique(type_pointe_pour_type);
		}
		else if (type_polymorphique->genre == GenreType::POLYMORPHIQUE) {
			auto type = type_polymorphique->comme_polymorphique();

			POUR (items) {
				if (it.premier == type->ident) {
					resultat = it.second;
					break;
				}
			}
		}
		else if (type_polymorphique->genre == GenreType::FONCTION) {
			auto type_fonction = type_polymorphique->comme_fonction();

			auto types_entrees = dls::tablet<Type *, 6>();
			types_entrees.reserve(type_fonction->types_entrees.taille);

			POUR (type_fonction->types_entrees) {
				if (it->drapeaux & TYPE_EST_POLYMORPHIQUE) {
					auto type_param = resoud_type_final(typeuse, it);
					types_entrees.pousse(type_param);
				}
				else {
					types_entrees.pousse(it);
				}
			}

			auto types_sorties = dls::tablet<Type *, 6>();
			types_sorties.reserve(type_fonction->types_sorties.taille);

			POUR (type_fonction->types_sorties) {
				if (it->drapeaux & TYPE_EST_POLYMORPHIQUE) {
					auto type_param = resoud_type_final(typeuse, it);
					types_sorties.pousse(type_param);
				}
				else {
					types_sorties.pousse(it);
				}
			}

			resultat = typeuse.type_fonction(types_entrees, types_sorties);
		}
		else {
			assert_rappel(false, [&]() { std::cerr << "Type inattendu dans la résolution de type polymorphique : " << chaine_type(type_polymorphique) << "\n"; });
		}

		return resultat;
	}
};

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
				if (it->lexeme->fichier == appelee->lexeme->fichier && it->lexeme->ligne >= appelee->lexeme->ligne && !it->possede_drapeau(EST_GLOBALE)) {
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
				auto type_struct = type_accede->comme_structure();

				if ((type_accede->drapeaux & TYPE_FUT_VALIDE) == 0) {
					if (type_struct->decl->unite == nullptr) {
						contexte.m_compilatrice.ordonnanceuse->cree_tache_pour_typage(&espace, type_struct->decl);
					}
					contexte.unite->attend_sur_type(type_accede);
					return true;
				}

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

		if (fonc_courante != nullptr && fonc_courante->possede_drapeau(FORCE_NULCTX)) {
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
				res.noeud_erreur = it.expr_ident;
				return false;
			}

			if ((args_rencontres.trouve(it.ident) != args_rencontres.fin()) && !param->possede_drapeau(EST_VARIADIQUE)) {
				res.etat = FONCTION_INTROUVEE;
				res.raison = RENOMMAGE_ARG;
				res.nom_arg = it.ident->nom;
				res.noeud_erreur = it.expr_ident;
				return false;
			}

			dernier_arg_variadique = param->possede_drapeau(EST_VARIADIQUE);

			args_rencontres.insere(it.ident);

			if (dernier_arg_variadique || index_param >= slots.taille()) {
				slots.pousse(it.expr);
			}
			else {
				if (slots[index_param] != param->expression) {
					res.etat = FONCTION_INTROUVEE;
					res.raison = RENOMMAGE_ARG;
					res.nom_arg = it.ident->nom;
					res.noeud_erreur = it.expr_ident;
					return false;
				}

				slots[index_param] = it.expr;
			}
		}
		else {
			if (arguments_nommes == true && dernier_arg_variadique == false) {
				res.etat = FONCTION_INTROUVEE;
				res.raison = MANQUE_NOM_APRES_VARIADIC;
				res.noeud_erreur = it.expr;
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

	dls::tablet<IdentifiantCode *, TAILLE_CANDIDATES_DEFAUT> params_manquants;

	for (auto i = 0; i < nombre_args - decl->est_variadique; ++i) {
		if (slots[i] == nullptr) {
			auto dp = decl->params[i];
			params_manquants.pousse(dp->ident);
		}
	}

	if (!params_manquants.est_vide()) {
		res.etat = FONCTION_INTROUVEE;
		res.raison = ARGUMENTS_MANQUANTS;
		res.arguments_manquants = params_manquants;
		return false;
	}

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

	auto monomorpheuse = Monomorpheuse();

	if (decl->est_polymorphe) {
		POUR ((*decl->bloc_constantes->membres.verrou_lecture())) {
			monomorpheuse.ajoute_item(it->ident);
		}

		for (auto i = 0l; i < slots.taille(); ++i) {
			auto index_arg = std::min(i, decl->params.taille - 1);
			auto param = decl->parametre_entree(index_arg);
			auto arg = param->valeur;
			auto slot = slots[i];

			if (arg->type->drapeaux & TYPE_EST_POLYMORPHIQUE) {
				if (!monomorpheuse.ajoute_paire_types(arg->type, slot->type)) {
					poids_args = 0.0;
					res.raison = METYPAGE_ARG;
					//res.type_attendu = type_de_l_expression;
					res.type_obtenu = slot->type;
					res.noeud_erreur = slot;
					return false;
				}
			}
		}

		if (!monomorpheuse.resoud_polymorphes(espace.typeuse)) {
			poids_args = 0.0;
			res.raison = IMPOSSIBLE_DE_DEFINIR_UN_TYPE_POLYMORPHIQUE;

			POUR (monomorpheuse.items) {
				if (it.second == nullptr) {
					res.ident_poly_manquant = it.premier;
				}
			}

			res.noeud_decl = decl;
			return false;
		}
	}

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
			type_du_parametre = monomorpheuse.resoud_type_final(espace.typeuse, type_du_parametre);
		}

		if (param->possede_drapeau(EST_VARIADIQUE)) {
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

				// allège les polymorphes pour que les versions déjà monomorphées soient préférées pour la selection de la meilleure candidate
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

			// allège les polymorphes pour que les versions déjà monomorphées soient préférées pour la selection de la meilleure candidate
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
			auto noeud_tableau = static_cast<NoeudTableauArgsVariadiques *>(contexte.m_tacheronne.assembleuse->cree_noeud(
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

	if (decl->est_polymorphe) {
		res.items_monomorphisation.reserve(monomorpheuse.items.taille());

		POUR (monomorpheuse.items) {
			res.items_monomorphisation.pousse({ it.premier, it.second, ResultatExpression(), true });
		}
	}

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

	if (decl_struct->est_polymorphe) {
		if (expr->params.taille != decl_struct->params_polymorphiques.taille) {
			resultat.etat = FONCTION_INTROUVEE;
			resultat.raison = MECOMPTAGE_ARGS;
			resultat.poids_args = 0.0;
			return false;
		}

		auto noms_rencontres = dls::ensemblon<IdentifiantCode *, 10>();

		// À FAIRE(poly) : cas où il manque des paramètres
		// À FAIRE(poly) : paramètres dans le désordre
		POUR (arguments) {
			if (it.ident == nullptr) {
				resultat.etat = FONCTION_TROUVEE;
				resultat.raison = NOM_ARGUMENT_REQUIS;
				resultat.poids_args = 0.0;
				resultat.noeud_erreur = it.expr;
				return false;
			}

			if (noms_rencontres.possede(it.ident)) {
				resultat.etat = FONCTION_TROUVEE;
				resultat.raison = RENOMMAGE_ARG;
				resultat.poids_args = 0.0;
				resultat.noeud_erreur = it.expr;
				return false;
			}

			noms_rencontres.insere(it.ident);

			auto param = NoeudDeclarationVariable::nul();

			for (auto &p : decl_struct->params_polymorphiques) {
				if (p->ident == it.ident) {
					param = p;
					break;
				}
			}

			if (param == nullptr) {
				resultat.etat = FONCTION_TROUVEE;
				resultat.raison = MENOMMAGE_ARG;
				resultat.poids_args = 0.0;
				resultat.noeud_erreur = it.expr;
				resultat.noeud_decl = decl_struct;
				return false;
			}

			// vérifie la contrainte
			if (param->possede_drapeau(EST_VALEUR_POLYMORPHIQUE)) {
				if (param->type->est_type_de_donnees()) {
					if (!it.expr->type->est_type_de_donnees()) {
						resultat.etat = FONCTION_TROUVEE;
						resultat.raison = METYPAGE_ARG;
						resultat.poids_args = 0.0;
						resultat.type_attendu = param->type;
						resultat.type_obtenu = it.expr->type;
						resultat.noeud_erreur = it.expr;
						resultat.noeud_decl = decl_struct;
						return false;
					}

					resultat.items_monomorphisation.pousse({ it.ident, it.expr->type, ResultatExpression(), true });
				}
				else {
					if (!(it.expr->type == param->type || (it.expr->type->est_entier_constant() && est_type_entier(param->type)))) {
						resultat.etat = FONCTION_TROUVEE;
						resultat.raison = METYPAGE_ARG;
						resultat.poids_args = 0.0;
						resultat.type_attendu = param->type;
						resultat.type_obtenu = it.expr->type;
						resultat.noeud_erreur = it.expr;
						resultat.noeud_decl = decl_struct;
						return false;
					}

					auto valeur = evalue_expression(&espace, it.expr->bloc_parent, it.expr);

					if (valeur.est_errone) {
						rapporte_erreur(&espace, it.expr, "La valeur n'est pas constante");
					}

					resultat.items_monomorphisation.pousse({ it.ident, param->type, valeur, false });
				}
			}
			else {
				assert_rappel(false, []() { std::cerr << "Les types polymorphiques ne sont pas supportés sur les structures pour le moment\n"; });
			}
		}

		// détecte les arguments polymorphiques dans les fonctions polymorphiques
		auto est_type_argument_polymorphique = false;
		POUR (arguments) {
			if (it.expr->type->est_type_de_donnees()) {
				auto type_connu = it.expr->type->comme_type_de_donnees()->type_connu;

				if (type_connu->drapeaux & TYPE_EST_POLYMORPHIQUE) {
					est_type_argument_polymorphique = true;
					break;
				}
			}
		}

		if (est_type_argument_polymorphique) {
			auto type_poly = espace.typeuse.cree_polymorphique(nullptr);

			type_poly->est_structure_poly = true;
			type_poly->structure = decl_struct;

			POUR (arguments) {
				if (it.expr->type->est_type_de_donnees()) {
					auto type_connu = it.expr->type->comme_type_de_donnees()->type_connu;

					if (type_connu->drapeaux & TYPE_EST_POLYMORPHIQUE) {
						type_poly->types_constants_structure.pousse(type_connu);
					}
				}
			}

			resultat.type = espace.typeuse.type_type_de_donnees(type_poly);
			resultat.note = CANDIDATE_EST_TYPE_POLYMORPHIQUE;
			resultat.etat = FONCTION_TROUVEE;
			resultat.poids_args = 1.0;
			return false;
		}

		resultat.noeud_decl = decl_struct;
		resultat.note = CANDIDATE_EST_INITIALISATION_STRUCTURE;
		resultat.etat = FONCTION_TROUVEE;
		resultat.poids_args = 1.0;
		return false;
	}

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

			args.pousse_front({ nullptr, nullptr, acces->expr1 });

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
					// @concurrence critique
					if (decl->unite == nullptr) {
						contexte.m_compilatrice.ordonnanceuse->cree_tache_pour_typage(&espace, decl);
					}
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

				if (!decl_fonc->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
					contexte.unite->attend_sur_declaration(decl_fonc);
					return true;
				}

				// @concurrence critique
				if (decl_fonc->corps->unite == nullptr && !decl_fonc->est_externe) {
					contexte.m_compilatrice.ordonnanceuse->cree_tache_pour_typage(&espace, decl_fonc->corps);
				}

				auto dc = DonneesCandidate();
				if (apparie_appel_fonction(espace, contexte, decl_fonc, args, dc)) {
					return true;
				}
				resultat.pousse(dc);
			}
			else if (decl->est_decl_var()) {
				auto type = decl->type;
				auto dc = DonneesCandidate();

				if ((decl->drapeaux & DECLARATION_FUT_VALIDEE) == 0) {
					// @concurrence critique
					if (decl->unite == nullptr) {
						contexte.m_compilatrice.ordonnanceuse->cree_tache_pour_typage(&espace, decl);
					}
					contexte.unite->attend_sur_declaration(decl->comme_decl_var());
					return true;
				}

				/* Nous pouvons avoir une constante polymorphique ou un alias. */
				if (type->est_type_de_donnees()) {
					auto type_de_donnees = decl->type->comme_type_de_donnees();
					auto type_connu = type_de_donnees->type_connu;

					if (!type_connu) {
						dc.etat = FONCTION_INTROUVEE;
						dc.raison = TYPE_N_EST_PAS_FONCTION;
						resultat.pousse(dc);
						return true;
					}

					if (type_connu->est_structure()) {
						auto type_struct = type_connu->comme_structure();

						if (apparie_appel_structure(espace, contexte, expr, type_struct->decl, args, dc)) {
							return true;
						}
						resultat.pousse(dc);
					}
					else if (type_connu->est_union()) {
						auto type_union = type_connu->comme_union();

						if (apparie_appel_structure(espace, contexte, expr, type_union->decl, args, dc)) {
							return true;
						}
						resultat.pousse(dc);
					}
					else {
						dc.etat = FONCTION_INTROUVEE;
						dc.raison = TYPE_N_EST_PAS_FONCTION;
						resultat.pousse(dc);
						return false;
					}
				}
				else if (type->est_fonction()) {
					if (apparie_appel_pointeur(expr, decl->type, espace, contexte, args, dc)) {
						return true;
					}
				}
				else {
					dc.etat = FONCTION_INTROUVEE;
					dc.raison = TYPE_N_EST_PAS_FONCTION;
					resultat.pousse(dc);
					return false;
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
		ContexteValidationCode &contexte,
		Compilatrice &compilatrice,
		EspaceDeTravail &espace,
		NoeudDeclarationEnteteFonction *decl,
		NoeudDeclarationEnteteFonction::tableau_item_monomorphisation const &items_monomorphisation)
{
	auto monomorphisations = decl->monomorphisations.verrou_ecriture();

	POUR (*monomorphisations) {
		if (it.premier.taille() != items_monomorphisation.taille()) {
			continue;
		}

		auto trouve = true;

		for (auto i = 0; i < items_monomorphisation.taille(); ++i) {
			if (it.premier[i] != items_monomorphisation[i]) {
				trouve = false;
				break;
			}
		}

		if (!trouve) {
			continue;
		}

		return { it.second, false };
	}

	auto copie = static_cast<NoeudDeclarationEnteteFonction *>(copie_noeud(contexte.m_tacheronne.assembleuse, decl, decl->bloc_parent));
	copie->est_monomorphisation = true;
	copie->est_polymorphe = false;

	// ajout de constantes dans le bloc, correspondants aux paires de monomorphisation
	POUR (items_monomorphisation) {
		// À FAIRE(poly) : lexème pour la  constante
		auto decl_constante = contexte.m_tacheronne.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, copie->lexeme)->comme_decl_var();
		decl_constante->drapeaux |= (EST_CONSTANTE | DECLARATION_FUT_VALIDEE);
		decl_constante->ident = it.ident;
		decl_constante->type = espace.typeuse.type_type_de_donnees(it.type);

		if (!it.est_type) {
			decl_constante->valeur_expression = it.valeur;
		}

		copie->bloc_constantes->membres->pousse(decl_constante);
	}

	monomorphisations->pousse({ items_monomorphisation, copie });

	compilatrice.ordonnanceuse->cree_tache_pour_typage(&espace, copie);
	compilatrice.ordonnanceuse->cree_tache_pour_typage(&espace, copie->corps);

	return { copie, true };
}

static NoeudStruct *monomorphise_au_besoin(
		ContexteValidationCode &contexte,
		EspaceDeTravail &espace,
		NoeudStruct *decl_struct,
		dls::tableau<ItemMonomorphisation> &&items_monomorphisation)
{
	auto monomorphisations = decl_struct->monomorphisations.verrou_ecriture();

	POUR (*monomorphisations) {
		if (it.premier.taille() != items_monomorphisation.taille()) {
			continue;
		}

		auto trouve = true;

		for (auto i = 0; i < items_monomorphisation.taille(); ++i) {
			if (it.premier[i] != items_monomorphisation[i]) {
				trouve = false;
				break;
			}
		}

		if (!trouve) {
			continue;
		}

		return it.second;
	}

	auto copie = copie_noeud(contexte.m_tacheronne.assembleuse, decl_struct, decl_struct->bloc_parent)->comme_structure();
	copie->est_monomorphisation = true;
	copie->polymorphe_de_base = decl_struct;

	if (decl_struct->est_union) {
		copie->type = espace.typeuse.reserve_type_union(copie);
	}
	else {
		copie->type = espace.typeuse.reserve_type_structure(copie);
	}

	// À FAIRE : n'efface pas les membres, mais la validation sémantique les rajoute...
	copie->bloc->membres->taille = 0;

	// ajout de constantes dans le bloc, correspondants aux paires de monomorphisation
	POUR (items_monomorphisation) {
		// À FAIRE(poly) : lexème pour la  constante
		auto decl_constante = contexte.m_tacheronne.assembleuse->cree_noeud(GenreNoeud::DECLARATION_VARIABLE, decl_struct->lexeme)->comme_decl_var();
		decl_constante->drapeaux |= (EST_CONSTANTE | DECLARATION_FUT_VALIDEE);
		decl_constante->ident = it.ident;
		decl_constante->type = it.type;

		if (!it.est_type) {
			decl_constante->valeur_expression = it.valeur;
		}

		copie->bloc->membres->pousse(decl_constante);
	}

	monomorphisations->pousse({ items_monomorphisation, copie });

	contexte.m_compilatrice.ordonnanceuse->cree_tache_pour_typage(&espace, copie);

	return copie;
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

#ifdef CHRONOMETRE_TYPAGE
	auto possede_erreur = true;
	dls::chrono::chrono_rappel_milliseconde chrono_([&](double temps) {
		if (possede_erreur) {
			contexte.m_tacheronne.stats_typage.validation_appel.fusionne_entree({ "tentatives râtées", temps });
		}
		contexte.m_tacheronne.stats_typage.validation_appel.fusionne_entree({ "valide_appel_fonction", temps });
	});
#endif

	auto fonction_courante = contexte.fonction_courante;
	auto &donnees_dependance = contexte.donnees_dependance;

	// ------------
	// valide d'abord les expressions, leurs types sont nécessaire pour trouver les candidates

	kuri::tableau<IdentifiantEtExpression> args;
	args.reserve(expr->params.taille);

	{
		CHRONO_TYPAGE(contexte.m_tacheronne.stats_typage.validation_appel, "prépare arguments");

		POUR (expr->params) {
			// l'argument est nommé
			if (it->est_assignation()) {
				auto assign = it->comme_assignation();
				auto nom_arg = assign->variable;
				auto arg = assign->expression;

				args.pousse({ nom_arg->ident, nom_arg, arg });
			}
			else {
				args.pousse({ nullptr, nullptr, it });
			}
		}
	}

	// ------------
	// trouve la fonction, pour savoir ce que l'on a

	auto candidates = dls::tablet<DonneesCandidate, 10>();
	{
		CHRONO_TYPAGE(contexte.m_tacheronne.stats_typage.validation_appel, "trouve candidate");

		if (trouve_candidates_pour_appel(espace, contexte, expr, args, candidates)) {
			return true;
		}
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

	CHRONO_TYPAGE(contexte.m_tacheronne.stats_typage.validation_appel, "copie données");

	expr->exprs.reserve(candidate->exprs.taille());

	for (auto enfant : candidate->exprs) {
		expr->exprs.pousse(enfant);
	}

	if (candidate->note == CANDIDATE_EST_APPEL_FONCTION) {
		auto decl_fonction_appelee = candidate->noeud_decl->comme_entete_fonction();

		/* pour les directives d'exécution, la fonction courante est nulle */
		if (fonction_courante != nullptr) {
			using dls::outils::possede_drapeau;
			auto decl_fonc = fonction_courante;

			if (possede_drapeau(decl_fonc->drapeaux, FORCE_NULCTX)) {
				auto decl_appel = decl_fonction_appelee;

				if (!decl_appel->est_externe && !decl_appel->possede_drapeau(FORCE_NULCTX)) {
					contexte.rapporte_erreur_fonction_nulctx(expr, decl_fonc, decl_appel);
					return true;
				}
			}
		}

		/* ---------------------- */

		if (!candidate->items_monomorphisation.est_vide()) {
			auto [noeud_decl, doit_monomorpher] = trouve_fonction_epandue_ou_crees_en_une(contexte, compilatrice, espace, decl_fonction_appelee, std::move(candidate->items_monomorphisation));

			if (doit_monomorpher || !noeud_decl->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
				contexte.unite->attend_sur_declaration(noeud_decl);
				return true;
			}

			decl_fonction_appelee = noeud_decl;
		}

		// nous devons monomorpher (ou avoir les types monomorphés) avant de pouvoir faire ça
		auto type_fonc = decl_fonction_appelee->type->comme_fonction();
		auto type_sortie = type_fonc->types_sorties[0];

		auto expr_gauche = !expr->possede_drapeau(DROITE_ASSIGNATION);
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
			contexte.transtype_si_necessaire(expr->exprs[i], candidate->transformations[i]);
		}

		/* les drapeaux pour les arguments variadics */
		if (!candidate->exprs.est_vide() && candidate->exprs.back()->genre == GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES) {
			auto noeud_tableau = static_cast<NoeudTableauArgsVariadiques *>(candidate->exprs.back());

			for (auto j = 0; i < nombre_args_variadics; ++i, ++j) {
				contexte.transtype_si_necessaire(noeud_tableau->exprs[j], candidate->transformations[i]);
			}
		}

		expr->noeud_fonction_appelee = decl_fonction_appelee;

		if (decl_fonction_appelee->est_externe || decl_fonction_appelee->possede_drapeau(FORCE_NULCTX)) {
			expr->drapeaux |= FORCE_NULCTX;
		}

		if (expr->type == nullptr) {
			// À FAIRE(retours multiples)
			expr->type = type_sortie;
		}

		donnees_dependance.fonctions_utilisees.insere(decl_fonction_appelee);
	}
	else if (candidate->note == CANDIDATE_EST_INITIALISATION_STRUCTURE) {
		if (candidate->noeud_decl) {
			auto decl_struct = candidate->noeud_decl->comme_structure();

			auto copie = monomorphise_au_besoin(contexte, espace, decl_struct, std::move(candidate->items_monomorphisation));
			expr->type = espace.typeuse.type_type_de_donnees(copie->type);

			if ((copie->type->drapeaux & TYPE_FUT_VALIDE) == 0) {
				// saute l'expression pour ne plus revenir
				contexte.unite->index_courant += 1;
				contexte.unite->attend_sur_type(copie->type);
				return true;
			}
		}
		else {
			expr->genre = GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE;
			expr->type = candidate->type;

			for (auto i = 0; i < expr->exprs.taille; ++i) {
				if (expr->exprs[i] != nullptr) {
					contexte.transtype_si_necessaire(expr->exprs[i], candidate->transformations[i]);
				}
			}
		}

		if (!expr->possede_drapeau(DROITE_ASSIGNATION)) {
			rapporte_erreur(&espace, expr, "La valeur de l'expression de construction de structure n'est pas utilisée. Peut-être vouliez-vous l'assigner à quelque variable ou l'utiliser comme type ?");
			return true;
		}
	}
	else if (candidate->note == CANDIDATE_EST_TYPE_POLYMORPHIQUE) {
		expr->type = candidate->type;
	}
	else if (candidate->note == CANDIDATE_EST_APPEL_POINTEUR) {
		expr->aide_generation_code = APPEL_POINTEUR_FONCTION;

		if (!candidate->requiers_contexte) {
			expr->drapeaux |= FORCE_NULCTX;
		}

		if (expr->type == nullptr) {
			// À FAIRE(retours multiples)
			expr->type = candidate->type->comme_fonction()->types_sorties[0];
		}

		for (auto i = 0; i < expr->exprs.taille; ++i) {
			contexte.transtype_si_necessaire(expr->exprs[i], candidate->transformations[i]);
		}

		auto expr_gauche = !expr->possede_drapeau(DROITE_ASSIGNATION);
		if (expr->type->genre != GenreType::RIEN && expr_gauche) {
			rapporte_erreur(&espace, expr, "La valeur de retour du pointeur de fonction n'est pas utilisée. Il est important de toujours utiliser les valeurs retournées par les fonctions, par exemple pour ne pas oublier de vérifier si une erreur existe.")
					.ajoute_message("Le type de retour du pointeur de fonctions est : ")
					.ajoute_message(chaine_type(expr->type))
					.ajoute_message("\n")
					.ajoute_conseil("si vous ne voulez pas utiliser la valeur de retour, vous pouvez utiliser « _ » comme identifiant pour la capturer et l'ignorer :\n")
					.ajoute_message("\t_ := appel_mais_ignore_le_retour()\n");
			return true;
		}
	}
	else if (candidate->note == CANDIDATE_EST_APPEL_INIT_DE) {
		// le type du retour
		expr->type = espace.typeuse[TypeBase::RIEN];
	}

#ifdef CHRONOMETRE_TYPAGE
	possede_erreur = false;
#endif

	assert(expr->type);
	return false;
}
