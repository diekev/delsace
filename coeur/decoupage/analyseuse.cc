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

#include <sstream>

#include "erreur.h"
#include "unicode.h"

Analyseuse::Analyseuse(const TamponSource &tampon)
	: m_tampon(tampon)
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

void Analyseuse::avance()
{
	++m_position;
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

	return m_identifiants[m_position].identifiant == id1
			&& m_identifiants[m_position + 1].identifiant == id2;
}

bool Analyseuse::sont_3_identifiants(int id1, int id2, int id3)
{
	if (m_position + 3 >= m_identifiants.size()) {
		return false;
	}

	return m_identifiants[m_position].identifiant == id1
			&& m_identifiants[m_position + 1].identifiant == id2
			&& m_identifiants[m_position + 2].identifiant == id3;
}

int Analyseuse::identifiant_courant() const
{
	if (m_position >= m_identifiants.size()) {
		return ID_INCONNU;
	}

	return m_identifiants[m_position].identifiant;
}

void Analyseuse::lance_erreur(const std::string &quoi)
{
	const auto ligne = m_identifiants[position()].ligne;
	const auto pos_mot = m_identifiants[position()].pos;
	const auto identifiant = m_identifiants[position()].identifiant;
	const auto &chaine = m_identifiants[position()].chaine;

	auto ligne_courante = m_tampon[ligne];

	std::stringstream ss;
	ss << "Erreur : ligne:" << ligne + 1 << ":\n";
	ss << ligne_courante;

	/* La position ligne est en octet, il faut donc compter le nombre d'octets
	 * de chaque point de code pour bien formater l'erreur. */
	for (size_t i = 0; i < pos_mot; i += static_cast<size_t>(nombre_octets(&ligne_courante[i]))) {
		if (ligne_courante[i] == '\t') {
			ss << '\t';
		}
		else {
			ss << ' ';
		}
	}

	ss << '^';

	for (size_t i = 0; i < chaine.size() - 1; ++i) {
		ss << '~';
	}

	ss << '\n';

	ss << quoi;
	ss << ", obtenu : " << chaine << " (" << chaine_identifiant(identifiant) << ')';

	throw erreur::frappe(ss.str().c_str());
}
