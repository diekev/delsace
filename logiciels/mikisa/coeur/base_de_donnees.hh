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

#include <string>

#include "biblinternes/structures/tableau.hh"

class Composite;
class Objet;
class Scene;

class BaseDeDonnees final {
	dls::tableau<Composite *> m_composites{};
	dls::tableau<Objet *> m_objets{};
	dls::tableau<Scene *> m_scenes{};

public:
	BaseDeDonnees() = default;

	~BaseDeDonnees();

	void reinitialise();

	/* ********************************************************************** */

	Objet *cree_objet(std::string const &nom);

	Objet *objet(std::string const &nom) const;

	void enleve_objet(Objet *objet);

	dls::tableau<Objet *> const &objets() const;

	/* ********************************************************************** */

	Scene *cree_scene(std::string const &nom);

	Scene *scene(std::string const &nom) const;

	dls::tableau<Scene *> const &scenes() const;

	/* ********************************************************************** */

	Composite *cree_composite(std::string const &nom);

	Composite *composite(std::string const &nom) const;

	dls::tableau<Composite *> const &composites() const;
};
