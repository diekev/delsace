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

#include "audition.h"

#include <algorithm>

/* ************************************************************************** */

Auditeur::~Auditeur()
{
}

void Auditeur::ecoute(Audite *audite)
{
	m_audite = audite;
	m_audite->ajoute_auditeur(this);
}

int Auditeur::montre_dialogue(int dialogue)
{
	return 0;
}

/* ************************************************************************** */

void Audite::ajoute_auditeur(Auditeur *auditeur)
{
	m_auditeurs.push_back(auditeur);
}

void Audite::enleve_auditeur(Auditeur *auditeur)
{
	auto iter = std::find(m_auditeurs.begin(), m_auditeurs.end(), auditeur);
	m_auditeurs.erase(iter);

	if (m_auditeur_dialogue == auditeur) {
		m_auditeur_dialogue = nullptr;
	}
}

void Audite::notifie_auditeurs(int evenement) const
{
	for (auto &auditeur : m_auditeurs) {
		auditeur->ajourne_etat(evenement);
	}
}

void Audite::ajoute_auditeur_dialogue(Auditeur *auditeur)
{
	m_auditeur_dialogue = auditeur;
}

int Audite::requiers_dialogue(int dialogue)
{
	if (m_auditeur_dialogue == nullptr) {
		return 0;
	}

	return m_auditeur_dialogue->montre_dialogue(dialogue);
}
