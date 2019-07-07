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
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>

#include "huffman.h"

BinaryTree::BinaryTree()
    : m_root(nullptr)
    , m_map()
    , m_leaves_count(0)
    , m_encode_length(0)
{}

BinaryTree::~BinaryTree()
{
	destroy_tree(m_root);
}

void BinaryTree::build_huffman_map()
{
	build_huffman_map(m_root, "");
}

void BinaryTree::build_tree_from_nodes(std::vector<Node *> &nodes)
{
	m_leaves_count = nodes.size();

	while (nodes.size() > 1) {
		Node *na = nodes[0];
		Node *nb = nodes[1];
		Node *parent = new Node(na, nb);

		nodes.erase(nodes.begin(), nodes.begin() + 2);

		auto lb = std::lower_bound(nodes.begin(), nodes.end(), parent, Node::compByFrequecy);

		nodes.insert(lb, parent);
	}

	m_root = nodes[0];
}

std::string BinaryTree::encode(const std::string &to_encode)
{
	std::string encoded("");
	encoded.reserve(m_encode_length);

	for (const auto &ch : to_encode) {
		auto iter = m_map.trouve(ch);
		encoded.append(iter->second);
	}

	return encoded;
}

std::string BinaryTree::decode(const std::string &to_decode)
{
	std::string str("");
	str.reserve(static_cast<size_t>(m_root->frequency));

	size_t i(0);
	while (i < to_decode.size()) {
		str.push_back(decode_ex(m_root, to_decode, i));
	}

	if (str.empty()) {
		std::cout << "decoded str is empty!\n";
	}

	return str;
}

float BinaryTree::entropy() const
{
	std::vector<float> probabilities;
	probabilities.reserve(m_leaves_count);

	auto const inv_frequency = 1.0f / static_cast<float>(m_root->frequency);
	compute_probability(m_root, inv_frequency, probabilities);

	auto entropy = 0.0f;

	for (const auto prob : probabilities) {
		entropy += (-std::log2(prob) * prob);
	}

	return entropy;
}

void BinaryTree::destroy_tree(Node *node)
{
	if (node != nullptr) {
		destroy_tree(node->left);
		destroy_tree(node->right);
		delete node;
	}
}

void BinaryTree::build_huffman_map(Node *node, const std::string &code)
{
	if (node == nullptr) {
		return;
	}

	if (node->left == nullptr && node->right == nullptr) {
		m_map.insere(std::make_pair(node->character, node->code));
		m_encode_length += static_cast<size_t>(node->frequency) * node->code.length();
		return;
	}

	node->left->code = code + "0";
	node->right->code = code + "1";

	build_huffman_map(node->left, node->left->code);
	build_huffman_map(node->right, node->right->code);
}

char BinaryTree::decode_ex(Node *node, const std::string &code, size_t &i)
{
	if (node == nullptr) {
		return '\0';
	}

	if (node->left == nullptr && node->right == nullptr) {
		return node->character;
	}

	auto c = static_cast<char>(code[i++] - '0');

	if (c) {
		return decode_ex(node->right, code, i);
	}

	return decode_ex(node->left, code, i);
}

void BinaryTree::compute_probability(Node *node, const float frequency, std::vector<float> &freqs) const
{
	if (node == nullptr) {
		return;
	}

	if (node->left == nullptr && node->right == nullptr) {
		freqs.push_back(static_cast<float>(node->frequency) * frequency);
		return;
	}

	compute_probability(node->left, frequency, freqs);
	compute_probability(node->right, frequency, freqs);
}

HuffManFileHeader::HuffManFileHeader()
    : m_MAGIC_NUMBER(uint64_t(MAGIC_NUMBER) << 32 | uint64_t(MAGIC_NUMBER))
    , m_letters{0}
{}

void HuffManFileHeader::setLetterDist(int letters[])
{
	std::copy_n(letters, 256, m_letters);
}

void HuffManFileHeader::copyLetterDist(int letters[])
{
	std::copy_n(m_letters, 256, letters);
}

void HuffManFileHeader::write(std::ostream &os)
{
	os.write(reinterpret_cast<char *>(&m_MAGIC_NUMBER), sizeof(uint64_t));
	os.write(reinterpret_cast<char *>(&m_letters), sizeof(int) * 256);
}

bool HuffManFileHeader::read(std::istream &is)
{
	uint64_t magic_num;
	is.read(reinterpret_cast<char *>(&magic_num), sizeof(uint64_t));

	if (magic_num == m_MAGIC_NUMBER) {
		is.read(reinterpret_cast<char *>(&m_letters), sizeof(int) * 256);

		return true;
	}

	return false;
}

HuffManFile::HuffManFile()
    : m_header(nullptr)
    , m_bytes(0)
{}

void HuffManFile::write(std::ostream &os)
{
	m_header->write(os);

	size_t num_bytes = m_bytes.size();
	os.write(reinterpret_cast<char *>(&num_bytes), sizeof(size_t));
	os.write(reinterpret_cast<char *>(&m_bytes[0]), static_cast<long>(m_bytes.size() * sizeof(unsigned char)));
}

bool HuffManFile::read(std::istream &in)
{
	m_header.reset(new HuffManFileHeader());

	if (m_header->read(in)) {
		size_t num_bytes;
		in.read(reinterpret_cast<char *>(&num_bytes), sizeof(size_t));

		m_bytes.resize(num_bytes);
		in.read(reinterpret_cast<char *>(&m_bytes[0]), static_cast<long>(sizeof(unsigned char) * num_bytes));

		return true;
	}

	return false;
}

HuffManFileHeader *HuffManFile::header() const
{
	return m_header.get();
}

void HuffManFile::setHeader(HuffManFileHeader *header)
{
	m_header.reset(header);
}

void HuffManFile::setBytesFromStr(const std::string &str)
{
	m_bytes.reserve((str.size() >> 3) + 1);

	for (size_t i(0); i < str.size();) {
		auto byte = 0;
		auto j = 8;

		while (j--) {
			byte |= ((str[i++] - '0') << j);
		}

		m_bytes.push_back(static_cast<unsigned char>(byte));
	}
}

std::string HuffManFile::strFromBytes() const
{
	std::string str;
	str.resize(m_bytes.size() << 3);

	for (size_t i(0); i < str.size();) {
		unsigned char byte = m_bytes[i >> 3];
		int j = 8;

		while (j--) {
			str[i++] = static_cast<char>('0' + ((byte & (1 << j)) >> j));
		}
	}

	return str;
}

std::vector<Node *> nodes_from_distribution(int letters[])
{
	std::vector<Node *> nodes;

	for (int i(0); i < 256; ++i) {
		if (letters[i] != 0) {
			auto node = new Node(static_cast<char>(i), letters[i]);
			nodes.push_back(node);
		}
	}

	std::sort(nodes.begin(), nodes.end(), Node::compByFrequecy);

	return nodes;
}
