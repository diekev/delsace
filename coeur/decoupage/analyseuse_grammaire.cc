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

#if 0
#include <stack>

enum {
	NOEUD_APPEL_FONCTION,
	NOEUD_DECLARE_FONCTION,
	NOEUD_EXPRESSION,
	NOEUD_VARIABLE,
	NOEUD_NOMBRE_DECIMAL,
	NOEUD_NOMBRE_ENTIER,
};

class Noeud {
	std::string m_chaine;
	std::vector<Noeud *> m_enfants;

public:
	Noeud(const std::string &chaine)
		: m_chaine(chaine)
	{}

	void ajoute_noeud(Noeud *noeud)
	{
		m_enfants.push_back(noeud);
	}

	virtual void imprime_code(std::ostream &os) = 0;
};

class assembleuse_arbre {
	Noeud *m_noeud_courant;
	std::stack<Noeud *> m_noeuds;

public:
	void ajoute_noeud(int type, const std::string &chaine)
	{
		auto noeud = cree_noeud(type, chaine);
		m_noeuds.top()->ajoute_noeud(noeud);
		m_noeuds.push(noeud);
	}

	Noeud *cree_noeud(int type, const std::string &chaine)
	{
		return new Noeud(chaine);
	}

	void sors_noeud(int type)
	{
		m_noeuds.pop();
	}
};

/* ajoute un noeud au noeud courant et utilise ce noeud comme noeud courant */
m_assembleuse.ajoute_noeud(NOEUD_APPEL_FONCTION, chaine);

/* sors du noeud courant en vérifiant que le type du noeud courant est bel et bien le type passé en paramètre */
m_assembleuse.sors_noeud(NOEUD_APPEL_FONCTION);

/* crée un noeud et retourne son pointeur */
auto noeud = m_assembleuse.cree_noeud(NOEUD_VARIABLE, chaine);

/* ajoute un noeud à la liste des noeuds du noeud */
noeud->ajoute_noeud(noeud);
#endif

static bool est_identifiant_type(int identifiant)
{
	switch (identifiant) {
		case IDENTIFIANT_E8:
		case IDENTIFIANT_E8NS:
		case IDENTIFIANT_E16:
		case IDENTIFIANT_E16NS:
		case IDENTIFIANT_E32:
		case IDENTIFIANT_E32NS:
		case IDENTIFIANT_E64:
		case IDENTIFIANT_E64NS:
		case IDENTIFIANT_R16:
		case IDENTIFIANT_R32:
		case IDENTIFIANT_R64:
		case IDENTIFIANT_BOOLEEN:
			return true;
		default:
			return false;
	}
}

/* ************************************************************************** */

void analyseuse_grammaire::lance_analyse(const std::vector<DonneesMorceaux> &identifiants)
{
	m_identifiants = identifiants;
	m_position = 0;

	if (m_identifiants.size() == 0) {
		return;
	}

	analyse_corps();
}

void analyseuse_grammaire::analyse_corps()
{
	if (est_identifiant(IDENTIFIANT_FONCTION)) {
		analyse_declaration_fonction();
	}
	else {
		avance();
		lance_erreur("Le script doit commencé par 'fonction'");
	}

	if (m_position != m_identifiants.size()) {
		analyse_corps();
	}
}

void analyseuse_grammaire::analyse_declaration_fonction()
{
	if (!requiers_identifiant(IDENTIFIANT_FONCTION)) {
		lance_erreur("Attendu la déclaration du mot-clé 'fonction'");
	}

	if (!requiers_identifiant(IDENTIFIANT_CHAINE_CARACTERE)) {
		lance_erreur("Attendu la déclaration du nom de la fonction");
	}

	if (!requiers_identifiant(IDENTIFIANT_PARENTHESE_OUVRANTE)) {
		lance_erreur("Attendu une parenthèse ouvrante après le nom de la fonction");
	}

	analyse_parametres_fonction();

	if (!requiers_identifiant(IDENTIFIANT_PARENTHESE_FERMANTE)) {
		lance_erreur("Attendu une parenthèse fermante après la liste des paramètres de la fonction");
	}

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante après la liste des paramètres de la fonction");
	}

	analyse_corps_fonction();

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante à la fin de la fonction");
	}
}

