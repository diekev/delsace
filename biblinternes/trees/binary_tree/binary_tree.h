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

template <typename T>
struct Node {
	using value_type = T;
	using Ptr = std::unique_ptr<Node>;

	Node::Ptr left{nullptr};
	Node::Ptr right{nullptr};
	T value{0};

	Node() = default;

	explicit Node(T v)
	    : Node()
	{
		value = v;
	}

	template <typename... Args>
	static Ptr create(Args &&... args)
	{
		return Ptr(new Node<T>(std::forward<Args>(args)...));
	}
};

template <typename NodeType>
class BinaryTree {
	using value_type = typename NodeType::value_type;
	using node_ptr = typename NodeType::Ptr;

	node_ptr m_root = nullptr;
	bool m_has_changed = false;
	int m_num_leaves = 0;
	value_type m_min_value = 0;
	value_type m_max_value = 0;

public:
	BinaryTree() = default;
	~BinaryTree() = default;

	void insert(const value_type &value)
	{
		m_has_changed = true;

		if (m_root == nullptr) {
			m_root = NodeType::create(value);
			return;
		}

		insert(value, m_root.get());
	}

	NodeType *search(const value_type &value) const
	{
		return search(value, m_root);
	}

	int leavesCount()
	{
		if (m_has_changed) {
			m_num_leaves = leavesCount(m_root);
			m_has_changed = false;
		}

		return m_num_leaves;
	}

	value_type minValue() const
	{
		return minValue(m_root);
	}

	value_type maxValue() const
	{
		return maxValue(m_root);
	}

	template <typename OpType>
	void traverse(OpType &op)
	{
		traverse(m_root.get(), op);
	}

	void insertBST(value_type value)
	{
		if (m_root == nullptr) {
			m_root = NodeType::create(value);
			return;
		}

		insertBST(*m_root, value);
	}

	bool is_heap() const
	{
		return is_heap(m_root);
	}

	bool is_search_tree() const
	{
		return is_search_tree(m_root);
	}

private:
	int leavesCount(NodeType *node) const
	{
		if (node == nullptr)
			return 0;

		if (node->right == nullptr && node->left == nullptr)
			return 1;

		int left_count = leavesCount(node->left);
		int right_count = leavesCount(node->right);
		return left_count + right_count;
	}

	/* the maximum value is always in the rightmost tree */
	value_type maxValue(NodeType *node) const
	{
		if (node == nullptr) {
			return value_type(0);
		}

		if (node->right == nullptr) {
			return node->value;
		}

		return maxValue(node->right);
	}

	/* the minimum value is always in the leftmost tree */
	value_type minValue(NodeType *node) const
	{
		if (node == nullptr) {
			return value_type(0);
		}

		if (node->left == nullptr) {
			return node->value;
		}

		return minValue(node->left);
	}

	void insert(const value_type &value, NodeType *leaf)
	{
		if (value < leaf->value) {
			if (leaf->left != nullptr) {
				insert(value, leaf->left.get());
			}
			else {
				leaf->left = NodeType::create(value);
			}
		}
		else if (value >= leaf->value) {
			if (leaf->right != nullptr) {
				insert(value, leaf->right.get());
			}
			else {
				leaf->right = NodeType::create(value);
			}
		}
	}

	NodeType *search(const value_type &value, NodeType *leaf) const
	{
		if (leaf != nullptr) {
			if (value == leaf->value) {
				return leaf;
			}

			if (value < leaf->value) {
				return search(value, leaf->left);
			}
			else {
				return search(value, leaf->right);
			}
		}

		return nullptr;
	}

	template <typename OpType>
	void traverse(NodeType *node, OpType &op)
	{
		if (node == nullptr) {
			return;
		}

		if (node->left != nullptr)
			op(node->left.get());


		if (node->right != nullptr)
			op(node->right.get());

		traverse(node->left.get(), op);
		traverse(node->right.get(), op);
	}

	void insert(node_ptr &node, int value)
	{
		if (node == nullptr) {
			node = NodeType::create(value);
			return;
		}

		if (value < node->value) {
			insert(node->left, value);
		}
		else {
			insert(node->right, value);
		}
	}

