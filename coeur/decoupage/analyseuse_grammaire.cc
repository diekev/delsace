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

#include "analyseuse_grammaire.h"

#include <iostream>
#include <set>
#include <stack>

#include "arbre_syntactic.h"
#include "expression.h"

#undef DEBOGUE_EXPRESSION
#undef DEBOGUE_ARBRE

/* À FAIRE :
 * - transtype<>()
 * - gabarit
 * - accès tableau [expr]
 */

static bool est_specifiant_type(int identifiant)
{
	switch (identifiant) {
		case ID_FOIS:
		case ID_ESPERLUETTE:
		case ID_CROCHET_OUVRANT:
			return true;
		default:
			return false;
	}
}

static bool est_identifiant_type(int identifiant)
{
	switch (identifiant) {
		case ID_E8:
		case ID_E8NS:
		case ID_E16:
		case ID_E16NS:
		case ID_E32:
		case ID_E32NS:
		case ID_E64:
		case ID_E64NS:
		case ID_R16:
		case ID_R32:
		case ID_R64:
		case ID_BOOL:
		case ID_RIEN:
			return true;
		default:
			return false;
	}
}

static bool est_nombre_entier(int identifiant)
{
	switch (identifiant) {
		case ID_NOMBRE_BINAIRE:
		case ID_NOMBRE_ENTIER:
		case ID_NOMBRE_HEXADECIMAL:
		case ID_NOMBRE_OCTAL:
			return true;
		default:
			return false;
	}
}

static bool est_nombre(int identifiant)
{
	return est_nombre_entier(identifiant) || (identifiant == ID_NOMBRE_REEL);
}

static bool est_operateur_simple(int identifiant)
{
	switch (identifiant) {
		case ID_AROBASE:
		case ID_EXCLAMATION:
		case ID_TILDE:
			return true;
		default:
			return false;
	}
}

static bool est_operateur_double(int identifiant)
{
	switch (identifiant) {
		case ID_PLUS:
		case ID_MOINS:
		case ID_FOIS:
		case ID_DIVISE:
		case ID_ESPERLUETTE:
		case ID_POURCENT:
		case ID_INFERIEUR:
		case ID_INFERIEUR_EGAL:
		case ID_SUPERIEUR:
		case ID_SUPERIEUR_EGAL:
		case ID_DECALAGE_DROITE:
		case ID_DECALAGE_GAUCHE:
		case ID_DIFFERENCE:
		case ID_ESP_ESP:
		case ID_EGALITE:
		case ID_BARRE_BARRE:
		case ID_BARRE:
		case ID_CHAPEAU:
			return true;
		default:
			return false;
	}
}

static bool est_operateur(int identifiant)
{
	return est_operateur_simple(identifiant) || est_operateur_double(identifiant);
}

/* ************************************************************************** */

analyseuse_grammaire::analyseuse_grammaire(const TamponSource &tampon, assembleuse_arbre *assembleuse)
	: Analyseuse(tampon)
	, m_assembleuse(assembleuse)
{}

void analyseuse_grammaire::lance_analyse(const std::vector<DonneesMorceaux> &identifiants)
{
	m_identifiants = identifiants;
	m_position = 0;

	if (m_identifiants.size() == 0) {
		return;
	}

	m_assembleuse->ajoute_noeud(NOEUD_RACINE, DonneesMorceaux{"racine", 0ul, 0ul, -1});

	analyse_corps();

#ifdef DEBOGUE_ARBRE
	m_assembleuse->imprime_code(std::cerr);
#endif
}

void analyseuse_grammaire::analyse_corps()
{
	if (est_identifiant(ID_FONCTION)) {
		analyse_declaration_fonction();
	}
	else if (est_identifiant(ID_SOIT)) {
		analyse_declaration_constante();
	}
	else if (est_identifiant(ID_STRUCTURE)) {
		analyse_declaration_structure();
	}
	else if (est_identifiant(ID_ENUM)) {
		analyse_declaration_enum();
	}
	else {
		avance();
		lance_erreur("Identifiant inattendu, doit être 'soit', 'fonction', 'structure', ou 'énum'");
	}

	if (m_position != m_identifiants.size()) {
		analyse_corps();
	}
}

