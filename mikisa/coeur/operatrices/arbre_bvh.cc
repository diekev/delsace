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

#include "arbre_bvh.hh"

#include <cassert>

#include <delsace/math/vecteur.hh>

#include "bibloc/logeuse_memoire.hh"

#include "../corps/triangulation.hh"

static constexpr auto MAX_TREETYPE = 32;

static auto implicit_needed_branches(int tree_type, int leafs)
{
	return std::max(1, (leafs + tree_type - 3) / (tree_type - 1));
}


static char get_largest_axis(const float *bv)
{
	float middle_point[3];

	middle_point[0] = (bv[1]) - (bv[0]); /* x axis */
	middle_point[1] = (bv[3]) - (bv[2]); /* y axis */
	middle_point[2] = (bv[5]) - (bv[4]); /* z axis */
	if (middle_point[0] > middle_point[1]) {
		if (middle_point[0] > middle_point[2])
			return 1;  /* max x axis */
		else
			return 5;  /* max z axis */
	}
	else {
		if (middle_point[1] > middle_point[2])
			return 3;  /* max y axis */
		else
			return 5;  /* max z axis */
	}
}

const dls::math::vec3f bvhtree_kdop_axes[13] = {
	dls::math::vec3f(1.0f,  0.0f,  0.0f),
	dls::math::vec3f(0.0f,  1.0f,  0.0f),
	dls::math::vec3f(0.0f,  0.0f,  1.0f),
	dls::math::vec3f(1.0f,  1.0f,  1.0f),
	dls::math::vec3f(1.0f, -1.0f,  1.0f),
	dls::math::vec3f(1.0f,  1.0f, -1.0f),
	dls::math::vec3f(1.0f, -1.0f, -1.0f),
	dls::math::vec3f(1.0f,  1.0f,  0.0f),
	dls::math::vec3f(1.0f,  0.0f,  1.0f),
	dls::math::vec3f(0.0f,  1.0f,  1.0f),
	dls::math::vec3f(1.0f, -1.0f,  0.0f),
	dls::math::vec3f(1.0f,  0.0f, -1.0f),
	dls::math::vec3f(0.0f,  1.0f, -1.0f)
};

static void node_minmax_init(const ArbreBVH *tree, ArbreBVH::Noeud *node)
{
	auto bv = reinterpret_cast<dls::math::vec2f *>(node->bv);

	for (auto axis_iter = tree->start_axis; axis_iter != tree->stop_axis; axis_iter++) {
		bv[static_cast<size_t>(axis_iter)][0] =  std::numeric_limits<float>::max();
		bv[static_cast<size_t>(axis_iter)][1] = -std::numeric_limits<float>::max();
	}
}

static void refit_kdop_hull(const ArbreBVH *tree, ArbreBVH::Noeud *node, int start, int end)
{
	float *bv = node->bv;

	node_minmax_init(tree, node);

	for (auto j = start; j < end; j++) {
		/* for all Axes. */
		for (auto axis_iter = tree->start_axis; axis_iter < tree->stop_axis; axis_iter++) {
			auto newmin = tree->nodes[j]->bv[(2 * axis_iter)];

			if ((newmin < bv[(2 * axis_iter)])) {
				bv[(2 * axis_iter)] = newmin;
			}

			auto newmax = tree->nodes[j]->bv[(2 * axis_iter) + 1];
			if ((newmax > bv[(2 * axis_iter) + 1])) {
				bv[(2 * axis_iter) + 1] = newmax;
			}
		}
	}
}

typedef struct BVHBuildHelper {
	int tree_type = 0;
	int totleafs = 0;

	/** Min number of leafs that are archievable from a node at depth N */
	int leafs_per_child[32];
	/** Number of nodes at depth N (tree_type^N) */
	int branches_on_level[32];

	/** Number of leafs that are placed on the level that is not 100% filled */
	int remain_leafs = 0;

} BVHBuildHelper;

