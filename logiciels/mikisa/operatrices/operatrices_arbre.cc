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

#include "operatrices_arbre.hh"

#include "biblinternes/math/quaternion.hh"

#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/outils/gna.hh"

#include "coeur/donnees_aval.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

#if 1

struct MTreeNode {
	/* position of node in local space */
	dls::math::vec3f position{};
	/* direction of node in local space */
	dls::math::vec3f direction{};
	/* radius of node */
	float radius = 0.0f;
	/* the extremities of the node. First child is the continuity of the branch */
	dls::tableau<MTreeNode *> children{};
	/* the id of the NodeFunction that created the node */
	int creator = 0;
	/* when growing nodes it is useful to know how much they should grow. this parameter gives the length a node has grown since grow was called */
	float growth = 0.0f;
	/* How much the node should be grown. */
	float growth_goal = 0.0f;
	/* The radius of the node when it first started growing */
	float growth_radius = 0.0f;
	/* if true the node can be splitted (can have more than 1 children) */
	bool can_be_splitted = true;
	/* 0 when node is at the begining of a branch, 1 when it is at the end */
	float position_in_branch = 0.0f;
	bool is_branch_origin = false;
	bool can_spawn_leaf = true;
	/* name of bone the branch is bound to, if any */
	dls::chaine bone_name = "";

	MTreeNode(dls::math::vec3f const &p, dls::math::vec3f const &d, float r, int c = 0)
		: position(p)
		, direction(d)
		, radius(r)
		, creator(c)
	{}

	~MTreeNode()
	{
		for (auto enfant : children) {
			memoire::deloge("MTreeNode", enfant);
		}
	}

	std::pair<float, float> get_grow_candidates(dls::tableau<MTreeNode *> &candidats, int creator_)
	{
		auto max_height = -std::numeric_limits<float>::max();
		auto min_height =  std::numeric_limits<float>::max();

		/* seuls les extremités peuvent être poussées */
		if (this->children.est_vide()) {
			if (this->creator == creator_) {
				candidats.pousse(this);
				max_height = min_height = this->position.z;
			}
		}

		for (auto &child : children) {
			auto min_max = child->get_grow_candidates(candidats, creator_);
			min_height = std::min(min_height, min_max.first);
			max_height = std::max(max_height, min_max.second);
		}

		return { min_height, max_height };
	}

	float set_positions_in_branches(float current_distance=0, float distance_from_parent=0)
	{
		/* set each node position_in_branch property to it's correct value
			to do so, the function returns the length of current branch */

		current_distance += distance_from_parent;//# increase current distance by the distance from node parent

		if (this->children.est_vide()) { // if node is the end of the branch, return the length of the branch and set the position_in_branch to 1
			this->position_in_branch = 1.0f;
			return current_distance;
		}
		for (auto i = 1; i < this->children.taille(); ++i) { // recursivelly call the function on all side children
			this->children[i]->set_positions_in_branches(0, 0); // the current_distance of a side child is 0 since it is the begining of a new branch
		}

		auto distance_to_child = longueur(this->position - this->children[0]->position);
		auto branch_length = this->children[0]->set_positions_in_branches(current_distance, distance_to_child);

		this->position_in_branch = (branch_length == 0.0f) ? 0.0f : current_distance / branch_length;
		return branch_length;
	}

	void get_split_candidates(dls::tableau<MTreeNode *> &candidates, int creator_, float start, float end)
	{
		if ((this->children.taille() == 1) && (!this->is_branch_origin) && (this->creator == creator_) && (end >= this->position_in_branch) && (this->position_in_branch >= start)) {
			if (end <= start) {
				this->position_in_branch = 0.0f;
			}
			else {
				this->position_in_branch = (this->position_in_branch - start) / (end-start); // transform the position in branch so that a position at offset is 0
			}

			candidates.pousse(this);
		}

		for (auto &child : this->children) {
			child->get_split_candidates(candidates, creator_, start, end);
		}
	}

	void get_leaf_candidates(dls::tableau<MTreeNode *> &candidates, float max_radius)
	{
		/* recursively populates a list with position, direction radius of all modules susceptible to create a leaf*/
		if ((this->radius <= max_radius) && (this->can_spawn_leaf)) {
			auto extremity = this->children.est_vide();
			auto direction_ = extremity ? this->direction : (this->children[0]->position - this->position);
			auto length = longueur(direction_);
			if (length != 0.0f) {
				direction_ /= length;
			}

			// XXX
			//candidates.pousse(memoire::loge<MTreeNode>("MTreeNode", this->position, direction, length, this->radius, extremity))
		}

		for (auto &child : this->children) {
			child->get_leaf_candidates(candidates, max_radius);
		}
	}

	void get_branches(
			dls::tableau<dls::tableau<dls::math::vec4f>> &positions,
			dls::tableau<dls::tableau<float>> &radii,
			bool first_branch = false,
			dls::math::vec3f *parent_pos = nullptr)
	{
		/* populate list of list of points of each branch */

		auto pos = first_branch ? *parent_pos : this->position;
		positions.back().pousse(dls::math::vec4f(pos.x, pos.y, pos.z, 0.0f));// # add position to last branch
		radii.back().pousse(this->radius); // add radius to last branch

		for (auto i = 0l; i < this->children.taille(); ++i) {
			if (i > 0) { // if child is begining of new branch
				positions.emplace_back(); // add empty branch for position
				radii.emplace_back(); // add empty branch for radius;
			}

			this->children[i]->get_branches(positions, radii, i > 0, &this->position);
		}
	}

	void get_armature_data(float min_radius, int *bone_index, dls::tableau<dls::tableau<MTreeNode *>> &armature_data, int parent_index)
	{
		/* armature data is list of list of (position_head, position_tail, radius_head, radius_tail, parent bone index) of each node. bone_index is a list of one int*/
		auto index = 0;

		if (this->radius > min_radius && !this->children.est_vide()) { // if radius is greater than max radius, add data to armature data
		   // auto child = this->children[0];
		   // armature_data.back().pousse(memoire::loge<MTreeNode>("MTreeNode", this->position, child.position, this->radius, child->radius, parent_index));
			this->bone_name = "bone_" + std::to_string(bone_index[0]);
			index = bone_index[0];
			bone_index[0] += 1;
		}
		else {
			this->bone_name = "bone_" + std::to_string(parent_index);
			index = parent_index;
		}

		for (auto i = 0l; i < this->children.taille(); ++i) {
			auto child = this->children[i];

			if ((i > 0) && (child->radius > min_radius)) {
				armature_data.emplace_back();
			}
			child->get_armature_data(min_radius, bone_index, armature_data, index);
		}
	}

	void recalculate_radius(float base_radius)
	{
		/* used when creating tree from grease pencil, rescales the radius of each branch according to its parent radius */
		this->radius *= base_radius;

		for (auto i = 0l; i < this->children.taille(); ++i) {
			if (i == 0) {
				this->children[i]->recalculate_radius(base_radius);
			}
			else {
				this->children[i]->recalculate_radius(this->radius * 0.9f);
			}
		}
	}
};

static auto gna = GNA();

auto random_on_unit_sphere()
{
	return normalise(dls::math::vec3f(
						 gna.uniforme(0.0f, 1.0f) - 0.5f,
						 gna.uniforme(0.0f, 1.0f) - 0.5f,
						 gna.uniforme(0.0f, 1.0f) - 0.5f));
}

auto random_tangent(dls::math::vec3f const &dir)
{
	auto v = random_on_unit_sphere();
	return produit_croix(v, dir);
}

auto lerp(dls::math::vec3f const &v1, dls::math::vec3f const &v2, float fac)
{
	return (1.0f - fac) * v1 + fac * v2;
}

auto projette(dls::math::vec3f const &v1, dls::math::vec3f const &v2)
{
	return v1 * produit_scalaire(v1, v2);
}

auto conjugue(dls::math::quaternion<float> const &q)
{
	return dls::math::quaternion<float>(-q.vecteur, q.poids);
}

auto rotate_quat(dls::math::vec3f const v, dls::math::quaternion<float> const &q)
{
	auto r = dls::math::quaternion<float>(v, 0.0f);
	auto q_conj = conjugue(q);
	return ((q * r) * q_conj).vecteur;
}

struct CandidatFeuille {
	dls::math::vec3f position{};
	dls::math::vec3f direction{};
	float length{};
	float radius{};
	bool is_end{};
};

auto add_candidates(dls::tableau<CandidatFeuille> &leaf_candidates, int dupli_number)
{
	/* create new leaf candidates by interpolating existing ones */
	auto new_candidates = dls::tableau<CandidatFeuille>{};

	for (auto candidat : leaf_candidates) {
		if (candidat.is_end) {// no new candidate can be created from end_leaf
			continue;
		}

		for (auto i = 0; i < dupli_number; ++i) {
			auto pos = candidat.position + candidat.direction * candidat.length * (static_cast<float>(i) + 1.0f) / (static_cast<float>(dupli_number) + 2.0f);
			new_candidates.pousse({pos, candidat.direction, candidat.length, candidat.radius, candidat.is_end});
		}
	}

	//leaf_candidates.extend(new_candidates);
	for (auto candidat : new_candidates) {
		leaf_candidates.pousse(candidat);
	}
}

struct MTree {
	MTreeNode *stem = nullptr;
	dls::tableau<dls::math::vec3f> verts{};
	dls::tableau<dls::math::vec4i> faces{};

	~MTree()
	{
		memoire::deloge("MTreeNode", stem);
	}

	void build_mesh_data()
	{

	}

