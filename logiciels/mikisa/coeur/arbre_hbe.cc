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

#include "arbre_hbe.hh"

static inline auto axe_depuis_direction(Axe const direction)
{
	if (direction == Axe::Y) {
		return 1u;
	}

	if (direction == Axe::Z) {
		return 2u;
	}

	return 0u;
}

double calcul_cout_scission(
		double const scission,
		const ArbreHBE::Noeud &noeud,
		dls::tableau<long> &references,
		Axe const direction,
		dls::tableau<BoiteEnglobante> const &boites_alignees,
		long &compte_gauche,
		long &compte_droite)
{
	auto const axe_scission = axe_depuis_direction(direction);

	auto boite_gauche = BoiteEnglobante{};
	auto gauche = 0;
	auto boite_droite = BoiteEnglobante{};
	auto droite = 0;

	/* Parcours les références du noeud et fusionne les dans 'gauche' ou 'droite' */
	for (auto i = 0; i < noeud.nombre_references; ++i) {
		auto const idx_ref = references[i];
		auto const &reference = boites_alignees[idx_ref];

		if (reference.centroide[axe_scission] <= scission) {
			gauche++;
			boite_gauche.etend(reference);
		}
		else {
			droite++;
			boite_droite.etend(reference);
		}
	}

	compte_gauche = gauche;
	compte_droite = droite;

	if (gauche == 0 || droite == 0) {
		return std::pow(10.0f, 100);
	}

	auto cout_gauche = gauche * boite_gauche.aire_surface();
	auto cout_droite = droite * boite_droite.aire_surface();

	return std::fabs(cout_gauche - cout_droite);
}

double trouve_meilleure_scission(
		ArbreHBE::Noeud &noeud,
		dls::tableau<long> &references,
		Axe const direction,
		unsigned int const qualite,
		dls::tableau<BoiteEnglobante> const &boites_alignees,
		long &compte_gauche,
		long &compte_droite)
{
	auto const axe_scission = axe_depuis_direction(direction);

	auto candidates = dls::tableau<double>(static_cast<long>(qualite));
	auto longueur_axe = noeud.limites.max[axe_scission] - noeud.limites.min[axe_scission];
	auto taille_scission = longueur_axe / static_cast<double>(qualite + 1);
	auto taille_courante = taille_scission;

	/* crée des scissions isométriques */
	for (auto &candidate : candidates) {
		candidate = noeud.limites.min[axe_scission] + taille_courante;
		taille_courante += taille_scission;
	}

	/* Calculons tous les coûts de scission pour ne garder que le plus petit. */
	auto gauche = 0l;
	auto droite = 0l;
	auto gauche_courante = 0l;
	auto drotie_courante = 0l;

	auto meilleur_cout = constantes<double>::INFINITE;
	auto meilleure_scission = constantes<double>::INFINITE;

	for (auto const &candidate : candidates) {
		auto cout = calcul_cout_scission(
					candidate,
					noeud,
					references,
					direction,
					boites_alignees,
					gauche_courante,
					drotie_courante);

		if (cout < meilleur_cout) {
			meilleur_cout = cout;
			meilleure_scission = candidate;
			gauche = gauche_courante;
			droite = drotie_courante;
		}
	}

	compte_gauche = gauche;
	compte_droite = droite;

	return meilleure_scission;
}

double calcul_point_plus_proche(
		ArbreHBE::Noeud const &noeud,
		dls::math::point3d const &point,
		dls::math::point3d &plus_proche)
{
	auto const &min = noeud.limites.min;
	auto const &max = noeud.limites.max;

	for (auto i = 0ul; i < 3; ++i) {
		auto val = point[i];

		if (min[i] > val) {
			val = min[i];
		}

		if (max[i] < val) {
			val = max[i];
		}

		plus_proche[i] = val;
	}

	return dls::math::longueur_carree(point - plus_proche);
}

