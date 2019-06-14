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

#include "octree.h"

#include <algorithm>
#include <cassert>

namespace octree {

bool contains(const dls::math::vec3i &min, const dls::math::vec3i &max, const dls::math::vec3i &pos)
{
	return min <= pos && pos <= max;
}

Octree::Octree()
    : m_background(0.0f)
{
	m_root = Node::create();
	m_root->size = 32;
	m_root->type = node_type::pseudo;

	for (auto &c : m_root->children) {
		c = nullptr;
	}
}

const float &Octree::at(const int x, const int y, const int z) const
{
	auto node = find_node(m_root.get(), dls::math::vec3i{ x, y, z });

	if (node == nullptr) {
		return m_background;
	}

	return node->value;
}

float &Octree::at(const int x, const int y, const int z)
{
	auto node = construct_node(m_root.get(), dls::math::vec3i{ x, y, z });
	return node->value;
}

const float &Octree::operator()(const int x, const int y, const int z) const
{
	std::cerr << "const operator()\n";
	return at(x, y, z);
}

float &Octree::operator()(const int x, const int y, const int z)
{
	std::cerr << "operator()\n";
	return at(x, y, z);
}

Node *Octree::find_node(Node *root, const dls::math::vec3i &pos) const
{
	if (root == nullptr) {
		return nullptr;
	}

	if (root->type == node_type::leaf) {
		return root;
	}

	Node *node = nullptr;

	for (const auto &child : root->children) {
		const auto &min = child->min, max = min + dls::math::vec3i(child->size);

		if (contains(min, max, pos)) {
			node = child.get();
			break;
		}
	}

	return find_node(node, pos);
}

Node *Octree::construct_node(Node *root, const dls::math::vec3i &pos)
{
	const auto &size = root->size / 2;
	const auto &type = (size == 1) ? node_type::leaf : node_type::internal;

	for (int i = 0; i < 8; ++i) {
		if (root->children[i] != nullptr) {
			continue;
		}

		root->children[i] = Node::create();
		root->children[i]->size = size;
		root->children[i]->type = type;
		root->children[i]->min = root->min + (CHILD_MIN_OFFSET[i] * size);
	}

	Node *node = nullptr;

	for (const auto &child : root->children) {
		const auto &min = child->min, max = min + dls::math::vec3i(child->size);

		if (contains(min, max, pos)) {
			node = child.get();
			break;
		}
	}

	assert(node != nullptr);

	if (type == node_type::leaf) {
		return node;
	}

	return construct_node(node, pos);
}

}  /* namespace octree */
