/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "acceleration.hh"

#include "outils.hh"

namespace geo {

static int implicit_needed_branches(int tree_type, int leafs)
{
    return std::max(1, (leafs + tree_type - 3) / (tree_type - 1));
}

static HierarchieBoiteEnglobante *bvhtree_new(int nombre_elements,
                                              float epsilon,
                                              char tree_type,
                                              char axis)
{
    int i;

    // BLI_assert(tree_type >= 2 && tree_type <= MAX_TREETYPE);

    auto tree = memoire::loge<HierarchieBoiteEnglobante>("HierarchieBoiteEnglobante");

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
        auto numnodes = nombre_elements + implicit_needed_branches(tree_type, nombre_elements) +
                        tree_type;

        tree->nodes.redimensionne(numnodes, nullptr);
        tree->nodebv.redimensionne(axis * numnodes);
        tree->nodechild.redimensionne(tree_type * numnodes, nullptr);
        tree->nodearray.redimensionne(numnodes, HierarchieBoiteEnglobante::Noeud());

        /* link the dynamic bv and child links */
        for (i = 0; i < numnodes; i++) {
            tree->nodearray[i].limites = &tree->nodebv[i * axis];
            tree->nodearray[i].enfants = &tree->nodechild[i * tree_type];
        }
    }
    return tree;
}

static const dls::math::vec3f bvhtree_kdop_axes[13] = {
    dls::math::vec3f{1.0f, 0.0f, 0.0f},
    dls::math::vec3f{0.0f, 1.0f, 0.0f},
    dls::math::vec3f{0.0f, 0.0f, 1.0f},
    dls::math::vec3f{1.0f, 1.0f, 1.0f},
    dls::math::vec3f{1.0f, -1.0f, 1.0f},
    dls::math::vec3f{1.0f, 1.0f, -1.0f},
    dls::math::vec3f{1.0f, -1.0f, -1.0f},
    dls::math::vec3f{1.0f, 1.0f, 0.0f},
    dls::math::vec3f{1.0f, 0.0f, 1.0f},
    dls::math::vec3f{0.0f, 1.0f, 1.0f},
    dls::math::vec3f{1.0f, -1.0f, 0.0f},
    dls::math::vec3f{1.0f, 0.0f, -1.0f},
    dls::math::vec3f{0.0f, 1.0f, -1.0f},
};

static void node_minmax_init(const HierarchieBoiteEnglobante *tree,
                             HierarchieBoiteEnglobante::Noeud *node)
{
    axis_t axis_iter;
    float(*bv)[2] = reinterpret_cast<float(*)[2]>(node->limites);

    for (axis_iter = tree->start_axis; axis_iter != tree->stop_axis; axis_iter++) {
        bv[axis_iter][0] = constantes<float>::INFINITE;
        bv[axis_iter][1] = -constantes<float>::INFINITE;
    }
}

static void create_kdop_hull(const HierarchieBoiteEnglobante *tree,
                             HierarchieBoiteEnglobante::Noeud *node,
                             dls::math::vec3f const *co,
                             int numpoints,
                             int moving)
{
    float newminmax;
    float *bv = node->limites;
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

static void insere(HierarchieBoiteEnglobante *tree,
                   int index,
                   dls::math::vec3f const *co,
                   int numpoints)
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
        node->limites[(2 * axis_iter)] -= tree->epsilon;     /* minimum */
        node->limites[(2 * axis_iter) + 1] += tree->epsilon; /* maximum */
    }
}

