// Ariel: FLIP Fluid Simulator
// Written by Yining Karl Li
//
// File: bvh.hpp. Adapted from Takua Render.
// Spatial BVH acceleration structure

#ifndef BVH_HPP
#define BVH_HPP

#ifdef __CUDACC__
#define HOST __host__
#define DEVICE __device__
#define SHARED __shared__
#else
#define HOST
#define DEVICE
#define SHARED
#endif

#include "aabb.hpp"
#include "spatial.hpp"
#include "../ray/ray.hpp"
#include "../utilities/utilities.h"
#include "../utilities/datastructures.hpp"

enum Axis{
	axis_x,
	axis_y,
	axis_z
};

namespace spaceCore {

//====================================
// Struct Declarations
//====================================

struct BvhNode {
	Aabb m_bounds{};
	unsigned int m_referenceOffset{}; //offset into bvh-wide reference index
	unsigned int m_numberOfReferences{};
	unsigned int m_left{};
	unsigned int m_right{};
	unsigned int m_nodeid{};

	BvhNode() = default;

	bool IsLeaf()
	{
		return (m_left == 0 && m_right == 0);
	}

	float FastIntersectionTest(const rayCore::Ray &r)
	{
		return m_bounds.FastIntersectionTest(r);
	}
};

//====================================
// Class Declarations
//===================================

template <typename T>
class Bvh {
public:
	Bvh(T basegeom);
	Bvh() = default;
	~Bvh();

	void BuildBvh(const unsigned int &maxDepth);
	void Traverse(const rayCore::Ray &r, TraverseAccumulator &result);

	BvhNode*                    m_nodes = nullptr;
	unsigned int                m_numberOfNodes = 0;
	unsigned int*               m_referenceIndices = nullptr;
	unsigned int                m_numberOfReferenceIndices = 0;
	unsigned int                m_id = 0;
	unsigned int                m_depth = 0;
	T                           m_basegeom = 0;

	Bvh(Bvh const &autre) = default;
	Bvh &operator=(Bvh const &autre) = default;

private:
	float FindBestSplit(BvhNode &Node, dls::tableau<unsigned int> &references,
						const Axis &direction, const unsigned int &quality, Aabb* aabbs,
						unsigned int &leftCount, unsigned int &rightCount);

	Axis FindLongestAxis(Aabb const &box);

