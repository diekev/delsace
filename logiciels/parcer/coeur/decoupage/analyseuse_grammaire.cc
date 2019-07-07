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

#include "analyseuse_grammaire.hh"

#include "biblinternes/chrono/outils.hh"
#include <iostream>

#include "arbre_syntactic.hh"
#include "contexte_generation_code.hh"
#include "erreur.hh"
#include "expression.hh"
#include "modules.hh"
#include "nombres.hh"

#undef DEBOGUE_EXPRESSION

#ifdef DEBOGUE_EXPRESSION
static constexpr auto g_log_expression = true;
#else
static constexpr auto g_log_expression = false;
#endif

#define DEB_LOG_EXPRESSION if (g_log_expression) { std::cerr
#define FIN_LOG_EXPRESSION '\n';}

/**
 * Limitation du nombre récursif de sous-expressions (par exemple :
 * f(g(h(i(j()))))).
 */
static constexpr auto PROFONDEUR_EXPRESSION_MAX = 32;

/* Tabulations utilisées au début des logs. */
static const char *tabulations[PROFONDEUR_EXPRESSION_MAX] = {
	"",
	" ",
	"  ",
	"   ",
	"    ",
	"     ",
	"      ",
	"       ",
	"        ",
	"         ",
	"          ",
	"           ",
	"            ",
	"             ",
	"              ",
	"               ",
	"                ",
	"                 ",
	"                  ",
	"                   ",
	"                    ",
	"                     ",
	"                      ",
	"                       ",
	"                        ",
	"                         ",
	"                          ",
	"                           ",
	"                            ",
	"                             ",
	"                              ",
	"                               ",
};

/**
 * Pointeur spécial utilisé pour représenter un noeud de type paranthèse
 * ouvrante dans l'arbre syntactic. Ce noeud n'est pas insérer dans l'arbre,
 * mais simplement utilisé pour compiler les arbres syntactics des expressions.
 */
static auto NOEUD_PARENTHESE = reinterpret_cast<noeud::base *>(id_morceau::PARENTHESE_OUVRANTE);

static bool est_specifiant_type(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::FOIS:
		case id_morceau::ESPERLUETTE:
		case id_morceau::CROCHET_OUVRANT:
			return true;
		default:
			return false;
	}
}

static bool est_identifiant_type(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::INT:
		case id_morceau::UNSIGNED:
		case id_morceau::SIGNED:
		case id_morceau::SHORT:
		case id_morceau::LONG:
		case id_morceau::FLOAT:
		case id_morceau::DOUBLE:
		case id_morceau::CHAR:
		case id_morceau::CHAR8_T:
		case id_morceau::CHAR16_T:
		case id_morceau::CHAR32_T:
		case id_morceau::BOOL:
		case id_morceau::VOID:
		case id_morceau::AUTO:
		case id_morceau::CHAINE_CARACTERE:
		case id_morceau::CONST:
		case id_morceau::CONSTEXPR:
			return true;
		default:
			return false;
	}
}

static bool est_nombre_entier(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::NOMBRE_BINAIRE:
		case id_morceau::NOMBRE_ENTIER:
		case id_morceau::NOMBRE_HEXADECIMAL:
		case id_morceau::NOMBRE_OCTAL:
			return true;
		default:
			return false;
	}
}

static bool est_nombre(id_morceau identifiant)
{
	return est_nombre_entier(identifiant) || (identifiant == id_morceau::NOMBRE_REEL);
}

static bool est_operateur_unaire(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::AROBASE:
		case id_morceau::EXCLAMATION:
		case id_morceau::TILDE:
		case id_morceau::CROCHET_OUVRANT:
		case id_morceau::PLUS_UNAIRE:
		case id_morceau::MOINS_UNAIRE:
			return true;
		default:
			return false;
	}
}

