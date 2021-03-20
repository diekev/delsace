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
#include "espace_de_travail.hh"
#include "monomorphisations.hh"
#include "portee.hh"
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
		POUR (items) {
			if (it.premier == ident) {
				return;
			}
		}

		items.ajoute({ident, nullptr});
	}

	bool ajoute_contrainte(IdentifiantCode *ident, Type *type_contrainte, Type *type_donne)
	{
		// si le type n'obéis pas à la contrainte, retourne
		if (type_contrainte->est_type_de_donnees() && !type_donne->est_type_de_donnees()) {
			return false;
		}

		auto type = type_donne->comme_type_de_donnees()->type_connu;

		if (!type) {
			return false;
		}

		POUR (items) {
			if (it.premier != ident) {
				continue;
			}

			it.second = type;
			return true;
		}

		return false;
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

			if (type_poly_fonction->types_entrees.taille() != type_cible_fonction->types_entrees.taille()) {
				return false;
			}

			for (auto i = 0; i < type_poly_fonction->types_entrees.taille(); ++i) {
				if (type_poly_fonction->types_entrees[i]->drapeaux & TYPE_EST_POLYMORPHIQUE) {
					if (!ajoute_paire_types(type_poly_fonction->types_entrees[i], type_cible_fonction->types_entrees[i])) {
						return false;
					}
				}
			}

			if (type_poly_fonction->type_sortie->drapeaux & TYPE_EST_POLYMORPHIQUE) {
				if (!ajoute_paire_types(type_poly_fonction->type_sortie, type_cible_fonction->type_sortie)) {
					return false;
				}
			}

			return true;
		}

		if (type_poly->est_opaque() && type_cible->est_opaque()) {
			paires_types.ajoute({type_poly->comme_opaque()->type_opacifie, type_cible->comme_opaque()->type_opacifie});
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
							if (it.nom == type1->ident) {
								type2 = it.type->comme_type_de_donnees()->type_connu;
								break;
							}
						}

						paires_types.ajoute({ type1, type2 });
					}

					table_structures.ajoute({type_polymorphique, type_cible});
					return true;
				}

				break;
			}

			if (type_courant->genre != type_courant_poly->genre) {
				return false;
			}

			// À FAIRE(tableau fixe)
			type_courant = type_dereference_pour(type_courant);
			type_courant_poly = type_dereference_pour(type_courant_poly);
		}

		paires_types.ajoute({type_polymorphique, type_cible});
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

			// À FAIRE(tableau fixe)
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
			types_entrees.reserve(type_fonction->types_entrees.taille());

			POUR (type_fonction->types_entrees) {
				if (it->drapeaux & TYPE_EST_POLYMORPHIQUE) {
					auto type_param = resoud_type_final(typeuse, it);
					types_entrees.ajoute(type_param);
				}
				else {
					types_entrees.ajoute(it);
				}
			}

			auto type_sortie = type_fonction->type_sortie;

			if (type_sortie->est_tuple()) {
				auto membres = dls::tablet<TypeCompose::Membre, 6>();

				auto tuple = type_sortie->comme_tuple();

				POUR (tuple->membres) {
					if (it.type->drapeaux & TYPE_EST_POLYMORPHIQUE) {
						membres.ajoute({ resoud_type_final(typeuse, it.type) });
					}
					else {
						membres.ajoute({ it.type });
					}
				}
			}
			else if (type_sortie->drapeaux & TYPE_EST_POLYMORPHIQUE) {
				type_sortie = resoud_type_final(typeuse, type_sortie);
			}

			resultat = typeuse.type_fonction(types_entrees, type_sortie);
		}
		else if (type_polymorphique->est_opaque()) {
			auto type_opaque = type_polymorphique->comme_opaque();
			auto type_opacifie = resoud_type_final(typeuse, type_opaque->type_opacifie);
			resultat = typeuse.monomorphe_opaque(type_opaque->decl, type_opacifie);
		}
		else {
			assert_rappel(false, [&]() { std::cerr << "Type inattendu dans la résolution de type polymorphique : " << chaine_type(type_polymorphique) << "\n"; });
		}

		return resultat;
	}
};

struct ApparieuseParams {
private:
	dls::tablet<IdentifiantCode *, 10> m_noms{};
	dls::tablet<NoeudExpression *, 10> m_slots{};
	dls::ensemblon<IdentifiantCode *, 10> args_rencontres{};
	bool m_arguments_nommes = false;
	bool m_dernier_argument_est_variadique = false;
	bool m_est_variadique = false;
	int m_index = 0;
	DonneesCandidate &res;

public:
	ApparieuseParams(DonneesCandidate &res_) : res(res_) {}

	void ajoute_param(IdentifiantCode *ident, NoeudExpression *valeur_defaut, bool est_variadique)
	{
		m_noms.ajoute(ident);

		// Ajoute uniquement la valeur défaut si le paramètre n'est pas variadique,
		// car le code d'appariement de type dépend de ce comportement.
		if (!est_variadique) {
			m_slots.ajoute(valeur_defaut);
		}

		m_est_variadique = est_variadique;
	}

	bool ajoute_expression(IdentifiantCode *ident, NoeudExpression *expr, NoeudExpression *expr_ident)
	{
		if (ident) {
			m_arguments_nommes = true;

			auto index_param = 0l;

			POUR (m_noms) {
				if (ident == it) {
					break;
				}

				index_param += 1;
			}

			if (index_param >= m_noms.taille()) {
				res.raison = MENOMMAGE_ARG;
				res.nom_arg = ident->nom;
				res.noeud_erreur = expr_ident;
				return false;
			}

			auto est_parametre_variadique = index_param == m_noms.taille() - 1 && m_est_variadique;

			if ((args_rencontres.possede(ident)) && !est_parametre_variadique) {
				res.raison = RENOMMAGE_ARG;
				res.nom_arg = ident->nom;
				res.noeud_erreur = expr_ident;
				return false;
			}

			m_dernier_argument_est_variadique = est_parametre_variadique;

			args_rencontres.insere(ident);

			if (m_dernier_argument_est_variadique || index_param >= m_slots.taille()) {
				m_slots.ajoute(expr);
			}
			else {
				m_slots[index_param] = expr;
			}
		}
		else {
			if (m_arguments_nommes == true && m_dernier_argument_est_variadique == false) {
				res.raison = MANQUE_NOM_APRES_VARIADIC;
				res.noeud_erreur = expr;
				return false;
			}

			if (m_dernier_argument_est_variadique || m_index >= m_slots.taille()) {
				args_rencontres.insere(m_noms[m_noms.taille() - 1]);
				m_slots.ajoute(expr);
				m_index++;
			}
			else {
				args_rencontres.insere(m_noms[m_index]);
				m_slots[m_index++] = expr;
			}
		}

		return true;
	}