static void build_implicit_tree_helper(const ArbreBVH *tree, BVHBuildHelper *data)
{
	int depth = 0;
	int remain;
	int nnodes;

	data->totleafs = tree->totleaf;
	data->tree_type = tree->tree_type;

	/* Calculate the smallest tree_type^n such that tree_type^n >= num_leafs */
	for (data->leafs_per_child[0] = 1;
		 data->leafs_per_child[0] <  data->totleafs;
		 data->leafs_per_child[0] *= data->tree_type)
	{
		/* pass */
	}

	data->branches_on_level[0] = 1;

	for (depth = 1; (depth < 32) && data->leafs_per_child[depth - 1]; depth++) {
		data->branches_on_level[depth] = data->branches_on_level[depth - 1] * data->tree_type;
		data->leafs_per_child[depth] = data->leafs_per_child[depth - 1] / data->tree_type;
	}

	remain = data->totleafs - data->leafs_per_child[1];
	nnodes = (remain + data->tree_type - 2) / (data->tree_type - 1);
	data->remain_leafs = remain + nnodes;
}

typedef struct BVHDivNodesData {
	const ArbreBVH *tree;
	ArbreBVH::Noeud *branches_array;
	ArbreBVH::Noeud **leafs_array;

	int tree_type;
	int tree_offset;

	const BVHBuildHelper *data;

	int depth;
	int i;
	int first_of_next_level;
} BVHDivNodesData;

static void bvh_insertionsort(ArbreBVH::Noeud **a, int lo, int hi, int axis)
{
	int i, j;
	ArbreBVH::Noeud *t;
	for (i = lo; i < hi; i++) {
		j = i;
		t = a[i];
		while ((j != lo) && (t->bv[axis] < (a[j - 1])->bv[axis])) {
			a[j] = a[j - 1];
			j--;
		}
		a[j] = t;
	}
}

static int bvh_partition(ArbreBVH::Noeud **a, int lo, int hi, ArbreBVH::Noeud *x, int axis)
{
	int i = lo, j = hi;
	while (1) {
		while (a[i]->bv[axis] < x->bv[axis]) {
			i++;
		}
		j--;
		while (x->bv[axis] < a[j]->bv[axis]) {
			j--;
		}
		if (!(i < j)) {
			return i;
		}
		std::swap(a[i], a[j]); // XXX
		i++;
	}
}

static ArbreBVH::Noeud *bvh_medianof3(ArbreBVH::Noeud **a, int lo, int mid, int hi, int axis)  /* returns Sortable */
{
	if ((a[mid])->bv[axis] < (a[lo])->bv[axis]) {
		if ((a[hi])->bv[axis] < (a[mid])->bv[axis])
			return a[mid];
		else {
			if ((a[hi])->bv[axis] < (a[lo])->bv[axis])
				return a[hi];
			else
				return a[lo];
		}
	}
	else {
		if ((a[hi])->bv[axis] < (a[mid])->bv[axis]) {
			if ((a[hi])->bv[axis] < (a[lo])->bv[axis])
				return a[lo];
			else
				return a[hi];
		}
		else
			return a[mid];
	}
}

static void partition_nth_element(ArbreBVH::Noeud **a, int begin, int end, const int n, const int axis)
{
	while (end - begin > 3) {
		const int cut = bvh_partition(a, begin, end, bvh_medianof3(a, begin, (begin + end) / 2, end - 1, axis), axis);
		if (cut <= n) {
			begin = cut;
		}
		else {
			end = cut;
		}
	}
	bvh_insertionsort(a, begin, end, axis);
}

static int implicit_leafs_index(const BVHBuildHelper *data, const int depth, const int child_index)
{
	int min_leaf_index = child_index * data->leafs_per_child[depth - 1];
	if (min_leaf_index <= data->remain_leafs)
		return min_leaf_index;
	else if (data->leafs_per_child[depth])
		return data->totleafs - (data->branches_on_level[depth - 1] - child_index) * data->leafs_per_child[depth];
	else
		return data->remain_leafs;
}

