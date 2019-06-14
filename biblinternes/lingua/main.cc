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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "tests.h"

#include "../../biblexternes/docopt/docopt.hh"

static const char usage[] = R"(
Lingua.

Usage:
    lingua --langage
    lingua --depsparser
    lingua --syntax_parser
    lingua (-h | --help)
    lingua --version

Options:
    -h, --help       Show this screen.
    --version        Show version.
    --syntax_parser  Test Syntax Parser.
    --langage        Test Language.
    --depsparser     Test Dependency parser.
)";

const char chinese_sample[] = "\xe4\xb8\xad\xe6\x96\x87";
const char arabic_sample[] = "\xd8\xa7\xd9\x84\xd8\xb9\xd8\xb1\xd8\xa8\xd9\x8a\xd8\xa9";
const char spanish_sample[] = "\x63\x61\xc3\xb1\xc3\xb3\x6e";

const char api_sample2[] = "ɑ̃dyʁɘ";
const char nihongo[] = "日本語";

int main(int argc, char **argv)
{
	auto args = dls::docopt::docopt(usage, { argv + 1, argv + argc }, true, "Lingua 0.1");

	const auto do_langage = dls::docopt::get_bool(args, "--langage");
	const auto do_syntax_parser = dls::docopt::get_bool(args, "--syntax_parser");
	const auto do_depsparser = dls::docopt::get_bool(args, "--depsparser");

	if (do_langage) {
		test_langage(std::cerr);
	}
	else if (do_syntax_parser) {
		test_syntax_parser(std::cerr);
	}
	else if (do_depsparser) {
		test_dependency_parser(std::cerr);
	}
}