	void add_trunk(float length, float radius, float end_radius, float shape, float resolution, float randomness, float axis_attraction, int creator)
	{
		this->stem = memoire::loge<MTreeNode>("MTreeNode",
					dls::math::vec3f(0.0f),
					dls::math::vec3f(0.0f, 0.0f, 1.0f),
					radius,
					creator);

		this->stem->is_branch_origin = true;
		auto remaining_length = length;
		auto extremity = this->stem;// extremity is always the current last node of the trunk

		while (remaining_length > 0) {
			if (remaining_length < 1.0f / resolution) {
				resolution = 1.0f / remaining_length; // last last branch is shorter so that the trunk is exactly of required length
			}

			auto tangent = random_tangent(extremity->direction);
			auto direction = extremity->direction + tangent * randomness / resolution; // direction of new TreeNode

			auto course_correction = dls::math::vec3f(-extremity->position.x,-extremity->position.y, 1.0f / direction.z);
			direction += course_correction * axis_attraction;
			direction = normalise(direction);
			auto position = extremity->position + extremity->direction / resolution; // position of new TreeNode
			auto rad = radius * std::pow(remaining_length/length, shape) + (1.0f - remaining_length/length) * end_radius; // radius of new TreeNode
			auto new_node = memoire::loge<MTreeNode>("MTreeNode", position, direction, rad, creator); // new TreeNode
			extremity->children.pousse(new_node); // Add new TreeNode to extremity's children
			extremity = new_node; // replace extremity by new TreeNode
			remaining_length -= 1.0f / resolution;
		}
	}

	void grow(
			float length,
			float shape_start,
			float shape_end,
			float shape_convexity,
			float resolution,
			float randomness,
			float split_proba,
			float split_angle,
			float split_radius,
			float split_flatten,
			float end_radius,
			float gravity_strength,
			float floor_avoidance,
			bool can_spawn_leaf,
			int creator,
			int selection)
	{
		auto grow_candidates = dls::tableau<MTreeNode *>{};
		this->stem->get_grow_candidates(grow_candidates, selection); // get all leafs of valid creator

		auto branch_length = 1.0f / resolution; // branch length is use multiple times so best to calculate it once

		auto shape_length = [&](float x)
		{
			/* returns y=f(x) so that f(0)=shape_start, f(1)=shape_end and f(0.5) = shape_convexity+1/2(shape_start+shape_end) */
			return -4*shape_convexity*x*(x-1) + x*shape_end + (1-x)*shape_start;
		};

		for (auto node : grow_candidates) {
			node->growth = 0;
			node->growth_goal = std::max(0.001f, length * shape_length(node->position_in_branch)); // add length to node growth goal
			node->growth_radius = node->radius;
		}

		//grow_candidates = deque(grow_candidates); // convert grow_candidates to deque for performance (lots of adding/removing last element)

		while (grow_candidates.taille() > 0) { // grow all candidates until there are none (all have grown to their respective length)
			auto node = grow_candidates[0];

			auto children_number = ((gna.uniforme(0.0f, 1.0f) > split_proba) || node->is_branch_origin) ? 1 : 2; // if 1 the branch grows normally, if more than 1 the branch forks into more branches
			auto tangent = random_tangent(node->direction);

			if ((tangent.z < 0) || (children_number > 1)) {
				tangent.z *= (1.0f - split_flatten);
				tangent = normalise(tangent);
			}

			for (auto i = 0; i < children_number; ++i) {
				auto deviation = children_number==1 ? randomness : split_angle; // how much the new direction will be changed by tangent
				auto direction = lerp(node->direction, tangent * (static_cast<float>(i) - 0.5f) * 2.0f, deviation); // direction of new node
				direction += dls::math::vec3f(0.0f, 0.0f, -1.0f) * gravity_strength / 10.0f / resolution; // apply gravity

				if (floor_avoidance != 0.0f) {
					auto below_ground = floor_avoidance < 0 ? -1.0f : 1.0f; // if -1 then the branches must stay below ground, if 1 they must stay above ground
					auto distance_from_floor = std::max(0.01f, std::abs(node->position.z));
					auto direction_toward_ground = std::max(0.0f, - direction.z * below_ground); // get how much the branch is going towards the floor
					auto floor_avoidance_strength = direction_toward_ground * 0.3f / distance_from_floor * floor_avoidance;

					if (floor_avoidance_strength > 0.1f * (1.0f + floor_avoidance)) { // if the branch is too much towards the floor, break it
						break;
					}

					direction += dls::math::vec3f(0.0f, 0.0f, 1.0f) * floor_avoidance_strength;
				}

				direction = normalise(direction);
				auto position = dls::math::vec3f();

				if (i == 0) {
					position = node->position + direction * branch_length;// # position of new node
				}
				else {
					auto t = normalise(tangent - projette(tangent, node->direction));
					position = (node->position + node->children[0]->position) / 2.0f + t*node->radius;
				}

				auto growth = std::min(node->growth_goal, node->growth + branch_length);// growth of new node

				auto radius = node->growth_radius * ((1.0f - node->growth / node->growth_goal) + end_radius * node->growth / node->growth_goal);// radius of new node
				if (i > 0) {
					radius *= split_radius;// # forked branches have smaller radii
				}

				auto child = memoire::loge<MTreeNode>("MTreeNode", position, direction, radius, creator);
				child->growth_goal = node->growth_goal;
				child->growth = growth;
				child->growth_radius = (i == 0) ? node->growth_radius : node->growth_radius * split_radius;
				child->can_spawn_leaf = can_spawn_leaf;

				if (i > 0) {
					child->is_branch_origin = true;
				}

				node->children.pousse(child);

				if (growth < node->growth_goal) {
					grow_candidates.pousse(child); // if child can still grow, add it to the grow candidates
				}
			}
		}
	}

	void split(
			int amount,
			float angle,
			int max_split_number,
			float radius,
			float start,
			float end,
			float flatten,
			int creator,
			int selection)
	{
		auto split_candidates = dls::tableau<MTreeNode *>{};
		this->stem->set_positions_in_branches();
		this->stem->get_split_candidates(split_candidates, selection, start, end);

		amount = std::min(amount, static_cast<int>(split_candidates.taille()));

		/* À FAIRE : échantillone aléatoirement */
		//split_candidates = sample(split_candidates, amount);

		for (auto &node : split_candidates) {
			auto n_children = gna.uniforme(1, max_split_number);
			auto tangent = random_tangent(node->direction);
			auto flatten_tangent = tangent;
			flatten_tangent.z = 0.0f;
			tangent = lerp(tangent, flatten_tangent, flatten);
			tangent = normalise(tangent);
			auto rot = dls::math::quaternion<float>(node->direction, constantes<float>::TAU / static_cast<float>(n_children));

			for (auto i = 0; i < n_children; ++i) {
				auto t = node->position_in_branch;
				auto direction = normalise(lerp(node->direction, tangent, angle * (1.0f - t / 2.0f)));
				auto position = (node->position + node->children[0]->position) / 2.0f;
				position += normalise(tangent - projette(tangent, node->direction)) * node->radius;
				auto rad = node->radius * radius;
				auto child = memoire::loge<MTreeNode>("MTreeNode", position, direction, rad, creator);
				child->position_in_branch = node->position_in_branch;
				child->is_branch_origin = true;
				child->can_spawn_leaf = false;
				node->children.pousse(child);
				tangent = rotate_quat(tangent, rot);
			}
		}
	}

	void add_branches(
			int amount,
			float angle,
			int max_split_number,
			float radius,
			float end_radius,
			float start,
			float length,
			float shape_start,
			float shape_end,
			float shape_convexity,
			float resolution,
			float randomness,
			float split_proba,
			float split_flatten,
			float gravity_strength,
			float floor_avoidance,
			bool can_spawn_leaf,
			float creator,
			float selection)
	{
			auto split_creator = creator - 0.5f;
			auto split_selection = selection;
			auto grow_selection = creator - 0.5f;
			auto grow_creator = creator;

			this->split(amount, angle, max_split_number, radius, start, 1, split_flatten, static_cast<int>(split_creator), static_cast<int>(split_selection));
			this->grow(length, shape_start, shape_end, shape_convexity, resolution, randomness, split_proba, 0.3f, 0.9f,
					  split_flatten, end_radius, gravity_strength, floor_avoidance, can_spawn_leaf, static_cast<int>(grow_creator), static_cast<int>(grow_selection));
	}

	void roots(
			float length,
			float resolution,
			float split_proba,
			float randomness,
			int creator)
	{
		if (this->stem->children.est_vide()) { // roots can only be added on a trunk on non 0 length
			return;
		}

		auto roots_origin = memoire::loge<MTreeNode>("MTreeNode", this->stem->position, -this->stem->direction, this->stem->radius, -1);
		roots_origin->is_branch_origin = true;
		this->stem->children.pousse(roots_origin); // stem is set as branch origin, so it cannot be splitted by split function. second children of stem will then always be root origin

		this->grow(length, 1, 1, 0, resolution, randomness, split_proba, 0.5f, 0.6f, 0, 0, -0.1f, -1, false, creator, -1);
	}

//	void get_leaf_emitter_data(
//			int number,
//			float weight,
//			float max_radius,
//			float spread,
//			float flatten,
//			bool extremity_only)
//	{
//		/* À FAIRE : quelle structure ? vector de vector ? */
//		auto leaf_candidates = dls::tableau<MTreeNode *>{};
//		this->stem->get_leaf_candidates(leaf_candidates, max_radius);

//		if (!extremity_only) {
//			if (number > static_cast<int>(leaf_candidates.taille())) {
//				auto factor = number / len([i for i in leaf_candidates if not i[-1]]); // remove extremities from factor because they won't participate in candidate addition
//				add_candidates(leaf_candidates, factor);
//			}

//			leaf_candidates = sample(leaf_candidates, number);
//		}
//		else {
//			leaf_candidates = [i for i in leaf_candidates if i[-1]];
//		}
//		verts = [];
//		faces = [];

//		for position, direction, length, radius, is_end in leaf_candidates {
//			tangent = Vector((0,0,1)).cross(direction).normalized();
//			if not is_end: // only change direction when leaf is not at a branch extremity
//				tangent = (randint(0,1) * 2 - 1) * tangent; // randomize sign of tangent
//				direction = direction.lerp(tangent, spread);
//				direction.z *= (1-flatten);
//				direction.z -= weight;
//				direction.normalize();
//			}
//			x_axis = direction.orthogonal();
//			y_axis = direction.cross(x_axis);
//			v1 = position + x_axis * .01;
//			v3 = position + y_axis * .01;
//			v2 = position - x_axis * .01;
//			n_verts = len(verts);
//			verts.extend([v3, v2, v1]);
//			faces.append((n_verts, n_verts+1, n_verts+2));
//		}

//		return verts, faces;
//	}