void analyseuse_grammaire::analyse_parametres_fonction()
{
	if (est_identifiant(IDENTIFIANT_PARENTHESE_FERMANTE)) {
		/* La liste est vide. */
		return;
	}

	if (!requiers_identifiant(IDENTIFIANT_CHAINE_CARACTERE)) {
		lance_erreur("Attendu le nom de la variable");
	}

	if (!requiers_identifiant(IDENTIFIANT_DOUBLE_POINT)) {
		lance_erreur("Attendu un double point après la déclaration de la variable");
	}

	if (!est_identifiant_type(identifiant_courant())) {
		lance_erreur("Attendu la déclaration d'un type");
	}

	avance();

	/* fin des paramètres */
	if (!requiers_identifiant(IDENTIFIANT_VIRGULE)) {
		recule();
		return;
	}

	analyse_parametres_fonction();
}

void analyseuse_grammaire::analyse_corps_fonction()
{
	/* assignement : soit x = a + b; */
	if (est_identifiant(IDENTIFIANT_SOIT)) {
		avance();
		analyse_expression_droite(IDENTIFIANT_POINT_VIRGULE);
	}
	/* retour : retourne a + b; */
	else if (est_identifiant(IDENTIFIANT_RETOURNE)) {
		avance();
		analyse_expression_droite(IDENTIFIANT_POINT_VIRGULE);
	}
	/* controle de flux : si */
	else if (est_identifiant(IDENTIFIANT_SI)) {
		avance();
		analyse_expression_droite(IDENTIFIANT_ACCOLADE_OUVRANTE);

		analyse_corps_fonction();

		if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_FERMANTE)) {
			lance_erreur("Attendu une accolade fermante à la fin du contrôle 'si'");
		}
	}
	/* controle de flux : sinon (si) */
	else if (est_identifiant(IDENTIFIANT_SINON)) {
		avance();

		if (est_identifiant(IDENTIFIANT_SI)) {
			avance();
			analyse_expression_droite(IDENTIFIANT_ACCOLADE_OUVRANTE);
		}
		else {
			if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_OUVRANTE)) {
				lance_erreur("Attendu une accolade ouvrante après 'sinon'");
			}
		}

		analyse_corps_fonction();

		if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_FERMANTE)) {
			lance_erreur("Attendu une accolade fermante à la fin du contrôle 'sinon'");
		}
	}
//	else if (est_identifiant(IDENTIFIANT_CHAINE_CARACTERE)) {
//		/* appel : fais_quelque_chose(); */
//		avance();
//		analyse_expression_droite();
//	}
	else {
		return;
	}

	analyse_corps_fonction();
}

struct Symbole {
	int identifiant;
	std::string chaine;
};

void analyseuse_grammaire::analyse_expression_droite(int identifiant_final)
{
	while (!est_identifiant(identifiant_final)) {
		if (est_identifiant(IDENTIFIANT_CHAINE_CARACTERE)) {
			avance();

			/* fonction : chaine + ( */
			if (est_identifiant(IDENTIFIANT_PARENTHESE_OUVRANTE)) {
				avance();

				analyse_appel_fonction();

				if (!requiers_identifiant(IDENTIFIANT_PARENTHESE_FERMANTE)) {
					lance_erreur("Attendu une paranthèse fermante après les paramètres de la fonction !");
				}
			}
			/* accès propriété : chaine + de + chaine */
			else if (est_identifiant(IDENTIFIANT_DE)) {
				/* À FAIRE : structure, classe */
				lance_erreur("L'accès de propriété de structure n'est pas implémenté");
			}
			/* variable : chaine */
			else {
			}
		}

		avance();
	}

	/* saute l'identifiant final */
	avance();
}

/* f(g(5, 6 + 3 * (2 - 5)), h()); */
void analyseuse_grammaire::analyse_appel_fonction()
{
	/* ici nous devons être au niveau du premier paramètre */

	/* aucun paramètre, ou la liste de paramètre est vide */
	if (est_identifiant(IDENTIFIANT_PARENTHESE_FERMANTE)) {
		return;
	}

	/* À FAIRE : le dernier paramètre s'arrête à une parenthèse fermante.
	 * si identifiant final == ')', alors l'algorithme s'arrête quand une
	 * paranthèse fermante est trouvé est que la pile est vide */
	analyse_expression_droite(IDENTIFIANT_VIRGULE);

	if (!est_identifiant(IDENTIFIANT_PARENTHESE_FERMANTE)) {
		analyse_appel_fonction();
	}
}
