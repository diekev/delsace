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

#include "analyseuse.hh"

#include "erreur.hh"

Analyseuse::Analyseuse(const std::vector<DonneesMorceaux> &identifiants, const TamponSource &tampon)
	: m_tampon(tampon)
	, m_identifiants(identifiants)
{}

bool Analyseuse::requiers_identifiant(int identifiant)
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

bool Analyseuse::est_identifiant(int identifiant)
{
	return identifiant == this->identifiant_courant();
}

bool Analyseuse::sont_2_identifiants(int id1, int id2)
{
	if (m_position + 2 >= m_identifiants.size()) {
		return false;
	}

	return m_identifiants[m_position].identifiant == static_cast<size_t>(id1)
			&& m_identifiants[m_position + 1].identifiant == static_cast<size_t>(id2);
}

bool Analyseuse::sont_3_identifiants(int id1, int id2, int id3)
{
	if (m_position + 3 >= m_identifiants.size()) {
		return false;
	}

	return m_identifiants[m_position].identifiant == static_cast<size_t>(id1)
			&& m_identifiants[m_position + 1].identifiant == static_cast<size_t>(id2)
			&& m_identifiants[m_position + 2].identifiant == static_cast<size_t>(id3);
}

int Analyseuse::identifiant_courant() const
{
	if (m_position >= m_identifiants.size()) {
		return ID_INCONNU;
	}

	return static_cast<int>(m_identifiants[m_position].identifiant);
}

void Analyseuse::lance_erreur(const std::string &quoi, int type)
{
	erreur::lance_erreur(quoi, m_tampon, m_identifiants[position()], type);
}