	void twig(
			float radius,
			float length,
			int branch_number,
			float randomness,
			float resolution,
			float gravity_strength,
			float flatten)
	{
		this->stem = memoire::loge<MTreeNode>(
					"MTreeNode",
					dls::math::vec3f(0.0f),
					dls::math::vec3f(1.0f, 0.0f, 0.0f),
					radius*0.1f, 0);

		this->grow(1, 1, 1, 0, resolution, randomness/2/resolution, 0, 0.2f, 0, 0, 0, 0.1f,0, true, 1, 0);
		this->add_branches(branch_number, 0.5f, 2, 0.7f, 0.1f, 0, length*0.7f, 0.5f, 0.5f, 0, resolution, randomness/resolution, 0.1f / resolution, flatten, gravity_strength/resolution, 0, true, 2, 1);

		/* À FAIRE */
//		leaf_candidates = [];
//		this->stem.get_leaf_candidates(leaf_candidates, radius);
//		return [i for i in leaf_candidates if i[-1]];
	}

	/* À FAIRE : get_armature_data, build_tree_from_grease_pencil */
};

class OpAjouteBranch final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Ajoute Branche";
	static constexpr auto AIDE = "Crée un arbre.";

	OpAjouteBranch(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		/* Pour générer des arbres, nous inversons la direction du flux :
		 * d'abord nous travaillons sur nos données, ensuite nous exécutons les
		 * noeuds en amont.
		 */

		if (!donnees_aval->possede("arbre")) {
			this->ajoute_avertissement("Aucun arbre en aval !");
			return res_exec::ECHOUEE;
		}

		auto arbre = std::any_cast<MTree *>(donnees_aval->table["arbre"]);

		auto creator = reinterpret_cast<long>(this);
		auto selection = 0.0f;
		auto amount = 20;
		auto split_angle = 0.6f;
		auto max_split_number = 3;
		auto radius = 0.6f;
		auto end_radius = 0.0f;
		auto min_height = 0.1f;
		auto length = 7.0f;
		auto shape_start = 1.0f;
		auto shape_end = 1.0f;
		auto shape_convexity = 0.3f;
		auto resolution = 1.0f;
		auto randomness = 0.15f;
		auto split_proba = 0.1f;
		auto split_flatten = 0.5f;
		auto can_spawn_leafs = true;
		auto gravity_strength = 0.3f;
		auto floor_avoidance = 1.0f;

		arbre->add_branches(
					amount,
					split_angle,
					max_split_number,
					radius,
					end_radius,
					min_height,
					length,
					shape_start,
					shape_end,
					shape_convexity,
					resolution,
					randomness,
					split_proba,
					split_flatten,
					gravity_strength,
					floor_avoidance,
					can_spawn_leafs,
					static_cast<float>(creator),
					selection);

		entree(0)->requiers_corps(contexte, donnees_aval);
		return res_exec::REUSSIE;
	}
};

class OpGrow final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Grow";
	static constexpr auto AIDE = "Crée un arbre.";

	OpGrow(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		/* Pour générer des arbres, nous inversons la direction du flux :
		 * d'abord nous travaillons sur nos données, ensuite nous exécutons les
		 * noeuds en amont.
		 */

		if (!donnees_aval->possede("arbre")) {
			this->ajoute_avertissement("Aucun arbre en aval !");
			return res_exec::ECHOUEE;
		}

		auto arbre = std::any_cast<MTree *>(donnees_aval->table["arbre"]);

		auto creator = reinterpret_cast<long>(this);
		auto selection = 0;
		//auto seed = 0;
		auto length = 7.0f;
		auto shape_start = 0.5f;
		auto shape_end = 0.5f;
		auto shape_convexity = 1.0f;
		auto resolution = 1.0f;
		auto randomness = 0.1f;
		auto split_proba = 0.1f;
		auto split_angle = 0.3f;
		auto split_radius = 0.9f;
		auto split_flatten = 0.5f;
		auto end_radius = 1.0f;
		auto can_spawn_leafs = true;
		auto gravity_strength = 0.1f;
		auto floor_avoidance = 1.0f;

		arbre->grow(
					length,
					shape_start,
					shape_end,
					shape_convexity,
					resolution,
					randomness,
					split_proba,
					split_angle,
					split_radius,
					split_flatten,
					end_radius,
					gravity_strength,
					floor_avoidance,
					can_spawn_leafs,
					static_cast<int>(creator),
					selection);

		entree(0)->requiers_corps(contexte, donnees_aval);
		return res_exec::REUSSIE;
	}
};

class OperatriceCreationArbre final : public OperatriceCorps {
	MTree *m_arbre = nullptr;

public:
	static constexpr auto NOM = "Création Arbre";
	static constexpr auto AIDE = "Crée un arbre.";

	OperatriceCreationArbre(Graphe &graphe_parent, Noeud &noeud_);

	OperatriceCreationArbre(OperatriceCreationArbre const &) = default;
	OperatriceCreationArbre &operator=(OperatriceCreationArbre const &) = default;

	~OperatriceCreationArbre() override;

	const char *chemin_entreface() const override;

	const char *nom_classe() const override;

	const char *texte_aide() const override;

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override;
};


OperatriceCreationArbre::OperatriceCreationArbre(Graphe &graphe_parent, Noeud &noeud_)
	: OperatriceCorps(graphe_parent, noeud_)
{
	entrees(1);
	sorties(1);
}

OperatriceCreationArbre::~OperatriceCreationArbre()
{
	memoire::deloge("MTree", m_arbre);
}

const char *OperatriceCreationArbre::chemin_entreface() const
{
	return "entreface/operatrice_creation_arbre.jo";
}

const char *OperatriceCreationArbre::nom_classe() const
{
	return NOM;
}

const char *OperatriceCreationArbre::texte_aide() const
{
	return AIDE;
}

static void construit_geometrie(Corps &corps, MTreeNode *noeud)
{
	auto points = corps.points_pour_ecriture();
	for (auto enfant : noeud->children) {
		points.ajoute_point(enfant->position);

		construit_geometrie(corps, enfant);
	}
}

res_exec OperatriceCreationArbre::execute(const ContexteEvaluation &contexte, DonneesAval *donnees_aval)
{
	m_corps.reinitialise();
	INUTILISE(donnees_aval);

	if (m_arbre != nullptr) {
		delete m_arbre;
	}

	m_arbre = memoire::loge<MTree>("Mtree");

	//auto seed = 1;
	auto length = 25.0f;
	auto radius = 0.5f;
	auto end_radius = 0.0f;
	auto resolution = 1.0f;
	auto shape = 1.0f;
	auto randomness = 0.1f;
	auto axis_attraction = 0.25f;

	m_arbre->add_trunk(length, radius, end_radius, shape, resolution, randomness, axis_attraction, 0);

	auto mes_donnees = DonneesAval{};
	mes_donnees.table.insere({"arbre", m_arbre});

	entree(0)->requiers_corps(contexte, &mes_donnees);

	construit_geometrie(m_corps, m_arbre->stem);

	return res_exec::REUSSIE;
}
#else
enum {
	FORME_CONIQUE = 0,
	FORME_SPHERIQUE = 1,
	FORME_HEMISPHERIQUE = 2,
	FORME_CYLINDRIQUE = 3,
	FORME_CYLINDRIQUE_TAPERED = 4,
	FORME_FLAME = 5,
	FORME_CONIQUE_INVERSE = 6,
	FORME_FLAME_TEND = 7,
	FORME_ENVELOPPE = 8,
};

struct Parametres {
	int Shape;
	float BaseSize;
	float Scale,ScaleV,ZScale,ZScaleV;
	int Levels;
	float Ratio,RatioPower;
	int Lobes;
	float LobeDepth;
	float Flare;

	float _0Scale, _0ScaleV;
	float _0Length, _0LengthV, _0Taper;
	float _0BaseSplits;
	float _0SegSplits,_0SplitAngle,_0SplitAngleV;
	float _0CurveRes,_0Curve,_0CurveBack,_0CurveV;

	float _1DownAngle,_1DownAngleV;
	float _1Rotate,_1RotateV,_1Branches;
	float _1Length,_1LengthV,_1Taper;
	float _1SegSplits,_1SplitAngle,_1SplitAngleV;
	float _1CurveRes,_1Curve,_1CurveBack,_1CurveV;

	float _2DownAngle,_2DownAngleV;
	float _2Rotate,_2RotateV,_2Branches;
	float _2Length,_2LengthV, _2Taper;
	float _2SegSplits,_2SplitAngle,_2SplitAngleV;
	float _2CurveRes,_2Curve,_2CurveBack,_2CurveV;

	float _3DownAngle,_3DownAngleV;
	float _3Rotate,_3RotateV,_3Branches;
	float _3Length,_3LengthV, _3Taper;
	float _3SegSplits,_3SplitAngle,_3SplitAngleV;
	float _3CurveRes,_3Curve,_3CurveBack,_3CurveV;

	float Leaves,LeafShape;
	float LeafScale,LeafScaleX;
	float AttractionUp;
	float PruneRatio;
	float PruneWidth,PruneWidthPeak;
	float PrunePowerLow,PrunePowerHigh;

	Parametres() = default;
	Parametres(Parametres const &autre) = default;
};