static void split_leafs(ArbreBVH::Noeud **leafs_array, const int nth[], const int partitions, const int split_axis)
{
	int i;
	for (i = 0; i < partitions - 1; i++) {
		if (nth[i] >= nth[partitions])
			break;

		partition_nth_element(leafs_array, nth[i], nth[partitions], nth[i + 1], split_axis);
	}
}

static void non_recursive_bvh_div_nodes(
		const ArbreBVH *tree,
		ArbreBVH::Noeud *branches_array,
		ArbreBVH::Noeud **leafs_array,
		int num_leafs)
{
	int i;

	const int tree_type   = tree->tree_type;
	/* this value is 0 (on binary trees) and negative on the others */
	const int tree_offset = 2 - tree->tree_type;

	const int num_branches = implicit_needed_branches(tree_type, num_leafs);

	BVHBuildHelper helper_data;
	int depth;

	{
		/* set parent from root node to NULL */
		auto root = &branches_array[1];
		root->parent = nullptr;

		/* Most of bvhtree code relies on 1-leaf trees having at least one branch
		 * We handle that special case here */
		if (num_leafs == 1) {
			refit_kdop_hull(tree, root, 0, num_leafs);
			root->main_axis = get_largest_axis(root->bv) / 2;
			root->totnode = 1;
			root->children[0] = leafs_array[0];
			root->children[0]->parent = root;
			return;
		}
	}

	build_implicit_tree_helper(tree, &helper_data);

	BVHDivNodesData cb_data;
	cb_data.tree = tree;
	cb_data.branches_array = branches_array;
	cb_data.leafs_array = leafs_array;
	cb_data.tree_type = tree_type;
	cb_data.tree_offset = tree_offset;
	cb_data.data = &helper_data;
	cb_data.first_of_next_level = 0;
	cb_data.depth = 0;
	cb_data.i = 0;

	/* Loop tree levels (log N) loops */
	for (i = 1, depth = 1; i <= num_branches; i = i * tree_type + tree_offset, depth++) {
		const int first_of_next_level = i * tree_type + tree_offset;
		/* index of last branch on this level */
		const int i_stop = std::min(first_of_next_level, num_branches + 1);

		/* Loop all branches on this level */
		cb_data.first_of_next_level = first_of_next_level;
		cb_data.i = i;
		cb_data.depth = depth;

		/* À FAIRE : boucle parallèle. */
		for (auto j = i; j < i_stop; ++j) {
			BVHDivNodesData *data = &cb_data;

			int k;
			auto parent_level_index = j - data->i;
			auto parent = &data->branches_array[j];
			int nth_positions[MAX_TREETYPE + 1];

			auto parent_leafs_begin = implicit_leafs_index(data->data, data->depth, parent_level_index);
			auto parent_leafs_end   = implicit_leafs_index(data->data, data->depth, parent_level_index + 1);

			/* This calculates the bounding box of this branch
			 * and chooses the largest axis as the axis to divide leafs */
			refit_kdop_hull(data->tree, parent, parent_leafs_begin, parent_leafs_end);
			auto split_axis = get_largest_axis(parent->bv);

			/* Save split axis (this can be used on raytracing to speedup the query time) */
			parent->main_axis = split_axis / 2;

			/* Split the childs along the split_axis, note: its not needed to sort the whole leafs array
			 * Only to assure that the elements are partitioned on a way that each child takes the elements
			 * it would take in case the whole array was sorted.
			 * Split_leafs takes care of that "sort" problem. */
			nth_positions[0] = parent_leafs_begin;
			nth_positions[data->tree_type] = parent_leafs_end;
			for (k = 1; k < data->tree_type; k++) {
				const int child_index = j * data->tree_type + data->tree_offset + k;
				/* child level index */
				const int child_level_index = child_index - data->first_of_next_level;
				nth_positions[k] = implicit_leafs_index(data->data, data->depth + 1, child_level_index);
			}

			split_leafs(data->leafs_array, nth_positions, data->tree_type, split_axis);

			/* Setup children and totnode counters
			 * Not really needed but currently most of BVH code
			 * relies on having an explicit children structure */
			for (k = 0; k < data->tree_type; k++) {
				const int child_index = j * data->tree_type + data->tree_offset + k;
				/* child level index */
				const int child_level_index = child_index - data->first_of_next_level;

				const int child_leafs_begin = implicit_leafs_index(data->data, data->depth + 1, child_level_index);
				const int child_leafs_end   = implicit_leafs_index(data->data, data->depth + 1, child_level_index + 1);

				if (child_leafs_end - child_leafs_begin > 1) {
					parent->children[k] = &data->branches_array[child_index];
					parent->children[k]->parent = parent;
				}
				else if (child_leafs_end - child_leafs_begin == 1) {
					parent->children[k] = data->leafs_array[child_leafs_begin];
					parent->children[k]->parent = parent;
				}
				else {
					break;
				}
			}
			parent->totnode = k;
		}
	}
}

