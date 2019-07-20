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

#include <functional>
#include <iostream>
#include "biblinternes/structures/tableau.hh"

#include "../../math/vecteur.hh"

// http://ngildea.blogspot.fr/2014/09/dual-contouring-chunked-terrain.html

#define CHUNK_SIZE 8

typedef std::function<bool(const dls::math::vec3i &, const dls::math::vec3i &)> FilterNodesFunc;
typedef std::function<bool(const dls::math::vec3i &, const dls::math::vec3i &)> FindNodesFunc;

namespace octree {

void find_nodes(OctreeNode *node, FindNodesFunc &func, dls::tableau<OctreeNode *> &nodes)
{
	if (!node) {
		return;
	}

	if (!func(node->min, node->min + dls::math::vec3i(node->size))) {
		return;
	}

	if (node->type == node_type::leaf) {
		nodes.pousse(node);
	}
	else {
		for (int i = 0; i < 8; ++i) {
			find_nodes(node->children[i], func, nodes);
		}
	}
}

}  /* namespace octree */

class Chunk {
	dls::math::vec3i m_min;
	OctreeNode *m_octree_root = nullptr;

public:
	dls::math::vec3i getMin() const
	{
		return m_min;
	}

	dls::tableau<OctreeNode *> findNodes(FilterNodesFunc filterFunc)
	{
		dls::tableau<OctreeNode *> nodes;
		octree::find_nodes(m_octree_root, filterFunc, nodes);
		return nodes;
	}
};

Chunk *GetChunk(const dls::math::vec3i &/*min*/)
{
	return nullptr;
}

dls::tableau<OctreeNode *> FindSeamNode(Chunk *chunk)
{
	const auto &baseChunkMin = chunk->getMin();
	const auto &seamValues = baseChunkMin + dls::math::vec3i(CHUNK_SIZE);

	FilterNodesFunc selectionFuncs[8] = {
		[&](const dls::math::vec3i &/*min*/, const dls::math::vec3i &max)
		{
			return max.x == seamValues.x || max.y == seamValues.y || max.z == seamValues.z;
		},

		[&](const dls::math::vec3i &min, const dls::math::vec3i &/*max*/)
		{
			return min.x == seamValues.x;
		},

		[&](const dls::math::vec3i &min, const dls::math::vec3i &/*max*/)
		{
			return min.z == seamValues.z;
		},

		[&](const dls::math::vec3i &min, const dls::math::vec3i &/*max*/)
		{
			return min.x == seamValues.x && min.z == seamValues.z;
		},

		[&](const dls::math::vec3i &min, const dls::math::vec3i &/*max*/)
		{
			return min.x == seamValues.y;
		},

		[&](const dls::math::vec3i &min, const dls::math::vec3i &/*max*/)
		{
			return min.x == seamValues.x && min.y == seamValues.y;
		},

		[&](const dls::math::vec3i &min, const dls::math::vec3i &/*max*/)
		{
			return min.x == seamValues.x && min.y == seamValues.y;
		},

		[&](const dls::math::vec3i &min, const dls::math::vec3i &/*max*/)
		{
			return min.x == seamValues.x && min.y == seamValues.y && min.z == seamValues.z;
		},
	};

	dls::tableau<OctreeNode *> seamNodes;

	for (int i = 0; i < 8; ++i) {
		const auto &offsetMin = CHILD_MIN_OFFSET[i] * CHUNK_SIZE;
		const auto &chunkMin = baseChunkMin + offsetMin;

		if (Chunk *c = GetChunk(chunkMin)) {
			auto chunkNodes = c->findNodes(selectionFuncs[i]);
			seamNodes.insere(dls::end(seamNodes), dls::begin(chunkNodes), dls::end(chunkNodes));
		}
	}

	return seamNodes;
}
