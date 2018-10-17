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

	this->analyse_page();
}

void analyseuse_grammaire::analyse_page()
{
	while (!(m_position != m_identifiants.size())) {
		if (est_identifiant(ID_CHAINE_CARACTERE)) {
			// m_assembleuse.ajoute_noeud(ID_CHAINE_CARACTERE, m_identifiants[position()]);
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

			// m_assembleuse.ajoute_noeud(ID_DEBUT_VARIABLE, m_identifiants[position()]);

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
	}
	else if (est_identifiant(ID_FINSI)) {
		// assert(m_assembleuse.noued_courant() == 'SI' || 'SINON');
		avance();
	}
	else if (est_identifiant(ID_FINPOUR)) {
		// assert(m_assembleuse.noued_courant() == 'POUR');
		avance();
	}
	else {
		lance_erreur("Identifiant inconnu");
	}

	if (!requiers_identifiant(ID_FIN_EXPRESSION)) {
		lance_erreur("Attendu '%}'");
	}
}

void analyseuse_grammaire::analyse_si()
{
	if (!requiers_identifiant(ID_SI)) {
		lance_erreur("Attendu 'si'");
	}

	if (!requiers_identifiant(ID_CHAINE_CARACTERE)) {
		lance_erreur("Attendu une chaîne de caractère après 'si'");
	}
}

void analyseuse_grammaire::analyse_pour()
{
	if (!requiers_identifiant(ID_POUR)) {
		lance_erreur("Attendu 'pour'");
	}

	if (!requiers_identifiant(ID_CHAINE_CARACTERE)) {
		lance_erreur("Attendu une chaîne de caractère après 'pour'");
	}

	if (!requiers_identifiant(ID_DANS)) {
		lance_erreur("Attendu 'dans'");
	}

	if (!requiers_identifiant(ID_CHAINE_CARACTERE)) {
		lance_erreur("Attendu une chaîne de caractère après 'dans'");
	}
}
