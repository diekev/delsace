#include <glm/glm.hpp>

float sphere(const glm::vec3 &pos, const glm::vec3 &origin, float radius)
{
	return glm::length(pos - origin) - radius;
}

const glm::ivec3 CHILD_MIN_OFFSET[] = {
	{ 0, 0, 0 },
	{ 0, 0, 1 },
	{ 0, 1, 0 },
	{ 0, 1, 1 },
	{ 1, 0, 0 },
	{ 1, 0, 1 },
	{ 1, 1, 0 },
	{ 1, 1, 1 }
};

enum {
	MATERIAL_SOLID = 0,
	MATERIAL_AIR   = 1,
};

struct OctreeDrawInfo {
	OctreeDrawInfo()
		: index(-1)
		, corners(0)
	{}

	int index;
	int corners;
	glm::vec3 position;
	glm::vec3 average_normal;
//	QEF qef;
};

enum NodeType {
	NONE,
	INTERNAL,
	PSEUDO,
	LEAF,
};

class OctreeNode {
public:
	glm::ivec3 min;
	int size;
	NodeType type;
	OctreeNode *children[8];
	OctreeDrawInfo *draw_info;

	OctreeNode(const NodeType &type)
		: min(0)
		, size(0)
		, type(type)
		, draw_info(nullptr)
	{
		for (int i = 0; i < 8; ++i) {
			children[i] = nullptr;
		}
	}

	OctreeNode()
		: OctreeNode(NONE)
	{}
};

OctreeNode *construct_leaf(OctreeNode *leaf)
{
	int corners = 0;

	for (int i = 0; i < 8; ++i) {
		const glm::ivec3 cornerpos = leaf->min + CHILD_MIN_OFFSET[i];
		const float density = density_func(cornerpos);
		const int material = density < 0.0f ? MATERIAL_SOLID : MATERIAL_AIR;
		corners |= (material << i);
	}

	if ((corners == 0) || (corners == 255)) {
		delete leaf;
		return nullptr;
	}

	leaf->type = LEAF;

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
		child->type = INTERNAL;

		node->children[i] = construct_octree_nodes(child);
		has_children |= (node->children[i] != nullptr);
	}

	if (!has_children) {
		delete node;
		return nullptr;
	}

	return node;
}
