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

#pragma once

#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/tablet.hh"

#include "structures/chaine_statique.hh"
#include "structures/tableau.hh"

#include "transformation_type.hh"

struct Compilatrice;
struct ContexteValidationCode;
struct EspaceDeTravail;
struct IdentifiantCode;
struct ItemMonomorphisation;
struct NoeudDeclaration;
struct NoeudExpression;
struct NoeudExpressionAppel;
struct Type;

enum class ResultatValidation : int;

struct IdentifiantEtExpression {
	IdentifiantCode *ident;
	NoeudExpression *expr_ident;
	NoeudExpression *expr;
};

enum {
	FONCTION_TROUVEE,
	FONCTION_INTROUVEE,
};

enum {
	AUCUNE_RAISON,

	EXPRESSION_MANQUANTE_POUR_UNION,
	MANQUE_NOM_APRES_VARIADIC,
	ARGUMENTS_MANQUANTS,
	MECOMPTAGE_ARGS,
	MENOMMAGE_ARG,
	METYPAGE_ARG,
	NOMMAGE_ARG_POINTEUR_FONCTION,
	RENOMMAGE_ARG,
	TROP_D_EXPRESSION_POUR_UNION,
	TYPE_N_EST_PAS_FONCTION,
	CONTEXTE_MANQUANT,
	EXPANSION_VARIADIQUE_FONCTION_EXTERNE,
	MULTIPLE_EXPANSIONS_VARIADIQUES,
	EXPANSION_VARIADIQUE_APRES_ARGUMENTS_VARIADIQUES,
	ARGUMENTS_VARIADIQEUS_APRES_EXPANSION_VARIAQUES,
	IMPOSSIBLE_DE_DEFINIR_UN_TYPE_POLYMORPHIQUE,
};

enum {
	NOTE_INVALIDE,
	CANDIDATE_EST_APPEL_FONCTION,
	CANDIDATE_EST_CUISSON_FONCTION,
	CANDIDATE_EST_APPEL_POINTEUR,
	CANDIDATE_EST_INITIALISATION_STRUCTURE,
	CANDIDATE_EST_TYPE_POLYMORPHIQUE,
	CANDIDATE_EST_APPEL_INIT_DE,
	CANDIDATE_EST_INITIALISATION_OPAQUE,
	CANDIDATE_EST_MONOMORPHISATION_OPAQUE,
};

struct DonneesCandidate {
	int etat = FONCTION_INTROUVEE;
	int raison = AUCUNE_RAISON;
	double poids_args = 0.0;
	kuri::chaine_statique nom_arg{};

	/* Ce que nous avons à gauche */
	int note = NOTE_INVALIDE;

	bool requiers_contexte = true;
	REMBOURRE(3);

	/* Le type de l'élément à gauche de l'expression (pour les structures et les pointeurs de fonctions) */
	Type *type = nullptr;

	/* les expressions remises dans l'ordre selon les noms, si la fonction est trouvée. */
	dls::tablet<NoeudExpression *, 10> exprs{};
	dls::tablet<IdentifiantCode *, 10> arguments_manquants{};
	Type *type_attendu{};
	Type *type_obtenu{};
	NoeudExpression const *noeud_erreur = nullptr;
	NoeudDeclaration *noeud_decl = nullptr;
	kuri::tableau<TransformationType, int> transformations{};
	kuri::tableau<ItemMonomorphisation, int> items_monomorphisation{};

	IdentifiantCode *ident_poly_manquant = nullptr;

	POINTEUR_NUL(DonneesCandidate)

	DonneesCandidate() = default;

	DonneesCandidate(DonneesCandidate const &) = default;

	DonneesCandidate(DonneesCandidate &&autre)
	{
		this->permute(autre);
	}

	DonneesCandidate &operator=(DonneesCandidate const &) = default;

	DonneesCandidate &operator=(DonneesCandidate &&autre)
	{
		this->permute(autre);
		return *this;
	}

	void permute(DonneesCandidate &autre)
	{
		if (this == &autre) {
			return;
		}

		std::swap(etat, autre.etat);
		std::swap(raison, autre.raison);
		std::swap(poids_args, autre.poids_args);
		std::swap(nom_arg, autre.nom_arg);
		std::swap(note, autre.note);
		std::swap(requiers_contexte, autre.requiers_contexte);
		std::swap(type, autre.type);
		std::swap(type_attendu, autre.type_attendu);
		std::swap(type_obtenu, autre.type_obtenu);
		std::swap(noeud_erreur, autre.noeud_erreur);
		std::swap(noeud_decl, autre.noeud_decl);
		std::swap(ident_poly_manquant, autre.ident_poly_manquant);
		exprs.permute(autre.exprs);
		arguments_manquants.permute(autre.arguments_manquants);
		transformations.permute(autre.transformations);
		items_monomorphisation.permute(autre.items_monomorphisation);
	}
};

ResultatValidation valide_appel_fonction(
		Compilatrice &compilatrice,
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte_validation,
		NoeudExpressionAppel *expr);
