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

#include "donnees_type.h"

#include "morceaux.h"

void DonneesType::pousse(int identifiant)
{
	m_donnees.push_back(identifiant);
}

void DonneesType::pousse(const DonneesType &autre)
{
	const auto taille = m_donnees.size();
	m_donnees.resize(taille + autre.m_donnees.size());
	std::copy(autre.m_donnees.begin(), autre.m_donnees.end(), m_donnees.begin() + taille);
}

int DonneesType::type_base() const
{
	return m_donnees.front();
}

bool DonneesType::est_invalide() const
{
	if (m_donnees.empty()) {
		return true;
	}

	switch (m_donnees.back()) {
		case ID_POINTEUR:
		case ID_REFERENCE:
		case ID_TABLEAU:
			return true;
	}

	return false;
}

DonneesType::iterateur_const DonneesType::begin() const
{
	return m_donnees.rbegin();
}

DonneesType::iterateur_const DonneesType::end() const
{
	return m_donnees.rend();
}

DonneesType DonneesType::derefence() const
{
	auto donnees = DonneesType{};

	for (size_t i = 1; i < m_donnees.size(); ++i) {
		donnees.pousse(m_donnees[i]);
	}

	return donnees;
}

std::ostream &operator<<(std::ostream &os, const DonneesType &donnees_type)
{
	for (const auto &donnee : donnees_type) {
		switch (donnee & 0xff) {
			case ID_POINTEUR:
				os << '*';
				break;
			case ID_TABLEAU:
				os << '[';
				os << (donnee >> 8);
				os << ']';
				break;
			case ID_N8:
				os << "n8";
				break;
			case ID_N16:
				os << "n16";
				break;
			case ID_N32:
				os << "n32";
				break;
			case ID_N64:
				os << "n64";
				break;
			case ID_R16:
				os << "r16";
				break;
			case ID_R32:
				os << "r32";
				break;
			case ID_R64:
				os << "r64";
				break;
			case ID_Z8:
				os << "z8";
				break;
			case ID_Z16:
				os << "z16";
				break;
			case ID_Z32:
				os << "z32";
				break;
			case ID_Z64:
				os << "z64";
				break;
			default:
				os << chaine_identifiant(donnee & 0xff);
				break;
		}
	}

	return os;
}