	bool tous_les_slots_sont_remplis() const
	{
		for (auto i = 0; i < m_noms.taille() - m_est_variadique; ++i) {
			if (m_slots[i] == nullptr) {
				res.arguments_manquants.ajoute(m_noms[i]);
			}
		}

		if (!res.arguments_manquants.est_vide()) {
			res.raison = ARGUMENTS_MANQUANTS;
			return false;
		}

		return true;
	}

	dls::tablet<NoeudExpression *, 10> &slots()
	{
		return m_slots;
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
	auto fichier = espace.fichier(appelee->lexeme->fichier);

	if (appelee->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
		auto declarations = dls::tablet<NoeudDeclaration *, 10>();
		trouve_declarations_dans_bloc_ou_module(declarations, appelee->bloc_parent, appelee->ident, fichier);

		POUR (declarations) {
			// on peut avoir des expressions du genre inverse := inverse(matrice),
			// À FAIRE : si nous enlevons la vérification du drapeau EST_GLOBALE, la compilation est bloquée dans une boucle infinie, il nous faudra un état pour dire qu'aucune candidate n'a été trouvée
			if (it->genre == GenreNoeud::DECLARATION_VARIABLE) {
				if (it->lexeme->fichier == appelee->lexeme->fichier && it->lexeme->ligne >= appelee->lexeme->ligne && !it->possede_drapeau(EST_GLOBALE)) {
					continue;
				}
			}

			candidates.ajoute({ CANDIDATE_EST_DECLARATION, it });
		}
	}
	else if (appelee->genre == GenreNoeud::EXPRESSION_REFERENCE_MEMBRE) {
		auto acces = static_cast<NoeudExpressionMembre *>(appelee);

		auto accede = acces->accedee;
		auto membre = acces->membre;

		if (accede->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION && fichier->importe_module(accede->ident)) {
			auto module = espace.module(accede->ident);
			auto declarations = dls::tablet<NoeudDeclaration *, 10>();
			trouve_declarations_dans_bloc(declarations, module->bloc, membre->ident);

			POUR (declarations) {
				candidates.ajoute({ CANDIDATE_EST_DECLARATION, it });
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
					if (it.nom == membre->ident) {
						acces->type = it.type;
						membre_trouve = true;
						break;
					}

					index_membre += 1;
				}

				if (membre_trouve != false) {
					candidates.ajoute({ CANDIDATE_EST_ACCES, acces });
					acces->index_membre = index_membre;
					return false;
				}
			}

			candidates.ajoute({ CANDIDATE_EST_APPEL_UNIFORME, acces });
		}
	}
	else if (appelee->genre == GenreNoeud::EXPRESSION_INIT_DE) {
		candidates.ajoute({ CANDIDATE_EST_INIT_DE, appelee });
	}
	else {
		if (appelee->type->genre == GenreType::FONCTION) {
			candidates.ajoute({ CANDIDATE_EST_EXPRESSION_QUELCONQUE, appelee });
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
	POUR (args) {
		if (it.ident == nullptr) {
			continue;
		}

		resultat.raison = NOMMAGE_ARG_POINTEUR_FONCTION;
		resultat.noeud_erreur = it.expr;
		return false;
	}

	/* vérifie la compatibilité des arguments pour déterminer
	 * s'il y aura besoin d'une transformation. */
	auto type_fonction = type->comme_fonction();

	auto debut_params = 0;

	if (type_fonction->types_entrees.taille() != 0 && type_fonction->types_entrees[0] == espace.typeuse.type_contexte) {
		debut_params = 1;

		if (!b->bloc_parent->possede_contexte) {
			resultat.noeud_erreur = b;
			resultat.raison = CONTEXTE_MANQUANT;
			return false;
		}
	}
	else {
		resultat.requiers_contexte = false;
	}

	if (type_fonction->types_entrees.taille() - debut_params != args.taille()) {
		resultat.noeud_erreur = b;
		resultat.type = type;
		resultat.raison = MECOMPTAGE_ARGS;
		return false;
	}

	auto exprs = dls::tablet<NoeudExpression *, 10>();
	exprs.reserve(type_fonction->types_entrees.taille() - debut_params);

	auto transformations = kuri::tableau<TransformationType, int>(type_fonction->types_entrees.taille() - debut_params);

	auto poids_args = 1.0;

	/* Validation des types passés en paramètre. */
	for (auto i = debut_params; i < type_fonction->types_entrees.taille(); ++i) {
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

		exprs.ajoute(arg);
	}

	resultat.note = CANDIDATE_EST_APPEL_POINTEUR;
	resultat.type = type_fonction;
	resultat.poids_args = poids_args;
	resultat.exprs = exprs;
	resultat.transformations = std::move(transformations);

	return false;
}

static auto apparie_appel_init_de(
		NoeudExpression *expr,
		kuri::tableau<IdentifiantEtExpression> const &args,
		DonneesCandidate &resultat)
{
	if (args.taille() > 1) {
		resultat.raison = MECOMPTAGE_ARGS;
		return;
	}

	auto type_fonction = expr->type->comme_fonction();
	auto type_pointeur = type_fonction->types_entrees[1];

	if (type_pointeur != args[0].expr->type) {
		resultat.raison = METYPAGE_ARG;
		resultat.type_attendu = type_pointeur;
		resultat.type_obtenu = args[0].expr->type;
		return;
	}

	auto exprs = dls::tablet<NoeudExpression *, 10>();
	exprs.ajoute(args[0].expr);

	auto transformations = kuri::tableau<TransformationType, int>(1);
	transformations[0] = { TypeTransformation::INUTILE };

	resultat.note = CANDIDATE_EST_APPEL_INIT_DE;
	resultat.type = expr->type;
	resultat.poids_args = 1.0;
	resultat.exprs = exprs;
	resultat.transformations = std::move(transformations);
	resultat.requiers_contexte = false;
}

/* ************************************************************************** */

static auto apparie_appel_fonction(
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		NoeudExpressionAppel *expr,
		NoeudDeclarationEnteteFonction *decl,
		kuri::tableau<IdentifiantEtExpression> const &args,
		DonneesCandidate &res)
{
	res.note = CANDIDATE_EST_APPEL_FONCTION;
	res.noeud_decl = decl;
	res.type = decl->type;

	if (expr->drapeaux & POUR_CUISSON) {
		if (!decl->est_polymorphe) {
			res.raison = METYPAGE_ARG;
			return false;
		}

		// prend les paramètres polymorphiques
		auto bloc_constantes = decl->bloc_constantes;

		if (bloc_constantes->membres->taille() != args.taille()) {
			res.raison = MECOMPTAGE_ARGS;
			return false;
		}

		auto noms_rencontres = dls::ensemblon<IdentifiantCode *, 10>();

		POUR (args) {
			if (noms_rencontres.possede(it.ident)) {
				res.raison = RENOMMAGE_ARG;
				res.poids_args = 0.0;
				res.noeud_erreur = it.expr;
				return false;
			}

			noms_rencontres.insere(it.ident);

			auto param = NoeudDeclarationVariable::nul();

			for (auto &p : (*bloc_constantes->membres.verrou_lecture())) {
				if (p->ident == it.ident) {
					param = p->comme_declaration_variable();
					break;
				}
			}

			if (param == nullptr) {
				res.raison = MENOMMAGE_ARG;
				res.poids_args = 0.0;
				res.noeud_erreur = it.expr;
				res.noeud_decl = decl;
				return false;
			}

			// À FAIRE : contraites, ceci ne gère que les cas suivant : a : $T
			auto type = it.expr->type->comme_type_de_donnees();
			res.items_monomorphisation.ajoute({ it.ident, type->type_connu, ValeurExpression(), true });
		}

		res.note = CANDIDATE_EST_CUISSON_FONCTION;
		res.poids_args = 1.0;
		return false;
	}

	auto const nombre_args = decl->params.taille();

	if (!decl->est_variadique && (args.taille() > nombre_args)) {
		res.raison = MECOMPTAGE_ARGS;
		return false;
	}

	if (nombre_args == 0 && args.taille() == 0) {
		res.poids_args = 1.0;
		return false;
	}

	/* mise en cache des paramètres d'entrées, accéder à cette fonction se voit dans les profiles */
	dls::tablet<NoeudDeclarationVariable *, 10> parametres_entree;
	for (auto i = 0; i < decl->params.taille(); ++i) {
		parametres_entree.ajoute(decl->parametre_entree(i));
	}

	auto apparieuse_params = ApparieuseParams(res);
	//slots.redimensionne(nombre_args - decl->est_variadique);

	for (auto i = 0; i < decl->params.taille(); ++i) {
		auto param = parametres_entree[i];
		apparieuse_params.ajoute_param(param->ident, param->expression, param->possede_drapeau(EST_VARIADIQUE));
	}

	POUR (args) {
		if (!apparieuse_params.ajoute_expression(it.ident, it.expr, it.expr_ident)) {
			// l'apparieuse aura déjà ajourné les données pour cette candidate
			return false;
		}
	}

	if (!apparieuse_params.tous_les_slots_sont_remplis()) {
		return false;
	}

	auto poids_args = 1.0;
	auto fonction_variadique_interne = decl->est_variadique && !decl->est_externe;
	auto expansion_rencontree = false;

	auto &slots = apparieuse_params.slots();
	auto transformations = dls::tablet<TransformationType, 10>(slots.taille());

	auto nombre_arg_variadiques_rencontres = 0;

	// utilisé pour déterminer le type des données des arguments variadiques
	// pour la création des tableaux ; nécessaire au cas où nous avons une
	// fonction polymorphique, au quel cas le type serait un type polymorphique
	auto dernier_type_parametre = decl->params[decl->params.taille() - 1]->type;

	if (dernier_type_parametre->genre == GenreType::VARIADIQUE) {
		dernier_type_parametre = type_dereference_pour(dernier_type_parametre);
	}

	auto type_donnees_argument_variadique = dernier_type_parametre;

	auto monomorpheuse = Monomorpheuse();

	if (decl->est_polymorphe) {
		decl->bloc_constantes->membres.avec_verrou_lecture([&monomorpheuse](const kuri::tableau<NoeudDeclaration *, int> &membres)
		{
			POUR (membres) {
				monomorpheuse.ajoute_item(it->ident);
			}
		});

		POUR (decl->params) {
			if (it->drapeaux & EST_VALEUR_POLYMORPHIQUE) {
				monomorpheuse.ajoute_item(it->ident);
			}
		}

		for (auto i = 0l; i < slots.taille(); ++i) {
			auto index_arg = std::min(i, static_cast<long>(decl->params.taille() - 1));
			auto param = parametres_entree[index_arg];
			auto arg = param->valeur;
			auto slot = slots[i];

			if (param->drapeaux & EST_VALEUR_POLYMORPHIQUE) {
				if (!monomorpheuse.ajoute_contrainte(param->ident, arg->type, slot->type)) {
					poids_args = 0.0;
					res.raison = METYPAGE_ARG;
					res.type_attendu = arg->type;
					res.type_obtenu = slot->type;
					res.noeud_erreur = slot;
					return false;
				}
			}

			if (arg->type->drapeaux & TYPE_EST_POLYMORPHIQUE) {
				if (!monomorpheuse.ajoute_paire_types(arg->type, slot->type)) {
					poids_args = 0.0;
					res.raison = METYPAGE_ARG;
					res.type_attendu = arg->type;
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
		auto index_arg = std::min(i, static_cast<long>(decl->params.taille() - 1));
		auto param = parametres_entree[index_arg];
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
						res.raison = EXPANSION_VARIADIQUE_FONCTION_EXTERNE;
						return false;
					}

					if (expansion_rencontree) {
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

						return false;
					}
				}

				transformations[i] = transformation;
			}
			else {
				if (slot->genre == GenreNoeud::EXPANSION_VARIADIQUE) {
					if (!fonction_variadique_interne) {
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
			auto noeud_tableau = contexte.m_tacheronne.assembleuse->cree_tableau_variadique(&lexeme_tableau);

			noeud_tableau->type = type_donnees_argument_variadique;
			// @embouteillage, ceci gaspille également de la mémoire si la candidate n'est pas sélectionné
			noeud_tableau->expressions.reserve(static_cast<int>(slots.taille()) - index_premier_var_arg);

			for (auto i = index_premier_var_arg; i < slots.taille(); ++i) {
				noeud_tableau->expressions.ajoute(slots[i]);
			}

			if (index_premier_var_arg >= slots.taille()) {
				slots.ajoute(noeud_tableau);
			}
			else {
				slots[index_premier_var_arg] = noeud_tableau;
			}

			slots.redimensionne(nombre_args);
		}
	}

	res.transformations.reserve(static_cast<int>(transformations.taille()));

	// Il faut supprimer de l'appel les constantes correspondant aux valeur polymorphiques.
	for (auto i = 0l; i < slots.taille(); ++i) {
		auto index_arg = std::min(i, static_cast<long>(decl->params.taille() - 1));
		auto param = parametres_entree[index_arg];

		if (param->drapeaux & EST_VALEUR_POLYMORPHIQUE) {
			continue;
		}

		res.exprs.ajoute(slots[i]);

		if (i < transformations.taille()) {
			res.transformations.ajoute(transformations[i]);
		}
	}

	for (auto i = slots.taille(); i < transformations.taille(); ++i) {
		res.transformations.ajoute(transformations[i]);
	}

	res.poids_args = poids_args;

	if (decl->est_polymorphe) {
		res.items_monomorphisation.reserve(static_cast<int>(monomorpheuse.items.taille()));

		POUR (monomorpheuse.items) {
			res.items_monomorphisation.ajoute({ it.premier, it.second, ValeurExpression(), true });
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
	auto type_compose = decl_struct->type->comme_compose();

	if (decl_struct->est_polymorphe) {
		if (expr->parametres.taille() != decl_struct->params_polymorphiques.taille()) {
			resultat.raison = MECOMPTAGE_ARGS;
			resultat.poids_args = 0.0;
			return false;
		}

		auto apparieuse_params = ApparieuseParams(resultat);

		POUR (decl_struct->params_polymorphiques) {
			apparieuse_params.ajoute_param(it->ident, nullptr, false);
		}

		POUR (arguments) {
			if (!apparieuse_params.ajoute_expression(it.ident, it.expr, it.expr_ident)) {
				// À FAIRE : si ceci est au début de la fonction, nous avons des messages d'erreurs assez étranges...
				resultat.noeud_decl = decl_struct;
				return false;
			}
		}

		if (!apparieuse_params.tous_les_slots_sont_remplis()) {
			return false;
		}

		auto index_param = 0;
		POUR (apparieuse_params.slots()) {
			auto param = decl_struct->params_polymorphiques[index_param];
			index_param += 1;

			// vérifie la contrainte
			if (param->possede_drapeau(EST_VALEUR_POLYMORPHIQUE)) {
				if (param->type->est_type_de_donnees()) {
					if (!it->type->est_type_de_donnees()) {
						resultat.raison = METYPAGE_ARG;
						resultat.poids_args = 0.0;
						resultat.type_attendu = param->type;
						resultat.type_obtenu = it->type;
						resultat.noeud_erreur = it;
						resultat.noeud_decl = decl_struct;
						return false;
					}

					resultat.items_monomorphisation.ajoute({ param->ident, it->type, ValeurExpression(), true });
				}
				else {
					if (!(it->type == param->type || (it->type->est_entier_constant() && est_type_entier(param->type)))) {
						resultat.raison = METYPAGE_ARG;
						resultat.poids_args = 0.0;
						resultat.type_attendu = param->type;
						resultat.type_obtenu = it->type;
						resultat.noeud_erreur = it;
						resultat.noeud_decl = decl_struct;
						return false;
					}

					auto valeur = evalue_expression(&espace, it->bloc_parent, it);

					if (valeur.est_errone) {
						rapporte_erreur(&espace, it, "La valeur n'est pas constante");
					}

					resultat.items_monomorphisation.ajoute({ param->ident, param->type, valeur.valeur, false });
				}
			}
			else {
				assert_rappel(false, []() { std::cerr << "Les types polymorphiques ne sont pas supportés sur les structures pour le moment\n"; });
			}
		}

		// détecte les arguments polymorphiques dans les fonctions polymorphiques
		auto est_type_argument_polymorphique = false;
		POUR (arguments) {
			// vérifie si l'argument est une valeur polymorphique de la fonction
			if (it.expr->est_reference_declaration()) {
				auto ref_decl = it.expr->comme_reference_declaration();

				if (ref_decl->declaration_referee->drapeaux & EST_VALEUR_POLYMORPHIQUE) {
					est_type_argument_polymorphique = true;
					break;
				}
			}

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
				if (it.expr->est_reference_declaration()) {
					auto ref_decl = it.expr->comme_reference_declaration();

					if (ref_decl->declaration_referee->drapeaux & EST_VALEUR_POLYMORPHIQUE) {
						type_poly->types_constants_structure.ajoute(it.expr->type);
						break;
					}
				}

				if (it.expr->type->est_type_de_donnees()) {
					auto type_connu = it.expr->type->comme_type_de_donnees()->type_connu;

					if (type_connu->drapeaux & TYPE_EST_POLYMORPHIQUE) {
						type_poly->types_constants_structure.ajoute(type_connu);
					}
				}
			}

			resultat.type = espace.typeuse.type_type_de_donnees(type_poly);
			resultat.note = CANDIDATE_EST_TYPE_POLYMORPHIQUE;
			resultat.poids_args = 1.0;
			return false;
		}

		resultat.noeud_decl = decl_struct;
		resultat.note = CANDIDATE_EST_INITIALISATION_STRUCTURE;
		resultat.poids_args = 1.0;
		return false;
	}

	if (decl_struct->est_union) {
		if (expr->parametres.taille() > 1) {
			resultat.raison = TROP_D_EXPRESSION_POUR_UNION;
			resultat.poids_args = 0.0;
			return false;
		}

		if (expr->parametres.taille() == 0) {
			resultat.raison = EXPRESSION_MANQUANTE_POUR_UNION;
			resultat.poids_args = 0.0;
			return false;
		}
	}

	// À FAIRE : détecte quand nous avons des constantes
	auto apparieuse_params = ApparieuseParams(resultat);

	POUR (type_compose->membres) {
		apparieuse_params.ajoute_param(it.nom, it.expression_valeur_defaut, false);
	}

	POUR (arguments) {
		if (!apparieuse_params.ajoute_expression(it.ident, it.expr, it.expr_ident)) {
			return false;
		}
	}

	auto transformations = kuri::tableau<TransformationType, int>(type_compose->membres.taille());
	auto poids_appariement = 1.0;

	auto index_membre = 0;
	POUR (apparieuse_params.slots()) {
		if (it == nullptr) {
			index_membre += 1;
			continue;
		}

		auto &membre = type_compose->membres[index_membre];

		auto transformation = TransformationType{};
		auto [erreur_dep, poids_pour_enfant] = verifie_compatibilite(espace, contexte, membre.type, it->type, it, transformation);

		if (erreur_dep) {
			return true;
		}

		poids_appariement *= poids_pour_enfant;

		if (poids_appariement == 0.0) {
			resultat.raison = METYPAGE_ARG;
			resultat.poids_args = 0.0;
			resultat.noeud_erreur = it;
			resultat.type_attendu = membre.type;
			resultat.type_obtenu = it->type;
			return false;
		}

		transformations[index_membre] = transformation;
		index_membre += 1;
	}

	resultat.type = decl_struct->type;
	resultat.note = CANDIDATE_EST_INITIALISATION_STRUCTURE;
	resultat.raison = AUCUNE_RAISON;
	resultat.poids_args = poids_appariement;
	resultat.exprs = apparieuse_params.slots();
	resultat.transformations = std::move(transformations);

	return false;
}

/* ************************************************************************** */

static auto apparie_construction_opaque(
		EspaceDeTravail &/*espace*/,
		ContexteValidationCode &/*contexte*/,
		NoeudExpressionAppel const */*expr*/,
		TypeOpaque *type_opaque,
		kuri::tableau<IdentifiantEtExpression> const &arguments,
		DonneesCandidate &resultat)
{
	if (arguments.taille() > 1) {
		resultat.raison = MECOMPTAGE_ARGS;
		resultat.poids_args = 0.0;
		return true;
	}

	auto arg = arguments[0].expr;

	if (type_opaque->drapeaux & TYPE_EST_POLYMORPHIQUE) {
		if (arg->type->est_type_de_donnees()) {
			resultat.type = type_opaque;
			resultat.note = CANDIDATE_EST_MONOMORPHISATION_OPAQUE;
			resultat.poids_args = 1.0;
			resultat.exprs.ajoute(arg);
			return false;
		}

		resultat.type = type_opaque;
		resultat.note = CANDIDATE_EST_INITIALISATION_OPAQUE;
		resultat.poids_args = 1.0;
		resultat.exprs.ajoute(arg);
		return false;
	}

	if (arguments[0].expr->type != type_opaque->type_opacifie) {
		resultat.raison = METYPAGE_ARG;
		resultat.poids_args = 0.0;
		return true;
	}

	resultat.type = type_opaque;
	resultat.note = CANDIDATE_EST_INITIALISATION_OPAQUE;
	resultat.poids_args = 1.0;
	resultat.exprs.ajoute(arg);
	return false;
}

/* ************************************************************************** */

struct ContexteValidationAppel {
	kuri::tableau<IdentifiantEtExpression> args{};
	dls::tablet<DonneesCandidate, 10> candidates{};
	int nombre_candidates = 0;

	DonneesCandidate &ajoute_donnees()
	{
		candidates.redimensionne(candidates.taille() + 1);
		return candidates[nombre_candidates++];
	}

	void efface()
	{
		nombre_candidates = 0;
		args.efface();

		POUR (candidates) {
			it.arguments_manquants.efface();
			it.exprs.efface();
			it.transformations.efface();
			it.items_monomorphisation.efface();
			it.poids_args = 0.0;
			it.raison = 0;
			it.nom_arg = "";
			it.requiers_contexte = true;
			it.type = nullptr;
			it.type_attendu = nullptr;
			it.type_obtenu = nullptr;
			it.noeud_erreur = nullptr;
			it.noeud_decl = nullptr;
			it.ident_poly_manquant = nullptr;
		}
	}
};

static auto trouve_candidates_pour_appel(
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		NoeudExpressionAppel *expr,
		kuri::tableau<IdentifiantEtExpression> &args,
		ContexteValidationAppel &resultat)
{
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
			if (trouve_candidates_pour_fonction_appelee(contexte, espace, acces->operande_droite, candidates)) {
				return true;
			}

			if (candidates.taille() == 0) {
				contexte.unite->attend_sur_symbole(acces->operande_droite->comme_reference_declaration());
				return true;
			}

			args.pousse_front({ nullptr, nullptr, acces->operande_gauche });

			for (auto c : candidates) {
				nouvelles_candidates.ajoute(c);
			}
		}
		else {
			nouvelles_candidates.ajoute(it);
		}
	}

	candidates_appel = nouvelles_candidates;

	POUR (candidates_appel) {
		if (it.quoi == CANDIDATE_EST_ACCES) {
			auto &dc = resultat.ajoute_donnees();
			if (apparie_appel_pointeur(expr, it.decl->type, espace, contexte, args, dc)) {
				return true;
			}
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

				auto &dc = resultat.ajoute_donnees();
				if (apparie_appel_structure(espace, contexte, expr, decl_struct, args, dc)) {
					return true;
				}
			}
			else if (decl->est_entete_fonction()) {
				auto decl_fonc = decl->comme_entete_fonction();

				if (!decl_fonc->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
					contexte.unite->attend_sur_declaration(decl_fonc);
					return true;
				}

				auto &dc = resultat.ajoute_donnees();
				if (apparie_appel_fonction(espace, contexte, expr, decl_fonc, args, dc)) {
					return true;
				}
			}
			else if (decl->est_declaration_variable()) {
				auto type = decl->type;
				auto &dc = resultat.ajoute_donnees();

				if ((decl->drapeaux & DECLARATION_FUT_VALIDEE) == 0) {
					// @concurrence critique
					if (decl->unite == nullptr) {
						contexte.m_compilatrice.ordonnanceuse->cree_tache_pour_typage(&espace, decl);
					}
					contexte.unite->attend_sur_declaration(decl->comme_declaration_variable());
					return true;
				}

				/* Nous pouvons avoir une constante polymorphique ou un alias. */
				if (type->est_type_de_donnees()) {
					auto type_de_donnees = decl->type->comme_type_de_donnees();
					auto type_connu = type_de_donnees->type_connu;

					if (!type_connu) {
						dc.raison = TYPE_N_EST_PAS_FONCTION;
						return true;
					}

					if (type_connu->est_structure()) {
						auto type_struct = type_connu->comme_structure();

						if (apparie_appel_structure(espace, contexte, expr, type_struct->decl, args, dc)) {
							return true;
						}
					}
					else if (type_connu->est_union()) {
						auto type_union = type_connu->comme_union();

						if (apparie_appel_structure(espace, contexte, expr, type_union->decl, args, dc)) {
							return true;
						}
					}
					else if (type_connu->est_opaque()) {
						auto type_opaque = type_connu->comme_opaque();

						if (apparie_construction_opaque(espace, contexte, expr, type_opaque, args, dc)) {
							return true;
						}
					}
					else {
						dc.raison = TYPE_N_EST_PAS_FONCTION;
						return false;
					}
				}
				else if (type->est_fonction()) {
					if (apparie_appel_pointeur(expr, decl->type, espace, contexte, args, dc)) {
						return true;
					}
				}
				else if (type->est_opaque()) {
					auto type_opaque = type->comme_opaque();

					if (apparie_construction_opaque(espace, contexte, expr, type_opaque, args, dc)) {
						return true;
					}
				}
				else {
					dc.raison = TYPE_N_EST_PAS_FONCTION;
					return false;
				}
			}
		}
		else if (it.quoi == CANDIDATE_EST_INIT_DE) {
			// ici nous pourrions directement retourner si le type est correcte...
			auto &dc = resultat.ajoute_donnees();
			apparie_appel_init_de(it.decl, args, dc);
		}
		else if (it.quoi == CANDIDATE_EST_EXPRESSION_QUELCONQUE) {
			auto &dc = resultat.ajoute_donnees();
			if (apparie_appel_pointeur(expr, it.decl->type, espace, contexte, args, dc)) {
				return true;
			}
		}
	}

	return false;
}

/* ************************************************************************** */

static std::pair<NoeudDeclarationEnteteFonction *, bool> monomorphise_au_besoin(
		ContexteValidationCode &contexte,
		Compilatrice &compilatrice,
		EspaceDeTravail &espace,
		NoeudDeclarationEnteteFonction *decl,
		kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation)
{
	auto monomorphisation = decl->monomorphisations->trouve_monomorphisation(items_monomorphisation);

	if (monomorphisation) {
		return { monomorphisation, false };
	}

	auto copie = static_cast<NoeudDeclarationEnteteFonction *>(copie_noeud(contexte.m_tacheronne.assembleuse, decl, decl->bloc_parent));
	copie->est_monomorphisation = true;
	copie->est_polymorphe = false;

	// ajout de constantes dans le bloc, correspondants aux paires de monomorphisation
	POUR (items_monomorphisation) {
		// À FAIRE(poly) : lexème pour la  constante
		auto decl_constante = contexte.m_tacheronne.assembleuse->cree_declaration_variable(copie->lexeme);
		decl_constante->drapeaux |= (EST_CONSTANTE | DECLARATION_FUT_VALIDEE);
		decl_constante->ident = it.ident;
		decl_constante->type = espace.typeuse.type_type_de_donnees(it.type);

		if (!it.est_type) {
			decl_constante->valeur_expression = it.valeur;
		}

		copie->bloc_constantes->membres->ajoute(decl_constante);
	}

	// Supprime les valeurs polymorphiques
	// À FAIRE : optimise
	auto nouveau_params = kuri::tableau<NoeudDeclarationVariable *, int>();
	POUR (copie->params) {
		if (it->drapeaux & EST_VALEUR_POLYMORPHIQUE) {
			continue;
		}

		nouveau_params.ajoute(it);
	}

	if (nouveau_params.taille() != copie->params.taille()) {
		copie->params = std::move(nouveau_params);
	}

	decl->monomorphisations->ajoute(items_monomorphisation, copie);

	compilatrice.ordonnanceuse->cree_tache_pour_typage(&espace, copie);
	compilatrice.ordonnanceuse->cree_tache_pour_typage(&espace, copie->corps);

	return { copie, true };
}

static NoeudStruct *monomorphise_au_besoin(
		ContexteValidationCode &contexte,
		EspaceDeTravail &espace,
		NoeudStruct *decl_struct,
		kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation)
{
	auto monomorphisation = decl_struct->monomorphisations->trouve_monomorphisation(items_monomorphisation);

	if (monomorphisation) {
		return monomorphisation;
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
	copie->bloc->membres->efface();

	// ajout de constantes dans le bloc, correspondants aux paires de monomorphisation
	POUR (items_monomorphisation) {
		// À FAIRE(poly) : lexème pour la  constante
		auto decl_constante = contexte.m_tacheronne.assembleuse->cree_declaration_variable(decl_struct->lexeme);
		decl_constante->drapeaux |= (EST_CONSTANTE | DECLARATION_FUT_VALIDEE);
		decl_constante->ident = it.ident;
		decl_constante->type = it.type;

		if (!it.est_type) {
			decl_constante->valeur_expression = it.valeur;
		}

		copie->bloc_constantes->membres->ajoute(decl_constante);
		copie->bloc->membres->ajoute(decl_constante);
	}

	decl_struct->monomorphisations->ajoute(items_monomorphisation, copie);

	contexte.m_compilatrice.ordonnanceuse->cree_tache_pour_typage(&espace, copie);

	return copie;
}

/* ************************************************************************** */

// À FAIRE : ajout d'un état de résolution des appels afin de savoir à quelle étape nous nous arrêté en cas d'erreur recouvrable (typage fait, tri des arguments fait, etc.)
ResultatValidation valide_appel_fonction(
		Compilatrice &compilatrice,
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		NoeudExpressionAppel *expr)
{
#ifdef STATISTIQUES_DETAILLEES
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

	static ContexteValidationAppel ctx;
	ctx.efface();

	auto &args = ctx.args;
	args.reserve(expr->parametres.taille());

	{
		CHRONO_TYPAGE(contexte.m_tacheronne.stats_typage.validation_appel, "prépare arguments");

		POUR (expr->parametres) {
			// l'argument est nommé
			if (it->est_assignation_variable()) {
				auto assign = it->comme_assignation_variable();
				auto nom_arg = assign->variable;
				auto arg = assign->expression;

				args.ajoute({ nom_arg->ident, nom_arg, arg });
			}
			else {
				args.ajoute({ nullptr, nullptr, it });
			}
		}
	}

	// ------------
	// trouve la fonction, pour savoir ce que l'on a

	{
		CHRONO_TYPAGE(contexte.m_tacheronne.stats_typage.validation_appel, "trouve candidate");

		if (trouve_candidates_pour_appel(espace, contexte, expr, args, ctx)) {
			return ResultatValidation::Erreur;
		}
	}

	ctx.candidates.redimensionne(ctx.nombre_candidates);
	auto candidate = DonneesCandidate::nul();
	auto poids = 0.0;

	POUR (ctx.candidates) {
		if (it.poids_args > poids) {
			candidate = &it;
			poids = it.poids_args;
		}
	}

	if (candidate == nullptr) {
		contexte.rapporte_erreur_fonction_inconnue(expr, ctx.candidates);
		return ResultatValidation::Erreur;
	}

	POUR (ctx.candidates) {
		if (&it == candidate) {
			continue;
		}

		if (it.poids_args == poids) {
			auto e = rapporte_erreur(&espace, expr, "Je ne peux pas déterminer quelle fonction appeler car plusieurs fonctions correspondent à l'expression d'appel.");

			if (it.noeud_decl && candidate->noeud_decl) {
				e.ajoute_message("Candidate possible :\n");
				e.ajoute_site(it.noeud_decl);
				e.ajoute_message("Candidate possible :\n");
				e.ajoute_site(candidate->noeud_decl);
			}
			else {
				e.ajoute_message("Erreur interne ! Aucun site pour les candidates possibles !");
			}

			return ResultatValidation::Erreur;
		}
	}

	// ------------
	// copie les données

	CHRONO_TYPAGE(contexte.m_tacheronne.stats_typage.validation_appel, "copie données");

#if 0
	struct StatisExprs {
		int nombre_exprs = 0;
		kuri::tableau<int> tailles{};

		~StatisExprs()
		{
			std::cerr << "Stats pour les tailles des expressions d'appels\n";
			std::cerr << "-- nombre d'expressions : " << nombre_exprs << '\n';
			for (int i = 0; i < tailles.taille(); ++i) {
				std::cerr << "-- taille " << i << " : " << tailles[i]
						  << " (" << static_cast<float>(tailles[i]) / static_cast<float>(nombre_exprs) << "%)" << '\n';
			}
		}

		void ajoute_taille(int t)
		{
			tailles.redimensionne(t + 1, 0);
			tailles[t] += 1;
			nombre_exprs += 1;
		}
	};

	static StatisExprs statis_exprs;
	statis_exprs.ajoute_taille(static_cast<int>(candidate->exprs.taille()));
#endif

	expr->parametres_resolus.reserve(static_cast<int>(candidate->exprs.taille()));

	for (auto enfant : candidate->exprs) {
		expr->parametres_resolus.ajoute(enfant);
	}

	if (candidate->note == CANDIDATE_EST_APPEL_FONCTION) {
		auto decl_fonction_appelee = candidate->noeud_decl->comme_entete_fonction();

		/* pour les directives d'exécution, la fonction courante est nulle */
		if (fonction_courante != nullptr) {
			using dls::outils::possede_drapeau;
			auto decl_fonc = fonction_courante;

			if (!expr->bloc_parent->possede_contexte) {
				auto decl_appel = decl_fonction_appelee;

				if (!decl_appel->est_externe && !decl_appel->possede_drapeau(FORCE_NULCTX)) {
					contexte.rapporte_erreur_fonction_nulctx(expr, decl_fonc, decl_appel);
					return ResultatValidation::Erreur;
				}
			}
		}

		/* ---------------------- */

		if (!candidate->items_monomorphisation.est_vide()) {
			auto [noeud_decl, doit_monomorpher] = monomorphise_au_besoin(contexte, compilatrice, espace, decl_fonction_appelee, std::move(candidate->items_monomorphisation));

			if (doit_monomorpher || !noeud_decl->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
				contexte.unite->attend_sur_declaration(noeud_decl);
				return ResultatValidation::Erreur;
			}

			decl_fonction_appelee = noeud_decl;
		}
		// @concurrence critique
		else if (decl_fonction_appelee->corps->unite == nullptr && !decl_fonction_appelee->est_externe) {
			contexte.m_compilatrice.ordonnanceuse->cree_tache_pour_typage(&espace, decl_fonction_appelee->corps);
		}

		// nous devons monomorpher (ou avoir les types monomorphés) avant de pouvoir faire ça
		auto type_fonc = decl_fonction_appelee->type->comme_fonction();
		auto type_sortie = type_fonc->type_sortie;

		auto expr_gauche = !expr->possede_drapeau(DROITE_ASSIGNATION);
		if (type_sortie->genre != GenreType::RIEN && expr_gauche) {
			rapporte_erreur(&espace, expr, "La valeur de retour de la fonction n'est pas utilisée. Il est important de toujours utiliser les valeurs retournées par les fonctions, par exemple pour ne pas oublier de vérifier si une erreur existe.")
					.ajoute_message("La fonction a été déclarée comme retournant une valeur :\n")
					.ajoute_site(decl_fonction_appelee)
					.ajoute_conseil("si vous ne voulez pas utiliser la valeur de retour, vous pouvez utiliser « _ » comme identifiant pour la capturer et l'ignorer :\n")
					.ajoute_message("\t_ := appel_mais_ignore_le_retour()\n");
			return ResultatValidation::Erreur;
		}

		/* met en place les drapeaux sur les enfants */

		auto nombre_args_simples = static_cast<int>(candidate->exprs.taille());
		auto nombre_args_variadics = nombre_args_simples;

		if (!candidate->exprs.est_vide() && candidate->exprs.back()->genre == GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES) {
			/* ne compte pas le tableau */
			nombre_args_simples -= 1;
			nombre_args_variadics = candidate->transformations.taille();

			/* ajoute le type du tableau */
			auto noeud_tabl = static_cast<NoeudTableauArgsVariadiques *>(candidate->exprs.back());
			auto taille_tableau = noeud_tabl->expressions.taille();
			auto &type_tabl = noeud_tabl->type;

			auto type_tfixe = espace.typeuse.type_tableau_fixe(type_tabl, taille_tableau);
			donnees_dependance.types_utilises.insere(type_tfixe);
		}

		auto i = 0;
		/* les drapeaux pour les arguments simples */
		for (; i < nombre_args_simples; ++i) {
			contexte.transtype_si_necessaire(expr->parametres_resolus[i], candidate->transformations[i]);
		}

		/* les drapeaux pour les arguments variadics */
		if (!candidate->exprs.est_vide() && candidate->exprs.back()->genre == GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES) {
			auto noeud_tableau = static_cast<NoeudTableauArgsVariadiques *>(candidate->exprs.back());

			for (auto j = 0; i < nombre_args_variadics; ++i, ++j) {
				contexte.transtype_si_necessaire(noeud_tableau->expressions[j], candidate->transformations[i]);
			}
		}

		expr->noeud_fonction_appelee = decl_fonction_appelee;

		if (decl_fonction_appelee->est_externe || decl_fonction_appelee->possede_drapeau(FORCE_NULCTX)) {
			expr->drapeaux |= FORCE_NULCTX;
		}

		if (expr->type == nullptr) {
			expr->type = type_sortie;
		}

		donnees_dependance.fonctions_utilisees.insere(decl_fonction_appelee);
	}
	else if (candidate->note == CANDIDATE_EST_CUISSON_FONCTION) {
		auto decl_fonction_appelee = candidate->noeud_decl->comme_entete_fonction();

		if (!candidate->items_monomorphisation.est_vide()) {
			auto [noeud_decl, doit_monomorpher] = monomorphise_au_besoin(contexte, compilatrice, espace, decl_fonction_appelee, std::move(candidate->items_monomorphisation));

			if (doit_monomorpher || !noeud_decl->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
				contexte.unite->attend_sur_declaration(noeud_decl);
				return ResultatValidation::Erreur;
			}

			decl_fonction_appelee = noeud_decl;
		}

		expr->type = decl_fonction_appelee->type;
		expr->appelee = decl_fonction_appelee;
	}
	else if (candidate->note == CANDIDATE_EST_INITIALISATION_STRUCTURE) {
		if (candidate->noeud_decl) {
			auto decl_struct = candidate->noeud_decl->comme_structure();

			auto copie = monomorphise_au_besoin(contexte, espace, decl_struct, std::move(candidate->items_monomorphisation));
			expr->type = espace.typeuse.type_type_de_donnees(copie->type);

			/* il est possible d'utiliser un type avant sa validation final, par exemple en paramètre d'une fonction de rappel qui est membre de la structure */
			if ((copie->type->drapeaux & TYPE_FUT_VALIDE) == 0 && copie->type != contexte.union_ou_structure_courante) {
				// saute l'expression pour ne plus revenir
				contexte.unite->index_courant += 1;
				contexte.unite->attend_sur_type(copie->type);
				return ResultatValidation::Erreur;
			}
		}
		else {
			expr->genre = GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE;
			expr->type = candidate->type;

			for (auto i = 0; i < expr->parametres_resolus.taille(); ++i) {
				if (expr->parametres_resolus[i] != nullptr) {
					contexte.transtype_si_necessaire(expr->parametres_resolus[i], candidate->transformations[i]);
				}
			}

			if (!expr->possede_drapeau(DROITE_ASSIGNATION)) {
				rapporte_erreur(&espace, expr, "La valeur de l'expression de construction de structure n'est pas utilisée. Peut-être vouliez-vous l'assigner à quelque variable ou l'utiliser comme type ?");
				return ResultatValidation::Erreur;
			}
		}
	}
	else if (candidate->note == CANDIDATE_EST_TYPE_POLYMORPHIQUE) {
		expr->type = candidate->type;
	}
	else if (candidate->note == CANDIDATE_EST_APPEL_POINTEUR) {
		if (!candidate->requiers_contexte) {
			expr->drapeaux |= FORCE_NULCTX;
		}

		if (expr->type == nullptr) {
			expr->type = candidate->type->comme_fonction()->type_sortie;
		}

		for (auto i = 0; i < expr->parametres_resolus.taille(); ++i) {
			contexte.transtype_si_necessaire(expr->parametres_resolus[i], candidate->transformations[i]);
		}

		auto expr_gauche = !expr->possede_drapeau(DROITE_ASSIGNATION);
		if (expr->type->genre != GenreType::RIEN && expr_gauche) {
			rapporte_erreur(&espace, expr, "La valeur de retour du pointeur de fonction n'est pas utilisée. Il est important de toujours utiliser les valeurs retournées par les fonctions, par exemple pour ne pas oublier de vérifier si une erreur existe.")
					.ajoute_message("Le type de retour du pointeur de fonctions est : ")
					.ajoute_message(chaine_type(expr->type))
					.ajoute_message("\n")
					.ajoute_conseil("si vous ne voulez pas utiliser la valeur de retour, vous pouvez utiliser « _ » comme identifiant pour la capturer et l'ignorer :\n")
					.ajoute_message("\t_ := appel_mais_ignore_le_retour()\n");
			return ResultatValidation::Erreur;
		}
	}
	else if (candidate->note == CANDIDATE_EST_APPEL_INIT_DE) {
		// le type du retour
		expr->type = espace.typeuse[TypeBase::RIEN];
		expr->drapeaux |= FORCE_NULCTX;
	}
	else if (candidate->note == CANDIDATE_EST_INITIALISATION_OPAQUE) {
		auto type_opaque = candidate->type->comme_opaque();

		if (type_opaque->drapeaux & TYPE_EST_POLYMORPHIQUE) {
			type_opaque = contexte.espace->typeuse.monomorphe_opaque(type_opaque->decl, candidate->exprs[0]->type);
			expr->type = type_opaque;
			expr->aide_generation_code = CONSTRUIT_OPAQUE;
		}
		else {
			expr->type = type_opaque;
			expr->aide_generation_code = CONSTRUIT_OPAQUE;
		}
	}
	else if (candidate->note == CANDIDATE_EST_MONOMORPHISATION_OPAQUE) {
		auto type_opaque = candidate->type->comme_opaque();
		auto type_opacifie = candidate->exprs[0]->type->comme_type_de_donnees();

		/* différencie entre Type($T) et Type(T) où T dans le deuxième cas est connu */
		if ((type_opacifie->type_connu->drapeaux & TYPE_EST_POLYMORPHIQUE) == 0) {
			type_opaque = contexte.espace->typeuse.monomorphe_opaque(type_opaque->decl, type_opacifie->type_connu);
		}

		expr->type = contexte.espace->typeuse.type_type_de_donnees(type_opaque);
		expr->aide_generation_code = MONOMORPHE_TYPE_OPAQUE;
	}

#ifdef STATISTIQUES_DETAILLEES
	possede_erreur = false;
#endif

	assert(expr->type);
	return ResultatValidation::OK;
}
