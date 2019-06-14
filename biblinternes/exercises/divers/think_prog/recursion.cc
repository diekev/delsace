#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

/* ********************************** ex 6.1 ********************************* */

template <typename Iterator>
int sum_positive(Iterator first, Iterator last)
{
	int sum = 0;

	if (first == last) {
		return sum;
	}

	if (*first % 2 == 0) {
		sum = *first;
	}

	return sum + sum_positive(++first, last);
}

void ex_6_1(std::ostream &os)
{
	std::vector<int> array(20);
	std::iota(array.begin(), array.end(), 0);

	os << sum_positive(array.begin(), array.end()) << "\n";
}

/* ********************************** ex 6.2 ********************************* */

// TODO: make it recursive
template <typename Iterator>
bool odd_parity(Iterator first, Iterator last)
{
	auto total = std::distance(first, last);
	auto num_odd = 0;

	while (first++ != last) {
		num_odd += *first;
	}

	return (num_odd > (total / 2));
}

void ex_6_2(std::ostream &os)
{
	std::vector<int> array(20);
	std::mt19937 rng(19937);
	std::uniform_int_distribution<int> dist(0, 1);

	for (auto &a : array) {
		a = 1 - dist(rng);
		os << a;
	}
	os << "\n";

	bool is_odd_parity = odd_parity(array.begin(), array.end());

	if (is_odd_parity) {
		os << "Binary str has an odd parity\n";
	}
	else {
		os << "Binary str does not have an odd parity\n";
	}
}

/* ********************************** ex 6.3 ********************************* */

template <typename Iterator>
int occurrence(Iterator first, Iterator last, const int target)
{
	if (first == last) {
		return 0;
	}

	return (*first == target) + occurrence(++first, last, target);
}

void ex_6_3(std::ostream &os)
{
	std::vector<int> array(200);
	std::mt19937 rng(19937);
	std::uniform_int_distribution<int> dist(0, 20);
	const int target = 10;

	for (auto &a : array) {
		a = dist(rng);
	}

	os << occurrence(array.begin(), array.end(), target) << "\n";
}

/* ********************************** ex 6.4 ********************************* */

template <typename Iterator>
Iterator find(Iterator first, Iterator last, const int target)
{
	Iterator found = last;

	if (first != last) {
		if (*first == target) {
			found = first;
		}
		else {
			found = find(++first, last, target);
		}
	}

	return found;
}

void ex_6_4(std::ostream &os)
{
	std::vector<int> array(20);
	std::iota(array.begin(), array.end(), 0);
	const int target = 25;

	auto iter = find(array.begin(), array.end(), target);

	if (iter != array.end()) {
		os << "Value found\n";
	}
	else {
		os << "Value not found\n";
	}
}

/* ********************************* ex 6.9 ********************************** */

class BinaryTree {
	struct Node {
		using Ptr = std::unique_ptr<Node>;

		Node::Ptr left, right;
		int value;

		Node(int v)
		    : left(nullptr)
		    , right(nullptr)
		    , value(v)
		{}

		static Ptr create(int v)
		{
			return Ptr(new Node(v));
		}
	};

	Node::Ptr m_root;

	void insert(Node::Ptr &node, int value)
	{
		if (node == nullptr) {
			node = Node::create(value);
			return;
		}

		if (value < node->value) {
			insert(node->left, value);
		}
		else {
			insert(node->right, value);
		}
	}

	bool is_heap(const Node::Ptr &node) const
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

	bool is_search_tree(const Node::Ptr &node) const
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

	void insertBST(Node &node, int value)
	{
		if (node.value > value) {
			if (node.left == nullptr) {
				node.left = Node::create(value);
			}
			else {
				insertBST(*node.left, value);
			}
		}
		else if (node.value < value) {
			if (node.right == nullptr) {
				node.right = Node::create(value);
			}
			else {
				insertBST(*node.right, value);
			}
		}
		else {
			Node::Ptr n = Node::create(value);
			if (node.left == nullptr) {
				node.left = Node::create(value);
			}
			else {

			}

		}
	}

public:
	BinaryTree()
	    : m_root(nullptr)
	{}

	void insert(int value)
	{
		if (m_root == nullptr) {
			m_root = Node::create(value);
			return;
		}

		insert(m_root, value);
	}

	void insertBST(int value)
	{
		if (m_root == nullptr) {
			m_root = Node::create(v);
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
};

void ex_6_9(std::ostream &os)
{
	BinaryTree tree;
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

/* ********************************* ex 6.10 ********************************* */

void ex_6_10(std::ostream &os)
{
	BinaryTree tree;
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

/* ****************************** End Exercices ****************************** */

int main()
{
	std::ostream &os = std::cout;
	ex_6_10(os);
}
