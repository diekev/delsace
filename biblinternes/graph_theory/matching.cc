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

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../math/matrice/matrice.hh"

struct Node;

struct Socket {
	std::string name;
	Socket *link;
	Node *parent;
	float weight;

	Socket(const std::string &name_, const float weight_, Node *node_)
		: name(name_)
	    , link(nullptr)
		, parent(node_)
		, weight(weight_)
	{}

	Socket(const Socket &) = delete;
	Socket &operator=(const Socket &) = delete;
};

struct Node {
	std::vector<Socket *> sockets{};
	std::string name = "";
	int degree = 0;

	using Ptr = std::unique_ptr<Node>;

	Node() = default;

	explicit Node(const std::string &name_)
		: Node()
	{
		this->name = name_;
	}

	~Node()
	{
		for (auto &socket : sockets) {
			delete socket;
		}
	}

	static Ptr create(const std::string &name)
	{
		return Ptr(new Node(name));
	}

	void addSockets(const std::initializer_list<std::string> &socket_list)
	{
		sockets.reserve(socket_list.size());

		int i = 1;
		for (const auto &socket : socket_list) {
			sockets.emplace_back(new Socket(socket, float(i++), this));
		}
	}

	void removeSocket(const std::string &name_)
	{
		auto iter = std::find_if(sockets.begin(), sockets.end(),
								 [&](Socket *sock) { return sock->name == name_; });

		if (iter != sockets.end()) {
			Socket *sock = *iter;
			sockets.erase(iter);
			delete sock;
		}
	}

	Socket *socket(const std::string &name_) const
	{
		auto iter = std::find_if(sockets.begin(), sockets.end(),
								 [&](Socket *sock) { return sock->name == name_; });

		if (iter != sockets.end()) {
			return *iter;
		}

		return nullptr;
	}

	friend std::ostream &operator<<(std::ostream &os, const Node &node)
	{
		os << "Node: " << node.name << ", degree: " << node.degree << "\n";
		return os;
	}
};

class MatchingGraph {
	std::vector<Node *> m_boys{};
	std::vector<Node *> m_girls{};

	dls::math::matrice<int> m_adjacency_matrix{};

public:
	MatchingGraph() = default;

	MatchingGraph(const MatchingGraph &) = delete;
	MatchingGraph &operator=(const MatchingGraph &) = delete;

	~MatchingGraph() = default;

	void addBoys(const std::initializer_list<Node *> &boys_list)
	{
		m_boys = boys_list;
	}

	void addGirls(const std::initializer_list<Node *> &girls_list)
	{
		m_girls = girls_list;
	}

	void match() const
	{
		int iterations = 0;

		while (true) {
			/* connect boys to girls */
			for (auto &node : m_boys) {
				connectBoy(node);
			}

			bool terminate = true;

			/* ensure girls only have only one connection */
			for (auto &girl : m_girls) {
				terminate &= (girl->degree == 1);
				disconnectBoys(girl);
			}

			++iterations;

			if (terminate) {
				break;
			}
		}
	}

	void printNodes(std::ostream &os) const
	{
		for (const auto &node : m_boys) {
			os << *node;
		}

		for (const auto &node : m_girls) {
			os << *node;
		}
	}

	friend std::ostream &operator<<(std::ostream &os, const MatchingGraph &graph)
	{
		bool is_perfect_graph = true;

		os << "Graph connections:\n";
		os.precision(2);

		for (const auto &node : graph.m_girls) {
			is_perfect_graph &= (node->degree == 1);

			for (const auto &socket : node->sockets) {
				Socket *link = socket->link;

				if (link != nullptr) {
					Node *so = link->parent;
					os << "\t" << node->name << " <---> " << so->name;

					/* Compute the "weight" (average preference of the couple in
					 * each other's list) of the relationship. */
					const float weight = (socket->weight + link->weight) * 0.5f;
					os << "    (weight: " << 1.0f / weight << ")\n";

					break;
				}
			}
		}

		for (const auto &node : graph.m_boys) {
			is_perfect_graph &= (node->degree == 1);
		}

		if (is_perfect_graph) {
			os << "The graph is perfect, all nodes have degree 1.\n";
		}

		return os;
	}

