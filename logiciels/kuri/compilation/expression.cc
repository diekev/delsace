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

#include "biblinternes/langage/nombres.hh"
#include "biblinternes/outils/conditions.h"

#include "assembleuse_arbre.h"
#include "contexte_generation_code.h"
#include "outils_morceaux.hh"

using denombreuse = lng::decoupeuse_nombre<id_morceau>;

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
		case id_morceau::TROIS_POINTS:
			return { dir_associativite::GAUCHE, 0 };
		case id_morceau::EGAL:
		case id_morceau::PLUS_EGAL:
		case id_morceau::MOINS_EGAL:
		case id_morceau::DIVISE_EGAL:
		case id_morceau::MULTIPLIE_EGAL:
		case id_morceau::MODULO_EGAL:
		case id_morceau::ET_EGAL:
		case id_morceau::OU_EGAL:
		case id_morceau::OUX_EGAL:
		case id_morceau::DEC_DROITE_EGAL:
		case id_morceau::DEC_GAUCHE_EGAL:
			return { dir_associativite::GAUCHE, 1 };
		case id_morceau::VIRGULE:
			return { dir_associativite::GAUCHE, 2 };
		case id_morceau::BARRE_BARRE:
			return { dir_associativite::GAUCHE, 3 };
		case id_morceau::ESP_ESP:
			return { dir_associativite::GAUCHE, 4 };
		case id_morceau::BARRE:
			return { dir_associativite::GAUCHE, 5 };
		case id_morceau::CHAPEAU:
			return { dir_associativite::GAUCHE, 6 };
		case id_morceau::ESPERLUETTE:
			return { dir_associativite::GAUCHE, 7 };
		case id_morceau::DIFFERENCE:
		case id_morceau::EGALITE:
			return { dir_associativite::GAUCHE, 8 };
		case id_morceau::INFERIEUR:
		case id_morceau::INFERIEUR_EGAL:
		case id_morceau::SUPERIEUR:
		case id_morceau::SUPERIEUR_EGAL:
			return { dir_associativite::GAUCHE, 9 };
		case id_morceau::DECALAGE_GAUCHE:
		case id_morceau::DECALAGE_DROITE:
			return { dir_associativite::GAUCHE, 10 };
		case id_morceau::PLUS:
		case id_morceau::MOINS:
			return { dir_associativite::GAUCHE, 11 };
		case id_morceau::FOIS:
		case id_morceau::DIVISE:
		case id_morceau::POURCENT:
			return { dir_associativite::GAUCHE, 12 };
		case id_morceau::EXCLAMATION:
		case id_morceau::TILDE:
		case id_morceau::AROBASE:
		case id_morceau::PLUS_UNAIRE:
		case id_morceau::MOINS_UNAIRE:
			return { dir_associativite::DROITE, 13 };
		case id_morceau::POINT:
		case id_morceau::CROCHET_OUVRANT:
			return { dir_associativite::GAUCHE, 14 };
		/* NOTE : pour l'instant ceci est désactivé, car la logique de
		 * correction de l'arbre syntactic est capillotractée et decevante.
		 *
		 * NOTE : 'de' et '[]' doivent avoir la même précédence pour pouvoir
		 * être dans le bon ordre quand nous avons des expressions du style :
		 * @tampon[0] de indir[0] de x;
		 * mais il faudra tout de même inverser leurs premiers enfants pour que
		 * l'arbre syntactic soit correcte ; voir l'inversion dans l'analyse
		 * sémantique de '[]'. */
		case id_morceau::DE:
#if 0
		case id_morceau::CROCHET_OUVRANT:
#endif
			return { dir_associativite::DROITE, 15 };
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

template <typename T>
static auto applique_operateur_unaire(id_morceau id, T &a)
{
	switch (id) {
		case id_morceau::EXCLAMATION:
		{
			a = !a;
			break;
		}
		case id_morceau::TILDE:
		{
			a = ~a;
			break;
		}
		case id_morceau::PLUS_UNAIRE:
		{
			break;
		}
		case id_morceau::MOINS_UNAIRE:
		{
			a = -a;
			break;
		}
		default:
		{
			a = 0;
			break;
		}
	}
}

static auto applique_operateur_unaire(id_morceau id, double &a)
{
	switch (id) {
		case id_morceau::PLUS_UNAIRE:
		{
			break;
		}
		case id_morceau::MOINS_UNAIRE:
		{
			a = -a;
			break;
		}
		default:
		{
			a = 0;
			break;
		}
	}
}

template <typename T>
static auto applique_operateur_binaire(id_morceau id, T a, T b)
{
	switch (id) {
		case id_morceau::PLUS:
		case id_morceau::PLUS_EGAL:
		{
			return a + b;
		}
		case id_morceau::MOINS:
		case id_morceau::MOINS_EGAL:
		{
			return a - b;
		}
		case id_morceau::FOIS:
		case id_morceau::MULTIPLIE_EGAL:
		{
			return a * b;
		}
		case id_morceau::DIVISE:
		case id_morceau::DIVISE_EGAL:
		{
			return a / b;
		}
		case id_morceau::POURCENT:
		case id_morceau::MODULO_EGAL:
		{
			return a % b;
		}
		case id_morceau::ESPERLUETTE:
		case id_morceau::ET_EGAL:
		{
			return a & b;
		}
		case id_morceau::OU_EGAL:
		case id_morceau::BARRE:
		{
			return a | b;
		}
		case id_morceau::CHAPEAU:
		case id_morceau::OUX_EGAL:
		{
			return a ^ b;
		}
		case id_morceau::DECALAGE_DROITE:
		case id_morceau::DEC_DROITE_EGAL:
		{
			return a >> b;
		}
		case id_morceau::DECALAGE_GAUCHE:
		case id_morceau::DEC_GAUCHE_EGAL:
		{
			return a << b;
		}
		default:
		{
			return T(0);
		}
	}
}