static void parametres_tremble(Parametres *parametres)
{
	parametres->Shape = FORME_FLAME_TEND;
	parametres->BaseSize = 0.4f;
	parametres->Scale = 13;
	parametres->ScaleV = 3;
	parametres->ZScale = 1;
	parametres->ZScaleV = 0;
	parametres->Levels = 3;
	parametres->Ratio = 0.015f;
	parametres->RatioPower = 1.2f;
	parametres->Lobes = 5;
	parametres->LobeDepth = 0.07f;
	parametres->Flare = 0.6f;

	parametres->_0Scale = 1;
	parametres->_0ScaleV = 0;
	parametres->_0Length = 1;
	parametres->_0LengthV = 0;
	parametres->_0BaseSplits = 0;
	parametres->_0SegSplits = 0;
	parametres->_0SplitAngle = 0;
	parametres->_0SplitAngleV = 0;
	parametres->_0CurveRes = 3;
	parametres->_0Curve = 0;
	parametres->_0CurveBack = 0;
	parametres->_0CurveV = 20;

	parametres->_1DownAngle = 60;
	parametres->_1DownAngleV = -50;
	parametres->_1Rotate = 140;
	parametres->_1RotateV = 0;
	parametres->_1Branches = 50;
	parametres->_1Length = 0.3f;
	parametres->_1LengthV = 0;
	parametres->_1Taper = 1;
	parametres->_1SegSplits = 0;
	parametres->_1SplitAngle = 0;
	parametres->_1SplitAngleV = 0;
	parametres->_1CurveRes = 5;
	parametres->_1Curve = -40;
	parametres->_1CurveBack = 0;
	parametres->_1CurveV = 50;

	parametres->_2DownAngle = 45;
	parametres->_2DownAngleV = 10;
	parametres->_2Rotate = 140;
	parametres->_2RotateV = 0;
	parametres->_2Branches = 30;
	parametres->_2Length = 0.6f;
	parametres->_2LengthV = 0;
	parametres->_2Taper = 1;
	parametres->_2SegSplits = 0;
	parametres->_2SplitAngle = 0;
	parametres->_2SplitAngleV = 0;
	parametres->_2CurveRes = 3;
	parametres->_2Curve = -40;
	parametres->_2CurveBack = 0;
	parametres->_2CurveV = 75;

	parametres->_3DownAngle = 45;
	parametres->_3DownAngleV = 10;
	parametres->_3Rotate = 77;
	parametres->_3RotateV = 0;
	parametres->_3Branches = 10;
	parametres->_3Length = 0;
	parametres->_3LengthV = 0;
	parametres->_3Taper = 1;
	parametres->_3SegSplits = 0;
	parametres->_3SplitAngle = 0;
	parametres->_3SplitAngleV = 0;
	parametres->_3CurveRes = 1;
	parametres->_3Curve = 0;
	parametres->_3CurveBack = 0;
	parametres->_3CurveV = 0;

	parametres->Leaves = 25;
	parametres->LeafShape = 0;
	parametres->LeafScale = 0.17f;
	parametres->LeafScaleX = 1;
	parametres->AttractionUp = 0.5f;
	parametres->PruneRatio = 0;
	parametres->PruneWidth = 0.5f;
	parametres->PruneWidthPeak = 0.5f;
	parametres->PrunePowerLow = 0.5f;
	parametres->PrunePowerHigh = 0.5f;
}

float ShapeRatio(int shape, float ratio)
{
	switch (shape) {
		case FORME_CONIQUE:
			return 0.2f + 0.8f * ratio;
		case FORME_SPHERIQUE:
			return 0.2f + 0.8f * std::sin(constantes<float>::PI * ratio);
		case FORME_HEMISPHERIQUE:
			return 0.2f + 0.8f * std::sin(0.5f * constantes<float>::PI * ratio);
		case FORME_CYLINDRIQUE:
			return 1.0f;
		case FORME_CYLINDRIQUE_TAPERED:
			return 0.5f + 0.5f * ratio;
		case FORME_FLAME:
			if (ratio <= 0.7f) {
				return ratio / 0.7f;
			}

			return (1.0f - ratio) / 0.3f;
		case FORME_CONIQUE_INVERSE:
			return 1.0f - 0.8f * ratio;
		case FORME_FLAME_TEND:
			if (ratio <= 0.7f) {
				return 0.5f + 0.5f * ratio / 0.7f;
			}

			return 0.5f + 0.5f * (1.0f - ratio) / 0.3f;
		default:
		case FORME_ENVELOPPE:
			/* À FAIRE  { utiliser l'enveloppe de taille */
			return 1.0f;
	}
}

/* ************************************************************************** */

#include "biblinternes/math/quaternion.hh"

struct BezierPoint {
	dls::math::vec3f handle_left{};
	dls::math::vec3f co{};
	dls::math::vec3f handle_right{};
	float radius;
	dls::chaine handle_left_type = "";
	dls::chaine handle_right_type = "";
};

struct Spline {
	dls::tableau<BezierPoint *> bezier_points{};
};

static auto zAxis = dls::math::vec3f(0, 0, 1);
static auto yAxis = dls::math::vec3f(0, 1, 0);
static auto xAxis = dls::math::vec3f(1, 0, 0);


void axis_angle_normalized_to_quat(dls::math::quaternion<float> &q, const dls::math::vec3f &axis, const float angle)
{
	const float phi = 0.5f * angle;
	const float si = std::sin(phi);
	const float co = std::cos(phi);
	//BLI_ASSERT_UNIT_V3(axis); assert(longueur(axis) == 1.0f);

	q.vecteur = dls::math::vec3f(co, axis[0] * si, axis[1] * si);
	q.poids = axis[2] * si;
}

inline float saacos(float fac)
{
	if      (fac <= -1.0f) return constantes<float>::PI;
	else if (fac >=  1.0f) return 0.0f;
	else                             return std::acos(fac);
}

static dls::math::mat3x3f quat_to_mat3(const dls::math::quaternion<float> q)
{
	auto const q0 = constantes<double>::SQRT2 * static_cast<double>(q.vecteur[0]);
	auto const q1 = constantes<double>::SQRT2 * static_cast<double>(q.vecteur[1]);
	auto const q2 = constantes<double>::SQRT2 * static_cast<double>(q.vecteur[2]);
	auto const q3 = constantes<double>::SQRT2 * static_cast<double>(q.poids);

	auto const qda = q0 * q1;
	auto const qdb = q0 * q2;
	auto const qdc = q0 * q3;
	auto const qaa = q1 * q1;
	auto const qab = q1 * q2;
	auto const qac = q1 * q3;
	auto const qbb = q2 * q2;
	auto const qbc = q2 * q3;
	auto const qcc = q3 * q3;

	auto m = dls::math::mat3x3f{};

	m[0][0] = static_cast<float>(1.0 - qbb - qcc);
	m[0][1] = static_cast<float>(qdc + qab);
	m[0][2] = static_cast<float>(-qdb + qac);

	m[1][0] = static_cast<float>(-qdc + qab);
	m[1][1] = static_cast<float>(1.0 - qaa - qcc);
	m[1][2] = static_cast<float>(qda + qbc);

	m[2][0] = static_cast<float>(qdb + qac);
	m[2][1] = static_cast<float>(-qda + qbc);
	m[2][2] = static_cast<float>(1.0 - qaa - qbb);

	return m;
}

auto rotate(dls::math::vec3f const &vec, dls::math::quaternion<float> const &quat)
{
	auto mat = quat_to_mat3(quat);

	return vec * mat;
}

inline float saasin(float fac)
{
	if      ((fac <= -1.0f)) return -constantes<float>::PI / 2.0f;
	else if ((fac >=  1.0f)) return  constantes<float>::PI / 2.0f;
	else                             return asinf(fac);
}
float angle_normalized_v3v3(const dls::math::vec3f &v1, const dls::math::vec3f &v2)
{
	/* double check they are normalized */
//	BLI_ASSERT_UNIT_V3(v1);
//	BLI_ASSERT_UNIT_V3(v2);

	/* this is the same as acos(dot_v3v3(v1, v2)), but more accurate */
	if (produit_scalaire(v1, v2) >= 0.0f) {
		return 2.0f * saasin(longueur(v1 - v2) / 2.0f);
	}
	else {
		return constantes<float>::PI - 2.0f * saasin(longueur(v1 - (-v2)) / 2.0f);
	}
}

void axis_angle_to_quat(dls::math::quaternion<float> &q, const  dls::math::vec3f &axis, const float angle)
{
	dls::math::vec3f nor;
	nor = normalise(axis);
	auto l = longueur(axis);

	if (l != 0.0f) {
		axis_angle_normalized_to_quat(q, nor, angle);
	}
	else {
		q.vecteur[0] = 1.0f;
		q.vecteur[1] = 0.0f;
		q.vecteur[2] = 0.0f;
		q.poids      = 0.0f;
	}
}

void ortho_v3_v3(dls::math::vec3f &out, const dls::math::vec3f &v)
{
	const int axis = dls::math::axe_dominant_abs(v);

	//BLI_assert(out != v);

	switch (axis) {
		case 0:
			out[0] = -v[1] - v[2];
			out[1] =  v[0];
			out[2] =  v[0];
			break;
		case 1:
			out[0] =  v[1];
			out[1] = -v[0] - v[2];
			out[2] =  v[1];
			break;
		case 2:
			out[0] =  v[2];
			out[1] =  v[2];
			out[2] = -v[0] - v[1];
			break;
	}
}
/* note: expects vectors to be normalized */
void rotation_between_vecs_to_quat(dls::math::quaternion<float> &q, const dls::math::vec3f &v1, const dls::math::vec3f &v2)
{
	auto axe = dls::math::vec3f();

	axe = produit_croix(v1, v2);

	auto l = longueur(axe);
	axe = normalise(axe);

	if (l > 1e-6f) {
		float angle;

		angle = angle_normalized_v3v3(v1, v2);

		axis_angle_normalized_to_quat(q, axe, angle);
	}
	else {
		/* degenerate case */

		if (produit_scalaire(v1, v2) > 0.0f) {
			/* Same vectors, zero rotation... */
			q.vecteur[0] = 1.0f;
			q.vecteur[1] = 0.0f;
			q.vecteur[2] = 0.0f;
			q.poids      = 0.0f;
		}
		else {
			/* Colinear but opposed vectors, 180 rotation... */
			ortho_v3_v3(axe, v1);
			axis_angle_to_quat(q, axe, constantes<float>::PI);
		}
	}
}
auto vec_to_quat(dls::math::vec3f const &vec, int axis, int upflag)
{
	/* first set the quat to unit */
	dls::math::quaternion<float> q(dls::math::vec3f(1.0f, 0.0f, 0.0f), 0.0f);

	const float eps = 1e-4f;
	float angle, si, co;
	dls::math::vec3f nor, tvec;

	assert(axis >= 0 && axis <= 5);
	assert(upflag >= 0 && upflag <= 2);

	auto len = longueur(vec);

	if (len == 0.0f) {
		return q;
	}

	/* rotate to axis */
	if (axis > 2) {
		tvec = vec;
		axis = axis - 3;
	}
	else {
		tvec = -vec;
	}

	/* nasty! I need a good routine for this...
	 * problem is a rotation of an Y axis to the negative Y-axis for example.
	 */

	if (axis == 0) { /* x-axis */
		nor[0] =  0.0;
		nor[1] = -tvec[2];
		nor[2] =  tvec[1];

		if (fabsf(tvec[1]) + fabsf(tvec[2]) < eps)
			nor[1] = 1.0f;

		co = tvec[0];
	}
	else if (axis == 1) { /* y-axis */
		nor[0] =  tvec[2];
		nor[1] =  0.0;
		nor[2] = -tvec[0];

		if (fabsf(tvec[0]) + fabsf(tvec[2]) < eps)
			nor[2] = 1.0f;

		co = tvec[1];
	}
	else { /* z-axis */
		nor[0] = -tvec[1];
		nor[1] =  tvec[0];
		nor[2] =  0.0;

		if (fabsf(tvec[0]) + fabsf(tvec[1]) < eps)
			nor[0] = 1.0f;

		co = tvec[2];
	}
	co /= len;

	nor = normalise(nor);

	axis_angle_normalized_to_quat(q, nor, saacos(co));

	if (axis != upflag) {
		dls::math::quaternion<float> q2;
		auto mat = quat_to_mat3(q);
		const auto fp = mat[2];

		if (axis == 0) {
			if (upflag == 1) angle =  0.5f * atan2f(fp[2], fp[1]);
			else             angle = -0.5f * atan2f(fp[1], fp[2]);
		}
		else if (axis == 1) {
			if (upflag == 0) angle = -0.5f * atan2f(fp[2], fp[0]);
			else             angle =  0.5f * atan2f(fp[0], fp[2]);
		}
		else {
			if (upflag == 0) angle =  0.5f * atan2f(-fp[1], -fp[0]);
			else             angle = -0.5f * atan2f(-fp[0], -fp[1]);
		}

		co = cosf(angle);
		si = sinf(angle) / len;
		q2.vecteur[0] = co;
		q2.vecteur[1] = tvec[0] * si;
		q2.vecteur[2] = tvec[1] * si;
		q2.poids = tvec[2] * si;

		q = q * q2;
	}

	return q;
}

