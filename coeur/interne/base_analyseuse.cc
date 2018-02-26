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

#include "base_analyseuse.h"

#include "erreur.h"

namespace kangao {

bool Analyseuse::requiers_identifiant(int identifiant)
{
	if (m_position >= m_identifiants.size()) {
		return false;
	}

	const auto est_bon = this->est_identifiant(identifiant);

	avance();

	return est_bon;
}

void Analyseuse::avance()
{
	++m_position;
}

void Analyseuse::recule()
{
	m_position -= 1;
}

int Analyseuse::position()
{
	return m_position - 1;
}

bool Analyseuse::est_identifiant(int identifiant)
{
	return identifiant == this->identifiant_courant();
}

int Analyseuse::identifiant_courant() const
{
	if (m_position >= m_identifiants.size()) {
		return IDENTIFIANT_NUL;
	}

	return m_identifiants[m_position].identifiant;
}

void Analyseuse::lance_erreur(const std::string &quoi)
{
	const auto numero_ligne = m_identifiants[position()].numero_ligne;
	const auto ligne = m_identifiants[position()].ligne;
	const auto position_ligne = m_identifiants[position()].position_ligne;
	const auto contenu = m_identifiants[position()].contenu;

	throw ErreurSyntactique(ligne, numero_ligne, position_ligne, quoi, contenu);
}

}  /* namespace kangao */
