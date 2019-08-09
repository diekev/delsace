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

#include "arborescence.h"
#include "arborescence_pile.h"

#include "binary_tree/binary_tree.h"

#include <iostream>

/* ************************************************************************** */

static void test_heap_tree(std::ostream &os)
{
	BinaryTree<Node<int>> tree;
	tree.insert(1);
	tree.insert(9);
	tree.insert(8);
	tree.insert(71);
	tree.insert(5);
	tree.insert(89);
	tree.insert(13);

	if (tree.is_heap()) {
		os << "Binary tree is a heap.\n";
	}
	else {
		os << "Binary tree is not a heap.\n";
	}
}

/* ************************************************************************** */

static void test_bsearch_tree(std::ostream &os)
{
	BinaryTree<Node<int>> tree;
	tree.insert(1);
	tree.insert(9);
	tree.insert(8);
	tree.insert(71);
	tree.insert(5);
	tree.insert(89);
	tree.insert(13);

	if (tree.is_search_tree()) {
		os << "Binary tree is a binary search tree.\n";
	}
	else {
		os << "Binary tree is not a binary search tree.\n";
	}
}

/* ************************************************************************** */

struct MinMaxOp {
	float min, max;

	void operator()(Node<float> *node)
	{
		if (node->value < min) {
			min = node->value;
		}
		else if (node->value > max) {
			max = node->value;
		}
	}
};

static void test_binary_tree(std::ostream &os)
{
	typedef Node<float> NodeType;
	typedef BinaryTree<NodeType> BTreeType;

	BTreeType tree;
	tree.insert(5.0f);
	tree.insert(3.5f);
	tree.insert(7.7f);
	tree.insert(8.9f);

	MinMaxOp op;
	op.min = 100.0f;
	op.max = 0.0f;

	tree.traverse(op);

	os << "min value: " << op.min << '\n';
	os << "max value: " << op.max << '\n';
}

/* ************************************************************************** */

static void test_arbre_binaire(std::ostream &os)
{
	typedef arbre_binaire<int, float> arbre_cle;

	arbre_cle arbre;
	arbre.insere(0, 5.0f);
	arbre.insere(0, 3.5f);
	arbre.insere(250, 7.7f);
	arbre.insere(0, 8.9f);

	os << "Taille arbre : " << arbre.taille() << '\n';

	auto temps = 150;
	auto valeur = arbre.valeur(150);

	os << "Valeur à " << temps << " : " << valeur << '\n';
}

/* ************************************************************************** */

struct object {
	dls::chaine nom = "";

	explicit object(dls::chaine nom_)
		: nom(std::move(nom_))
	{}
};

template <typename T>
static void genere_enfants(
		foret::arborescence<T> &arbre,
		const typename foret::arborescence<T>::iterator &iter_noeud)
{
	arbre.insert(iter_noeud, object("enfant_a"));
	arbre.insert(iter_noeud, object("enfant_b"));
	arbre.insert(iter_noeud, object("enfant_c"));
	arbre.insert(iter_noeud, object("enfant_d"));
}

static void test_arborescence(std::ostream &os)
{
	foret::arborescence<object> arbre;

	auto noeud_a = arbre.insert(arbre.begin(), object("noeud_a"));
	genere_enfants(arbre, noeud_a);

	auto noeud_b = arbre.insert(arbre.begin(), object("noeud_b"));
	genere_enfants(arbre, noeud_b);

	auto noeud_c = arbre.insert(arbre.begin(), object("noeud_c"));
	genere_enfants(arbre, noeud_c);

	auto noeud_d = arbre.insert(arbre.begin(), object("noeud_d"));
	genere_enfants(arbre, noeud_d);

	os << "Taille de l'arbre : " << arbre.taille() << '\n';

	auto iterations = 0;

	for (const object &ob : arbre) {
		os << ob.nom << '\n';
		++iterations;
	}

	os << "Nombre d'itérations : " << iterations << '\n';
}

#ifdef ADOBE_FORET

#include "forest.hpp"

template <typename Arbre, typename Iter>
static void genere_enfants(
		Arbre &arbre,
		const Iter &iter_noeud)
{
	arbre.insert(iter_noeud, object("enfant_a"));
	arbre.insert(iter_noeud, object("enfant_b"));
	arbre.insert(iter_noeud, object("enfant_c"));
	arbre.insert(iter_noeud, object("enfant_d"));
}

static void test_forest(std::ostream &os)
{
	os << __func__ << '\n';
	adobe::forest<object> arbre;

	auto noeud_a = arbre.insert(arbre.begin(), object("noeud_a"));
	//genere_enfants(arbre, noeud_a);

	auto noeud_b = arbre.insert(noeud_a, object("noeud_b"));
	//genere_enfants(arbre, noeud_b);

	auto noeud_c = arbre.insert(noeud_b, object("noeud_c"));
	//genere_enfants(arbre, noeud_c);

	/*auto noeud_d =*/ arbre.insert(noeud_c, object("noeud_d"));
	//genere_enfants(arbre, noeud_d);

	os << "Taille de l'arbre : " << arbre.taille() << '\n';

	auto iterations = 0;

//	for (const object &ob : arbre) {
//		os << ob.nom << '\n';
//		++iterations;
//	}

	std::for_each(arbre.begin(), arbre.fin(), [&](const object &ob)
	{
		os << ob.nom << '\n';
		++iterations;
	});

	os << "Nombre d'itérations : " << iterations << '\n';
}

#endif

/* ************************************************************************** */

static void test_arborescence_pile(std::ostream &os)
{
	using namespace pile;

	Arborescence noeuds;
	noeuds.racine = new Noeud("/", false);

	Noeud noeud_a("/noeud_a", true);
	Noeud noeud_b("/noeud_b", true);
	Noeud noeud_c("/noeud_c", true);
	Noeud noeud_d("/noeud_d", true);

	noeuds.push_back(&noeud_a);
	noeuds.push_back(&noeud_b);
	noeuds.push_back(&noeud_c);
	noeuds.push_back(&noeud_d);

	for (Noeud *noeud : arborescence_iterator(noeuds.racine)) {
		os << noeud->nom << '\n';
	}
}

/* ************************************************************************** */

int main()
{
	constexpr auto test = 6;

	switch (test) {
		case 0:
			test_heap_tree(std::cerr);
			break;
		case 1:
			test_bsearch_tree(std::cerr);
			break;
		case 2:
			test_binary_tree(std::cerr);
			break;
		case 3:
			test_arborescence_pile(std::cerr);
			break;
		case 4:
			test_arborescence(std::cerr);
			break;
#ifdef ADOBE_FORET
		case 5:
			test_forest(std::cerr);
			break;
#endif
		case 6:
			test_arbre_binaire(std::cerr);
			break;
	}
}
