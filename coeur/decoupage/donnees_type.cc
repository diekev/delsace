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
	for (int identifiant : autre.m_donnees) {
		m_donnees.push_back(identifiant);
	}
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