static void create_kdop_hull(const ArbreBVH *tree, ArbreBVH::Noeud *node, Triangle const &tri, bool moving)
{

	/* don't init boudings for the moving case */
	if (!moving) {
		node_minmax_init(tree, node);
	}

	float *bv = node->bv;

	/* À FAIRE : généralise à plus qu'aux triangles */

	/* for all Axes. */
	for (auto axis_iter = tree->start_axis; axis_iter < tree->stop_axis; axis_iter++) {
		auto newminmax = produit_scalaire(tri.v0, bvhtree_kdop_axes[static_cast<size_t>(axis_iter)]);

		if (newminmax < bv[2 * axis_iter]) {
			bv[2 * axis_iter] = newminmax;
		}

		if (newminmax > bv[(2 * axis_iter) + 1]) {
			bv[(2 * axis_iter) + 1] = newminmax;
		}
	}
	/* for all Axes. */
	for (auto axis_iter = tree->start_axis; axis_iter < tree->stop_axis; axis_iter++) {
		auto newminmax = produit_scalaire(tri.v1, bvhtree_kdop_axes[static_cast<size_t>(axis_iter)]);

		if (newminmax < bv[2 * axis_iter]) {
			bv[2 * axis_iter] = newminmax;
		}

		if (newminmax > bv[(2 * axis_iter) + 1]) {
			bv[(2 * axis_iter) + 1] = newminmax;
		}
	}
	/* for all Axes. */
	for (auto axis_iter = tree->start_axis; axis_iter < tree->stop_axis; axis_iter++) {
		auto newminmax = produit_scalaire(tri.v2, bvhtree_kdop_axes[static_cast<size_t>(axis_iter)]);

		if (newminmax < bv[2 * axis_iter]) {
			bv[2 * axis_iter] = newminmax;
		}

		if (newminmax > bv[(2 * axis_iter) + 1]) {
			bv[(2 * axis_iter) + 1] = newminmax;
		}
	}
}

void ArbreBVH::insert_triangle(int index, Triangle const &tri)
{
	/* insert should only possible as long as tree->totbranch is 0 */
	assert(this->totbranch <= 0);
	assert(this->totleaf < this->nodes.taille());

	auto node = this->nodes[this->totleaf] = &(this->nodearray[this->totleaf]);
	this->totleaf++;

	create_kdop_hull(this, node, tri, false);
	node->index = index;

	/* inflate the bv with some epsilon */
	for (auto axis_iter = this->start_axis; axis_iter < this->stop_axis; axis_iter++) {
		node->bv[(2 * axis_iter)] -= this->epsilon; /* minimum */
		node->bv[(2 * axis_iter) + 1] += this->epsilon; /* maximum */
	}
}

