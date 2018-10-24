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

void DonneesType::pousse(id_morceau identifiant)
{
	m_donnees.push_back(identifiant);
}

void DonneesType::pousse(const DonneesType &autre)
{
	const auto taille = m_donnees.size();
	m_donnees.resize(taille + autre.m_donnees.size());
	std::copy(autre.m_donnees.begin(), autre.m_donnees.end(), m_donnees.begin() + taille);
}

id_morceau DonneesType::type_base() const
{
	return m_donnees.front();
}

bool DonneesType::est_invalide() const
{
	if (m_donnees.empty()) {
		return true;
	}

	switch (m_donnees.back()) {
		case id_morceau::POINTEUR:
		case id_morceau::REFERENCE:
		case id_morceau::TABLEAU:
			return true;
		default:
			return false;
	}
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
	if (donnees_type.est_invalide()) {
		os << "type invalide";
	}
	else {
		auto debut = donnees_type.end() - 1;
		auto fin = donnees_type.begin() - 1;

		for (;debut != fin; --debut) {
			auto donnee = *debut;
			switch (donnee & 0xff) {
				case id_morceau::POINTEUR:
					os << '*';
					break;
				case id_morceau::TABLEAU:
					os << '[';
					os << static_cast<size_t>(donnee >> 8);
					os << ']';
					break;
				case id_morceau::N8:
					os << "n8";
					break;
				case id_morceau::N16:
					os << "n16";
					break;
				case id_morceau::N32:
					os << "n32";
					break;
				case id_morceau::N64:
					os << "n64";
					break;
				case id_morceau::R16:
					os << "r16";
					break;
				case id_morceau::R32:
					os << "r32";
					break;
				case id_morceau::R64:
					os << "r64";
					break;
				case id_morceau::Z8:
					os << "z8";
					break;
				case id_morceau::Z16:
					os << "z16";
					break;
				case id_morceau::Z32:
					os << "z32";
					break;
				case id_morceau::Z64:
					os << "z64";
					break;
				default:
					os << chaine_identifiant(donnee & 0xff);
					break;
			}
		}
	}

	return os;
}
