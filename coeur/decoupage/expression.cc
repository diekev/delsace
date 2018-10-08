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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "expression.h"

#include "arbre_syntactic.h"
#include "assembleuse_arbre.h"
#include "erreur.h"
#include "morceaux.h"
#include "nombres.h"

enum {
	GAUCHE,
	DROITE,
};

struct DonneesPrecedence {
	int direction;
	int priorite;
};

static DonneesPrecedence associativite(int identifiant)
{
	switch (identifiant) {
		case ID_BARRE_BARRE:
			return { GAUCHE, 0 };
		case ID_ESP_ESP:
			return { GAUCHE, 1 };
		case ID_BARRE:
			return { GAUCHE, 2};
		case ID_CHAPEAU:
			return { GAUCHE, 3};
		case ID_ESPERLUETTE:
			return { GAUCHE, 4};
		case ID_DIFFERENCE:
		case ID_EGALITE:
			return { GAUCHE, 5};
		case ID_INFERIEUR:
		case ID_INFERIEUR_EGAL:
		case ID_SUPERIEUR:
		case ID_SUPERIEUR_EGAL:
			return { GAUCHE, 6};
		case ID_DECALAGE_GAUCHE:
		case ID_DECALAGE_DROITE:
			return { GAUCHE, 7};
		case ID_PLUS:
		case ID_MOINS:
			return { GAUCHE, 8};
		case ID_FOIS:
		case ID_DIVISE:
		case ID_POURCENT:
			return { GAUCHE, 9};
		case ID_EXCLAMATION:
		case ID_TILDE:
		case ID_AROBASE:
			return { DROITE, 10 };
		case ID_CROCHET_OUVRANT:
			return { GAUCHE, 11 };
	}

	return { GAUCHE, 0 };
}

bool precedence_faible(int identifiant1, int identifiant2)
{
	auto p1 = associativite(identifiant1);
	auto p2 = associativite(identifiant2);

	return (p1.direction == GAUCHE && p1.priorite <= p2.priorite)
			|| ((p2.direction == DROITE) && (p1.priorite < p2.priorite));
}

/* ************************************************************************** */

static bool est_operation_arithmetique(int op)
{
	switch (op) {
		case ID_PLUS:
		case ID_MOINS:
		case ID_FOIS:
		case ID_DIVISE:
		case ID_POURCENT:
		case ID_DECALAGE_DROITE:
		case ID_DECALAGE_GAUCHE:
		case ID_ESPERLUETTE:
		case ID_BARRE:
		case ID_CHAPEAU:
			return true;
		default:
			return false;
	}
}

static bool est_operation_arithmetique_reel(int op)
{
	switch (op) {
		case ID_PLUS:
		case ID_MOINS:
		case ID_FOIS:
		case ID_DIVISE:
			return true;
		default:
			return false;
	}
}

static bool est_operation_comparaison(int op)
{
	switch (op) {
		case ID_INFERIEUR:
		case ID_INFERIEUR_EGAL:
		case ID_SUPERIEUR:
		case ID_SUPERIEUR_EGAL:
		case ID_EGALITE:
		case ID_DIFFERENCE:
			return true;
		default:
			return false;
	}
}

static bool est_operation_booleenne(int op)
{
	switch (op) {
		case ID_ESP_ESP:
		case ID_BARRE_BARRE:
			return true;
		default:
			return false;
	}
}

static long calcul_expression_nombre_entier(int op, long n1, long n2)
{
	switch (op) {
		case ID_PLUS:
			return n1 + n2;
		case ID_MOINS:
			return n1 - n2;
		case ID_FOIS:
			return n1 * n2;
		case ID_DIVISE:
			if (n2 == 0) {
				return 0;
			}

			return n1 / n2;
		case ID_ESPERLUETTE:
			return n1 & n2;
		case ID_POURCENT:
			return n1 % n2;
		case ID_DECALAGE_DROITE:
			return n1 >> n2;
		case ID_DECALAGE_GAUCHE:
			return n1 << n2;
		case ID_BARRE:
			return n1 | n2;
		case ID_CHAPEAU:
			return n1 ^ n2;
		default:
			return 0;
	}
}

static double calcul_expression_nombre_reel(int op, double n1, double n2)
{
	switch (op) {
		case ID_PLUS:
			return n1 + n2;
		case ID_MOINS:
			return n1 - n2;
		case ID_FOIS:
			return n1 * n2;
		case ID_DIVISE:
			if (n2 == 0.0) {
				return 0;
			}

			return n1 / n2;
		default:
			return 0;
	}
}

template <typename T>
static bool calcul_expression_comparaison(int op, T n1, T n2)
{
	switch (op) {
		case ID_INFERIEUR:
			return n1 < n2;
		case ID_INFERIEUR_EGAL:
			return n1 <= n2;
		case ID_SUPERIEUR:
			return n1 > n2;
		case ID_SUPERIEUR_EGAL:
			return n1 >= n2;
		case ID_DIFFERENCE:
			return n1 != n2;
		case ID_EGALITE:
			return n1 == n2;
		default:
			return false;
	}
}

