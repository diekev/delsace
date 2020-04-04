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

#include "biblinternes/structures/tableau.hh"
#include "biblinternes/structures/tablet.hh"
#include "biblinternes/structures/vue_chaine_compacte.hh"

#include "contexte_generation_code.h"
#include "structures.hh"

struct ContexteGenerationCode;
struct NoeudBase;
struct NoeudDeclarationFonction;
struct NoeudExpression;
struct NoeudExpressionAppel;
struct Type;

struct IdentifiantEtExpression {
	IdentifiantCode *ident;
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
	MECOMPTAGE_ARGS,
	MENOMMAGE_ARG,
	METYPAGE_ARG,
	NOMMAGE_ARG_POINTEUR_FONCTION,
	NOM_ARGUMENT_REQUIS,
	RENOMMAGE_ARG,
	TROP_D_EXPRESSION_POUR_UNION,
	TYPE_N_EST_PAS_FONCTION,
};

enum {
	NOTE_INVALIDE,
	CANDIDATE_EST_APPEL_FONCTION,
	CANDIDATE_EST_APPEL_POINTEUR,
	CANDIDATE_EST_INITIALISATION_STRUCTURE,
};

struct DonneesCandidate {
	int etat = FONCTION_INTROUVEE;
	int raison = AUCUNE_RAISON;
	double poids_args = 0.0;
	dls::vue_chaine_compacte nom_arg{};

	/* Ce que nous avons à gauche */
	int note = NOTE_INVALIDE;

	bool requiers_contexte = true;
	REMBOURRE(3);

	/* Le type de l'élément à gauche de l'expression (pour les structures et les pointeurs de fonctions) */
	Type *type = nullptr;

	/* les expressions remises dans l'ordre selon les noms, si la fonction est trouvée. */
	dls::tablet<NoeudExpression *, 10> exprs{};
	Type *type1{};
	Type *type2{};
	NoeudBase const *noeud_decl = nullptr;
	NoeudDeclaration const *decl_fonc = nullptr;
	dls::tableau<TransformationType> transformations{};
	dls::tableau<std::pair<dls::vue_chaine_compacte, Type *>> paires_expansion_gabarit{};
};

struct ResultatRecherche {
	dls::tablet<DonneesCandidate, 10> candidates{};
};

ResultatRecherche trouve_candidates_pour_appel(
		ContexteGenerationCode &contexte,
		NoeudExpressionAppel *expr,
		kuri::tableau<IdentifiantEtExpression> &args);

void valide_appel_fonction(
		ContexteGenerationCode &contexte,
		NoeudExpressionAppel *expr,
		bool expr_gauche);