auto to_track_quat(dls::math::vec3f const &vec, int track, int up)
{
	return vec_to_quat(-vec, track, up);
}

auto rotate(dls::math::vec3f const &vec, dls::math::quaternion<float> const &quat)
{
	auto mat = quat_to_mat3(quat);
	return mat * vec;
}

struct stemSpline {
	Spline *spline = nullptr;
	BezierPoint *p = nullptr;
	float curv = 0.0f;
	float curvV = 0.0f;
	int vertAtt = 0;
	int seg = 0;
	int segMax = 0;
	float segL = 0.0f;
	int children = 0;
	float radS = 0.0f;
	float radE = 0.0f;
	int splN = 0;
	float offsetLen = 0;
	int curvSignx = 1;
	int curvSigny = 1;
	float pad;
	dls::math::quaternion<float> patentQuat;// = nullptr;

	stemSpline() = default;

	stemSpline(Spline *spline, float curvature, float curvatureV, int attractUp, int segments, int maxSegs,
				 float segLength, int childStems, float stemRadStart, float stemRadEnd, int splineNum, float ofst, dls::math::quaternion<float> pquat)
	{
		this->spline = spline;
		this->p = spline->bezier_points.back();
		this->curv = curvature;
		this->curvV = curvatureV;
		this->vertAtt = attractUp;
		this->seg = segments;
		this->segMax = maxSegs;
		this->segL = segLength;
		this->children = childStems;
		this->radS = stemRadStart;
		this->radE = stemRadEnd;
		this->splN = splineNum;
		this->offsetLen = ofst;
		this->patentQuat = pquat;
		this->curvSignx = 1;
		this->curvSigny = 1;
	}

	// This method determines the quaternion of the end of the spline
	dls::math::quaternion<float> quat()
	{
		if (this->spline->bezier_points.taille() == 1) {
			return to_track_quat(normalise(this->spline->bezier_points.back()->handle_right -
					 this->spline->bezier_points.back()->co), 2, 1);
		}

		return to_track_quat(normalise(this->spline->bezier_points.back()->co - this->spline->bezier_points[this->spline->bezier_points.taille() - 2]->co), 2, 1);
	}

	// Determine the declination
	float dec()
	{
		auto tempVec = rotate(zAxis, this->quat());
		return produit_scalaire(zAxis, tempVec);// zAxis.angle(tempVec);
	}

	// Update the end of the spline and increment the segment count
	void updateEnd()
	{
		this->p = this->spline->bezier_points.back();
		this->seg += 1;
	}
};

struct Curve {
	int resolution_u = 0;
	dls::tableau<Spline *> splines;
};

void kickstart_trunk(
		dls::tableau<stemSpline> &addstem,
		int levels,
		int leaves,
		dls::math::vec3f branches,
		Curve &cu,
		dls::math::vec3f curve,
		dls::math::vec3f curveRes,
		dls::math::vec3f curveV,
		dls::math::vec3f attractUp,
		dls::math::vec3f length,
		float lengthV,
		float ratio,
		float ratioPower,
		int resU,
		float scale0,
		float scaleV0,
		float scaleVal,
		dls::math::vec3f taper,
		float minRadius,
		float rootFlare)
{
	auto newSpline = new Spline; // 'BEZIER'
	cu.resolution_u = resU;
	auto point_bezier = new BezierPoint{};
	newSpline->bezier_points.pousse(point_bezier);

	auto newPoint = newSpline->bezier_points.back();
	newPoint->co = dls::math::vec3f(0.0f, 0.0f, 0.0f);
	newPoint->handle_right = dls::math::vec3f(0.0f, 0.0f, 1.0f);
	newPoint->handle_left = dls::math::vec3f(0.0f, 0.0f, -1.0f);
	// (newPoint.handle_right_type, newPoint.handle_left_type) = ('VECTOR', 'VECTOR');

	auto branchL = scaleVal * length[0];
	auto curveVal = curve[0] / curveRes[0];
	// curveVal = curveVal * (branchL / scaleVal);
	auto childStems = (levels == 1) ? leaves : branches[1];

	/* À FAIRE  { uniform RNG */
	auto startRad = scaleVal * ratio * scale0 /* * uniform(1 - scaleV0, 1 + scaleV0)*/;  // * (scale0 + uniform(-scaleV0, scaleV0));
   auto  endRad = std::pow(startRad * (1.0f - taper[0]), ratioPower);
	startRad = std::max(startRad, minRadius);
	endRad = std::max(endRad, minRadius);
	newPoint->radius = startRad * rootFlare;

	auto stem = stemSpline(newSpline, curveVal, curveV[0] / curveRes[0], attractUp[0],
			0, curveRes[0], branchL / curveRes[0],
			childStems, startRad, endRad, 0, 0, {}
			);

	addstem.pousse(stem);
}

struct childPoint {

	dls::math::vec3f co;
	dls::math::quaternion<float> quat;
	dls::math::vec3d radiusPar;
	int offset = 0;
	int stemOffset;
	double lengthPar;
	int parBone;

	childPoint(
			dls::math::vec3f const &coords,
			dls::math::quaternion<float> const &quaternion,
			dls::math::vec3d _radiusPar,
			int _offset,
			int _sOfst,
			double _lengthPar,
			int _parBone)
		: co(coords)
		, quat(quaternion)
		, radiusPar(_radiusPar)
		, offset(_offset)
		, stemOffset(_sOfst)
		, lengthPar(_lengthPar)
		, parBone(_parBone)
	{}
};

double shapeRatio(int shape, double ratio, double pruneWidthPeak=0.0, double prunePowerHigh=0.0, double prunePowerLow=0.0, double *custom=nullptr)
{
	if (shape == 0) {
		return 0.05 + 0.95 * ratio; // 0.2 + 0.8 * ratio
	}

	if (shape == 1) {
		return 0.2 + 0.8 * std::sin(constantes<double>::PI * ratio);
	}

	if (shape == 2) {
		return 0.2 + 0.8 * std::sin(0.5 * constantes<double>::PI * ratio);
	}
	if (shape == 3) {
		return 1.0;
	}
	if (shape == 4) {
		return 0.5 + 0.5 * ratio;
	}
	if (shape == 5) {
		if (ratio <= 0.7) {
			return 0.05 + 0.95 * ratio / 0.7;
		}

		return 0.05 + 0.95 * (1.0 - ratio) / 0.3;
	}
	if (shape == 6) {
		return 1.0 - 0.8 * ratio;
	}
	if (shape == 7) {
		if (ratio <= 0.7) {
			return 0.5 + 0.5 * ratio / 0.7;
		}

		return 0.5 + 0.5 * (1.0 - ratio) / 0.3;
	}
	if (shape == 8) {
		auto r = 1.0 - ratio;
		auto v = 0.0;

		if (r == 1.0) {
			v = custom[3];
		}
		else if (r >= custom[2]) {
			auto pos = (r - custom[2]) / (1 - custom[2]);
			// if (custom[0] >= custom[1] <= custom[3]) or (custom[0] <= custom[1] >= custom[3]) {
			pos = pos * pos;
			v = (pos * (custom[3] - custom[1])) + custom[1];
		}
		else{
			auto pos = r / custom[2];
			// if (custom[0] >= custom[1] <= custom[3]) or (custom[0] <= custom[1] >= custom[3]) {
			pos = 1 - (1 - pos) * (1 - pos);
			v = (pos * (custom[1] - custom[0])) + custom[0];
		}

		return v;
	}

	if (shape == 9) {
		if ((ratio < (1 - pruneWidthPeak)) && (ratio > 0.0)) {
			return std::pow((ratio / (1 - pruneWidthPeak)), prunePowerHigh);
		}
		else if ((ratio >= (1 - pruneWidthPeak)) && (ratio < 1.0)) {
			return std::pow(((1 - ratio) / pruneWidthPeak), prunePowerLow);
		}
		else {
			return 0.0;
		}
	}

	if (shape == 10) {
		return 0.5 + 0.5 * (1 - ratio);
	}

	return 0.0;
}


