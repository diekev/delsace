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
#include <unordered_map>
#include <vector>

constexpr int MAGIC_NUMBER = 0x48554646;

struct Node {
	Node *left, *right;
	std::string code;
	char character;
	int frequency;

	explicit Node(const char ch, const int freq = 1)
	    : left(nullptr)
	    , right(nullptr)
	    , code("")
	    , character(ch)
	    , frequency(freq)
	{}

	Node(Node *a, Node *b)
	    : left(a)
	    , right(b)
	    , code("")
	    , character('\0')
	    , frequency(a->frequency + b->frequency)
	{}

	Node(const Node &) = delete;
	Node &operator=(const Node &) = delete;

	static bool compByFrequecy(Node *a, Node *b)
	{
		return a->frequency < b->frequency;
	}
};

std::vector<Node *> nodes_from_distribution(int letters[]);

class BinaryTree {
	using HuffMap = std::unordered_map<char, std::string>;

	Node *m_root;
	HuffMap m_map;
	size_t m_leaves_count;
	size_t m_encode_length;

	void destroy_tree(Node *node);
	void build_huffman_map(Node *node, const std::string &code);
	char decode_ex(Node *node, const std::string &code, size_t &i);
	void compute_probability(Node *node, const float frequency, std::vector<float> &freqs) const;

public:
	BinaryTree();
	~BinaryTree();

	BinaryTree(const BinaryTree &) = delete;
	BinaryTree &operator=(const BinaryTree &) = delete;

	void build_huffman_map();
	void build_tree_from_nodes(std::vector<Node *> &nodes);

	std::string encode(const std::string &to_encode);
	std::string decode(const std::string &to_decode);

	float entropy() const;
};

class HuffManFileHeader {
	uint64_t m_MAGIC_NUMBER;
	int m_letters[256];

public:
	HuffManFileHeader();
	~HuffManFileHeader() = default;

	void setLetterDist(int letters[]);
	void copyLetterDist(int letters[]);

	void write(std::ostream &os);
	bool read(std::istream &is);
};

class HuffManFile {
	std::unique_ptr<HuffManFileHeader> m_header;
	std::vector<unsigned char> m_bytes;

public:
	HuffManFile();
	~HuffManFile() = default;

	HuffManFile(const HuffManFile &) = delete;
	HuffManFile &operator=(const HuffManFile &) = delete;

	void write(std::ostream &os);
	bool read(std::istream &in);

	HuffManFileHeader *header() const;
	void setHeader(HuffManFileHeader *header);

	void setBytesFromStr(const std::string &str);
	std::string strFromBytes() const;
};