#ifdef VERIFY_ARBRE
static void bvhtree_verify(ArbreBVH *tree)
{
	int i, j, check = 0;

	/* check the pointer list */
	for (i = 0; i < tree->totleaf; i++) {
		if (tree->nodes[i]->parent == nullptr) {
			printf("Leaf has no parent: %d\n", i);
		}
		else {
			for (j = 0; j < tree->tree_type; j++) {
				if (tree->nodes[i]->parent->children[j] == tree->nodes[i])
					check = 1;
			}
			if (!check) {
				printf("Parent child relationship doesn't match: %d\n", i);
			}
			check = 0;
		}
	}

	/* check the leaf list */
	for (i = 0; i < tree->totleaf; i++) {
		if (tree->nodearray[i].parent == nullptr) {
			printf("Leaf has no parent: %d\n", i);
		}
		else {
			for (j = 0; j < tree->tree_type; j++) {
				if (tree->nodearray[i].parent->children[j] == &tree->nodearray[i])
					check = 1;
			}
			if (!check) {
				printf("Parent child relationship doesn't match: %d\n", i);
			}
			check = 0;
		}
	}

	printf("branches: %d, leafs: %d, total: %d\n",
		   tree->totbranch, tree->totleaf, tree->totbranch + tree->totleaf);
}
#endif

/* ************************************************************************** */

void ArbreBVH::balance()
{
	auto leafs_array = this->nodes.donnees();

	/* This function should only be called once
		 * (some big bug goes here if its being called more than once per tree) */
	assert(this->totbranch == 0);

	/* Build the implicit tree */
	non_recursive_bvh_div_nodes(this, this->nodearray.donnees() + (this->totleaf - 1), leafs_array, this->totleaf);

	/* current code expects the branches to be linked to the nodes array
		 * we perform that linkage here */
	this->totbranch = implicit_needed_branches(this->tree_type, this->totleaf);
	for (int i = 0; i < this->totbranch; i++) {
		this->nodes[this->totleaf + i] = &this->nodearray[this->totleaf + i];
	}

	/* À FAIRE : info, skip tree */
#ifdef VERIFIE_ARBRE
	bvhtree_verify(arbre);
#endif
}

ArbreBVH *nouvelle_arbre_bvh(int maxsize, float epsilon, unsigned char tree_type, unsigned char axis)
{
	assert(tree_type >= 2 && tree_type <= MAX_TREETYPE);

	auto arbre = memoire::loge<ArbreBVH>();

	/* tree epsilon must be >= FLT_EPSILON
	 * so that tangent rays can still hit a bounding volume..
	 * this bug would show up when casting a ray aligned with a kdop-axis
	 * and with an edge of 2 faces */
	epsilon = std::max(std::numeric_limits<float>::epsilon(), epsilon);

	if (arbre == nullptr) {
		return static_cast<ArbreBVH *>(nullptr);
	}

	arbre->epsilon = epsilon;
	arbre->tree_type = tree_type;
	arbre->axis = axis;

	if (axis == 26) {
		arbre->start_axis = 0;
		arbre->stop_axis = 13;
	}
	else if (axis == 18) {
		arbre->start_axis = 7;
		arbre->stop_axis = 13;
	}
	else if (axis == 14) {
		arbre->start_axis = 0;
		arbre->stop_axis = 7;
	}
	else if (axis == 8) { /* AABB */
		arbre->start_axis = 0;
		arbre->stop_axis = 4;
	}
	else if (axis == 6) { /* OBB */
		arbre->start_axis = 0;
		arbre->stop_axis = 3;
	}
	else {
		/* should never happen! */
		memoire::deloge(arbre);
		assert(false);
		return static_cast<ArbreBVH *>(nullptr);
	}


	/* Allocate arrays */
	auto numnodes = maxsize + implicit_needed_branches(tree_type, maxsize) + tree_type;

	arbre->nodes.redimensionne(numnodes);
	arbre->nodebv.redimensionne(axis * numnodes);
	arbre->nodechild.redimensionne(tree_type * numnodes);
	arbre->nodearray.redimensionne(numnodes);

	/* link the dynamic bv and child links */
	for (auto i = 0; i < numnodes; i++) {
		arbre->nodearray[i].bv = &arbre->nodebv[i * axis];
		arbre->nodearray[i].children = &arbre->nodechild[i * tree_type];
	}

	return arbre;
}