namespace bli {

static int implicit_needed_branches(int tree_type, int leafs)
{
  return std::max(1, (leafs + tree_type - 3) / (tree_type - 1));
}

BVHTree *bvhtree_new(int nombre_elements, float epsilon, char tree_type, char axis)
{
	int i;

	//BLI_assert(tree_type >= 2 && tree_type <= MAX_TREETYPE);

	auto tree = memoire::loge<BVHTree>("BVHTree");

	/* tree epsilon must be >= FLT_EPSILON
	 * so that tangent rays can still hit a bounding volume..
	 * this bug would show up when casting a ray aligned with a kdop-axis
	 * and with an edge of 2 faces */
	epsilon = std::max(1e-6f, epsilon);

	if (tree) {
		tree->epsilon = epsilon;
		tree->tree_type = tree_type;
		tree->axis = static_cast<axis_t>(axis);

		if (axis == 26) {
			tree->start_axis = 0;
			tree->stop_axis = 13;
		}
		else if (axis == 18) {
			tree->start_axis = 7;
			tree->stop_axis = 13;
		}
		else if (axis == 14) {
			tree->start_axis = 0;
			tree->stop_axis = 7;
		}
		else if (axis == 8) { /* AABB */
			tree->start_axis = 0;
			tree->stop_axis = 4;
		}
		else if (axis == 6) { /* OBB */
			tree->start_axis = 0;
			tree->stop_axis = 3;
		}
		else {
			/* should never happen! */
			assert(false);

			delete tree;
			return nullptr;
		}

		/* Allocate arrays */
		auto numnodes = nombre_elements + implicit_needed_branches(tree_type, nombre_elements) + tree_type;

		tree->nodes.redimensionne(numnodes);
		tree->nodebv.redimensionne(axis * numnodes);
		tree->nodechild.redimensionne(tree_type * numnodes);
		tree->nodearray.redimensionne(numnodes);

//		if (((!tree->nodes) || (!tree->nodebv) || (!tree->nodechild) || (!tree->nodearray))) {
//			goto fail;
//		}

		/* link the dynamic bv and child links */
		for (i = 0; i < numnodes; i++) {
			tree->nodearray[i].bv = &tree->nodebv[i * axis];
			tree->nodearray[i].children = &tree->nodechild[i * tree_type];
		}
	}
	return tree;
}

const dls::math::vec3f bvhtree_kdop_axes[13] = {
	dls::math::vec3f{1.0f,  0.0f,  0.0f},
	dls::math::vec3f{0.0f,  1.0f,  0.0f},
	dls::math::vec3f{0.0f,  0.0f,  1.0f},
	dls::math::vec3f{1.0f,  1.0f,  1.0f},
	dls::math::vec3f{1.0f, -1.0f,  1.0f},
	dls::math::vec3f{1.0f,  1.0f, -1.0f},
	dls::math::vec3f{1.0f, -1.0f, -1.0f},
	dls::math::vec3f{1.0f,  1.0f,  0.0f},
	dls::math::vec3f{1.0f,  0.0f,  1.0f},
	dls::math::vec3f{0.0f,  1.0f,  1.0f},
	dls::math::vec3f{1.0f, -1.0f,  0.0f},
	dls::math::vec3f{1.0f,  0.0f, -1.0f},
	dls::math::vec3f{0.0f,  1.0f, -1.0f},
};

static void node_minmax_init(const BVHTree *tree, BVHNode *node)
{
	axis_t axis_iter;
	float(*bv)[2] = reinterpret_cast<float(*)[2]>(node->bv);

	for (axis_iter = tree->start_axis; axis_iter != tree->stop_axis; axis_iter++) {
		bv[axis_iter][0] =  constantes<float>::INFINITE;
		bv[axis_iter][1] = -constantes<float>::INFINITE;
	}
}

static void create_kdop_hull(
		const BVHTree *tree, BVHNode *node, dls::math::vec3f const *co, int numpoints, int moving)
{
	float newminmax;
	float *bv = node->bv;
	int k;
	axis_t axis_iter;

	/* don't init boudings for the moving case */
	if (!moving) {
		node_minmax_init(tree, node);
	}

	for (k = 0; k < numpoints; k++) {
		/* for all Axes. */
		for (axis_iter = tree->start_axis; axis_iter < tree->stop_axis; axis_iter++) {
			newminmax = produit_scalaire(co[k], bvhtree_kdop_axes[axis_iter]);
			if (newminmax < bv[2 * axis_iter]) {
				bv[2 * axis_iter] = newminmax;
			}
			if (newminmax > bv[(2 * axis_iter) + 1]) {
				bv[(2 * axis_iter) + 1] = newminmax;
			}
		}
	}
}

void insere(BVHTree *tree, int index, dls::math::vec3f const *co, int numpoints)
{
	/* insert should only possible as long as tree->totbranch is 0 */
	// assert(tree->totbranch <= 0);
	//  assert((size_t)tree->totleaf < MEM_allocN_len(tree->nodes) / sizeof(*(tree->nodes)));

	auto node = tree->nodes[tree->totleaf] = &(tree->nodearray[tree->totleaf]);
	tree->totleaf++;

	create_kdop_hull(tree, node, co, numpoints, 0);
	node->index = index;

	/* inflate the bv with some epsilon */
	for (auto axis_iter = tree->start_axis; axis_iter < tree->stop_axis; axis_iter++) {
		node->bv[(2 * axis_iter)] -= tree->epsilon;     /* minimum */
		node->bv[(2 * axis_iter) + 1] += tree->epsilon; /* maximum */
	}
}

static void refit_kdop_hull(const BVHTree *tree, BVHNode *node, int start, int end)
{
  float newmin, newmax;
  float *__restrict bv = node->bv;
  int j;
  axis_t axis_iter;

  node_minmax_init(tree, node);

  for (j = start; j < end; j++) {
	float *__restrict node_bv = tree->nodes[j]->bv;

	/* for all Axes. */
	for (axis_iter = tree->start_axis; axis_iter < tree->stop_axis; axis_iter++) {
	  newmin = node_bv[(2 * axis_iter)];
	  if ((newmin < bv[(2 * axis_iter)])) {
		bv[(2 * axis_iter)] = newmin;
	  }

	  newmax = node_bv[(2 * axis_iter) + 1];
	  if ((newmax > bv[(2 * axis_iter) + 1])) {
		bv[(2 * axis_iter) + 1] = newmax;
	  }
	}
  }
}

/**
 * only supports x,y,z axis in the moment
 * but we should use a plain and simple function here for speed sake */
static char get_largest_axis(const float *bv)
{
  float middle_point[3];

  middle_point[0] = (bv[1]) - (bv[0]); /* x axis */
  middle_point[1] = (bv[3]) - (bv[2]); /* y axis */
  middle_point[2] = (bv[5]) - (bv[4]); /* z axis */
  if (middle_point[0] > middle_point[1]) {
	if (middle_point[0] > middle_point[2]) {
	  return 1; /* max x axis */
	}
	else {
	  return 5; /* max z axis */
	}
  }
  else {
	if (middle_point[1] > middle_point[2]) {
	  return 3; /* max y axis */
	}
	else {
	  return 5; /* max z axis */
	}
  }
}

typedef struct BVHBuildHelper {
  int tree_type;
  int totleafs;

  /** Min number of leafs that are archievable from a node at depth N */
  int leafs_per_child[32];
  /** Number of nodes at depth N (tree_type^N) */
  int branches_on_level[32];

  /** Number of leafs that are placed on the level that is not 100% filled */
  int remain_leafs;

} BVHBuildHelper;

static void build_implicit_tree_helper(const BVHTree *tree, BVHBuildHelper *data)
{
  int depth = 0;
  int remain;
  int nnodes;

  data->totleafs = tree->totleaf;
  data->tree_type = tree->tree_type;

  /* Calculate the smallest tree_type^n such that tree_type^n >= num_leafs */
  for (data->leafs_per_child[0] = 1; data->leafs_per_child[0] < data->totleafs;
	   data->leafs_per_child[0] *= data->tree_type) {
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
  const BVHTree *tree;
  BVHNode *branches_array;
  BVHNode **leafs_array;

  int tree_type;
  int tree_offset;

  const BVHBuildHelper *data;

  int depth;
  int i;
  int first_of_next_level;
} BVHDivNodesData;

static constexpr auto MAX_TREETYPE = 32;

static void bvh_insertionsort(BVHNode **a, int lo, int hi, int axis)
{
  int i, j;
  BVHNode *t;
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

static int bvh_partition(BVHNode **a, int lo, int hi, BVHNode *x, int axis)
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
	std::swap(a[i], a[j]);
	i++;
  }
}
static BVHNode *bvh_medianof3(BVHNode **a, int lo, int mid, int hi, int axis)
{
  if ((a[mid])->bv[axis] < (a[lo])->bv[axis]) {
	if ((a[hi])->bv[axis] < (a[mid])->bv[axis]) {
	  return a[mid];
	}
	else {
	  if ((a[hi])->bv[axis] < (a[lo])->bv[axis]) {
		return a[hi];
	  }
	  else {
		return a[lo];
	  }
	}
  }
  else {
	if ((a[hi])->bv[axis] < (a[mid])->bv[axis]) {
	  if ((a[hi])->bv[axis] < (a[lo])->bv[axis]) {
		return a[lo];
	  }
	  else {
		return a[hi];
	  }
	}
	else {
	  return a[mid];
	}
  }
}

static void partition_nth_element(BVHNode **a, int begin, int end, const int n, const int axis)
{
  while (end - begin > 3) {
	const int cut = bvh_partition(
		a, begin, end, bvh_medianof3(a, begin, (begin + end) / 2, end - 1, axis), axis);
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
  if (min_leaf_index <= data->remain_leafs) {
	return min_leaf_index;
  }
  else if (data->leafs_per_child[depth]) {
	return data->totleafs -
		   (data->branches_on_level[depth - 1] - child_index) * data->leafs_per_child[depth];
  }
  else {
	return data->remain_leafs;
  }
}
static void split_leafs(BVHNode **leafs_array,
						const int nth[],
						const int partitions,
						const int split_axis)
{
  int i;
  for (i = 0; i < partitions - 1; i++) {
	if (nth[i] >= nth[partitions]) {
	  break;
	}

	partition_nth_element(leafs_array, nth[i], nth[partitions], nth[i + 1], split_axis);
  }
}

static void non_recursive_bvh_div_nodes_task_cb(void *__restrict userdata,
												const int j)
{
  BVHDivNodesData *data = static_cast<BVHDivNodesData *>(userdata);

  int k;
  const int parent_level_index = j - data->i;
  BVHNode *parent = &data->branches_array[j];
  int nth_positions[MAX_TREETYPE + 1];
  char split_axis;

  int parent_leafs_begin = implicit_leafs_index(data->data, data->depth, parent_level_index);
  int parent_leafs_end = implicit_leafs_index(data->data, data->depth, parent_level_index + 1);

  /* This calculates the bounding box of this branch
   * and chooses the largest axis as the axis to divide leafs */
  refit_kdop_hull(data->tree, parent, parent_leafs_begin, parent_leafs_end);
  split_axis = get_largest_axis(parent->bv);

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

	const int child_leafs_begin = implicit_leafs_index(
		data->data, data->depth + 1, child_level_index);
	const int child_leafs_end = implicit_leafs_index(
		data->data, data->depth + 1, child_level_index + 1);

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
  parent->totnode = static_cast<char>(k);
}

static void non_recursive_bvh_div_nodes(const BVHTree *tree,
										BVHNode *branches_array,
										BVHNode **leafs_array,
										int num_leafs)
{
	int i;

	const int tree_type = tree->tree_type;
	/* this value is 0 (on binary trees) and negative on the others */
	const int tree_offset = 2 - tree->tree_type;

	const int num_branches = implicit_needed_branches(tree_type, num_leafs);

	BVHBuildHelper data;
	int depth;

	{
		/* set parent from root node to NULL */
		BVHNode *root = &branches_array[1];
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

	build_implicit_tree_helper(tree, &data);

	BVHDivNodesData cb_data;
	cb_data.tree = tree;
	cb_data.branches_array = branches_array;
	cb_data.leafs_array = leafs_array;
	cb_data.tree_type = tree_type;
	cb_data.tree_offset = tree_offset;
	cb_data.data = &data;
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

			/* Less hassle for debugging. */
			//TaskParallelTLS tls = {0};
			for (int i_task = i; i_task < i_stop; i_task++) {
				non_recursive_bvh_div_nodes_task_cb(&cb_data, i_task);
			}
	}
}

void balance(BVHTree *tree)
{
	BVHNode **leafs_array = &tree->nodes[0];

	/* This function should only be called once
	 * (some big bug goes here if its being called more than once per tree) */
	//BLI_assert(tree->totbranch == 0);

	/* Build the implicit tree */
	non_recursive_bvh_div_nodes(
				tree, tree->nodearray.donnees() + (tree->totleaf - 1), leafs_array, tree->totleaf);

	/* current code expects the branches to be linked to the nodes array
	 * we perform that linkage here */
	tree->totbranch = implicit_needed_branches(tree->tree_type, tree->totleaf);
	for (int i = 0; i < tree->totbranch; i++) {
		tree->nodes[tree->totleaf + i] = &tree->nodearray[tree->totleaf + i];
	}
}

}
