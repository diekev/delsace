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
 * The Original Code is Copyright (c) 2013 Jared Grubb.
 * Modifications Copyright (c) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "docopt_value.hh"

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico.hh"
#include "biblinternes/structures/tableau.hh"

namespace dls {
namespace docopt {

// Usage string could not be parsed (ie, the developer did something wrong)
struct DocoptLanguageError : std::runtime_error {
	using runtime_error::runtime_error;
};

// Arguments passed by user were incorrect (ie, developer was good, user is wrong)
struct DocoptArgumentError : std::runtime_error {
	using runtime_error::runtime_error;
};

// Arguments contained '--help' and parsing was aborted early
struct DocoptExitHelp : std::runtime_error {
	DocoptExitHelp() : std::runtime_error("Docopt --help argument encountered")
	{}
};

// Arguments contained '--version' and parsing was aborted early
struct DocoptExitVersion : std::runtime_error {
	DocoptExitVersion() : std::runtime_error("Docopt --version argument encountered")
	{}
};

/// Parse user options from the given option string.
///
/// @param doc   The usage string
/// @param argv  The user-supplied arguments
/// @param help  Whether to end early if '-h' or '--help' is in the argv
/// @param version Whether to end early if '--version' is in the argv
/// @param options_first  Whether options must precede all args (true), or if args and options
///                can be arbitrarily mixed.
///
/// @throws DocoptLanguageError if the doc usage string had errors itself
/// @throws DocoptExitHelp if 'help' is true and the user has passed the '--help' argument
/// @throws DocoptExitVersion if 'version' is true and the user has passed the '--version' argument
/// @throws DocoptArgumentError if the user's argv did not match the usage patterns
dls::dico<dls::chaine, value> docopt_parse(
		const dls::chaine &doc,
		const dls::tableau<dls::chaine> &argv,
		bool help = true,
		bool version = true,
		bool options_first = false);

/// Parse user options from the given string, and exit appropriately
///
/// Calls 'docopt_parse' and will terminate the program if any of the exceptions above occur:
///  * DocoptLanguageError - print error and terminate (with exit code -1)
///  * DocoptExitHelp - print usage string and terminate (with exit code 0)
///  * DocoptExitVersion - print version and terminate (with exit code 0)
///  * DocoptArgumentError - print error and usage string and terminate (with exit code -1)
dls::dico<dls::chaine, value> docopt(
		const dls::chaine &doc,
		const dls::tableau<dls::chaine> &argv,
		bool help = true,
		const dls::chaine &version = {},
		bool options_first = false) noexcept;

/**
 * Retourne la valeur bool associée avec l'argument dont le nom est passé en
 * paramètre. Retourne 'false' si l'argument est introuvable ou n'a pas été
 * donné sur la ligne de commande.
 */
bool get_bool(
		const dls::dico<dls::chaine, value> &args,
		const dls::chaine &nom_argument) noexcept;

/**
 * Retourne la valeur long associée avec l'argument dont le nom est passé en
 * paramètre. Retourne '0ul' si l'argument est introuvable ou n'a pas été
 * donné sur la ligne de commande.
 */
long get_long(
		const dls::dico<dls::chaine, value> &args,
		const dls::chaine &nom_argument) noexcept;

/**
 * Retourne la valeur string associée avec l'argument dont le nom est passé en
 * paramètre. Retourne une string vide si l'argument est introuvable ou n'a pas
 * été donné sur la ligne de commande.
 */
dls::chaine get_string(
		const dls::dico<dls::chaine, value> &args,
		const dls::chaine &nom_argument) noexcept;

/**
 * Retourne la liste de string associée avec l'argument dont le nom est passé en
 * paramètre. Retourne une liste vide si l'argument est introuvable ou n'a pas
 * été donné sur la ligne de commande.
 */
dls::tableau<dls::chaine> get_string_list(
		const dls::dico<dls::chaine, value> &args,
		const dls::chaine &nom_argument) noexcept;

}  /* namespace docopt */
}  /* namespace dls */
