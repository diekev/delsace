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

#include "analyseuse_logique.h"

#include <iostream>

#include "biblinternes/structures/pile.hh"

#include "expression.h"
#include "graphe_contrainte.h"
#include "manipulable.h"

//#define DEBOGUE_ANALYSEUSE
#define DEBOGUE_EXPRESSION

#ifdef DEBOGUE_ANALYSEUSE
# define LOG std::cout
#else
struct Loggeuse {};

template <typename T>
Loggeuse &operator<<(Loggeuse &l, const T &) { return l; }

static Loggeuse loggeuse_analyseuse;

# define LOG loggeuse_analyseuse
#endif

namespace danjo {

enum {
	EXPRESSION_ENTREE,
	EXPRESSION_INTERFACE,
	EXPRESSION_RELATION,
	EXPRESSION_SORTIE,
};

AnalyseuseLogique::AnalyseuseLogique(
		Manipulable *manipulable,
		lng::tampon_source const &tampon,
		dls::tableau<DonneesMorceaux> &identifiants,
		bool initialise_manipulable)
	: base_analyseuse(tampon, identifiants)
	, m_manipulable(manipulable)
	, m_initialise_manipulable(initialise_manipulable)
{}

void AnalyseuseLogique::lance_analyse(std::ostream &)
{
	if (m_identifiants.est_vide()) {
		return;
	}

	m_position = 0;

	if (!requiers_identifiant(id_morceau::FEUILLE)) {
		lance_erreur("Le script doit commencer avec 'feuille' !");
	}

	if (!requiers_identifiant(id_morceau::CHAINE_LITTERALE)) {
		lance_erreur("Attendu le nom de la feuille après 'feuille' !");
	}

	if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante !");
	}

	analyse_corps();

	if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante à la fin du script !");
	}
}

void AnalyseuseLogique::analyse_corps()
{
	LOG << __func__ << '\n';

	if (est_identifiant(id_morceau::ENTREE)) {
		analyse_entree();
	}
	else if (est_identifiant(id_morceau::ENTREFACE)) {
		analyse_entreface();
	}
	else if (est_identifiant(id_morceau::LOGIQUE)) {
		analyse_logique();
	}
	else if (est_identifiant(id_morceau::SORTIE)) {
		analyse_sortie();
	}
	else {
		return;
	}

	analyse_corps();

	LOG << __func__ << " fin\n";
}

void AnalyseuseLogique::analyse_entree()
{
	LOG << __func__ << '\n';

	if (!requiers_identifiant(id_morceau::ENTREE)) {
		lance_erreur("Attendu la déclaration 'entrée' !");
	}

	if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante !");
	}

	analyse_declaration(EXPRESSION_ENTREE);

	if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante à la fin de l'entrée !");
	}

	LOG << __func__ << " fin\n";
}

void AnalyseuseLogique::analyse_declaration(const int type)
{
	LOG << __func__ << '\n';

	if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
		recule();
		return;
	}

	const auto nom = donnees().chaine;

	if (!requiers_identifiant(id_morceau::EGAL)) {
		lance_erreur("Attendu '=' !");
	}

	analyse_expression(dls::chaine{nom}, type);

	if (!requiers_identifiant(id_morceau::POINT_VIRGULE)) {
		lance_erreur("Attendu un point virgule !");
	}

	analyse_declaration(type);

	LOG << __func__ << " fin\n";
}

