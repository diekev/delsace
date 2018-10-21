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
#include "nombres.h"

enum class dir_associativite : int {
	GAUCHE,
	DROITE,
};

struct DonneesPrecedence {
	dir_associativite direction;
	int priorite;
};

static DonneesPrecedence associativite(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::EGAL:
			return { dir_associativite::GAUCHE, 0 };
		case id_morceau::BARRE_BARRE:
			return { dir_associativite::GAUCHE, 1 };
		case id_morceau::ESP_ESP:
			return { dir_associativite::GAUCHE, 2 };
		case id_morceau::BARRE:
			return { dir_associativite::GAUCHE, 3 };
		case id_morceau::CHAPEAU:
			return { dir_associativite::GAUCHE, 4 };
		case id_morceau::ESPERLUETTE:
			return { dir_associativite::GAUCHE, 5 };
		case id_morceau::DIFFERENCE:
		case id_morceau::EGALITE:
			return { dir_associativite::GAUCHE, 6 };
		case id_morceau::INFERIEUR:
		case id_morceau::INFERIEUR_EGAL:
		case id_morceau::SUPERIEUR:
		case id_morceau::SUPERIEUR_EGAL:
			return { dir_associativite::GAUCHE, 7 };
		case id_morceau::DECALAGE_GAUCHE:
		case id_morceau::DECALAGE_DROITE:
			return { dir_associativite::GAUCHE, 8 };
		case id_morceau::PLUS:
		case id_morceau::MOINS:
			return { dir_associativite::GAUCHE, 9 };
		case id_morceau::FOIS:
		case id_morceau::DIVISE:
		case id_morceau::POURCENT:
			return { dir_associativite::GAUCHE, 10 };
		case id_morceau::EXCLAMATION:
		case id_morceau::TILDE:
		case id_morceau::AROBASE:
		case id_morceau::DE:
			return { dir_associativite::DROITE, 11 };
		case id_morceau::CROCHET_OUVRANT:
			return { dir_associativite::GAUCHE, 12 };
		default:
			assert(false);
			return { static_cast<dir_associativite>(-1), -1 };
	}
}

bool precedence_faible(id_morceau identifiant1, id_morceau identifiant2)
{
	auto p1 = associativite(identifiant1);
	auto p2 = associativite(identifiant2);

	return (p1.direction == dir_associativite::GAUCHE && p1.priorite <= p2.priorite)
			|| ((p2.direction == dir_associativite::DROITE) && (p1.priorite < p2.priorite));
}

/* ************************************************************************** */

static bool est_operation_arithmetique(id_morceau op)
{
	switch (op) {
		case id_morceau::PLUS:
		case id_morceau::MOINS:
		case id_morceau::FOIS:
		case id_morceau::DIVISE:
		case id_morceau::POURCENT:
		case id_morceau::DECALAGE_DROITE:
		case id_morceau::DECALAGE_GAUCHE:
		case id_morceau::ESPERLUETTE:
		case id_morceau::BARRE:
		case id_morceau::CHAPEAU:
			return true;
		default:
			return false;
	}
}

static bool est_operation_arithmetique_reel(id_morceau op)
{
	switch (op) {
		case id_morceau::PLUS:
		case id_morceau::MOINS:
		case id_morceau::FOIS:
		case id_morceau::DIVISE:
			return true;
		default:
			return false;
	}
}

static bool est_operation_comparaison(id_morceau op)
{
	switch (op) {
		case id_morceau::INFERIEUR:
		case id_morceau::INFERIEUR_EGAL:
		case id_morceau::SUPERIEUR:
		case id_morceau::SUPERIEUR_EGAL:
		case id_morceau::EGALITE:
		case id_morceau::DIFFERENCE:
			return true;
		default:
			return false;
	}
}

static bool est_operation_booleenne(id_morceau op)
{
	switch (op) {
		case id_morceau::ESP_ESP:
		case id_morceau::BARRE_BARRE:
			return true;
		default:
			return false;
	}
}

