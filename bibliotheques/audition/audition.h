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

#pragma once

#include <vector>

class Audite;

/* ************************************************************************** */

class Auditeur {
protected:
	Audite *m_audite = nullptr;

public:
	virtual ~Auditeur();

	void ecoute(Audite *audite);

	virtual int montre_dialogue(int dialogue);

	virtual void ajourne_etat(int evenement) = 0;
};

/* ************************************************************************** */

class Audite {
	std::vector<Auditeur *> m_auditeurs{};
	Auditeur *m_auditeur_dialogue = nullptr;

public:
	Audite() = default;

	/* l'auditeur_dialogue peut-être partagé */
	Audite(Audite const &) = default;
	Audite &operator=(Audite const &) = default;

	void ajoute_auditeur(Auditeur *auditeur);

	void enleve_auditeur(Auditeur *auditeur);

	void notifie_auditeurs(int evenement) const;

	void ajoute_auditeur_dialogue(Auditeur *auditeur);

	int requiers_dialogue(int dialogue);
};