void fabricate_stems(
		double addsplinetobone,
		dls::tableau<stemSpline> &addstem,
		double baseSize,
		dls::tableau<double> &branches,
		dls::tableau<childPoint> &childP,
		Curve &cu,
		dls::tableau<double> &curve,
		double curveBack,
		dls::tableau<double> &curveRes,
		dls::tableau<double> &curveV,
		dls::tableau<double> &attractUp,
		dls::tableau<double> &downAngle,
		dls::tableau<double> &downAngleV,
		double leafDist,
		double leaves,
		dls::tableau<double> &length,
		dls::tableau<double> &lengthV,
		int levels,
		size_t n,
		double ratioPower,
		int resU,
		dls::tableau<double> &rotate,
		dls::tableau<double> &rotateV,
		double scaleVal,
		int shape,
		int storeN,
		dls::tableau<double> &taper,
		int shapeS,
		double minRadius,
		dls::tableau<double> &radiusTweak,
		double *customShape,
		dls::chaine const &rMode,
		double segSplits,
		bool useOldDownAngle,
		bool useParentAngle,
		double boneStep)
{
	// prevent baseSize from going to 1.0
	baseSize = std::min(0.999, baseSize);

	// Store the old rotation to allow new stems to be rotated away from the previous one.
	auto oldRotate = 0;

	auto rot_a = dls::tableau<int>{};

	// use fancy child point selection / rotation
	if ((n == 1) && (rMode != "original")) {
		auto childP_T0 = dls::dico_desordonne<int, dls::tableau<childPoint>>{};
		auto childP_L = dls::tableau<childPoint>{};

		for (auto const &p : childP) {
			if (p.offset == 1) {
				childP_L.pousse(p);
			}
			else {
				auto iter = std::find(childP_T0.debut(), childP_T0.fin(), p.offset);

				if (iter == childP_T0.fin()) {
					childP_T0[p.offset] = { p };
				}
				else {
					childP_T0[p.offset].pousse(p);
				}
			}
		}

		auto childP_T = dls::tableau<dls::tableau<childPoint>>{}; //[childP_T[k] for k in sorted(childP_T.keys())];

		childP = dls::tableau<childPoint>{};
		auto rot_a = dls::tableau<int>{};

		for (auto &p : childP_T) {
			if (rMode == "rotate") {
				if (rotate[n] < 0.0) {
					oldRotate = -copysign(rotate[n], oldRotate);
				}
				else {
					oldRotate += rotate[n];
				}

				/* À FAIRE  { uniform() */
				auto bRotate = oldRotate;// + uniform(-rotateV[n], rotateV[n]);

				// choose start point whose angle is closest to the rotate angle
				auto a1 = std::fmod(bRotate, constantes<double>::TAU);
				auto a_diff = dls::tableau<double>{};

				for (auto &a  { p) {
					auto a2 = std::atan2(a.co[0], -a.co[1]);
					auto d = std::min(std::fmod(a1 - a2 + constantes<double>::TAU, constantes<double>::TAU), std::fmod(a2 - a1 + constantes<double>::TAU, constantes<double>::TAU));
					a_diff.pousse(d);
				}

				auto idx = 0ul;// a_diff.index(min(a_diff));

				// find actual rotate angle from branch location
				auto br = p[idx];
				auto b = br.co;
				auto vx = std::sin(bRotate);
				auto vy = std::cos(bRotate);
				auto v = dls::math::vec2d(vx, vy);

				auto bD = std::pow(b[0] * b[0] + b[1] * b[1], 0.5);
				auto bL = br.lengthPar * length[1] * shapeRatio(shape, (1 - br.offset) / (1 - baseSize), 0.0, 0.0, 0.0, customShape);

				// account for down angle
				auto downA = 0.0;

				if (downAngleV[1] > 0) {
					downA = downAngle[n] + std::pow(-downAngleV[n] * (1 - (1 - br.offset) / (1 - baseSize)), 2.0);
				}
				else {
					downA = downAngle[n];
				}

				if (downA < (.5 * constantes<double>::PI)) {
					downA = std::pow(std::sin(downA), 2);
					bL *= downA;
				}

				bL *= 0.33;
				v *= (bD + bL);

				auto bv = dls::math::vec2d(b[0], -b[1]);
				auto cv = v - bv;
				auto a = std::atan2(cv[0], cv[1]);
				// rot_a.append(a)
				/*
				// add fill points at top  #experimental
				fillHeight = 1 - degrees(rotateV[3]) // 0.8
				if fillHeight < 1 {
					w = (p[0].offset - fillHeight) / (1- fillHeight)
					prob_b = random() < w
				else {
					prob_b = false

				if (p[0].offset > fillHeight) { // prob_b and (len(p) > 1) {  ##(p[0].offset > fillHeight) and
					childP.append(p[randint(0, len(p)-1)])
					rot_a.append(bRotate)// + pi)
				*/
				childP.pousse(p[idx]);
				rot_a.pousse(a);

			}
			else {
				// À FAIRE
				auto idx = 0ul; //randint(0, p.taille() - 1);
				childP.pousse(p[idx]);
			}
			// childP.append(p[idx])

			for (auto const &p  { childP_L) {
				childP.pousse(p);
				rot_a.pousse(0);
			}

			oldRotate = 0;
		}
	}

	auto i = 0ul;
	for (auto const &p : childP) {
		// Add a spline and set the coordinate of the first point.
		auto newSpline = new Spline{}; // 'BEZIER'
		cu.splines.pousse(newSpline);
		cu.resolution_u = resU;

		auto newPoint = newSpline->bezier_points.back();
		newPoint->co = p.co;

		auto tempPos = zAxis;
		// If the -ve flag for downAngle is used we need a special formula to find it
		auto downV = 0.0;

		if (useOldDownAngle) {
			if (downAngleV[n] < 0.0) {
				downV = downAngleV[n] * (1 - 2 * (.2 + .8 * ((1 - p.offset) / (1 - baseSize))));
			}
			// Otherwise just find a random value
			else {
				downV = 0.0; // À FAIRE uniform(-downAngleV[n], downAngleV[n]);
			}
		}
		else {
			if (downAngleV[n] < 0.0) {
				downV = 0.0; // À FAIRE uniform(-downAngleV[n], downAngleV[n]);
			}
			else {
				downV = -downAngleV[n] * std::pow((1 - (1 - p.offset) / (1 - baseSize)), 2);//  // (110, 80) = (60, -50);
			}
		}

		auto downRotMat = dls::math::mat3x3f{};

		if (p.offset == 1) {
			// À FAIRE downRotMat = Matrix.Rotation(0, 3, 'X');
		}
		else {
			// À FAIRE downRotMat = Matrix.Rotation(downAngle[n] + downV, 3, 'X');
		}

		// If the -ve flag for rotate is used we need to find which side of the stem
		// the last child point was and then grow in the opposite direction
		if (rotate[n] < 0.0) {
			oldRotate = -copysign(rotate[n], oldRotate);
		}
		// Otherwise just generate a random number in the specified range
		else {
			oldRotate += rotate[n];
		}
		auto bRotate = oldRotate; // À FAIRE + uniform(-rotateV[n], rotateV[n])

		if ((n == 1) && (rMode == "rotate")) {
			bRotate = rot_a[i];
		}

		auto rotMat = dls::math::mat3x3f{};// À FAIRE  Matrix.Rotation(bRotate, 3, 'Z');

		// Rotate the direction of growth and set the new point coordinates
		tempPos = downRotMat * tempPos;//tempPos.rotate(downRotMat);
		tempPos = rotMat * tempPos;//tempPos.rotate(rotMat);

		// use quat angle
		if ((rMode == "rotate") and (n == 1) and (p.offset != 1)) {
			if (useParentAngle){
				auto edir = p.quat.to_euler('XYZ', Euler((0, 0, bRotate), 'XYZ'));
				edir[0] = 0;
				edir[1] = 0;

				edir[2] = -edir[2];
				tempPos.rotate(edir);

				dec = declination(p.quat);
				tempPos.rotate(Matrix.Rotation(radians(dec), 3, 'X'));

				edir[2] = -edir[2];
				tempPos.rotate(edir);
			}
		}
		else {
			tempPos.rotate(p.quat);
		}

		newPoint->handle_right = p.co + tempPos;

		// Make length variation inversely proportional to segSplits
		// lenV = (1 - min(segSplits[n], 1)) * lengthV[n]

		// Find branch length and the number of child stems.
		auto maxbL = scaleVal;
		for (auto l = 0; l < n + 1; ++l) {
			maxbL *= length[l];
		}

		auto lMax = length[n]; //  * uniform(1 - lenV, 1 + lenV);

		auto lShape = 0.0;

		if (n == 1) {
			lShape = shapeRatio(shape, (1 - p.stemOffset) / (1 - baseSize), 0.0, 0.0, 0.0, customShape);
		}
		else {
			lShape = shapeRatio(shapeS, (1 - p.stemOffset) / (1 - baseSize));
		}

		auto branchL = p.lengthPar * lMax * lShape;
		auto childStems = branches[std::min(3ul, n + 1)] * (0.1 + 0.9 * (branchL / maxbL));

		// If this is the last level before leaves then we need to generate the child points differently
		if (storeN == levels - 1){
			if (leaves < 0){
				childStems = false;
			}
			else {
				childStems = leaves * (0.1 + 0.9 * (branchL / maxbL)) * shapeRatio(leafDist, (1 - p.offset));
			}
		}

		// print("n=%d, levels=%d, n'=%d, childStems=%s"%(n, levels, storeN, childStems))

		// Determine the starting and ending radii of the stem using the tapering of the stem
		auto startRad = std::min(p.radiusPar[0] * (std::pow((branchL / p.lengthPar), ratioPower) * radiusTweak[n]), p.radiusPar[1]);

		if (p.offset == 1 ) {
			startRad = p.radiusPar[1];
		}

		auto endRad = std::pow(startRad * (1 - taper[n]), ratioPower);
		startRad = std::max(startRad, minRadius);
		endRad = std::max(endRad, minRadius);
		newPoint->radius = static_cast<float>(startRad);

		// stem curvature
		auto curveVal = curve[n] / curveRes[n];
		auto curveVar = curveV[n] / curveRes[n];

		// curveVal = curveVal * (branchL / scaleVal)

		// Add the new stem to list of stems to grow and define which bone it will be parented to
		auto stem =
				stemSpline(
					newSpline, curveVal, curveVar, attractUp[n],
					0, curveRes[n], branchL / curveRes[n], childStems,
					startRad, endRad, cu.splines.taille() - 1, 0, p.quat
					);

		addstem.pousse(stem);

		//  auto bone = roundBone(p.parBone, boneStep[n - 1]);
		//isend == (p.offset == 1);

		//addsplinetobone((bone, isend))
				i++;
	}
}

void perform_pruning(
		double baseSize,
		double baseSplits,
		double childP,
		Curve &cu,
		double currentMax,
		double currentMin,
		double currentScale,
		double curve,
		double curveBack,
		dls::tableau<int> &curveRes,
		bool deleteSpline,
		bool forceSprout,
		double handles,
		size_t n,
		double oldMax,
		double orginalSplineToBone,
		dls::math::vec3f const &originalCo,
		double originalCurv,
		double originalCurvV,
		dls::math::vec3f const &originalHandleL,
		dls::math::vec3f const &originalHandleR,
		double originalLength,
		double originalSeg,
		bool prune,
		double prunePowerHigh,
		double prunePowerLow,
		double pruneRatio,
		double pruneWidth,
		double pruneBase,
		double pruneWidthPeak,
		double randState,
		double ratio,
		double scaleVal,
		dls::tableau<double> &segSplits,
		double splineToBone,
		double splitAngle,
		double splitAngleV,
		stemSpline &st,
		bool startPrune,
		double branchDist,
		dls::tableau<double> &length,
		double splitByLen,
		double closeTip,
		double nrings,
		double splitBias,
		double splitHeight,
		double attractOut,
		double rMode,
		dls::tableau<double> &lengthV,
		double taperCrown,
		double boneStep,
		double rotate,
		double rotateV)
{
	dls::tableau<stemSpline> splineList;

	while (startPrune && ((currentMax - currentMin) > 0.005)) {
		// À FAIRE setstate(randState)

		// If the search will halt after this iteration, then set the adjustment of stem
		// length to take into account the pruning ratio
		if ((currentMax - currentMin) < 0.01) {
			currentScale = (currentScale - 1) * pruneRatio + 1;
			startPrune = false;
			forceSprout = true;
		}

		// Change the segment length of the stem by applying some scaling
		st.segL = originalLength * currentScale;
		// To prevent millions of splines being created we delete any old ones and
		// replace them with only their first points to begin the spline again
		if (deleteSpline) {
			for (auto &x : splineList) {
				// À FAIRE cu.splines.remove(x->spline);
			}

			auto newSpline = new Spline; //cu.splines.new('BEZIER');
			cu.splines.pousse(newSpline);

			auto newPoint = newSpline->bezier_points.back();
			newPoint->co = originalCo;
			newPoint->handle_right = originalHandleR;
			newPoint->handle_left = originalHandleL;
			newPoint->handle_left_type = "VECTOR";
			newPoint->handle_right_type = "VECTOR";

			st.spline = newSpline;
			st.curv = originalCurv;
			st.curvV = originalCurvV;
			st.seg = originalSeg;
			st.p = newPoint;
			newPoint->radius = st.radS;
			splineToBone = orginalSplineToBone;
		}

		// Initialise the spline list for those contained in the current level of branching
		splineList = dls::tableau<stemSpline>{};
		splineList.pousse(st);

		// split length variation
		auto stemsegL = splineList[0].segL;  // initial segment length used for variation
		splineList[0].segL = stemsegL;// À FAIRE * uniform(1 - lengthV[n], 1 + lengthV[n]);  // variation for first stem

		// For each of the segments of the stem which must be grown we have to add to each spline in splineList
		for (auto k = 0; k < curveRes[n]; ++k) {
			// Make a copy of the current list to avoid continually adding to the list we're iterating over
			auto tempList = splineList;
			// print('Leng { ', len(tempList))

			// for curve variation
			auto kp = (curveRes[n] > 1) ? (k / (curveRes[n] - 1)) /* * 2 */ : 1.0;

			// split bias
			auto splitValue = segSplits[n];

			if (n == 0) {
				splitValue = ((2 * splitBias) * (kp - .5) + 1) * splitValue;
				splitValue = std::max(splitValue, 0.0);
			}

			// For each of the splines in this list set the number of splits and then grow it
			for (auto spl : tempList) {
				// adjust numSplit
				auto lastsplit = getattr(spl, 'splitlast', 0);
				auto splitVal = splitValue;

				if (lastsplit == 0) {
					splitVal = splitValue * 1.33;
				}
				else if (lastsplit == 1) {
					splitVal = splitValue * splitValue;
				}

				auto numSplit = 0;

				if (k == 0) {
					numSplit = 0;
				}
				else if ((n == 0) && (k < ((curveRes[n] - 1) * splitHeight)) && (k != 1)) {
					numSplit = 0;
				}
				else if ((k == 1) && (n == 0)) {
					numSplit = baseSplits;
				}
				// allways split at splitHeight
				else if ((n == 0) && (k == int((curveRes[n] - 1) * splitHeight) + 1) && (splitVal > 0)) {
					numSplit = 1;
				}
				else {
					if ((n >= 1) && splitByLen) {
						auto L = ((spl.segL * curveRes[n]) / scaleVal);
						auto lf = 1;

						for (auto l = 0ul; l < n + 1; ++l) {
							lf *= length[l];
						}
						L = L / lf;
						numSplit = splits2(splitVal * L);
					}
					else {
						numSplit = splits2(splitVal);
					}
				}

				if (k == int(curveRes[n] / 2 + 0.5)) and (curveBack[n] != 0) {
					spl.curv += 2 * (curveBack[n] / curveRes[n]);  // was -4 *
				}

				growSpline(
						n, spl, numSplit, splitAngle[n], splitAngleV[n], splineList,
						handles, splineToBone, closeTip, kp, splitHeight, attractOut[n],
						stemsegL, lengthV[n], taperCrown, boneStep, rotate, rotateV
						);

		// If pruning is enabled then we must to the check to see if the end of the spline is within the evelope
		if (prune) {
			// Check each endpoint to see if it is inside
			for (auto &s : splineList) {
				coordMag = (s.spline.bezier_points[-1].co.xy).length;
				ratio = (scaleVal - s.spline.bezier_points[-1].co.z) / (scaleVal * max(1 - pruneBase, 1e-6));
				// Don't think this if part is needed
				if (n == 0) and (s.spline.bezier_points[-1].co.z < pruneBase * scaleVal) {
					insideBool = true;  // Init to avoid UnboundLocalError later
				}
				else {
					insideBool = (
					(coordMag / scaleVal) < pruneWidth * shapeRatio(9, ratio, pruneWidthPeak, prunePowerHigh,
																	prunePowerLow));
				}
				// If the point is not inside then we adjust the scale and current search bounds
				if (!insideBool) {
					oldMax = currentMax;
					currentMax = currentScale;
					currentScale = 0.5 * (currentMax + currentMin);
					break;
				}
			}
			// If the scale is the original size and the point is inside then
			// we need to make sure it won't be pruned or extended to the edge of the envelope
			if (insideBool and (currentScale != 1)) {
				currentMin = currentScale;
				currentMax = oldMax;
				currentScale = 0.5 * (currentMax + currentMin);
			}

			if (insideBool and ((currentMax - currentMin) == 1)) {
				currentMin = 1;
			}

		// If the search will halt on the next iteration then we need
		// to make sure we sprout child points to grow the next splines or leaves
		if ((((currentMax - currentMin) < 0.005) || !prune) || forceSprout) {
			if (n == 0) and (rMode != "original") {
				tVals = findChildPoints2(splineList, st.children);
			}
				else {
				tVals = findChildPoints(splineList, st.children);
			}

			// print("debug tvals[%d] , splineList[%d], %s" % ( len(tVals), len(splineList), st.children))
			// If leaves is -ve then we need to make sure the only point which sprouts is the end of the spline
			if not st.children {
				tVals = [1.0];
			}

			// remove some of the points because of baseSize
			trimNum = int(baseSize * (len(tVals) + 1));
			//tVals = tVals[trimNum:];

			// grow branches in rings
			if ((n == 0) && (nrings > 0)) {
				// tVals = [(floor(t * nrings)) / nrings for t in tVals[ {-1]]
				tVals = [(floor(t * nrings) / nrings) * uniform(.995, 1.005) for t in tVals[:-1]];
				tVals.append(1);
				tVals = [t for t in tVals if t > baseSize];
			}

			// branch distribution
			if n == 0 {
				tVals = [((t - baseSize) / (1 - baseSize)) for t in tVals];
				if branchDist < 1.0 {
					tVals = [t ** (1 / branchDist) for t in tVals];
				}
				else {
					tVals = [1 - (1 - t) ** branchDist for t in tVals];
				}

				tVals = [t * (1 - baseSize) + baseSize for t in tVals];
			}

			// For all the splines, we interpolate them and add the new points to the list of child points
			maxOffset = max([s.offsetLen + (len(s.spline.bezier_points) - 1) * s.segL for s in splineList]);
			for s in splineList {
				// print(str(n)+'level { ', s.segMax*s.segL)
				childP.extend(interpStem(s, tVals, s.segMax * s.segL, s.radS, maxOffset, baseSize));
			}

		// Force the splines to be deleted
		deleteSpline = true;
		// If pruning isn't enabled then make sure it doesn't loop
		if (!prune) {
			startPrune = false;
		}
	}
	return ratio, splineToBone
}

class OperatriceCreationArbre final  { public OperatriceCorps {
public {
	static constexpr auto NOM = "Création Arbre";
	static constexpr auto AIDE = "Crée un arbre.";

	OperatriceCreationArbre(Graphe &graphe_parent, Noeud &noeud_)
		 { OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(0);
		sorties(1);
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_creation_arbre.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	void params_depuis_entreface(Parametres *parametres)
	{
		parametres->Shape = evalue_entier("Shape");
		parametres->BaseSize = evalue_decimal("BaseSize");
		parametres->Scale = evalue_decimal("Scale");
		parametres->ScaleV = evalue_decimal("ScaleV");
		parametres->ZScale = evalue_decimal("ZScale");
		parametres->ZScaleV = evalue_decimal("ZScaleV");
		parametres->Levels = evalue_entier("Levels");
		parametres->Ratio = evalue_decimal("Ratio");
		parametres->RatioPower = evalue_decimal("RatioPower");
		parametres->Lobes = evalue_entier("Lobes");
		parametres->LobeDepth = evalue_decimal("LobeDepth");
		parametres->Flare = evalue_decimal("Flare");

		parametres->_0Scale = evalue_decimal("_0Scale");
		parametres->_0ScaleV = evalue_decimal("_0ScaleV");
		parametres->_0Length = evalue_decimal("_0Length");
		parametres->_0LengthV = evalue_decimal("_0LengthV");
		parametres->_0BaseSplits = evalue_decimal("_0BaseSplits");
		parametres->_0SegSplits = evalue_decimal("_0SegSplits");
		parametres->_0SplitAngle = evalue_decimal("_0SplitAngle");
		parametres->_0SplitAngleV = evalue_decimal("_0SplitAngleV");
		parametres->_0CurveRes = evalue_decimal("_0CurveRes");
		parametres->_0Curve = evalue_decimal("_0Curve");
		parametres->_0CurveBack = evalue_decimal("_0CurveBack");
		parametres->_0CurveV = evalue_decimal("_0CurveV");

		parametres->_1DownAngle = evalue_decimal("_1DownAngle");
		parametres->_1DownAngleV = evalue_decimal("_1DownAngleV");
		parametres->_1Rotate = evalue_decimal("_1Rotate");
		parametres->_1RotateV = evalue_decimal("_1RotateV");
		parametres->_1Branches = evalue_decimal("_1Branches");
		parametres->_1Length = evalue_decimal("_1Length");
		parametres->_1LengthV = evalue_decimal("_1LengthV");
		parametres->_1Taper = evalue_decimal("_1Taper");
		parametres->_1SegSplits = evalue_decimal("_1SegSplits");
		parametres->_1SplitAngle = evalue_decimal("_1SplitAngle");
		parametres->_1SplitAngleV = evalue_decimal("_1SplitAngleV");
		parametres->_1CurveRes = evalue_decimal("_1CurveRes");
		parametres->_1Curve = evalue_decimal("_1Curve");
		parametres->_1CurveBack = evalue_decimal("_1CurveBack");
		parametres->_1CurveV = evalue_decimal("_1CurveV");

		parametres->_2DownAngle = evalue_decimal("_2DownAngle");
		parametres->_2DownAngleV = evalue_decimal("_2DownAngleV");
		parametres->_2Rotate = evalue_decimal("_2Rotate");
		parametres->_2RotateV = evalue_decimal("_2RotateV");
		parametres->_2Branches = evalue_decimal("_2Branches");
		parametres->_2Length = evalue_decimal("_2Length");
		parametres->_2LengthV = evalue_decimal("_2LengthV");
		parametres->_2Taper = evalue_decimal("_2Taper");
		parametres->_2SegSplits = evalue_decimal("_2SegSplits");
		parametres->_2SplitAngle = evalue_decimal("_2SplitAngle");
		parametres->_2SplitAngleV = evalue_decimal("_2SplitAngleV");
		parametres->_2CurveRes = evalue_decimal("_2CurveRes");
		parametres->_2Curve = evalue_decimal("_2Curve");
		parametres->_2CurveBack = evalue_decimal("_2CurveBack");
		parametres->_2CurveV = evalue_decimal("_2CurveV");

		parametres->_3DownAngle = evalue_decimal("_3DownAngle");
		parametres->_3DownAngleV = evalue_decimal("_3DownAngleV");
		parametres->_3Rotate = evalue_decimal("_3Rotate");
		parametres->_3RotateV = evalue_decimal("_3RotateV");
		parametres->_3Branches = evalue_decimal("_3Branches");
		parametres->_3Length = evalue_decimal("_3Length");
		parametres->_3LengthV = evalue_decimal("_3LengthV");
		parametres->_3Taper = evalue_decimal("_3Taper");
		parametres->_3SegSplits = evalue_decimal("_3SegSplits");
		parametres->_3SplitAngle = evalue_decimal("_3SplitAngle");
		parametres->_3SplitAngleV = evalue_decimal("_3SplitAngleV");
		parametres->_3CurveRes = evalue_decimal("_3CurveRes");
		parametres->_3Curve = evalue_decimal("_3Curve");
		parametres->_3CurveBack = evalue_decimal("_3CurveBack");
		parametres->_3CurveV = evalue_decimal("_3CurveV");

		parametres->Leaves = evalue_decimal("Leaves");
		parametres->LeafShape = evalue_decimal("LeafShape");
		parametres->LeafScale = evalue_decimal("LeafScale");
		parametres->LeafScaleX = evalue_decimal("LeafScaleX");
		parametres->AttractionUp = evalue_decimal("AttractionUp");
		parametres->PruneRatio = evalue_decimal("PruneRatio");
		parametres->PruneWidth = evalue_decimal("PruneWidth");
		parametres->PruneWidthPeak = evalue_decimal("PruneWidthPeak");
		parametres->PrunePowerLow = evalue_decimal("PrunePowerLow");
		parametres->PrunePowerHigh = evalue_decimal("PrunePowerHigh");
	}

	int execute(Rectangle const &rectangle, const int temps) override
	{
		INUTILISE(rectangle);
		INUTILISE(temps);

		m_corps.reinitialise();

		Parametres params;
		parametres_tremble(&params);

		/* Tronc */
#if 0
		auto taille_tronc = params.Scale * params._0Scale;
		auto taille_segment = taille_tronc / params._0CurveRes;

		auto origine = dls::math::vec3f(0.0f, 0.0f, 0.0f);

		for (int i = 0; i < static_cast<int>(params._0CurveRes); ++i) {
			m_corps.ajoute_point(origine.x, origine.y, origine.z);
			origine.y += taille_segment;
		}

		auto poly = m_corps.ajoute_polygone(type_polygone::OUVERT, static_cast<int>(params._0CurveRes));

		for (int i = 0; i < static_cast<int>(params._0CurveRes); ++i) {
			 m_corps.ajoute_sommet(poly, i);
		}
#else
		auto closeTip = true;
		auto levelCount = dls::tableau<size_t>{};
		auto cu = Curve{};
		auto baseSize = 1.0f;
		auto baseSize_s = 1.0f;
		auto levels = params.Levels;

		for (auto n = 0; n < params.Levels; ++n) {
			auto storeN = n;
			auto stemList = dls::tableau<stemSpline>{};
			// If n is used as an index to access parameters for the tree
			// it must be at most 3 or it will reference outside the array index
			n = std::min(3, n);
			auto splitError = 0.0;

			// closeTip only on last level
			auto closeTipp = closeTip && (n == params.Levels - 1);

			// If this is the first level of growth (the trunk) then we need some special work to begin the tree
			if (n == 0) {
				kickstart_trunk(stemList, levels, leaves, branches, cu, curve, curveRes,
								curveV, attractUp, length, lengthV, ratio, ratioPower, resU,
								scale0, scaleV0, scaleVal, taper, minRadius, rootFlare);
			}
			// If this isn't the trunk then we may have multiple stem to intialise
			else {
				// For each of the points defined in the list of stem starting points we need to grow a stem.
				fabricate_stems(addsplinetobone, stemList, baseSize, branches, childP, cu, curve, curveBack,
								curveRes, curveV, attractUp, downAngle, downAngleV, leafDist, leaves, length, lengthV,
								levels, n, ratioPower, resU, rotate, rotateV, scaleVal, shape, storeN,
								taper, shapeS, minRadius, radiusTweak, customShape, rMode, segSplits,
								useOldDownAngle, useParentAngle, boneStep);
			}

			// change base size for each level
			if (n > 0) {
				baseSize *= baseSize_s ; // decrease at each level;
			}
			if (n == levels - 1) {
				baseSize = 0;
			}

			auto childP = dls::tableau<int>{};
			// Now grow each of the stems in the list of those to be extended
			for (auto &st : stemList) {
				// When using pruning, we need to ensure that the random effects
				// will be the same for each iteration to make sure the problem is linear
				auto randState = 1.0f;// getstate();
				auto startPrune = true;
				auto lengthTest = 0.0;
				// Store all the original values for the stem to make sure
				// we have access after it has been modified by pruning
				auto originalLength = st.segL;
				auto originalCurv = st.curv;
				auto originalCurvV = st.curvV;
				auto originalSeg = st.seg;
				auto originalHandleR = st.p->handle_right;
				auto originalHandleL = st.p->handle_left;
				auto originalCo = st.p->co;
				auto currentMax = 1.0;
				auto currentMin = 0.0;
				auto currentScale = 1.0;
				auto oldMax = 1.0;
				auto deleteSpline = false;
				//auto orginalSplineToBone = copy.copy(splineToBone)
				auto forceSprout = false;
				// Now do the iterative pruning, this uses a binary search and halts once the difference
				// between upper and lower bounds of the search are less than 0.005
				ratio, splineToBone = perform_pruning(
							baseSize, baseSplits, childP, cu, currentMax, currentMin,
							currentScale, curve, curveBack, curveRes, deleteSpline, forceSprout,
							handles, n, oldMax, orginalSplineToBone, originalCo, originalCurv,
							originalCurvV, originalHandleL, originalHandleR, originalLength,
							originalSeg, prune, prunePowerHigh, prunePowerLow, pruneRatio,
							pruneWidth, pruneBase, pruneWidthPeak, randState, ratio, scaleVal,
							segSplits, splineToBone, splitAngle, splitAngleV, st, startPrune,
							branchDist, length, splitByLen, closeTipp, nrings, splitBias,
							splitHeight, attractOut, rMode, lengthV, taperCrown, boneStep,
							rotate, rotateV
							);

				levelCount.pousse(cu.splines.taille());
			}
		}
#endif

		return res_exec::REUSSIE;
	}
};
#endif

/* ************************************************************************** */

void enregistre_operatrices_arbre(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceCreationArbre>());
	usine.enregistre_type(cree_desc<OpAjouteBranch>());
}

#pragma clang diagnostic pop
