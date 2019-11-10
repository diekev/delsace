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

#include "huffman.h"
#include "lz.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>

#include "../chrono/chronometre_de_portee.hh"
#include "../../biblexternes/docopt/docopt.hh"

#include "../flux/outils.h"

int test_huffman(const dls::chaine &filename, bool encode)
{
	CHRONOMETRE_PORTEE(__func__, std::cerr);

	std::unique_ptr<HuffManFileHeader> header(new HuffManFileHeader());
	std::unique_ptr<HuffManFile> hfile(new HuffManFile());
	dls::chaine str;

	int dist[256];

	if (encode) {
		std::ifstream ifs(filename.c_str());

		dls::flux::pour_chaque_ligne(ifs, [&str](const dls::chaine &line)
		{
			str.append(line + "\n");
		});

		std::fill_n(dist, 256, 0);
		for (const auto ch : str) {
			dist[static_cast<int>(ch)]++;
		}

		header->setLetterDist(dist);
		hfile->setHeader(header.release());
	}
	else {
		std::ifstream is;
		is.open(filename.c_str(), std::ios::in | std::ios::binary);

		if (!hfile->read(is)) {
			std::cerr << "Cannot read file!\n";
			return 1;
		}

		hfile->header()->copyLetterDist(dist);
	}

	auto nodes = nodes_from_distribution(dist);

	std::unique_ptr<BinaryTree> tree(new BinaryTree);
	tree->build_tree_from_nodes(nodes);
	tree->build_huffman_map();

	if (encode) {
		auto encoded = tree->encode(str);
		hfile->setBytesFromStr(encoded);

		std::ofstream ofile("/tmp/encoded.huff", std::ios::out | std::ios::binary);

		hfile->write(ofile);

		std::cout << "Message entropy: " << tree->entropy() << "\n";
	}
	else {
		dls::chaine bytes = hfile->strFromBytes();
		dls::chaine decoded = tree->decode(bytes);

		std::cout << decoded;
	}

	return 0;
}

static void test_lz(const dls::chaine &filename, const bool encode)
{
	if (encode) {
		dictionnary_t dictionnary;
		build_dictionnary(dictionnary);
		dls::chaine w("");

		std::ifstream ifs(filename.c_str());
		dls::chaine str((std::istreambuf_iterator<char>(ifs)),
		                 std::istreambuf_iterator<char>());

		std::ofstream ofile("/tmp/encoded.lz", std::ios::out | std::ios::binary);

		for (const auto &c : str) {
			const auto &seq = w + c;
			const auto &iter = std::find(dictionnary.debut(), dictionnary.fin(), seq);

			if (iter != dictionnary.fin()) {
				w = seq;
			}
			else {
				encode_sequence(w, dictionnary, ofile);
				dictionnary.pousse(seq);
				w = c;
			}
		}

		encode_sequence(w, dictionnary, ofile);
	}
	else {
		std::ifstream ifs("/tmp/encoded.lz", std::ios::in | std::ios::binary);
		dls::chaine str((std::istreambuf_iterator<char>(ifs)),
		                 std::istreambuf_iterator<char>());
		decode(str);
	}
}

static const char usage[] = R"(
Compiler.

Usage:
    compression --huffman (--encode | --decode) FILE
    compression --lz (--encode | --decode) FILE
    compression (-h | --help)
    compression --version

Options:
    -h, --help    Show this screen.
    --version     Show version.
    --huffman     Test Huffman compression.
    --lz          Test LZ compression.
)";

int main(int argc, char **argv)
{
	const auto args = dls::docopt::docopt(usage, { argv + 1, argv + argc }, true, "Compression 0.1");

	const auto filename = dls::docopt::get_string(args, "FILE");
	const auto do_huffman = dls::docopt::get_bool(args, "--huffman");
	const auto do_encode = dls::docopt::get_bool(args, "--encode");
	const auto do_decode = dls::docopt::get_bool(args, "--decode");
	const auto do_lz = dls::docopt::get_bool(args, "--lz");

	if (do_lz) {
		test_lz(filename, (do_encode || !do_decode));
	}
	else if (do_huffman) {
		test_huffman(filename, (do_encode || !do_decode));
	}
}