template <typename T>
static bool calcul_expression_boolenne(int op, T n1, T n2)
{
	switch (op) {
		case ID_ESP_ESP:
			return n1 && n2;
		case ID_BARRE_BARRE:
			return n1 || n2;
		default:
			return false;
	}
}

static bool sont_compatibles(int id1, int id2)
{
	return id1 == id2;
}

static inline long extrait_nombre_entier(Noeud *n)
{
	return n->calcule ? n->valeur_entiere : converti_chaine_nombre_entier(n->chaine(), n->identifiant());
}

static inline double extrait_nombre_reel(Noeud *n)
{
	return n->calcule ? n->valeur_reelle : converti_chaine_nombre_reel(n->chaine(), n->identifiant());
}

static inline bool extrait_valeur_bool(Noeud *n)
{
	return n->calcule ? n->valeur_boolenne : (n->chaine() == "vrai");
}

Noeud *calcul_expression_double(assembleuse_arbre &assembleuse, Noeud *op, Noeud *n1, Noeud *n2)
{
	if (!sont_compatibles(n1->identifiant(), n2->identifiant())) {
		return nullptr;
	}

	if (n1->identifiant() == ID_NOMBRE_REEL) {
		auto v1 = extrait_nombre_reel(n1);
		auto v2 = extrait_nombre_reel(n2);

		if (est_operation_arithmetique_reel(op->identifiant())) {
			n1->valeur_reelle = calcul_expression_nombre_reel(op->identifiant(), v1, v2);
			n1->calcule = true;

			assembleuse.supprime_noeud(op);
			assembleuse.supprime_noeud(n2);

			return n1;
		}

		if (est_operation_comparaison(op->identifiant())) {
			auto noeud = assembleuse.cree_noeud(NOEUD_BOOLEEN, { "", 0ul, ID_BOOL });
			noeud->valeur_boolenne = calcul_expression_comparaison(op->identifiant(), v1, v2);
			noeud->calcule = true;

			assembleuse.supprime_noeud(op);
			assembleuse.supprime_noeud(n1);
			assembleuse.supprime_noeud(n2);

			return noeud;
		}

		return nullptr;
	}

	if (n1->identifiant() == ID_NOMBRE_ENTIER) {
		auto v1 = extrait_nombre_entier(n1);
		auto v2 = extrait_nombre_entier(n2);

		if (est_operation_arithmetique(op->identifiant())) {
			n1->valeur_entiere = calcul_expression_nombre_entier(op->identifiant(), v1, v2);
			n1->calcule = true;

			assembleuse.supprime_noeud(op);
			assembleuse.supprime_noeud(n2);

			return n1;
		}

		if (est_operation_comparaison(op->identifiant())) {
			auto noeud = assembleuse.cree_noeud(NOEUD_BOOLEEN, { "", 0ul, ID_BOOL });
			noeud->valeur_boolenne = calcul_expression_comparaison(op->identifiant(), v1, v2);
			noeud->calcule = true;

			assembleuse.supprime_noeud(op);
			assembleuse.supprime_noeud(n1);
			assembleuse.supprime_noeud(n2);

			return noeud;
		}

		if (est_operation_booleenne(op->identifiant())) {
			auto noeud = assembleuse.cree_noeud(NOEUD_BOOLEEN, { "", 0ul, ID_BOOL });
			noeud->valeur_boolenne = calcul_expression_boolenne(op->identifiant(), v1, v2);
			noeud->calcule = true;

			assembleuse.supprime_noeud(op);
			assembleuse.supprime_noeud(n1);
			assembleuse.supprime_noeud(n2);

			return noeud;
		}

		return nullptr;
	}

	if (n1->identifiant() == ID_BOOL) {
		auto v1 = extrait_valeur_bool(n1);
		auto v2 = extrait_valeur_bool(n2);

		if (est_operation_comparaison(op->identifiant())) {
			n1->valeur_boolenne = calcul_expression_comparaison(op->identifiant(), v1, v2);
			n1->calcule = true;

			assembleuse.supprime_noeud(op);
			assembleuse.supprime_noeud(n2);

			return n1;
		}

		if (est_operation_booleenne(op->identifiant())) {
			n1->valeur_boolenne = calcul_expression_boolenne(op->identifiant(), v1, v2);
			n1->calcule = true;

			assembleuse.supprime_noeud(op);
			assembleuse.supprime_noeud(n2);

			return n1;
		}
	}

	return nullptr;
}

/* ************************************************************************** */

Noeud *calcul_expression_simple(assembleuse_arbre &assembleuse, Noeud *op, Noeud *n1)
{
#if 0
	auto res = assembleuse.cree_noeud(NOEUD_NOMBRE_ENTIER, {});
	assembleuse.supprime_noeud(op);
	assembleuse.supprime_noeud(n1);
#endif
	op->ajoute_noeud(n1);
	return op;
}
