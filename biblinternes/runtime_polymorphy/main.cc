//#include "document.h"
#include <iostream>
#include <memory>
#include <vector>

class Node {
	struct node_concept {
		virtual ~node_concept() = default;
		virtual void process() const = 0;
	};

	template <typename T>
	struct node_model : public node_concept {
		explicit node_model(T x)
		    : m_data(std::move(x))
		{}

		void process() const
		{
			process_node(m_data);
		}

		T m_data;
	};

	std::shared_ptr<const node_concept> m_self;

public:
	template <typename T>
	explicit Node(T x)
	    : m_self(std::make_shared<node_model<T>>(std::move(x)))
	{}

	friend void process_node(const Node &node)
	{
		node.m_self->process();
	}
};

using graph_t = std::vector<Node>;

class MathNode {
public:
	std::string name() const
	{
		return std::string("MathNode");
	}
};

void process_node(const MathNode &node)
{
	std::cout << "Processing node: " << node.name() << "\n";
}

class StringNode {
public:
	std::string name() const
	{
		return std::string("StringNode");
	}
};

void process_node(const StringNode &node)
{
	std::cout << "Processing node: " << node.name() << "\n";
}

class AdvectNode {
public:
	std::string name() const
	{
		return std::string("AdvectNode");
	}
};

void process_node(const AdvectNode &node)
{
	std::cout << "Processing node: " << node.name() << "\n";
}

class SimulationNode {
	graph_t m_graph;

public:
	SimulationNode()
	    : m_graph()
	{
		m_graph.emplace_back(AdvectNode());
		m_graph.emplace_back(MathNode());
		m_graph.emplace_back(AdvectNode());
		m_graph.emplace_back(MathNode());
		m_graph.emplace_back(AdvectNode());
	}

	std::string name() const
	{
		return std::string("SimulationNode");
	}

	const graph_t &graph() const
	{
		return m_graph;
	}
};

void process_node(const SimulationNode &snode)
{
	std::cout << "========================\n";
	for (const auto &node : snode.graph()) {
		process_node(node);
	}
	std::cout << "========================\n";
}

int main()
{
	graph_t graph;
	graph.reserve(10);

	graph.emplace_back(MathNode());
	graph.emplace_back(StringNode());
	graph.emplace_back(MathNode());
	graph.emplace_back(StringNode());
	graph.emplace_back(StringNode());
	graph.emplace_back(MathNode());
	graph.emplace_back(SimulationNode());
	graph.emplace_back(MathNode());

	for (const auto &node : graph) {
		process_node(node);
	}
}