static auto applique_operateur_binaire(id_morceau id, double a, double b)
{
	switch (id) {
		case id_morceau::PLUS:
		case id_morceau::PLUS_EGAL:
		{
			return a + b;
		}
		case id_morceau::MOINS:
		case id_morceau::MOINS_EGAL:
		{
			return a - b;
		}
		case id_morceau::FOIS:
		case id_morceau::MULTIPLIE_EGAL:
		{
			return a * b;
		}
		case id_morceau::DIVISE:
		case id_morceau::DIVISE_EGAL:
		{
			return a / b;
		}
		default:
		{
			return 0.0;
		}
	}
}

template <typename T>
static auto applique_operateur_binaire_comp(id_morceau id, T a, T b)
{
	switch (id) {
		case id_morceau::INFERIEUR:
		case id_morceau::INFERIEUR_EGAL:
		{
			return a < b;
		}
		case id_morceau::SUPERIEUR:
		case id_morceau::SUPERIEUR_EGAL:
		{
			return a > b;
		}
		case id_morceau::DIFFERENCE:
		{
			return a != b;
		}
		case id_morceau::ESP_ESP:
		{
			return a && b;
		}
		case id_morceau::EGALITE:
		{
			return a == b;
		}
		case id_morceau::BARRE_BARRE:
		{
			return a || b;
		}
		default:
		{
			return false;
		}
	}
}

/**
 * Évalue l'expression dont « b » est la racine. L'expression doit être
 * constante, c'est à dire ne contenir que des noeuds dont la valeur est connue
 * lors de la compilation.
 *
 * Dans le future, ce sera sans doute la base d'un interpreteur pour exécuter de
 * manière arbitraire du code lors de la compilation. Pour cela, la prochaine
 * étape sera de pouvoir évaluer des fonctions entières.
 */
ResultatExpression evalue_expression(ContexteGenerationCode &contexte, noeud::base *b)
{
	switch (b->type) {
		default:
		{
			auto res = ResultatExpression();
			res.est_errone = true;
			res.noeud_erreur = b;
			res.message_erreur = "L'expression n'est pas constante et ne peut être calculée !";

			return res;
		}
		case type_noeud::TAILLE_DE:
		{
			auto index_dt = std::any_cast<long>(b->valeur_calculee);
			auto const &donnees = contexte.magasin_types.donnees_types[index_dt];

			auto res = ResultatExpression();
			res.type = type_expression::ENTIER;
			res.entier = taille_type_octet(contexte, donnees);

			return res;
		}
		case type_noeud::BOOLEEN:
		{
			auto res = ResultatExpression();
			res.type = type_expression::ENTIER;
			res.entier = b->chaine() == "vrai";

			return res;
		}
		case type_noeud::NOMBRE_ENTIER:
		{
			auto res = ResultatExpression();
			res.type = type_expression::ENTIER;
			res.entier = lng::converti_nombre_entier(dls::vue_chaine(b->chaine().pointeur(), b->chaine().taille()));

			return res;
		}
		case type_noeud::NOMBRE_REEL:
		{
			auto res = ResultatExpression();
			res.type = type_expression::REEL;
			res.reel = lng::converti_nombre_reel(dls::vue_chaine(b->chaine().pointeur(), b->chaine().taille()));

			return res;
		}
		case type_noeud::SAUFSI:
		case type_noeud::SI:
		{
			auto const nombre_enfants = b->enfants.taille();
			auto iter_enfant = b->enfants.debut();

			auto enfant1 = *iter_enfant++;

			auto res = evalue_expression(contexte, enfant1);

			if (res.est_errone) {
				return res;
			}

			if (res.type != type_expression::ENTIER) {
				res.est_errone = true;
				res.noeud_erreur = b;
				res.message_erreur = "L'expression n'est pas de type booléen !";
				return res;
			}

			auto enfant2 = *iter_enfant++;

			if (res.condition == (b->type == type_noeud::SI)) {
				res = evalue_expression(contexte, enfant2);
			}
			else {
				if (nombre_enfants == 3) {
					auto enfant3 = *iter_enfant++;
					res = evalue_expression(contexte, enfant3);
				}
			}

			return res;
		}
		case type_noeud::OPERATION_UNAIRE:
		{
			auto res = evalue_expression(contexte, b->enfants.front());

			if (res.est_errone) {
				return res;
			}

			if (res.type == type_expression::REEL) {
				applique_operateur_unaire(b->identifiant(), res.reel);
			}
			else {
				applique_operateur_unaire(b->identifiant(), res.entier);
			}

			return res;
		}
		case type_noeud::OPERATION_BINAIRE:
		{
			auto res1 = evalue_expression(contexte, b->enfants.front());

			if (res1.est_errone) {
				return res1;
			}

			auto res2 = evalue_expression(contexte, b->enfants.back());

			if (res2.est_errone) {
				return res2;
			}

			auto res = ResultatExpression();
			res.type = res1.type;

			if (est_operateur_bool(b->identifiant())) {
				if (res.type == type_expression::REEL) {
					res.condition = applique_operateur_binaire_comp(b->identifiant(), res1.reel, res2.reel);
				}
				else {
					res.condition = applique_operateur_binaire_comp(b->identifiant(), res1.entier, res2.entier);
				}
			}
			else {
				if (res.type == type_expression::REEL) {
					res.reel = applique_operateur_binaire(b->identifiant(), res1.reel, res2.reel);
				}
				else {
					res.entier = applique_operateur_binaire(b->identifiant(), res1.entier, res2.entier);
				}
			}

			return res;
		}
	}
}