	void build_adjacency_matrix()
	{
		const auto dimensions = dls::math::Dimensions(
									dls::math::Hauteur(static_cast<int>(m_boys.size())),
									dls::math::Largeur(static_cast<int>(m_girls.size())));

		m_adjacency_matrix.redimensionne(dimensions);
		m_adjacency_matrix.remplie(0);

		int i = 0;

		for (const auto &boy : m_girls) {
			for (const auto &socket : boy->sockets) {
				Socket *link = socket->link;

				if (link != nullptr) {
					int j = 0;
					for (const auto &node : m_boys) {
						if (node == link->parent) {
							break;
						}

						++j;
					}

					m_adjacency_matrix[i][j] = 1;
				}
			}

			++i;
		}

		std::cout << "Adjacency matrix: \n";
		std::cout << m_adjacency_matrix;
	}

private:
	Node *findGirl(const std::string &name) const
	{
		auto iter = std::find_if(m_girls.begin(), m_girls.end(),
		                         [&](Node *node) { return node->name == name; });

		if (iter != m_girls.end()) {
			return *iter;
		}

		return nullptr;
	}

	void connectBoy(Node *boy) const
	{
		/* the first socket represent its prefered "girl" */
		Socket *bsock = boy->sockets[0];

		/* already linked */
		if (bsock->link != nullptr) {
			return;
		}

		Node *girl = findGirl(bsock->name);
		Socket *gsock = girl->socket(boy->name);

		bsock->link = gsock;
		gsock->link = bsock;

		++boy->degree;
		++girl->degree;
	}

	void disconnectBoys(Node *girl) const
	{
		/* Sockets are sorted by order of preference so iterate from the last
		 * socket to the first, and remove connections ("reject boys") until
		 * there is only one left. */
		size_t s = girl->sockets.size();
		while (girl->degree > 1 && s--) {
			Socket *gsock = girl->sockets[s];

			if (gsock->link == nullptr) {
				continue;
			}

			Node *boy = gsock->link->parent;
			gsock->link = nullptr;
			boy->removeSocket(girl->name);

			--girl->degree;
			--boy->degree;
		}
	}
};

#if 0
int main()
{
	MatchingGraph graph;

	// boys
	Node::Ptr b1 = Node::create("1");
	b1->addSockets({"C", "B", "E", "F", "A", "D"});

	Node::Ptr b2 = Node::create("2");
	b2->addSockets({"A", "B", "E", "C", "D", "F"});

	Node::Ptr b3 = Node::create("3");
	b3->addSockets({"D", "F", "C", "B", "A", "E"});

	Node::Ptr b4 = Node::create("4");
	b4->addSockets({"A", "C", "D", "B", "F", "E"});

	Node::Ptr b5 = Node::create("5");
	b5->addSockets({"F", "A", "B", "D", "E", "C"});

	Node::Ptr b6 = Node::create("6");
	b6->addSockets({"B", "D", "F", "A", "C", "E"});

	// girls
	Node::Ptr g1 = Node::create("A");
	g1->addSockets({"3", "6", "5", "2", "1", "4"});

	Node::Ptr g2 = Node::create("B");
	g2->addSockets({"6", "5", "2", "1", "4", "3"});

	Node::Ptr g3 = Node::create("C");
	g3->addSockets({"4", "3", "5", "1", "6", "2"});

	Node::Ptr g4 = Node::create("D");
	g4->addSockets({"1", "2", "6", "3", "4", "5"});

	Node::Ptr g5 = Node::create("E");
	g5->addSockets({"2", "3", "4", "6", "1", "5"});

	Node::Ptr g6 = Node::create("F");
	g6->addSockets({"3", "1", "4", "2", "5", "6"});

	graph.addBoys({b1.get(), b2.get(), b3.get(), b4.get(), b5.get(), b6.get()});
	graph.addGirls({g1.get(), g2.get(), g3.get(), g4.get(), g5.get(), g6.get()});

	graph.match();

	std::cout << graph;

	graph.build_adjacency_matrix();
}
#endif