static bool est_operateur_binaire(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::PLUS:
		case id_morceau::MOINS:
		case id_morceau::FOIS:
		case id_morceau::DIVISE:
		case id_morceau::PLUS_EGAL:
		case id_morceau::MOINS_EGAL:
		case id_morceau::DIVISE_EGAL:
		case id_morceau::MULTIPLIE_EGAL:
		case id_morceau::MODULO_EGAL:
		case id_morceau::ET_EGAL:
		case id_morceau::OU_EGAL:
		case id_morceau::OUX_EGAL:
		case id_morceau::ESPERLUETTE:
		case id_morceau::POURCENT:
		case id_morceau::INFERIEUR:
		case id_morceau::INFERIEUR_EGAL:
		case id_morceau::SUPERIEUR:
		case id_morceau::SUPERIEUR_EGAL:
		case id_morceau::DECALAGE_DROITE:
		case id_morceau::DECALAGE_GAUCHE:
		case id_morceau::DIFFERENCE:
		case id_morceau::ESP_ESP:
		case id_morceau::EGALITE:
		case id_morceau::BARRE_BARRE:
		case id_morceau::BARRE:
		case id_morceau::CHAPEAU:
		case id_morceau::POINT:
		case id_morceau::EGAL:
		case id_morceau::TROIS_POINTS:
		case id_morceau::VIRGULE:
		case id_morceau::CROCHET_OUVRANT:
			return true;
		default:
			return false;
	}
}

static bool est_operateur_constant(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::PLUS:
		case id_morceau::MOINS:
		case id_morceau::FOIS:
		case id_morceau::DIVISE:
		case id_morceau::ESPERLUETTE:
		case id_morceau::POURCENT:
		case id_morceau::INFERIEUR:
		case id_morceau::INFERIEUR_EGAL:
		case id_morceau::SUPERIEUR:
		case id_morceau::SUPERIEUR_EGAL:
		case id_morceau::DECALAGE_DROITE:
		case id_morceau::DECALAGE_GAUCHE:
		case id_morceau::DIFFERENCE:
		case id_morceau::ESP_ESP:
		case id_morceau::EGALITE:
		case id_morceau::BARRE_BARRE:
		case id_morceau::BARRE:
		case id_morceau::CHAPEAU:
		case id_morceau::EXCLAMATION:
		case id_morceau::TILDE:
		case id_morceau::PLUS_UNAIRE:
		case id_morceau::MOINS_UNAIRE:
			return true;
		default:
			return false;
	}
}

/**
 * Retourne vrai se l'identifiant passé en paramètre peut-être un identifiant
 * valide pour précèder un opérateur unaire '+' ou '-'.
 */
static bool precede_unaire_valide(id_morceau dernier_identifiant)
{
	if (dernier_identifiant == id_morceau::PARENTHESE_FERMANTE) {
		return false;
	}

	if (dernier_identifiant == id_morceau::CROCHET_FERMANT) {
		return false;
	}

	if (dernier_identifiant == id_morceau::CHAINE_CARACTERE) {
		return false;
	}

	if (est_nombre(dernier_identifiant)) {
		return false;
	}

	/* À FAIRE : ceci mélange a[-i] et a[i] - b[i] */
	if (dernier_identifiant == id_morceau::CROCHET_OUVRANT) {
		return false;
	}

	return true;
}

/* ************************************************************************** */

analyseuse_grammaire::analyseuse_grammaire(
		ContexteGenerationCode &contexte,
		DonneesModule *module,
		dls::chaine const &racine_kuri)
	: lng::analyseuse<DonneesMorceaux>(module->morceaux)
	, m_contexte(contexte)
	, m_assembleuse(contexte.assembleuse)
	, m_paires_vecteurs(PROFONDEUR_EXPRESSION_MAX)
	, m_racine_kuri(racine_kuri)
	, m_module(module)
{}

void analyseuse_grammaire::lance_analyse(std::ostream &os)
{
	m_position = 0;

	if (m_identifiants.taille() == 0) {
		return;
	}

	m_module->temps_analyse = 0.0;
	m_debut_analyse = dls::chrono::maintenant();
	analyse_corps(os);
	m_module->temps_analyse += dls::chrono::delta(m_debut_analyse);
}

// declaration fonction :
// type chaine (...) ;
// implémentation fonction :
// type chaine (...) { ... }

void analyseuse_grammaire::analyse_corps(std::ostream &os)
{
	while (!fini()) {
		if (est_identifiant(type_id::STRUCT)) {
			//analyse_declaration_structure();
		}
		else {
			//analyse_expression();
		}
	}
}

