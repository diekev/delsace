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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <cctype>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

/*
   TODO: intern strings instead of allocation.
*/
/*
	gcc:
        g++ -Wall -DDEBUG tinyxml2.cpp xmltest.cpp -o gccxmltest.exe

    Formatting, Artistic Style:
        AStyle.exe --style=1tbs --indent-switches --break-closing-brackets --indent-preprocessor tinyxml2.cpp tinyxml2.h
*/

#if defined(_DEBUG) || defined(DEBUG) || defined (__DEBUG__)
#	ifndef DEBUG
#		define DEBUG
#	endif
#endif

#if defined(DEBUG)
#	include <assert.h>
#	define TIXMLASSERT assert
#else
#	define TIXMLASSERT(x) {}
#endif


/* Versioning, past 1.0.14:
	http://semver.org/
*/
static const int TIXML2_MAJOR_VERSION = 3;
static const int TIXML2_MINOR_VERSION = 0;
static const int TIXML2_PATCH_VERSION = 0;

namespace dls {
namespace xml {

class Document;
class Element;
class Attribut;
class Commentaire;
class Texte;
class Declaration;
class Inconnu;
class XMLPrinter;

class PaireString;

}  /* namespace xml */
}  /* namespace dls */
