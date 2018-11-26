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

#include "object.h"

#include "sdk/context.h"
#include "sdk/primitive.h"

#include "operatrices/operatrices_standards.h"

#include "scene.h"
#include "task.h"

Object::Object(const Context &contexte)
	: m_graph(contexte)
{
	add_input("Parent");
	add_output("Child");

	ajoute_propriete("position", danjo::TypePropriete::VECTEUR, dls::math::vec3f(0.0));
	ajoute_propriete("rotation", danjo::TypePropriete::VECTEUR, dls::math::vec3f(0.0));
	ajoute_propriete("taille", danjo::TypePropriete::VECTEUR, dls::math::vec3f(1.0));

	updateMatrix();
}

PrimitiveCollection *Object::collection() const
{
	return m_collection;
}

void Object::collection(PrimitiveCollection *coll)
{
	m_collection = coll;
}

void Object::matrix(const dls::math::mat4x4d &m)
{
	m_matrix = m;
}

const dls::math::mat4x4d &Object::matrix() const
{
	return m_matrix;
}

void Object::ajoute_noeud(Noeud *noeud)
{
	m_graph.ajoute(noeud);
	m_graph.noeud_actif(noeud);

	/* À FAIRE : quand on ouvre un fichier de sauvegarde, il y a un crash quand
	 * on clique dans l'éditeur de graphe. Ceci n'est sans doute pas la bonne
	 * correction. */
	m_graph.ajoute_selection(noeud);
}

Graph *Object::graph()
{
	return &m_graph;
}

const Graph *Object::graph() const
{
	return &m_graph;
}

void Object::updateMatrix()
{
	const auto m_pos = evalue_vecteur("position");
	const auto m_rotation = evalue_vecteur("rotation");
	const auto m_scale = evalue_vecteur("taille");

	auto const angle_x = static_cast<double>(dls::math::degrees_vers_radians(m_rotation.x));
	auto const angle_y = static_cast<double>(dls::math::degrees_vers_radians(m_rotation.y));
	auto const angle_z = static_cast<double>(dls::math::degrees_vers_radians(m_rotation.z));

	m_matrix = dls::math::mat4x4d(1.0);
	m_matrix = dls::math::translation(m_matrix, dls::math::vec3d(m_pos));
	m_matrix = dls::math::rotation(m_matrix, angle_x, dls::math::vec3d(1.0f, 0.0f, 0.0f));
	m_matrix = dls::math::rotation(m_matrix, angle_y, dls::math::vec3d(0.0f, 1.0f, 0.0f));
	m_matrix = dls::math::rotation(m_matrix, angle_z, dls::math::vec3d(0.0f, 0.0f, 1.0f));
	m_matrix = dls::math::dimension(m_matrix, dls::math::vec3d(m_scale));

	m_inv_matrix = dls::math::inverse(m_matrix);
}

void Object::addChild(Object *child)
{
	m_children.push_back(child);
	child->parent(this);
}

void Object::removeChild(Object *child)
{
	auto iter = std::find(m_children.begin(), m_children.end(), child);
	assert(iter != m_children.end());
	m_children.erase(iter);
	child->parent(nullptr);
}

const std::vector<Object *> &Object::children() const
{
	return m_children;
}

Object *Object::parent() const
{
	return m_parent;
}

void Object::parent(Object *parent)
{
	m_parent = parent;
}