	bool is_heap(const node_ptr &node) const
	{
		if (node == nullptr) {
			return true;
		}

		bool is_heap_ = false;

		if (node->left != nullptr) {
			is_heap_ = node->value > node->left->value;
		}

		if (node->right != nullptr) {
			is_heap_ &= node->value > node->right->value;
		}

		if (!is_heap_) {
			return false;
		}

		is_heap_ &= is_heap(node->left);
		is_heap_ &= is_heap(node->right);

		return is_heap_;
	}

	bool is_search_tree(const node_ptr &node) const
	{
		if (node == nullptr) {
			return true;
		}

		bool is_bsearch = false;

		if (node->left != nullptr) {
			is_bsearch = node->value > node->left->value;
		}

		if (node->right != nullptr) {
			is_bsearch &= node->value < node->right->value;
		}

		if (!is_bsearch) {
			return false;
		}

		is_bsearch &= is_search_tree(node->left);
		is_bsearch &= is_search_tree(node->right);

		return is_bsearch;
	}

	void insertBST(NodeType &node, int value)
	{
		if (node.value < value) {
			if (node.left == nullptr) {
				node.left = NodeType::create(value);
			}
			else {
				insertBST(*node.left, value);
			}
		}
		else if (node.value > value) {
			if (node.right == nullptr) {
				node.right = NodeType::create(value);
			}
			else {
				insertBST(*node.right, value);
			}
		}
		else {
			auto n = NodeType::create(value);
			if (node.left == nullptr) {
				node.left = NodeType::create(value);
			}
			else {

			}
		}
	}
};

template <typename __type_cle, typename __type_valeur>
class arbre_binaire {
	struct noeud {
		__type_cle cle = static_cast<__type_cle>(0);
		__type_valeur valeur = static_cast<__type_valeur>(0);

		noeud *gauche = nullptr;
		noeud *droit = nullptr;

		noeud() = default;

		noeud(__type_cle cle_, __type_valeur valeur_)
			: cle(cle_)
			, valeur(valeur_)
		{}

		noeud(const noeud &cote_droit) = default;
		noeud &operator=(const noeud &cote_droit) = default;

		~noeud()
		{
			delete gauche;
			delete droit;
		}
	};

	noeud *m_racine = nullptr;

public:
	arbre_binaire() = default;

	~arbre_binaire()
	{
		delete m_racine;
	}

	arbre_binaire(const arbre_binaire &cote_droit) = default;
	arbre_binaire &operator=(const arbre_binaire &cote_droit) = default;

	void insere(__type_cle cle, __type_valeur valeur)
	{
		return insere_ex(m_racine, cle, valeur);
	}

	size_t taille() const
	{
		return taille_ex(m_racine);
	}

	__type_valeur valeur(__type_cle cle) const
	{
		return valeur_ex(m_racine, cle);
	}

private:
	void insere_ex(noeud *&racine, __type_cle cle, __type_valeur valeur)
	{
		if (racine == nullptr) {
			racine = new noeud(cle, valeur);
			return;
		}

		if (racine->cle > cle) {
			insere_ex(racine->gauche, cle, valeur);
		}
		else if (racine->cle < cle) {
			insere_ex(racine->droit, cle, valeur);
		}
		else {
			racine->cle = cle;
		}
	}

	size_t taille_ex(const noeud *racine) const
	{
		if (racine == nullptr) {
			return 0;
		}

		if (racine->gauche == nullptr && racine->droit == nullptr) {
			return 1;
		}

		auto taille_gauche = taille_ex(racine->gauche);
		auto taille_droit = taille_ex(racine->droit);

		return 1 + taille_gauche + taille_droit;
	}

	__type_valeur valeur_ex(const noeud *racine, __type_cle cle) const
	{
		if (racine == nullptr) {
			return static_cast<__type_valeur>(0);
		}

		if (racine->cle == cle) {
			return racine->valeur;
		}

		if (racine->gauche != nullptr && racine->droit != nullptr) {
			if (racine->gauche->cle < cle && cle < racine->gauche->cle) {
				return static_cast<__type_valeur>(0);
			}
		}

		return static_cast<__type_valeur>(0);
	}
};
