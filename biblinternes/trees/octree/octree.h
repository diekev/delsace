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

#pragma once

#include <memory>

#include "octree_node.h"

namespace octree {

struct Node {
	using Ptr = std::unique_ptr<Node>;

	Ptr children[8] = { nullptr };

	float value{0.0f};
	dls::math::vec3i min{0, 0, 0};
	int size{0};
	node_type type{node_type::none};

	Node() = default;

	explicit Node(float val)
		: Node()
	{
		value = val;
	}

	~Node() = default;

	Node(const Node &) = delete;
	Node &operator=(const Node &) = delete;

	template <typename... Args>
	static Ptr create(Args &&... args)
	{
		return Ptr(new Node(std::forward<Args>(args)...));
	}
};

class Octree {
	Node::Ptr m_root = nullptr;
	float m_background = 0.0f;

public:
	Octree();

	Octree(const Octree &) = delete;
	Octree &operator=(const Octree &) = delete;

	~Octree() = default;

	float &at(const int x, const int y, const int z);
	const float &at(const int x, const int y, const int z) const;

	float &operator()(const int x, const int y, const int z);
	const float &operator()(const int x, const int y, const int z) const;

private:
	Node *find_node(Node *root, const dls::math::vec3i &pos) const;
	Node *construct_node(Node *root, const dls::math::vec3i &pos);
};

}  /* namespace octree */
