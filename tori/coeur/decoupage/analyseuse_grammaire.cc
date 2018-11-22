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

#include <iostream>
#include <set>

#include "arbre_syntactic.hh"
#include "erreur.hh"

/* ************************************************************************** */

analyseuse_grammaire::analyseuse_grammaire(const std::vector<DonneesMorceaux> &identifiants, const TamponSource &tampon)
	: Analyseuse(identifiants, tampon)
{}

void analyseuse_grammaire::lance_analyse()
{
	m_position = 0;

	if (m_identifiants.size() == 0) {
		return;
	}

	m_assembleuse.empile_noeud(type_noeud::BLOC, {});

	this->analyse_page();

	m_assembleuse.imprime_arbre(std::cerr);

	m_assembleuse.depile_noeud(type_noeud::BLOC);
}

void analyseuse_grammaire::analyse_page()
{
	while ((m_position != m_identifiants.size())) {
		if (est_identifiant(ID_CHAINE_CARACTERE)) {
			m_assembleuse.ajoute_noeud(type_noeud::CHAINE_CARACTERE, m_identifiants[position()]);
			avance();
		}
		else if (est_identifiant(ID_DEBUT_EXPRESSION)) {
			analyse_expression();
		}
		else if (est_identifiant(ID_DEBUT_VARIABLE)) {
			avance();

			if (!requiers_identifiant(ID_CHAINE_CARACTERE)) {
				lance_erreur("Attendu identifiant de la variable après '{{'");
			}

			m_assembleuse.ajoute_noeud(type_noeud::VARIABLE, m_identifiants[position()]);

			if (!requiers_identifiant(ID_FIN_VARIABLE)) {
				lance_erreur("Attendu '}}' à la fin de la déclaration d'une variable");
			}
		}
	}
}

void analyseuse_grammaire::analyse_expression()
{
	if (!requiers_identifiant(ID_DEBUT_EXPRESSION)) {
		lance_erreur("Attendu '{%'");
	}

	if (est_identifiant(ID_SI)) {
		analyse_si();
	}
	else if (est_identifiant(ID_POUR)) {
		analyse_pour();
	}
	else if (est_identifiant(ID_SINON)) {
		// assert(m_assembleuse.noued_courant() == 'SI');
		avance();

		if (!requiers_identifiant(ID_FIN_EXPRESSION)) {
			lance_erreur("Attendu '%}'");
		}

		m_assembleuse.empile_noeud(type_noeud::BLOC, m_identifiants[position()]);

		analyse_page();

		m_assembleuse.depile_noeud(type_noeud::BLOC);
	}
	else if (est_identifiant(ID_FINSI)) {
		avance();

		m_assembleuse.depile_noeud(type_noeud::SI);

		if (!requiers_identifiant(ID_FIN_EXPRESSION)) {
			lance_erreur("Attendu '%}'");
		}
	}
	else if (est_identifiant(ID_FINPOUR)) {
		avance();

		m_assembleuse.depile_noeud(type_noeud::POUR);

		if (!requiers_identifiant(ID_FIN_EXPRESSION)) {
			lance_erreur("Attendu '%}'");
		}
	}
	else {
		lance_erreur("Identifiant inconnu");
	}
}

void analyseuse_grammaire::analyse_si()
{
	if (!requiers_identifiant(ID_SI)) {
		lance_erreur("Attendu 'si'");
	}

	m_assembleuse.empile_noeud(type_noeud::SI, m_identifiants[position()]);

	if (!requiers_identifiant(ID_CHAINE_CARACTERE)) {
		lance_erreur("Attendu une chaîne de caractère après 'si'");
	}

	m_assembleuse.ajoute_noeud(type_noeud::VARIABLE, m_identifiants[position()]);

	if (!requiers_identifiant(ID_FIN_EXPRESSION)) {
		lance_erreur("Attendu '%}'");
	}

	m_assembleuse.empile_noeud(type_noeud::BLOC, m_identifiants[position()]);

	analyse_page();

	m_assembleuse.depile_noeud(type_noeud::BLOC);
}

void analyseuse_grammaire::analyse_pour()
{
	if (!requiers_identifiant(ID_POUR)) {
		lance_erreur("Attendu 'pour'");
	}

	m_assembleuse.empile_noeud(type_noeud::POUR, m_identifiants[position()]);

	if (!requiers_identifiant(ID_CHAINE_CARACTERE)) {
		lance_erreur("Attendu une chaîne de caractère après 'pour'");
	}

	m_assembleuse.ajoute_noeud(type_noeud::VARIABLE, m_identifiants[position()]);

	if (!requiers_identifiant(ID_DANS)) {
		lance_erreur("Attendu 'dans'");
	}

	if (!requiers_identifiant(ID_CHAINE_CARACTERE)) {
		lance_erreur("Attendu une chaîne de caractère après 'dans'");
	}

	m_assembleuse.ajoute_noeud(type_noeud::VARIABLE, m_identifiants[position()]);

	if (!requiers_identifiant(ID_FIN_EXPRESSION)) {
		lance_erreur("Attendu '%}'");
	}

	m_assembleuse.empile_noeud(type_noeud::BLOC, m_identifiants[position()]);

	analyse_page();

	m_assembleuse.depile_noeud(type_noeud::BLOC);
}
