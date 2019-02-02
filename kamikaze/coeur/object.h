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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "graphs/object_graph.h"
#include "graphs/scene_node.h"

class Node;
class Noeud;
class PrimitiveCollection;

class Object : public SceneNode {
	PrimitiveCollection *m_collection = nullptr;

	dls::math::mat4x4d m_matrix = dls::math::mat4x4d(1.0);
	dls::math::mat4x4d m_inv_matrix = dls::math::mat4x4d(1.0);

	Graph m_graph;

	Object *m_parent = nullptr;
	std::vector<Object *> m_children{};

public:
	explicit Object(Context const &contexte);
	~Object() = default;

	Object(Object const &) = default;
	Object &operator=(Object const &) = default;

	PrimitiveCollection *collection() const;
	void collection(PrimitiveCollection *coll);

	/* Return the object's matrix. */
	void matrix(dls::math::mat4x4d const &m);
	dls::math::mat4x4d const &matrix() const;

	/* Noeuds. */
	void ajoute_noeud(Noeud *noeud);

	Graph *graph();
	const Graph *graph() const;

	void updateMatrix();

	void addChild(Object *child);
	void removeChild(Object *child);
	const std::vector<Object *> &children() const;

	Object *parent() const;
	void parent(Object *parent);
};