static long calcul_expression_nombre_entier(id_morceau op, long n1, long n2)
{
	switch (op) {
		case id_morceau::PLUS:
			return n1 + n2;
		case id_morceau::MOINS:
			return n1 - n2;
		case id_morceau::FOIS:
			return n1 * n2;
		case id_morceau::DIVISE:
			if (n2 == 0) {
				return 0;
			}

			return n1 / n2;
		case id_morceau::ESPERLUETTE:
			return n1 & n2;
		case id_morceau::POURCENT:
			return n1 % n2;
		case id_morceau::DECALAGE_DROITE:
			return n1 >> n2;
		case id_morceau::DECALAGE_GAUCHE:
			return n1 << n2;
		case id_morceau::BARRE:
			return n1 | n2;
		case id_morceau::CHAPEAU:
			return n1 ^ n2;
		default:
			return 0;
	}
}

static double calcul_expression_nombre_reel(id_morceau op, double n1, double n2)
{
	switch (op) {
		case id_morceau::PLUS:
			return n1 + n2;
		case id_morceau::MOINS:
			return n1 - n2;
		case id_morceau::FOIS:
			return n1 * n2;
		case id_morceau::DIVISE:
			if (n2 == 0.0) {
				return 0;
			}

			return n1 / n2;
		default:
			return 0;
	}
}

template <typename T>
static bool calcul_expression_comparaison(id_morceau op, T n1, T n2)
{
	switch (op) {
		case id_morceau::INFERIEUR:
			return n1 < n2;
		case id_morceau::INFERIEUR_EGAL:
			return n1 <= n2;
		case id_morceau::SUPERIEUR:
			return n1 > n2;
		case id_morceau::SUPERIEUR_EGAL:
			return n1 >= n2;
		case id_morceau::DIFFERENCE:
			if constexpr (std::is_floating_point<T>::value) {
				return std::abs(n1 - n2) > std::numeric_limits<T>::epsilon();
			}
			else {
				return n1 != n2;
			}
		case id_morceau::EGALITE:
			if constexpr (std::is_floating_point<T>::value) {
				return std::abs(n1 - n2) <= std::numeric_limits<T>::epsilon();
			}
			else {
				return n1 == n2;
			}
		default:
			return false;
	}
}

template <typename T>
static bool calcul_expression_boolenne(id_morceau op, T n1, T n2)
{
	switch (op) {
		case id_morceau::ESP_ESP:
			return n1 && n2;
		case id_morceau::BARRE_BARRE:
			return n1 || n2;
		default:
			return false;
	}
}

static bool sont_compatibles(id_morceau id1, id_morceau id2)
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

	if (n1->identifiant() == id_morceau::NOMBRE_REEL) {
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
			auto noeud = assembleuse.cree_noeud(type_noeud::BOOLEEN, { "", 0ul, id_morceau::BOOL });
			noeud->valeur_boolenne = calcul_expression_comparaison(op->identifiant(), v1, v2);
			noeud->calcule = true;

			assembleuse.supprime_noeud(op);
			assembleuse.supprime_noeud(n1);
			assembleuse.supprime_noeud(n2);

			return noeud;
		}

		return nullptr;
	}

	if (n1->identifiant() == id_morceau::NOMBRE_ENTIER) {
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
			auto noeud = assembleuse.cree_noeud(type_noeud::BOOLEEN, { "", 0ul, id_morceau::BOOL });
			noeud->valeur_boolenne = calcul_expression_comparaison(op->identifiant(), v1, v2);
			noeud->calcule = true;

			assembleuse.supprime_noeud(op);
			assembleuse.supprime_noeud(n1);
			assembleuse.supprime_noeud(n2);

			return noeud;
		}

		if (est_operation_booleenne(op->identifiant())) {
			auto noeud = assembleuse.cree_noeud(type_noeud::BOOLEEN, { "", 0ul, id_morceau::BOOL });
			noeud->valeur_boolenne = calcul_expression_boolenne(op->identifiant(), v1, v2);
			noeud->calcule = true;

			assembleuse.supprime_noeud(op);
			assembleuse.supprime_noeud(n1);
			assembleuse.supprime_noeud(n2);

			return noeud;
		}

		return nullptr;
	}

	if (n1->identifiant() == id_morceau::BOOL) {
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
	auto res = assembleuse.cree_noeud(type_noeud::NOMBRE_ENTIER, {});
	assembleuse.supprime_noeud(op);
	assembleuse.supprime_noeud(n1);
#endif
	op->ajoute_noeud(n1);
	return op;
}
