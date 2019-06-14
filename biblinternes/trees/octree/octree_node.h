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

#include "../../math/vecteur.hh"

const dls::math::vec3i CHILD_MIN_OFFSET[] = {
	dls::math::vec3i{ 0, 0, 0 },
	dls::math::vec3i{ 0, 0, 1 },
	dls::math::vec3i{ 0, 1, 0 },
	dls::math::vec3i{ 0, 1, 1 },
	dls::math::vec3i{ 1, 0, 0 },
	dls::math::vec3i{ 1, 0, 1 },
	dls::math::vec3i{ 1, 1, 0 },
	dls::math::vec3i{ 1, 1, 1 }
};

enum class material_type {
	solid = 0,
	air   = 1,
};

enum class node_type {
	none     = 0,
	internal = 1,
	pseudo   = 2,
	leaf     = 3,
};

struct OctreeDrawInfo {
	int index = -1;
	int corners = 0;
	dls::math::vec3f position{0.0f, 0.0f, 0.0f};
	dls::math::vec3f average_normal{0.0f, 0.0f, 0.0f};
//	QEF qef;
};

class OctreeNode {
public:
	dls::math::vec3i min;
	int size;
	node_type type;
	OctreeNode *children[8];
	OctreeDrawInfo *draw_info;

	OctreeNode()
		: OctreeNode(node_type::none)
	{}

	explicit OctreeNode(const node_type &type_)
		: min(0)
		, size(0)
		, type(type_)
		, draw_info(nullptr)
	{
		for (int i = 0; i < 8; ++i) {
			children[i] = nullptr;
		}
	}
};

OctreeNode *construct_leaf(OctreeNode *leaf);
OctreeNode *construct_octree_nodes(OctreeNode *node);