void analyseuse_grammaire::analyse_expression_droite(
		id_morceau identifiant_final,
		id_morceau racine_expr,
		bool const calcul_expression)
{
	/* Algorithme de Dijkstra pour générer une notation polonaise inversée. */
	auto profondeur = m_profondeur++;

	if (profondeur >= m_paires_vecteurs.size()) {
		lance_erreur("Excès de la pile d'expression autorisée");
	}

	auto &expression = m_paires_vecteurs[profondeur].first;
	expression.clear();

	auto &pile = m_paires_vecteurs[profondeur].second;
	pile.clear();

	auto vide_pile_operateur = [&](id_morceau id_operateur)
	{
		while (!pile.empty()
			   && pile.back() != NOEUD_PARENTHESE
			   && (precedence_faible(id_operateur, pile.back()->identifiant())))
		{
			expression.push_back(pile.back());
			pile.pop_back();
		}
	};

	/* Nous tenons compte du nombre de paranthèse pour pouvoir nous arrêter en
	 * cas d'analyse d'une expression en dernier paramètre d'un appel de
	 * fontion. */
	auto paren = 0;
	auto dernier_identifiant = (m_position == 0) ? id_morceau::INCONNU : donnees().identifiant;

	/* utilisé pour terminer la boucle quand elle nous atteignons une parenthèse
	 * fermante */
	auto termine_boucle = false;

	auto assignation = false;

	auto drapeaux = static_cast<unsigned short>(0);

	DEB_LOG_EXPRESSION << tabulations[profondeur] << "Vecteur :" << FIN_LOG_EXPRESSION;

	while (!requiers_identifiant(identifiant_final)) {
		auto &morceau = donnees();

		DEB_LOG_EXPRESSION << tabulations[profondeur] << '\t' << chaine_identifiant(morceau.identifiant) << FIN_LOG_EXPRESSION;

		switch (morceau.identifiant) {
			case id_morceau::CHAINE_CARACTERE:
			{
				/* appel fonction : chaine + ( */
				if (est_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
					avance();

					auto noeud = m_assembleuse->empile_noeud(type_noeud::APPEL_FONCTION, m_contexte, morceau, false);

					//analyse_appel_fonction(noeud);

					m_assembleuse->depile_noeud(type_noeud::APPEL_FONCTION);

					expression.push_back(noeud);
				}
				/* construction structure : chaine + { */
				else if (racine_expr == id_morceau::EGAL && est_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
					auto noeud = m_assembleuse->empile_noeud(type_noeud::CONSTRUIT_STRUCTURE, m_contexte, morceau, false);

					avance();

				//	analyse_construction_structure(noeud);

					m_assembleuse->depile_noeud(type_noeud::CONSTRUIT_STRUCTURE);

					expression.push_back(noeud);
				}
				/* variable : chaine */
				else {
					auto noeud = m_assembleuse->cree_noeud(type_noeud::VARIABLE, m_contexte, morceau);
					expression.push_back(noeud);

					noeud->drapeaux |= drapeaux;
					drapeaux = 0;
				}

				break;
			}
			case id_morceau::NOMBRE_REEL:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::NOMBRE_REEL, m_contexte, morceau);
				expression.push_back(noeud);
				break;
			}
			case id_morceau::NOMBRE_BINAIRE:
			case id_morceau::NOMBRE_ENTIER:
			case id_morceau::NOMBRE_HEXADECIMAL:
			case id_morceau::NOMBRE_OCTAL:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::NOMBRE_ENTIER, m_contexte, morceau);
				expression.push_back(noeud);
				break;
			}
			case id_morceau::CHAINE_LITTERALE:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::CHAINE_LITTERALE, m_contexte, morceau);
				expression.push_back(noeud);
				break;
			}
			case id_morceau::CARACTERE:
			{
				auto noeud = m_assembleuse->cree_noeud(type_noeud::CARACTERE, m_contexte, morceau);
				expression.push_back(noeud);
				break;
			}
			case id_morceau::REINTERPRET_CAST:
			case id_morceau::CONST_CAST:
			case id_morceau::STATIC_CAST:
			case id_morceau::DYNAMIC_CAST:
			{
				/* À FAIRE */
				break;
			}
			case id_morceau::PARENTHESE_OUVRANTE:
			{
				++paren;
				pile.push_back(NOEUD_PARENTHESE);
				break;
			}
			case id_morceau::PARENTHESE_FERMANTE:
			{
				/* S'il n'y a pas de parenthèse ouvrante, c'est que nous avons
				 * atteint la fin d'une déclaration d'appel de fonction. */
				if (paren == 0) {
					/* recule pour être synchroniser avec la sortie dans
					 * analyse_appel_fonction() */
					recule();

					/* À FAIRE */
					termine_boucle = true;
					break;
				}

				if (pile.empty()) {
					lance_erreur("Il manque une paranthèse dans l'expression !");
				}

				while (pile.back() != NOEUD_PARENTHESE) {
					expression.push_back(pile.back());
					pile.pop_back();
				}

				/* Enlève la parenthèse restante de la pile. */
				pile.pop_back();

				--paren;
				break;
			}
			/* opérations binaire */
			case id_morceau::PLUS:
			case id_morceau::MOINS:
			{
				auto id_operateur = morceau.identifiant;
				auto noeud = static_cast<noeud::base *>(nullptr);

				if (precede_unaire_valide(dernier_identifiant)) {
					if (id_operateur == id_morceau::PLUS) {
						id_operateur = id_morceau::PLUS_UNAIRE;
					}
					else if (id_operateur == id_morceau::MOINS) {
						id_operateur = id_morceau::MOINS_UNAIRE;
					}

					morceau.identifiant = id_operateur;
					noeud = m_assembleuse->cree_noeud(type_noeud::OPERATION_UNAIRE, m_contexte, morceau);
				}
				else {
					noeud = m_assembleuse->cree_noeud(type_noeud::OPERATION_BINAIRE, m_contexte, morceau);
				}

				vide_pile_operateur(id_operateur);

				pile.push_back(noeud);

				break;
			}
			case id_morceau::FOIS:
			case id_morceau::DIVISE:
			case id_morceau::ESPERLUETTE:
			case id_morceau::POURCENT:
			case id_morceau::INFERIEUR:
			case id_morceau::INFERIEUR_EGAL:
			case id_morceau::SUPERIEUR:
			case id_morceau::SUPERIEUR_EGAL:
			case id_morceau::DECALAGE_DROITE:
			case id_morceau::DECALAGE_GAUCHE:
			case id_morceau::DIFFERENCE:
			case id_morceau::ESP_ESP:
			case id_morceau::EGALITE:
			case id_morceau::BARRE_BARRE:
			case id_morceau::BARRE:
			case id_morceau::CHAPEAU:
			case id_morceau::PLUS_EGAL:
			case id_morceau::MOINS_EGAL:
			case id_morceau::DIVISE_EGAL:
			case id_morceau::MULTIPLIE_EGAL:
			case id_morceau::MODULO_EGAL:
			case id_morceau::ET_EGAL:
			case id_morceau::OU_EGAL:
			case id_morceau::OUX_EGAL:
			case id_morceau::VIRGULE:
			{
				/* Correction de crash d'aléatest, improbable dans la vrai vie. */
				if (expression.empty() && est_operateur_binaire(morceau.identifiant)) {
					lance_erreur("Opérateur binaire utilisé en début d'expression");
				}

				vide_pile_operateur(morceau.identifiant);
				auto noeud = m_assembleuse->cree_noeud(type_noeud::OPERATION_BINAIRE, m_contexte, morceau);
				pile.push_back(noeud);

				break;
			}
			case id_morceau::TROIS_POINTS:
			{
				vide_pile_operateur(morceau.identifiant);
				auto noeud = m_assembleuse->cree_noeud(type_noeud::PLAGE, m_contexte, morceau);
				pile.push_back(noeud);
				break;
			}
			case id_morceau::POINT:
			{
				vide_pile_operateur(morceau.identifiant);
				auto noeud = m_assembleuse->cree_noeud(type_noeud::ACCES_MEMBRE_POINT, m_contexte, morceau);
				pile.push_back(noeud);
				break;
			}
			case id_morceau::EGAL:
			{
				if (assignation) {
					lance_erreur("Ne peut faire d'assignation dans une expression droite", erreur::type_erreur::ASSIGNATION_INVALIDE);
				}

				assignation = true;

				vide_pile_operateur(morceau.identifiant);

				auto noeud = m_assembleuse->cree_noeud(type_noeud::ASSIGNATION_VARIABLE, m_contexte, morceau);
				pile.push_back(noeud);
				break;
			}
			case id_morceau::CROCHET_OUVRANT:
			{
				/* l'accès à un élément d'un tableau est chaine[index] */
				if (dernier_identifiant == id_morceau::CHAINE_CARACTERE
						|| dernier_identifiant == id_morceau::CHAINE_LITTERALE
						 || dernier_identifiant == id_morceau::CROCHET_OUVRANT) {
					vide_pile_operateur(morceau.identifiant);

					auto noeud = m_assembleuse->empile_noeud(type_noeud::OPERATION_BINAIRE, m_contexte, morceau, false);
					pile.push_back(noeud);

					analyse_expression_droite(id_morceau::CROCHET_FERMANT, id_morceau::CROCHET_OUVRANT);

					/* Extrait le noeud enfant, il sera de nouveau ajouté dans
					 * la compilation de l'expression à la fin de la fonction. */
					auto noeud_expr = noeud->enfants.front();
					noeud->enfants.clear();

					/* Si la racine de l'expression est un opérateur, il faut
					 * l'empêcher d'être prise en compte pour l'expression
					 * courante. */
					noeud_expr->drapeaux |= IGNORE_OPERATEUR;

					expression.push_back(noeud_expr);

					m_assembleuse->depile_noeud(type_noeud::OPERATION_BINAIRE);
				}
				else {
					/* change l'identifiant pour ne pas le confondre avec l'opérateur binaire [] */
					morceau.identifiant = id_morceau::TABLEAU;
					auto noeud = m_assembleuse->empile_noeud(type_noeud::CONSTRUIT_TABLEAU, m_contexte, morceau, false);

					analyse_expression_droite(id_morceau::CROCHET_FERMANT, id_morceau::CROCHET_OUVRANT);

					m_assembleuse->depile_noeud(type_noeud::CONSTRUIT_TABLEAU);

					expression.push_back(noeud);
				}

				break;
			}
			/* opérations unaire */
			case id_morceau::AROBASE:
			case id_morceau::EXCLAMATION:
			case id_morceau::TILDE:
			case id_morceau::PLUS_UNAIRE:
			case id_morceau::MOINS_UNAIRE:
			{
				vide_pile_operateur(morceau.identifiant);
				auto noeud = m_assembleuse->cree_noeud(type_noeud::OPERATION_UNAIRE, m_contexte, morceau);
				pile.push_back(noeud);
				break;
			}
			case id_morceau::ACCOLADE_FERMANTE:
			{
				/* une accolade fermante marque généralement la fin de la
				 * construction d'une structure */
				termine_boucle = true;
				/* recule pour être synchroniser avec la sortie dans
				 * analyse_construction_structure() */
				recule();
				break;
			}
			default:
			{
				lance_erreur("Identifiant inattendu dans l'expression");
			}
		}

		if (termine_boucle) {
			break;
		}

		dernier_identifiant = morceau.identifiant;
	}

	/* Retourne s'il n'y a rien dans l'expression, ceci est principalement pour
	 * éviter de crasher lors des fuzz-tests. */
	if (expression.empty()) {
		--m_profondeur;
		return;
	}

	while (!pile.empty()) {
		if (pile.back() == NOEUD_PARENTHESE) {
			lance_erreur("Il manque une paranthèse dans l'expression !");
		}

		expression.push_back(pile.back());
		pile.pop_back();
	}

	pile.reserve(expression.size());

	DEB_LOG_EXPRESSION << tabulations[profondeur] << "Expression :" << FIN_LOG_EXPRESSION;

	for (auto noeud : expression) {
		DEB_LOG_EXPRESSION << tabulations[profondeur] << '\t' << chaine_identifiant(noeud->identifiant()) << FIN_LOG_EXPRESSION;

		if (!possede_drapeau(noeud->drapeaux, IGNORE_OPERATEUR) && est_operateur_binaire(noeud->identifiant())) {
			if (pile.size() < 2) {
				erreur::lance_erreur(
							"Expression malformée pour opérateur binaire",
							m_contexte,
							noeud->donnees_morceau(),
							erreur::type_erreur::NORMAL);
			}

			auto n2 = pile.back();
			pile.pop_back();

			auto n1 = pile.back();
			pile.pop_back();

			if (est_constant(n1) && est_constant(n2)) {
				if (est_operateur_constant(noeud->identifiant())) {
					noeud = calcul_expression_double(*m_assembleuse, m_contexte, noeud, n1, n2);

					if (noeud == nullptr) {
						lance_erreur("Ne peut pas calculer l'expression");
					}
				}
				else if (calcul_expression) {
					lance_erreur("Ne peut pas calculer l'expression car l'opérateur n'est pas constant");
				}
				else {
					noeud->ajoute_noeud(n1);
					noeud->ajoute_noeud(n2);
				}
			}
			else if (calcul_expression) {
				if (pile.size() < 1) {
					erreur::lance_erreur(
								"Expression malformée pour opérateur unaire",
								m_contexte,
								noeud->donnees_morceau(),
								erreur::type_erreur::NORMAL);
				}

				lance_erreur("Ne peut pas calculer l'expression pour la constante");
			}
			else {
				noeud->ajoute_noeud(n1);
				noeud->ajoute_noeud(n2);
			}

			pile.push_back(noeud);
		}
		else if (!possede_drapeau(noeud->drapeaux, IGNORE_OPERATEUR) && est_operateur_unaire(noeud->identifiant())) {
			auto n1 = pile.back();
			pile.pop_back();

			if (est_constant(n1)) {
				if (est_operateur_constant(noeud->identifiant())) {
					noeud = calcul_expression_simple(*m_assembleuse, noeud, n1);

					if (noeud == nullptr) {
						lance_erreur("Ne peut pas calculer l'expression");
					}
				}
				else if (calcul_expression) {
					lance_erreur("Ne peut pas calculer l'expression car l'opérateur n'est pas constant");
				}
			}
			else if (calcul_expression) {
				lance_erreur("Ne peut pas calculer l'expression car le noeud n'est pas constant");
			}
			else {
				noeud->ajoute_noeud(n1);
			}

			pile.push_back(noeud);
		}
		else {
			pile.push_back(noeud);
		}
	}

	m_assembleuse->ajoute_noeud(pile.back());
	pile.pop_back();

	if (pile.size() != 0) {
		auto premier_noeud = pile.back();
		auto dernier_noeud = premier_noeud;
		pile.pop_back();

		auto pos_premier = premier_noeud->donnees_morceau().ligne_pos & 0xffffffff;
		auto pos_dernier = pos_premier;

		while (!pile.empty()) {
			auto n = pile.back();
			pile.pop_back();

			auto pos_n = n->donnees_morceau().ligne_pos & 0xffffffff;

			if (pos_n < pos_premier) {
				premier_noeud = n;
			}
			if (pos_n > pos_dernier) {
				dernier_noeud = n;
			}
		}

		erreur::lance_erreur_plage(
					"Expression malformée, il est possible qu'il manque un opérateur",
					m_contexte,
					premier_noeud->donnees_morceau(),
					dernier_noeud->donnees_morceau());
	}

	--m_profondeur;
}

bool analyseuse_grammaire::requiers_identifiant_type()
{
	auto const ok = est_identifiant_type(this->identifiant_courant());
	avance();
	return ok;
}

bool analyseuse_grammaire::requiers_nombre_entier()
{
	auto const ok = est_nombre_entier(this->identifiant_courant());
	avance();
	return ok;
}

void analyseuse_grammaire::lance_erreur(const dls::chaine &quoi, erreur::type_erreur type)
{
	erreur::lance_erreur(quoi, m_contexte, donnees(), type);
}
