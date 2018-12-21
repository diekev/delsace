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

/**
 * Implémentation du patron de conception "Observateur".
 * https://fr.wikipedia.org/wiki/Observateur_(patron_de_conception)
 *
 * NOTE : ceci n'est qu'un graphe de dépendance déguisé ; peut-être serait-ce
 * intéressant de rendre le graphe explicite et d'utiliser la théorie des
 * graphes pour résoudre ce problème.
 */

class Sujette;

/* ************************************************************************** */

class Observatrice {
protected:
	Sujette *m_sujette = nullptr;

public:
	virtual ~Observatrice() = default;

	void observe(Sujette *sujette);

	virtual int montre_dialogue(int dialogue);

	virtual void ajourne_etat(int evenement) = 0;
};

/* ************************************************************************** */

class Sujette {
	std::vector<Observatrice *> m_observatrices{};
	Observatrice *m_observatrice_dialogue = nullptr;

public:
	Sujette() = default;

	/* l'observatrice_dialogue peut être partagée */
	Sujette(Sujette const &) = default;
	Sujette &operator=(Sujette const &) = default;

	void ajoute_observatrice(Observatrice *observatrice);

	void enleve_observatrice(Observatrice *observatrice);

	void notifie_observatrices(int evenement) const;

	void ajoute_observatrice_dialogue(Observatrice *observatrice);

	int requiers_dialogue(int dialogue);
};
