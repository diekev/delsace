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
#include <functional>

#include "biblinternes/structures/chaine.hh"

namespace dls::flux {

using type_op_pour_chaque_ligne = std::function<void(dls::chaine const &)>;

/**
 * Itère sur un fichier et appèle op(ligne) à chaque ligne.
 *
 * @param ifs Le fichier d'entrée.
 * @param op  Un foncteur de type <tt>void(dls::chaine const &)</tt>
 *
 * @par Exemple:
 * Compte le nombre de lignes dans le fichier.
 * @code
 * struct OpCompteLigne {
 *     size_t lignes = 0;
 *
 *     void operator()(dls::chaine const &)
 *     {
 * 	        lines++;
 *     }
 * };
 *
 * OpCompteLigne op;
 * pour_chaque_ligne(fichier, op);
 * auto lignes = op.lignes;
 * @endcode
 * ou avec un lambda:
 * @code
 * auto lignes = 0;
 * pour_chaque_ligne(infile, [&](dls::chaine const &) { lignes++; });
 * @endcode
 */
inline void pour_chaque_ligne(std::ifstream &ifs, type_op_pour_chaque_ligne &&op)
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
void format_number_array(char *first, char *last);

/**
 * @brief Print an array of chars as if it were an array of ints.
 * @param first A pointer to the first element in the array.
 * @param last A pointer to the last element in the array.
 * @param os The output stream.
 */
void print_array(const char *first, const char *last, std::ostream &os);

/**
 * @brief Print an array of chars as if it were an array of ints.
 * @param first A pointer to the first element in the array.
 * @param last A pointer to the last element in the array.
 * @param os The output stream.
 * @param prefix An optional string which will be printed before the array.
 * @param suffix An optional string which will be printed after the array.
 */
void print_array(const char *first, const char *last, std::ostream &os,
				 const char *prefix, const char *suffix);

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

}  /* namespace dls::flux */
