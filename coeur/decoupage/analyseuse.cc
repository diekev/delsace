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

#include "analyseuse.h"

#ifdef DEBOGUE_IDENTIFIANT
#include <algorithm>
#include <iostream>
#endif

Analyseuse::Analyseuse(const std::vector<DonneesMorceaux> &identifiants, ContexteGenerationCode &contexte)
	: m_contexte(contexte)
	, m_identifiants(identifiants)
{}

#ifdef DEBOGUE_IDENTIFIANT
void Analyseuse::imprime_identifiants_plus_utilises(std::ostream &os, size_t nombre)
{
	std::vector<std::pair<int, int>> tableau(m_tableau_identifiant.size());
	std::copy(m_tableau_identifiant.begin(), m_tableau_identifiant.end(), tableau.begin());

	std::sort(tableau.begin(), tableau.end(),
			  [](const std::pair<int, int> &a, const std::pair<int, int> &b)
	{
		return a.second > b.second;
	});

	os << "--------------------------------\n";
	os << "Identifiant les plus comparées :\n";
	for (size_t i = 0; i < nombre; ++i) {
		os << chaine_identifiant(tableau[i].first) << " : " << tableau[i].second << '\n';
	}
	os << "--------------------------------\n";
}
#endif

bool Analyseuse::requiers_identifiant(id_morceau identifiant)
{
	if (m_position >= m_identifiants.size()) {
		return false;
	}

	const auto est_bon = this->est_identifiant(identifiant);

	avance();

	return est_bon;
}

void Analyseuse::avance(size_t n)
{
	m_position += n;
}

void Analyseuse::recule()
{
	m_position -= 1;
}

size_t Analyseuse::position()
{
	return m_position - 1;
}

bool Analyseuse::est_identifiant(id_morceau identifiant)
{
#ifdef DEBOGUE_IDENTIFIANT
	m_tableau_identifiant[identifiant]++;
#endif
	return identifiant == this->identifiant_courant();
}

bool Analyseuse::sont_2_identifiants(id_morceau id1, id_morceau id2)
{
	if (m_position + 2 >= m_identifiants.size()) {
		return false;
	}

#ifdef DEBOGUE_IDENTIFIANT
	m_tableau_identifiant[id1]++;
	m_tableau_identifiant[id2]++;
#endif

	return m_identifiants[m_position].identifiant == id1
			&& m_identifiants[m_position + 1].identifiant == id2;
}

bool Analyseuse::sont_3_identifiants(id_morceau id1, id_morceau id2, id_morceau id3)
{
	if (m_position + 3 >= m_identifiants.size()) {
		return false;
	}

#ifdef DEBOGUE_IDENTIFIANT
	m_tableau_identifiant[id1]++;
	m_tableau_identifiant[id2]++;
	m_tableau_identifiant[id3]++;
#endif

	return m_identifiants[m_position].identifiant == id1
			&& m_identifiants[m_position + 1].identifiant == id2
			&& m_identifiants[m_position + 2].identifiant == id3;
}

id_morceau Analyseuse::identifiant_courant() const
{
	if (m_position >= m_identifiants.size()) {
		return id_morceau::INCONNU;
	}

	return m_identifiants[m_position].identifiant;
}

void Analyseuse::lance_erreur(const std::string &quoi, erreur::type_erreur type)
{
	erreur::lance_erreur(quoi, m_contexte, m_identifiants[position()], type);
}
