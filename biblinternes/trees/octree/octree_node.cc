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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "octree_node.h"

float sphere(const dls::math::vec3f &pos, const dls::math::vec3f &origin, float radius)
{
	return dls::math::longueur(pos - origin) - radius;
}

float density_func(const dls::math::vec3i &)
{
	return 0.0f;
}

OctreeNode *construct_leaf(OctreeNode *leaf)
{
	int corners = 0;

	for (int i = 0; i < 8; ++i) {
		const auto cornerpos = leaf->min + CHILD_MIN_OFFSET[i];
		const auto density = density_func(cornerpos);
		const auto material = ((density < 0.0f) ? material_type::solid : material_type::air);
		corners |= (static_cast<int>(material) << i);
	}

	if ((corners == 0) || (corners == 255)) {
		delete leaf;
		return nullptr;
	}

	leaf->type = node_type::leaf;

	return leaf;
}

OctreeNode *construct_octree_nodes(OctreeNode *node)
{
	if (!node) {
		return nullptr;
	}

	if (node->size == 1) {
		return construct_leaf(node);
	}

	const int child_size = node->size >> 1;
	bool has_children = false;

	for (int i = 0; i < 8; ++i) {
		OctreeNode *child = new OctreeNode;
		child->size = child_size;
		child->min = node->min + (CHILD_MIN_OFFSET[i] * child_size);
		child->type = node_type::internal;

		node->children[i] = construct_octree_nodes(child);
		has_children |= (node->children[i] != nullptr);
	}

	if (!has_children) {
		delete node;
		return nullptr;
	}

	return node;
}
