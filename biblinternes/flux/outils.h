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

#include <fstream>

namespace dls {
namespace flux {

/**
 * Iterate over a file and at each step call @c op(line).
 *
 * @param ifs The input file to iterate on.
 * @param op  A functor of the form <tt>void op(const std::string &)</tt>
 *
 * @par Example:
 * Count the number of lines in a file.
 * @code
 * struct CountLinesOp {
 *     size_t lines;
 *
 *     CountLinesOp()
 *         : lines(0)
 *     {}
 *
 *     void operator()(const std::string &)
 *     {
 * 	        lines++;
 *     }
 *
 *     size_t result() const
 *     {
 * 	        return lines;
 *     }
 * };
 *
 * CountLinesOp op;
 * foreach_line(infile, op);
 * size_t lines = op.result();
 * @endcode
 * or with a lambda:
 * @code
 * size_t lines(0);
 * foreach_line(infile, [&](const std::string &) { lines++; });
 * @endcode
 */
template <typename ForEachLineOp>
void foreach_line(std::ifstream &ifs, ForEachLineOp &&op)
{
	std::string line;
	while (std::getline(ifs, line)) {
		op(line);
	}
}

/**
 * @brief Format a char array containing values in range [0, 9] to be printable.
 * @param first A pointer to the first element in the array.
 * @param last A pointer to the last element in the array.
 */
void format_number_array(char *first, char *last)
{
	bool is_null = true;

	while (last-- > first) {
		if (is_null && *last == 0) {
			continue;
		}

		is_null = false;
		*last = static_cast<char>(*last + '0');
	}
}

/**
 * @brief Print an array of chars as if it were an array of ints.
 * @param first A pointer to the first element in the array.
 * @param last A pointer to the last element in the array.
 * @param os The output stream.
 */
void print_array(const char *first, const char *last, std::ostream &os)
{
	while (first < last) {
		os << *first;
		++first;
	}
}

/**
 * @brief Print an array of chars as if it were an array of ints.
 * @param first A pointer to the first element in the array.
 * @param last A pointer to the last element in the array.
 * @param os The output stream.
 * @param prefix An optional string which will be printed before the array.
 * @param suffix An optional string which will be printed after the array.
 */
void print_array(const char *first, const char *last, std::ostream &os,
                 const char *prefix, const char *suffix)
{
	if (prefix) {
		os << prefix;
	}

	print_array(first, last, os);

	if (suffix) {
		os << suffix;
	}
}

namespace couleur {

#define FONCTION_COULEUR(name, id) \
	template <typename CharT, typename Traits> \
	inline std::basic_ostream<CharT, Traits> &name(std::basic_ostream<CharT, Traits> &os) \
	{ \
		os << id; \
		return os; \
	}

FONCTION_COULEUR(black,       "\033[0;30m")
FONCTION_COULEUR(black_bold,  "\033[1;30m")
FONCTION_COULEUR(red,         "\033[0;31m")
FONCTION_COULEUR(red_bold,    "\033[1;31m")
FONCTION_COULEUR(green,       "\033[0;32m")
FONCTION_COULEUR(green_bold,  "\033[1;32m")
FONCTION_COULEUR(yellow,      "\033[0;33m")
FONCTION_COULEUR(yellow_bold, "\033[1;33m")
FONCTION_COULEUR(blue,        "\033[0;34m")
FONCTION_COULEUR(blue_bold,   "\033[1;34m")
FONCTION_COULEUR(purple,      "\033[0;35m")
FONCTION_COULEUR(purple_bold, "\033[1;35m")
FONCTION_COULEUR(cyan,        "\033[0;36m")
FONCTION_COULEUR(cyan_bold,   "\033[1;36m")
FONCTION_COULEUR(white,       "\033[0;37m")
FONCTION_COULEUR(white_bold,  "\033[1;37m")
FONCTION_COULEUR(neutral,     "\033[0m")

#undef FONCTION_COULEUR

}  /* namespace couleur */

}  /* namespace flux */
}  /* namespace dls */
