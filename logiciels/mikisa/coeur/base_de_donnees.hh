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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/tableau.hh"

#include "noeud.hh"

enum class type_objet : char;

class Composite;
class Objet;

class BaseDeDonnees final {
	dls::tableau<Composite *> m_composites{};
	dls::tableau<Objet *> m_objets{};

	Noeud m_racine{};
	Noeud m_racine_composites{};
	Noeud m_racine_objets{};

public:
	BaseDeDonnees();

	~BaseDeDonnees();

	void reinitialise();

	/* ********************************************************************** */

	Noeud *racine();

	Noeud const *racine() const;

	/* ********************************************************************** */

	Objet *cree_objet(dls::chaine const &nom, type_objet type);

	Objet *objet(dls::chaine const &nom) const;

	void enleve_objet(Objet *objet);

	dls::tableau<Objet *> const &objets() const;

	Graphe *graphe_objets();

	Graphe const *graphe_objets() const;

	/* ********************************************************************** */

	Composite *cree_composite(dls::chaine const &nom);

	Composite *composite(dls::chaine const &nom) const;

	void enleve_composite(Composite *compo);

	dls::tableau<Composite *> const &composites() const;

	Graphe *graphe_composites();

	Graphe const *graphe_composites() const;
};

Noeud *cherche_noeud_pour_chemin(BaseDeDonnees &base, dls::chaine const &chemin);

Noeud const *cherche_noeud_pour_chemin(BaseDeDonnees const &base, dls::chaine const &chemin);