	double CalculateSplitCost(const float &split, const BvhNode &node,
							  dls::tableau<unsigned int> &references, const Axis &direction,
							  Aabb* aabbs, unsigned int &leftCount, unsigned int &rightCount);
};


template <typename T>
Bvh<T>::Bvh(T basegeom)
	: Bvh<T>()
{
	m_basegeom = basegeom;
}

template <typename T>
Bvh<T>::~Bvh()
{
	/*if (m_nodes!=nullptr) {
		delete [] m_nodes;
	}
	if (m_referenceIndices!=nullptr) {
		delete [] m_referenceIndices;
	}*/
}

template <typename T>
void Bvh<T>::Traverse(const rayCore::Ray &r, TraverseAccumulator &result)
{
	ShortStack<BvhNode*> stack;
	stack.Push(&m_nodes[1]);
	auto current = &m_nodes[1];
	//check for root node hit/miss
	auto distanceToCurrent = current->FastIntersectionTest(r);

	if (distanceToCurrent < -0.5f) {
		return;
	}

	distanceToCurrent = 10000000000000.0f;

	while (stack.Empty()==false) {
		//traverse and push back to stack until leaf node is reached
		auto empty = false;

		while (current->IsLeaf()==false && empty==false) {
			auto left = &m_nodes[current->m_left];
			auto right = &m_nodes[current->m_right];
			//find nearest and farthest child
			auto distanceToLeft = left->FastIntersectionTest(r);
			auto distanceToRight = right->FastIntersectionTest(r);
			//if ray intersects both children, push current node onto stack and set current
			//to nearest child
			if (distanceToLeft>-0.5f && distanceToRight>-0.5f) {
				stack.Push(current);

				if (distanceToLeft<distanceToRight) {
					current = left;
				}
				else {
					current = right;
				}
				//else if intersect only the left or right child
			}
			else if (distanceToLeft>-0.5f && distanceToRight<-0.5f) {
				current = left;
			}
			else if (distanceToRight>-0.5f && distanceToLeft<-0.5f) {
				current = right;
				//else if intersect neither children, abort loop and move onto next node in stack
			}
			else {
				empty = true;
			}
		}

		if (current->IsLeaf()) {
			for (unsigned int i=0; i<current->m_numberOfReferences; i++) {
				auto primID = m_referenceIndices[current->m_referenceOffset+i];
				auto rhit = m_basegeom.IntersectElement(primID, r);

				if (rhit.m_hit) {
					//make sure hit is actually in front of origin
					auto n = normalise(rhit.m_point - r.m_origin);
					auto degree = std::acos(produit_scalaire(n, r.m_direction));

					if (degree < (constantes<float>::PI / 2.0f) || degree != degree) {
						result.RecordIntersection(rhit, current->m_nodeid);
					}
				}
			}
		}

		//if stack is not empty, pop current node and pick farthest child
		if (stack.Empty()==false) {
			current = stack.Pop();
			auto left = &m_nodes[current->m_left];
			auto right = &m_nodes[current->m_right];
			//find nearest and farthest child
			auto distanceToLeft = left->FastIntersectionTest(r);
			auto distanceToRight = right->FastIntersectionTest(r);

			if (distanceToLeft>=distanceToRight) {
				current = left;
			}
			else {
				current = right;
			}
		}
	}
}

template <typename T>
void Bvh<T>::BuildBvh(const unsigned int &maxDepth)
{
	//assemble aabb list
	auto numberOfAabbs = m_basegeom.GetNumberOfElements();
	auto aabbs = new spaceCore::Aabb[numberOfAabbs];

	for (unsigned int i=0; i<numberOfAabbs; i++) {
		aabbs[i] = m_basegeom.GetElementAabb(i);
	}

	//create vectors to store nodes during construction
	dls::tableau<BvhNode> tree;
	tree.reserve(static_cast<long>(std::pow(2.0f, static_cast<float>(maxDepth))));

	auto nodeCount = 0u;
	auto layerCount = 0u;

	dls::tableau< dls::tableau<unsigned int> > currentRefs;
	currentRefs.reserve(static_cast<long>(std::pow(2.0f, static_cast<float>(maxDepth))));

	dls::tableau<BvhNode*> currentLayer;
	dls::tableau<unsigned int> finalRefList;
	//create null node to place in index 0. tree begins at index 1.
	tree.pousse(BvhNode());
	nodeCount++;
	//    BvhNode* nullnode = &tree[0];
	//create root node with aabb that is combined from all input aabbs
	tree.pousse(BvhNode());
	nodeCount++;
	auto root = &tree[1];

	for (unsigned int i=0; i<numberOfAabbs; i++) {
		root->m_bounds.ExpandAabb(aabbs[i].m_min, aabbs[i].m_max);
	}

	//populate root node with all ids
	root->m_numberOfReferences = numberOfAabbs;
	root->m_referenceOffset = 0;
	root->m_nodeid = 1;

	dls::tableau<unsigned int> rootreferences;
	rootreferences.reserve(numberOfAabbs);

	for (unsigned int i=0; i<numberOfAabbs; i++) {
		rootreferences.pousse(static_cast<unsigned>(aabbs[i].m_id));
	}

	currentLayer.pousse(root);
	currentRefs.pousse(rootreferences);

	//for each layer, split nodes and move on to next layer
	for (unsigned int layer=0; layer<maxDepth; layer++) {
		//set up temp storage buffers for next layer
		dls::tableau< dls::tableau<unsigned int> > nextLayerRefs;
		dls::tableau<BvhNode*> nextLayer;
		//for each node in current layer, split and push result
		auto numberOfNodesInLayer = currentLayer.taille();

		if (numberOfNodesInLayer > 0) {
			layerCount++;
		}

		for (unsigned int i=0; i<numberOfNodesInLayer; i++) {
			//if <20 prims or last layer, make node a leaf node
			auto refCount = static_cast<unsigned int>(currentRefs[i].taille());
			auto node = currentLayer[i];

			if (refCount <= 5 || layer==maxDepth-1) {
				node->m_left = 0;
				node->m_right = 0;
				node->m_numberOfReferences = refCount;
				node->m_referenceOffset = static_cast<unsigned int>(finalRefList.taille());

				for (unsigned int j=0; j<node->m_numberOfReferences; j++) {
					finalRefList.pousse(currentRefs[i][j]);
				}
			}
			else { //else, find the best split via SAH and add children to next layer
				//split axis should be the longest axis
				auto splitAxis = FindLongestAxis(node->m_bounds);
				//call SAH and get best split candidate
				auto leftCount = 0u;
				auto rightCount = 0u;
				auto bestSplit = FindBestSplit(*node, currentRefs[i], splitAxis, 10, aabbs,
											   leftCount, rightCount);
				if (leftCount==0 || rightCount==0) {
					leftCount = 0;
					rightCount = 0;

					if (splitAxis==axis_x) {
						splitAxis = axis_y;
					}
					else if (splitAxis==axis_y) {
						splitAxis = axis_z;
					}
					else if (splitAxis==axis_z) {
						splitAxis = axis_x;
					}

					bestSplit = FindBestSplit(*node, currentRefs[i], splitAxis, 10, aabbs,
											  leftCount, rightCount);
				}

				if (leftCount==0 || rightCount==0) {
					leftCount = 0;
					rightCount = 0;

					if (splitAxis==axis_x) {
						splitAxis = axis_y;
					}
					else if (splitAxis==axis_y) {
						splitAxis = axis_z;
					}
					else if (splitAxis==axis_z) {
						splitAxis = axis_x;
					}

					bestSplit = FindBestSplit(*node, currentRefs[i], splitAxis, 10, aabbs,
											  leftCount, rightCount);
				}

				if (leftCount==0 || rightCount==0) {
					node->m_left = 0;
					node->m_right = 0;
					node->m_numberOfReferences = refCount;
					node->m_referenceOffset = static_cast<unsigned int>(finalRefList.taille());

					for (unsigned int j=0; j<node->m_numberOfReferences; j++) {
						finalRefList.pousse(currentRefs[i][j]);
					}
				}
				else {
					//create left and right nodes
					tree.pousse(BvhNode());
					nodeCount++;
					auto leftID = static_cast<unsigned int>(tree.taille()-1);
					auto left = &tree[leftID];
					left->m_nodeid = leftID;
					tree.pousse(BvhNode());
					nodeCount++;
					auto rightID = static_cast<unsigned int>(tree.taille() - 1);
					auto right = &tree[rightID];
					right->m_nodeid = rightID;
					node->m_left = leftID;
					node->m_right = rightID;
					//create lists of left and right objects based on centroid location
					dls::tableau<unsigned int> leftRefs;
					dls::tableau<unsigned int> rightRefs;
					leftRefs.reserve(leftCount);
					rightRefs.reserve(rightCount);
					left->m_numberOfReferences = leftCount;
					right->m_numberOfReferences = rightCount;

					for (unsigned int j=0; j<node->m_numberOfReferences; j++) {
						auto const referenceIndex = currentRefs[i][j];
						auto const &reference = aabbs[referenceIndex];

						if (reference.m_centroid[splitAxis]<=bestSplit) {
							leftRefs.pousse(referenceIndex);
							left->m_bounds.ExpandAabb(reference.m_min, reference.m_max);
						}
						else {
							rightRefs.pousse(referenceIndex);
							right->m_bounds.ExpandAabb(reference.m_min, reference.m_max);
						}
					}

					//add left and right children to next later
					nextLayer.pousse(left);
					nextLayerRefs.pousse(leftRefs);
					nextLayer.pousse(right);
					nextLayerRefs.pousse(rightRefs);
					//cleanup
					leftRefs.efface();
					rightRefs.efface();
				}
			}
		}
		//clear current layer's temp buffers, swap in next layer
		currentLayer.efface();

		for (auto &ref : currentRefs) {
			ref.efface();
		}

		currentRefs.efface();
		currentLayer = nextLayer;
		currentRefs = nextLayerRefs;
	}

	m_numberOfNodes = tree.taille();
	m_nodes = new BvhNode[m_numberOfNodes];
	copy(tree.debut(), tree.fin(), m_nodes);

	m_numberOfReferenceIndices = finalRefList.taille();
	m_referenceIndices = new unsigned int[m_numberOfReferenceIndices];
	copy(finalRefList.debut(), finalRefList.fin(), m_referenceIndices);

	delete [] aabbs;

	std::cout << "Built BVH with " << nodeCount << " nodes and depth " << layerCount << std::endl;
}

template <typename T>
float Bvh<T>::FindBestSplit(
		BvhNode &node,
		dls::tableau<unsigned int> &references,
		Axis const &direction,
		unsigned int const &quality,
		Aabb *aabbs,
		unsigned int &leftCount,
		unsigned int &rightCount)
{
	auto splitAxis = 0u;

	if (direction==axis_y) {
		splitAxis = 1;
	}
	else if (direction==axis_z) {
		splitAxis = 2;
	}

	//create N even candidates, where N = quality
	auto candidates = new float[quality];
	auto axisLength = node.m_bounds.m_max[splitAxis] - node.m_bounds.m_min[splitAxis];
	auto evenSplitSize = (axisLength) / static_cast<float>(quality + 1);

	for (unsigned int i=0; i<quality; i++) { //create even splits
		float candidate = node.m_bounds.m_min[splitAxis] + (i+1)*evenSplitSize;
		candidates[i] = candidate;
	}

	//Calculate all split costs and keep the lowest cost split
	auto left = 0u;
	auto right = 0u;
	auto currentLeft = 0u;
	auto currentRight = 0u;
	auto bestCost = CalculateSplitCost(candidates[0], node, references, direction, aabbs,
			left, right);
	auto bestSplit = candidates[0];

	for (unsigned int i=1; i<quality; i++) {
		auto cost = CalculateSplitCost(candidates[i], node, references, direction, aabbs,
									   currentLeft, currentRight);
		if (cost<bestCost) {
			bestCost = cost;
			bestSplit = candidates[i];
			left = currentLeft;
			right = currentRight;
		}
	}

	leftCount = left;
	rightCount = right;
	delete [] candidates;

	return bestSplit;
}

template <typename T>
double Bvh<T>::CalculateSplitCost(
		float const &split,
		BvhNode const &node,
		dls::tableau<unsigned int> &references,
		Axis const &direction,
		Aabb *aabbs,
		unsigned int &leftCount,
		unsigned int &rightCount)
{
	auto splitAxis = 0u;

	if (direction==axis_y) {
		splitAxis = 1;
	}
	else if (direction==axis_z) {
		splitAxis = 2;
	}

	auto leftBox = Aabb{};
	auto left = 0u;
	auto rightBox = Aabb{};
	auto right = 0u;

	//run through all references in current node and merge into either left or right
	for (unsigned int i=0; i<node.m_numberOfReferences; i++) {
		auto const referenceIndex = references[i];
		auto const &reference = aabbs[referenceIndex];

		if (reference.m_centroid[splitAxis] <= split) {
			left++;
			leftBox.ExpandAabb(reference.m_min, reference.m_max);
		}
		else {
			right++;
			rightBox.ExpandAabb(reference.m_min, reference.m_max);
		}
	}

	auto leftCost = left * leftBox.CalculateSurfaceArea();
	auto rightCost = right * rightBox.CalculateSurfaceArea();
	leftCount = left;
	rightCount = right;

	// std::cout << left << " "  << right << " " << fabs(leftCost-rightCost) << std::endl;
	if (left==0 || right==0) {
		return std::pow(10.0f, 100);
	}

	return std::fabs(leftCost - rightCost);
}

/* À FAIRE : fonction similaire dans dls::math */
template <typename T>
Axis Bvh<T>::FindLongestAxis(Aabb const &box)
{
	//find max axis and return. pretty simple.
	auto axisXlength = box.m_max.x - box.m_min.x;
	auto axisYlength = box.m_max.y - box.m_min.y;
	auto axisZlength = box.m_max.z - box.m_min.z;

	if (axisXlength>=axisYlength && axisXlength>=axisZlength) {
		return axis_x;
	}

	if (axisYlength>=axisXlength && axisYlength>=axisZlength) {
		return axis_y;
	}

	return axis_z;
}

}

#endif
