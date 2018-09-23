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

#include "bibliotheques/outils/iterateurs.h"

#include <vector>

class Courbes;
class Maillage;
class NuagePoints;

struct Corps;

/**
 * Une Collection est une liste de Corps composants un Objet.
 */
class Collection {
	std::vector<Corps *> m_corps;

public:
	using plage_corps = plage_iterable<std::vector<Corps *>::iterator>;
	using plage_corps_const = plage_iterable<std::vector<Corps *>::const_iterator>;

	/**
	 * Détruit la collection et supprime tous les corps lui appertenant.
	 */
	~Collection();

	/**
	 * Crée un maillage dans cette collection, et retourne le pointeur du
	 * maillage ainsi créé.
	 */
	Maillage *cree_maillage();

	/**
	 * Crée des courbes dans cette collection, et retourne le pointeur des
	 * courbes ainsi créé.
	 */
	Courbes *cree_courbes();

	/**
	 * Crée des points dans cette collection, et retourne le pointeur des
	 * points ainsi créé.
	 */
	NuagePoints *cree_points();

	/**
	 * Ajoute un corps à cette collection.
	 */
	void ajoute_corps(Corps *corps);

	/**
	 * Vide la liste de corps de cette collection. Si supprime_corps est vrai,
	 * les corps de cette collection seront supprimés et l'utilisant de tout
	 * pointeur vers ceux-ci est indéfini (menant sans doute à un crash).
	 */
	void reinitialise(bool supprime_corps = false);

	/**
	 * Transfers les corps de cette collection à celle spécifiée. Après cette
	 * opération, cette collection sera vide.
	 */
	void transfers_corps_a(Collection &autre);

	/**
	 * Réserve de la place (mémoire) supplémentaire dans cette collection. Cette
	 * méthode est généralement appelée avant d'ajouter une grande quantité de
	 * Corps à cette collection, par exemple via un appel à `transfers_corps_a`.
	 */
	void reserve_supplement(const size_t nombre);

	/**
	 * Retourne une plage itérable sur les corps de cette collection.
	 */
	plage_corps plage();

	/**
	 * Retourne une plage itérable constante sur les corps de cette collection.
	 */
	plage_corps_const plage() const;
};