void analyseuse_grammaire::analyse_declaration_fonction()
{
	if (!requiers_identifiant(ID_FONCTION)) {
		lance_erreur("Attendu la déclaration du mot-clé 'fonction'");
	}

	if (!requiers_identifiant(ID_CHAINE_CARACTERE)) {
		lance_erreur("Attendu la déclaration du nom de la fonction");
	}

	// crée noeud fonction
	const auto nom_fonction = m_identifiants[position()].chaine;
	auto noeud = m_assembleuse->ajoute_noeud(NOEUD_DECLARATION_FONCTION, m_identifiants[position()]);
	auto noeud_declaration = dynamic_cast<NoeudDeclarationFonction *>(noeud);

	if (!requiers_identifiant(ID_PARENTHESE_OUVRANTE)) {
		lance_erreur("Attendu une parenthèse ouvrante après le nom de la fonction");
	}

	analyse_parametres_fonction(noeud_declaration);

	if (!requiers_identifiant(ID_PARENTHESE_FERMANTE)) {
		lance_erreur("Attendu une parenthèse fermante après la liste des paramètres de la fonction");
	}

	/* vérifie si le type de la fonction est explicit. */
	if (est_identifiant(ID_DOUBLE_POINTS)) {
		analyse_declaration_type();
		noeud_declaration->type_retour = m_identifiants[position()].identifiant;
	}

	if (!requiers_identifiant(ID_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante après la liste des paramètres de la fonction");
	}

	analyse_corps_fonction();

	if (!requiers_identifiant(ID_ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante à la fin de la fonction");
	}

	m_assembleuse->sors_noeud(NOEUD_DECLARATION_FONCTION);
}

void analyseuse_grammaire::analyse_parametres_fonction(NoeudDeclarationFonction *noeud)
{
	if (est_identifiant(ID_PARENTHESE_FERMANTE)) {
		/* La liste est vide. */
		return;
	}

	ArgumentFonction arg;

	if (!requiers_identifiant(ID_CHAINE_CARACTERE)) {
		lance_erreur("Attendu le nom de la variable");
	}

	arg.chaine = m_identifiants[position()].chaine;

	analyse_declaration_type();

	arg.id_type = m_identifiants[position()].identifiant;

	noeud->ajoute_argument(arg);

	/* fin des paramètres */
	if (!requiers_identifiant(ID_VIRGULE)) {
		recule();
		return;
	}

	analyse_parametres_fonction(noeud);
}

void analyseuse_grammaire::analyse_corps_fonction()
{
	/* assignement : soit x = a + b; */
	if (est_identifiant(ID_SOIT)) {
		avance();

		if (!requiers_identifiant(ID_CHAINE_CARACTERE)) {
			lance_erreur("Attendu une chaîne de caractère après 'soit'");
		}

		const auto &morceau = m_identifiants[position()];
		auto type = -1;

		if (est_identifiant(ID_DOUBLE_POINTS)) {
			analyse_declaration_type();
			type = m_identifiants[position()].identifiant;
		}

		if (!requiers_identifiant(ID_EGAL)) {
			lance_erreur("Attendu '=' après chaîne de caractère");
		}

		auto noeud = m_assembleuse->ajoute_noeud(NOEUD_ASSIGNATION_VARIABLE, morceau);
		noeud->type = type;

		analyse_expression_droite(ID_POINT_VIRGULE);

		m_assembleuse->sors_noeud(NOEUD_ASSIGNATION_VARIABLE);
	}
	/* retour : retourne a + b; */
	else if (est_identifiant(ID_RETOURNE)) {
		avance();
		m_assembleuse->ajoute_noeud(NOEUD_RETOUR, m_identifiants[position()]);

		/* Considération du cas où l'on ne retourne rien 'retourne;'. */
		if (!est_identifiant(ID_POINT_VIRGULE)) {
			analyse_expression_droite(ID_POINT_VIRGULE);
		}
		else {
			avance();
		}

		m_assembleuse->sors_noeud(NOEUD_RETOUR);
	}
	/* controle de flux : si */
	else if (est_identifiant(ID_SI)) {
		avance();
		analyse_expression_droite(ID_ACCOLADE_OUVRANTE);

		analyse_corps_fonction();

		if (!requiers_identifiant(ID_ACCOLADE_FERMANTE)) {
			lance_erreur("Attendu une accolade fermante à la fin du contrôle 'si'");
		}
	}
	/* controle de flux : sinon (si) */
	else if (est_identifiant(ID_SINON)) {
		avance();

		if (est_identifiant(ID_SI)) {
			avance();
			analyse_expression_droite(ID_ACCOLADE_OUVRANTE);
		}
		else {
			if (!requiers_identifiant(ID_ACCOLADE_OUVRANTE)) {
				lance_erreur("Attendu une accolade ouvrante après 'sinon'");
			}
		}

		analyse_corps_fonction();

		if (!requiers_identifiant(ID_ACCOLADE_FERMANTE)) {
			lance_erreur("Attendu une accolade fermante à la fin du contrôle 'sinon'");
		}
	}
	/* appel : fais_quelque_chose(); */
	else if (sont_2_identifiants(ID_CHAINE_CARACTERE, ID_PARENTHESE_OUVRANTE)) {
		analyse_expression_droite(ID_POINT_VIRGULE);
	}
	else {
		return;
	}

	analyse_corps_fonction();
}

struct Symbole {
	std::string chaine;
	int identifiant;
	int pad;
};

void analyseuse_grammaire::analyse_expression_droite(int identifiant_final)
{
	/* Algorithme de Dijkstra pour générer une notation polonaise inversée. */

	std::vector<Noeud *> expression;
	std::stack<DonneesMorceaux> pile;

	/* Nous tenons compte du nombre de paranthèse pour pouvoir nous arrêter en
	 * cas d'analyse d'une expression en dernier paramètre d'un appel de
	 * fontion. */
	auto paren = 0;

	while (!est_identifiant(identifiant_final)) {
		const auto &morceau = m_identifiants[position() + 1];

		/* appel fonction : chaine + ( */
		if (sont_2_identifiants(ID_CHAINE_CARACTERE, ID_PARENTHESE_OUVRANTE)) {
			avance();
			avance();

			auto noeud = m_assembleuse->ajoute_noeud(NOEUD_APPEL_FONCTION, morceau, false);

			analyse_appel_fonction(dynamic_cast<NoeudAppelFonction *>(noeud));

			m_assembleuse->sors_noeud(NOEUD_APPEL_FONCTION);

			expression.push_back(noeud);
		}
		/* accès propriété : chaine + de + chaine */
		else if (sont_3_identifiants(ID_CHAINE_CARACTERE, ID_DE, ID_CHAINE_CARACTERE)) {
			/* À FAIRE : structure, classe */
			lance_erreur("L'accès de propriété de structure n'est pas implémentée");
		}
		/* variable : chaine */
		else if (est_identifiant(ID_CHAINE_CARACTERE)) {
			auto noeud = m_assembleuse->cree_noeud(NOEUD_VARIABLE, morceau);
			expression.push_back(noeud);
		}
		else if (identifiant_courant() == ID_NOMBRE_REEL) {
			auto noeud = m_assembleuse->cree_noeud(NOEUD_NOMBRE_REEL, morceau);
			expression.push_back(noeud);
		}
		else if (est_nombre_entier(identifiant_courant())) {
			auto noeud = m_assembleuse->cree_noeud(NOEUD_NOMBRE_ENTIER, morceau);
			expression.push_back(noeud);
		}
		else if (est_operateur(identifiant_courant())) {
			while (!pile.empty()
				   && est_operateur(pile.top().identifiant)
				   && (precedence_faible(morceau.identifiant, pile.top().identifiant)))
			{
				auto noeud = m_assembleuse->cree_noeud(NOEUD_OPERATION, pile.top());
				expression.push_back(noeud);
				pile.pop();
			}

			pile.push(morceau);
		}
		else if (est_identifiant(ID_PARENTHESE_OUVRANTE)) {
			++paren;
			pile.push(morceau);
		}
		else if (est_identifiant(ID_PARENTHESE_FERMANTE)) {
			/* S'il n'y a pas de parenthèse ouvrante, c'est que nous avons
			 * atteint la fin d'une déclaration d'appel de fonction. */
			if (paren == 0) {
				/* recule pour être synchroniser avec la sortie dans
				 * analyse_appel_fonction() */
				recule();
				break;
			}

			if (pile.empty()) {
				lance_erreur("Il manque une paranthèse dans l'expression !");
			}

			while (pile.top().identifiant != ID_PARENTHESE_OUVRANTE) {
				auto noeud = m_assembleuse->cree_noeud(NOEUD_OPERATION, pile.top());
				expression.push_back(noeud);
				pile.pop();
			}

			/* Enlève la parenthèse restante de la pile. */
			if (pile.top().identifiant == ID_PARENTHESE_OUVRANTE) {
				pile.pop();
			}

			--paren;
		}
		else {
			avance();
			lance_erreur("Identifiant inattendu dans l'expression");
		}

		avance();
	}

	while (!pile.empty()) {
		if (pile.top().identifiant == ID_PARENTHESE_OUVRANTE) {
			lance_erreur("Il manque une paranthèse dans l'expression !");
		}

		auto noeud = m_assembleuse->cree_noeud(NOEUD_OPERATION, pile.top());
		expression.push_back(noeud);
		pile.pop();
	}

#ifdef DEBOGUE_EXPRESSION
	std::cerr << "Expression : " ;
	for (const Symbole &symbole : expression) {
		std::cerr << symbole.chaine << ' ';
	}
	std::cerr << '\n';
#endif

	std::stack<Noeud *> pile_noeud;

	for (Noeud *noeud : expression) {
		if (est_operateur_double(noeud->identifiant())) {
			auto n2 = pile_noeud.top();
			pile_noeud.pop();

			auto n1 = pile_noeud.top();
			pile_noeud.pop();

			noeud->ajoute_noeud(n1);
			noeud->ajoute_noeud(n2);

			pile_noeud.push(noeud);
		}
		else if (est_operateur_simple(noeud->identifiant())) {
			auto n1 = pile_noeud.top();
			pile_noeud.pop();

			noeud->ajoute_noeud(n1);

			pile_noeud.push(noeud);
		}
		else {
			pile_noeud.push(noeud);
		}
	}

	m_assembleuse->ajoute_noeud(pile_noeud.top());
	pile_noeud.pop();

	if (pile_noeud.size() != 0) {
		std::cerr << "Il reste plus d'un noeud dans la pile ! :";

		while (!pile_noeud.empty()) {
			auto noeud = pile_noeud.top();
			pile_noeud.pop();
			std::cerr << '\t' << chaine_identifiant(noeud->identifiant()) << '\n';
		}
	}

	/* saute l'identifiant final */
	avance();
}

/* f(g(5, 6 + 3 * (2 - 5)), h()); */
void analyseuse_grammaire::analyse_appel_fonction(NoeudAppelFonction *noeud)
{
	/* ici nous devons être au niveau du premier paramètre */

	auto arguments_nommees = false;
	std::set<std::string> args;

	while (true) {
		/* aucun paramètre, ou la liste de paramètre est vide */
		if (est_identifiant(ID_PARENTHESE_FERMANTE)) {
			return;
		}

		if (sont_2_identifiants(ID_CHAINE_CARACTERE, ID_EGAL)) {
			avance();
			arguments_nommees = true;

			auto nom_argument = m_identifiants[position()].chaine;

			if (args.find(nom_argument) != args.end()) {
				lance_erreur("Argument déjà nommé");
			}

			args.insert(nom_argument);
			noeud->ajoute_nom_argument(nom_argument);

			avance();
		}
		else if (arguments_nommees == true) {
			avance();
			lance_erreur("Attendu le nom de l'argument");
		}

		/* À FAIRE : le dernier paramètre s'arrête à une parenthèse fermante.
		 * si identifiant final == ')', alors l'algorithme s'arrête quand une
		 * paranthèse fermante est trouvé et que la pile est vide */
		analyse_expression_droite(ID_VIRGULE);
	}
}

void analyseuse_grammaire::analyse_declaration_constante()
{
	if (!requiers_identifiant(ID_SOIT)) {
		lance_erreur("Attendu la déclaration 'soit'");
	}

	if (!requiers_identifiant(ID_CONSTANTE)) {
		lance_erreur("Attendu la déclaration 'constante' après 'soit'");
	}

	if (!requiers_identifiant(ID_CHAINE_CARACTERE)) {
		lance_erreur("Attendu une chaîne de caractère après 'constante'");
	}

	auto pos = position();
	auto type = -1;

	/* Vérifie s'il y a typage explicit */
	if (est_identifiant(ID_DOUBLE_POINTS)) {
		analyse_declaration_type();
		type = m_identifiants[position()].identifiant;;
	}

	auto noeud = m_assembleuse->ajoute_noeud(NOEUD_CONSTANTE, m_identifiants[pos]);
	noeud->type = type;

	if (!requiers_identifiant(ID_EGAL)) {
		lance_erreur("Attendu '=' après la déclaration de la constante");
	}

	analyse_expression_droite(ID_POINT_VIRGULE);

	m_assembleuse->sors_noeud(NOEUD_CONSTANTE);
}

void analyseuse_grammaire::analyse_declaration_structure()
{
	if (!requiers_identifiant(ID_STRUCTURE)) {
		lance_erreur("Attendu la déclaration 'structure'");
	}

	if (!requiers_identifiant(ID_CHAINE_CARACTERE)) {
		lance_erreur("Attendu une chaîne de caractères après 'structure'");
	}

	if (!requiers_identifiant(ID_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu '{'");
	}

	/* chaine : type ; */
	while (true) {
		if (!requiers_identifiant(ID_CHAINE_CARACTERE)) {
			/* nous avons terminé */
			recule();
			break;
		}

		analyse_declaration_type();

		if (!requiers_identifiant(ID_POINT_VIRGULE)) {
			lance_erreur("Attendu ';'");
		}
	}

	if (!requiers_identifiant(ID_ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu '}' à la fin de la déclaration de la structure");
	}
}

void analyseuse_grammaire::analyse_declaration_enum()
{
	if (!requiers_identifiant(ID_ENUM)) {
		lance_erreur("Attendu la déclaration 'énum'");
	}

	if (!requiers_identifiant(ID_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu '{' après 'énum'");
	}

	while (true) {
		if (!requiers_identifiant(ID_CHAINE_CARACTERE)) {
			/* nous avons terminé */
			recule();
			break;
		}

		auto noeud = m_assembleuse->ajoute_noeud(NOEUD_CONSTANTE, m_identifiants[position()]);
		noeud->type = ID_E32;

		if (est_identifiant(ID_EGAL)) {
			avance();

			// À FAIRE : analyse_expression_droite(ID_VIRGULE);
			if (!est_nombre_entier(this->identifiant_courant())) {
				avance();
				lance_erreur("Attendu un nombre entier après '='");
			}

			avance();


			m_assembleuse->ajoute_noeud(NOEUD_NOMBRE_ENTIER, m_identifiants[position()]);
			m_assembleuse->sors_noeud(NOEUD_NOMBRE_ENTIER);
		}

		m_assembleuse->sors_noeud(NOEUD_CONSTANTE);

		if (!requiers_identifiant(ID_VIRGULE)) {
			lance_erreur("Attendu ',' à la fin de la déclaration");
		}
	}

	if (!requiers_identifiant(ID_ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu '}' à la fin de la déclaration de l'énum");
	}
}

void analyseuse_grammaire::analyse_declaration_type()
{
	if (!requiers_identifiant(ID_DOUBLE_POINTS)) {
		lance_erreur("Attendu ':'");
	}

	while (est_specifiant_type(identifiant_courant())) {
		if (this->identifiant_courant() == ID_CROCHET_OUVRANT) {
			avance();

			if (this->identifiant_courant() != ID_CROCHET_FERMANT) {
				/* À FAIRE : expression */
				if (!est_nombre_entier(this->identifiant_courant())) {
					avance();

					lance_erreur("Attendu un nombre entier après [");
				}

				avance();
			}

			if (this->identifiant_courant() != ID_CROCHET_FERMANT) {
				avance();
				lance_erreur("Attendu ']'");
			}
		}

		avance();
	}

	if (!est_identifiant_type(this->identifiant_courant())) {
		avance();
		lance_erreur("Attendu la déclaration d'un type");
	}

	avance();
}