void AnalyseuseLogique::analyse_expression(const dls::chaine &nom, const int type)
{
	LOG << __func__ << '\n';

	const auto construit_graphe = !m_initialise_manipulable
								  && (type == EXPRESSION_RELATION
									  || type == EXPRESSION_SORTIE);
	contrainte *cont = nullptr;
	Variable *sortie = nullptr;

	if (!m_assembleuse.variable_connue(nom)) {
		sortie = m_assembleuse.ajoute_variable(nom);
	}
	else {
		sortie = m_assembleuse.variable(nom);
	}

	if (construit_graphe) {
		/* Crée une contrainte. */
		cont = new contrainte;
		cont->m_sortie = sortie;
	}

	/* Algorithme de Dijkstra pour générer une notation polonaise inversée. */

	dls::tableau<Symbole> expression;
	dls::pile<Symbole> pile;

	Symbole symbole;
	dls::chaine valeur;

	while (!est_identifiant(id_morceau::POINT_VIRGULE)) {
		symbole.identifiant = identifiant_courant();
		valeur = m_identifiants[position() + 1].chaine;

		if (est_identifiant(id_morceau::NOMBRE)) {
			symbole.valeur = std::any(std::stoi(valeur.c_str()));
			expression.pousse(symbole);
		}
		else if (est_identifiant(id_morceau::NOMBRE_DECIMAL)) {
			symbole.valeur = std::any(std::stof(valeur.c_str()));
			expression.pousse(symbole);
		}
		else if (est_identifiant(id_morceau::VRAI) || est_identifiant(id_morceau::FAUX)) {
			symbole.valeur = (valeur == "vrai");
			symbole.identifiant = id_morceau::BOOL;
			expression.pousse(symbole);
		}
		else if (est_identifiant(id_morceau::COULEUR)) {
			/* À FAIRE */
			expression.pousse(symbole);
		}
		else if (est_identifiant(id_morceau::VECTEUR)) {
			/* À FAIRE */
			expression.pousse(symbole);
		}
		else if (est_identifiant(id_morceau::CHAINE_LITTERALE)) {
			symbole.valeur = valeur;
			expression.pousse(symbole);
		}
		else if (est_identifiant(id_morceau::CHAINE_CARACTERE)) {
			if (!m_assembleuse.variable_connue(valeur)) {
				lance_erreur("Variable inconnue : " + valeur);
			}

			if (construit_graphe) {
				auto variable = m_assembleuse.variable(valeur);
				connecte(cont, variable);
			}

			symbole.valeur = std::any(valeur);
			expression.pousse(symbole);
		}
		else if (est_operateur(symbole.identifiant)) {
			while (!pile.est_vide()
				   && est_operateur(pile.haut().identifiant)
				   && (precedence_faible(symbole.identifiant, pile.haut().identifiant)))
			{
				expression.pousse(pile.haut());
				pile.depile();
			}

			symbole.valeur = std::any(valeur);
			pile.empile(symbole);
		}
		else if (est_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
			pile.empile(symbole);
		}
		else if (est_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
			if (pile.est_vide()) {
				lance_erreur("Il manque une paranthèse dans l'expression !");
			}

			while (pile.haut().identifiant != id_morceau::PARENTHESE_OUVRANTE) {
				expression.pousse(pile.haut());
				pile.depile();
			}

			/* Enlève la parenthèse restante de la pile. */
			if (pile.haut().identifiant == id_morceau::PARENTHESE_OUVRANTE) {
				pile.depile();
			}
		}

		avance();
	}

	while (!pile.est_vide()) {
		if (pile.haut().identifiant == id_morceau::PARENTHESE_OUVRANTE) {
			lance_erreur("Il manque une paranthèse dans l'expression !");
		}

		expression.pousse(pile.haut());
		pile.depile();
	}

#ifdef DEBOGUE_EXPRESSION
	std::ostream &os = std::cerr;

	os << "Expression (" << nom << ") : " ;

	for (const Symbole &sym : expression) {
		imprime_valeur_symbole(sym, os);
	}

	os << '\n';
#endif

	if (type == EXPRESSION_ENTREE || type == EXPRESSION_INTERFACE) {
		auto resultat = evalue_expression(expression, m_manipulable);

		switch (resultat.identifiant) {
			default:
				break;
			case id_morceau::NOMBRE:
				m_manipulable->ajoute_propriete(nom, TypePropriete::ENTIER, resultat.valeur);
				break;
			case id_morceau::NOMBRE_DECIMAL:
				m_manipulable->ajoute_propriete(nom, TypePropriete::DECIMAL, resultat.valeur);
				break;
			case id_morceau::BOOL:
				m_manipulable->ajoute_propriete(nom, TypePropriete::BOOL, resultat.valeur);
				break;
			case id_morceau::CHAINE_LITTERALE:
				m_manipulable->ajoute_propriete(nom, TypePropriete::CHAINE_CARACTERE, resultat.valeur);
				break;
			case id_morceau::COULEUR:
				m_manipulable->ajoute_propriete(nom, TypePropriete::COULEUR, resultat.valeur);
				break;
			case id_morceau::VECTEUR:
				m_manipulable->ajoute_propriete(nom, TypePropriete::VECTEUR, resultat.valeur);
				break;
		}

#ifdef DEBOGUE_EXPRESSION
		os << "Résultat : ";
		imprime_valeur_symbole(resultat, os);
		os << '\n';
#endif
	}

	if (construit_graphe) {
		m_assembleuse.ajoute_contrainte(cont);
		cont->m_expression = expression;
	}

	LOG << __func__ << " fin\n";
}

void AnalyseuseLogique::analyse_entreface()
{
	LOG << __func__ << '\n';

	if (!requiers_identifiant(id_morceau::ENTREFACE)) {
		lance_erreur("Attendu la déclaration 'entreface' !");
	}

	if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante !");
	}

	analyse_declaration(EXPRESSION_INTERFACE);

	if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante à la fin de l'entreface !");
	}

	LOG << __func__ << " fin\n";
}

void AnalyseuseLogique::analyse_logique()
{
	LOG << __func__ << '\n';

	if (!requiers_identifiant(id_morceau::LOGIQUE)) {
		lance_erreur("Attendu la déclaration 'logique' !");
	}

	if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante !");
	}

	analyse_relation();

	if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante à la fin de la logique !");
	}

	LOG << __func__ << " fin\n";
}

void AnalyseuseLogique::analyse_relation()
{
	LOG << __func__ << '\n';

	if (!est_identifiant(id_morceau::RELATION) && !est_identifiant(id_morceau::QUAND)) {
		return;
	}

	if (est_identifiant(id_morceau::QUAND)) {
		avance();

		if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
			lance_erreur("Attendu une paranthèse ouvrante !");
		}

		if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
			lance_erreur("Attendu le nom d'une variable !");
		}

		if (!requiers_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
			lance_erreur("Attendu une paranthèse fermante !");
		}
	}

	if (!requiers_identifiant(id_morceau::RELATION)) {
		lance_erreur("Attendu la déclaration 'relation' !");
	}

	if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante !");
	}

	analyse_declaration(EXPRESSION_RELATION);

	if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante à la fin de la relation !");
	}

	analyse_relation();

	LOG << __func__ << " fin\n";
}

void AnalyseuseLogique::analyse_sortie()
{
	LOG << __func__ << '\n';

	if (!requiers_identifiant(id_morceau::SORTIE)) {
		lance_erreur("Attendu la déclaration 'sortie' !");
	}

	if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante !");
	}

	analyse_declaration(EXPRESSION_SORTIE);

	if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante à la fin de la sortie !");
	}

	LOG << __func__ << " fin\n";
}

}  /* namespace danjo */
