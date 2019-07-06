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

#include "observation.hh"

#include "../outils/definitions.hh"

#include <algorithm>

/* ************************************************************************** */

void Observatrice::observe(Sujette *sujette)
{
	m_sujette = sujette;
	m_sujette->ajoute_observatrice(this);
}

int Observatrice::montre_dialogue(int dialogue)
{
	INUTILISE(dialogue);
	return 0;
}

/* ************************************************************************** */

void Sujette::ajoute_observatrice(Observatrice *observatrice)
{
	m_observatrices.push_back(observatrice);
}

void Sujette::enleve_observatrice(Observatrice *observatrice)
{
	auto iter = std::find(m_observatrices.begin(), m_observatrices.end(), observatrice);
	m_observatrices.erase(iter);

	if (m_observatrice_dialogue == observatrice) {
		m_observatrice_dialogue = nullptr;
	}
}

void Sujette::notifie_observatrices(int evenement) const
{
	for (auto &observatrice : m_observatrices) {
		observatrice->ajourne_etat(evenement);
	}
}

void Sujette::ajoute_observatrice_dialogue(Observatrice *observatrice)
{
	m_observatrice_dialogue = observatrice;
}

int Sujette::requiers_dialogue(int dialogue)
{
	if (m_observatrice_dialogue == nullptr) {
		return 0;
	}

	return m_observatrice_dialogue->montre_dialogue(dialogue);
}