static void refit_kdop_hull(const HierarchieBoiteEnglobante *tree,
                            HierarchieBoiteEnglobante::Noeud *node,
                            int start,
                            int end)
{
    float newmin, newmax;
    float *__restrict bv = node->limites;
    int j;
    axis_t axis_iter;

    node_minmax_init(tree, node);

    for (j = start; j < end; j++) {
        float *__restrict node_bv = tree->nodes[j]->limites;

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

static void build_implicit_tree_helper(const HierarchieBoiteEnglobante *tree, BVHBuildHelper *data)
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
    const HierarchieBoiteEnglobante *tree;
    HierarchieBoiteEnglobante::Noeud *branches_array;
    HierarchieBoiteEnglobante::Noeud **leafs_array;

    int tree_type;
    int tree_offset;

    const BVHBuildHelper *data;

    int depth;
    int i;
    int first_of_next_level;
} BVHDivNodesData;

static constexpr auto MAX_TREETYPE = 32;

static void bvh_insertionsort(HierarchieBoiteEnglobante::Noeud **a, int lo, int hi, int axis)
{
    int i, j;
    HierarchieBoiteEnglobante::Noeud *t;
    for (i = lo; i < hi; i++) {
        j = i;
        t = a[i];
        while ((j != lo) && (t->limites[axis] < (a[j - 1])->limites[axis])) {
            a[j] = a[j - 1];
            j--;
        }
        a[j] = t;
    }
}

static int bvh_partition(HierarchieBoiteEnglobante::Noeud **a,
                         int lo,
                         int hi,
                         HierarchieBoiteEnglobante::Noeud *x,
                         int axis)
{
    int i = lo, j = hi;
    while (1) {
        while (a[i]->limites[axis] < x->limites[axis]) {
            i++;
        }
        j--;
        while (x->limites[axis] < a[j]->limites[axis]) {
            j--;
        }
        if (!(i < j)) {
            return i;
        }
        std::swap(a[i], a[j]);
        i++;
    }
}
static HierarchieBoiteEnglobante::Noeud *bvh_medianof3(
    HierarchieBoiteEnglobante::Noeud **a, int lo, int mid, int hi, int axis)
{
    if ((a[mid])->limites[axis] < (a[lo])->limites[axis]) {
        if ((a[hi])->limites[axis] < (a[mid])->limites[axis]) {
            return a[mid];
        }
        else {
            if ((a[hi])->limites[axis] < (a[lo])->limites[axis]) {
                return a[hi];
            }
            else {
                return a[lo];
            }
        }
    }
    else {
        if ((a[hi])->limites[axis] < (a[mid])->limites[axis]) {
            if ((a[hi])->limites[axis] < (a[lo])->limites[axis]) {
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

static void partition_nth_element(
    HierarchieBoiteEnglobante::Noeud **a, int begin, int end, const int n, const int axis)
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
static void split_leafs(HierarchieBoiteEnglobante::Noeud **leafs_array,
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

static void non_recursive_bvh_div_nodes_task_cb(void *__restrict userdata, const int j)
{
    BVHDivNodesData *data = static_cast<BVHDivNodesData *>(userdata);

    int k;
    const int parent_level_index = j - data->i;
    HierarchieBoiteEnglobante::Noeud *parent = &data->branches_array[j];
    int nth_positions[MAX_TREETYPE + 1];
    char split_axis;

    int parent_leafs_begin = implicit_leafs_index(data->data, data->depth, parent_level_index);
    int parent_leafs_end = implicit_leafs_index(data->data, data->depth, parent_level_index + 1);

    /* This calculates the bounding box of this branch
     * and chooses the largest axis as the axis to divide leafs */
    refit_kdop_hull(data->tree, parent, parent_leafs_begin, parent_leafs_end);
    split_axis = get_largest_axis(parent->limites);

    /* Save split axis (this can be used on raytracing to speedup the query time) */
    parent->axe_principal = split_axis / 2;

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
            parent->enfants[k] = &data->branches_array[child_index];
            parent->enfants[k]->parent = parent;
        }
        else if (child_leafs_end - child_leafs_begin == 1) {
            parent->enfants[k] = data->leafs_array[child_leafs_begin];
            parent->enfants[k]->parent = parent;
        }
        else {
            break;
        }
    }
    parent->nombre_enfants = static_cast<char>(k);
}

static void non_recursive_bvh_div_nodes(const HierarchieBoiteEnglobante *tree,
                                        HierarchieBoiteEnglobante::Noeud *branches_array,
                                        HierarchieBoiteEnglobante::Noeud **leafs_array,
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
        HierarchieBoiteEnglobante::Noeud *root = &branches_array[1];
        root->parent = nullptr;

        /* Most of bvhtree code relies on 1-leaf trees having at least one branch
         * We handle that special case here */
        if (num_leafs == 1) {
            refit_kdop_hull(tree, root, 0, num_leafs);
            root->axe_principal = get_largest_axis(root->limites) / 2;
            root->nombre_enfants = 1;
            root->enfants[0] = leafs_array[0];
            root->enfants[0]->parent = root;
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
        // TaskParallelTLS tls = {0};
        for (int i_task = i; i_task < i_stop; i_task++) {
            non_recursive_bvh_div_nodes_task_cb(&cb_data, i_task);
        }
    }
}

static void balance(HierarchieBoiteEnglobante *tree)
{
    HierarchieBoiteEnglobante::Noeud **leafs_array = &tree->nodes[0];

    /* This function should only be called once
     * (some big bug goes here if its being called more than once per tree) */
    // BLI_assert(tree->totbranch == 0);

    /* Build the implicit tree */
    non_recursive_bvh_div_nodes(
        tree, tree->nodearray.données() + (tree->totleaf - 1), leafs_array, tree->totleaf);

    /* current code expects the branches to be linked to the nodes array
     * we perform that linkage here */
    tree->totbranch = implicit_needed_branches(tree->tree_type, tree->totleaf);
    for (int i = 0; i < tree->totbranch; i++) {
        tree->nodes[tree->totleaf + i] = &tree->nodearray[tree->totleaf + i];
    }
}

HierarchieBoiteEnglobante *cree_hierarchie_boite_englobante(const Maillage &maillage)
{
    auto const epsilon = 0.0f;
    auto const tree_type = 4;
    auto const axis = 6;

    auto nombre_element = maillage.nombreDePolygones();
    kuri::tableau<int> temp_access_index_sommet;

    auto arbre_hbe = bvhtree_new(nombre_element, epsilon, tree_type, axis);
    auto cos = kuri::tableau<dls::math::vec3f>();

    for (auto i = 0; i < nombre_element; ++i) {
        auto nombre_de_sommets = maillage.nombreDeSommetsPolygone(i);
        temp_access_index_sommet.redimensionne(nombre_de_sommets);

        maillage.indexPointsSommetsPolygone(i, temp_access_index_sommet.données());

        cos.redimensionne(nombre_de_sommets);
        for (int64_t j = 0; j < nombre_de_sommets; j++) {
            cos[j] = maillage.pointPourIndex(temp_access_index_sommet[j]);
        }

        insere(arbre_hbe, i, cos.données(), nombre_de_sommets);
    }

    balance(arbre_hbe);

    return arbre_hbe;
}

/* ********************************************************************************** */

static void construit_niveau_pour_noeud(HierarchieBoiteEnglobante::Noeud *noeud, int niveau)
{
    for (int i = 0; i < noeud->nombre_enfants; i++) {
        auto enfant = noeud->enfants[i];
        enfant->niveau = niveau + 1;
        construit_niveau_pour_noeud(enfant, niveau + 1);
    }
}

static void ajoute_boites_pour_limites(Maillage &maillage, const kuri::tableau<float *> &limites)
{
    auto nombre_de_polygones = 6 * limites.taille();
    auto nombre_de_points = 8 * limites.taille();

    maillage.reserveNombreDePoints(nombre_de_points);
    maillage.reserveNombreDePolygones(nombre_de_polygones);

    const int polygones[6][4] = {
        {0, 4, 7, 3},  // min x
        {5, 1, 2, 6},  // max x
        {1, 5, 4, 0},  // min y
        {3, 7, 6, 2},  // max y
        {1, 0, 3, 2},  // min z
        {4, 5, 6, 7},  // max z
    };

    dls::math::vec3f min, max;

    int decalage_points = 0;

    for (auto limite : limites) {
        min.x = limite[0];
        max.x = limite[1];

        min.y = limite[2];
        max.y = limite[3];

        min.z = limite[4];
        max.z = limite[5];

        const dls::math::vec3f sommets[8] = {dls::math::vec3f(min[0], min[1], min[2]),
                                             dls::math::vec3f(max[0], min[1], min[2]),
                                             dls::math::vec3f(max[0], max[1], min[2]),
                                             dls::math::vec3f(min[0], max[1], min[2]),
                                             dls::math::vec3f(min[0], min[1], max[2]),
                                             dls::math::vec3f(max[0], min[1], max[2]),
                                             dls::math::vec3f(max[0], max[1], max[2]),
                                             dls::math::vec3f(min[0], max[1], max[2])};

        for (auto const &sommet : sommets) {
            maillage.ajouteUnPoint(sommet.x, sommet.y, sommet.z);
        }

        for (int64_t i = 0; i < 6; ++i) {
            int polygone[4];

            for (int j = 0; j < 4; j++) {
                polygone[j] = polygones[i][j] + decalage_points;
            }

            maillage.ajouteUnPolygone(polygone, 4);
        }

        decalage_points += 8;
    }
}

void visualise_hierarchie_au_niveau(HierarchieBoiteEnglobante &hierarchie,
                                    int niveau,
                                    Maillage &maillage)
{
    if (hierarchie.nodes.est_vide()) {
        return;
    }

    auto racine = hierarchie.nodes[hierarchie.totleaf];
    kuri::tableau<float *> limites;
    limites.réserve(hierarchie.totleaf);

    if (niveau <= 0) {
        limites.ajoute(racine->limites);
    }
    else {
        for (auto &noeud : hierarchie.nodearray) {
            noeud.niveau = -1;
        }

        construit_niveau_pour_noeud(racine, 0);

        for (auto &noeud : hierarchie.nodearray) {
            if (noeud.niveau != niveau) {
                continue;
            }

            limites.ajoute(noeud.limites);
        }
    }

    ajoute_boites_pour_limites(maillage, limites);
}

}  // namespace geo
