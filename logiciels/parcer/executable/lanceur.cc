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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include <iostream>
#include <sstream>

#include <clang-c/Index.h>

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

std::ostream& operator<<(std::ostream& stream, const CXString& str)
{
	stream << clang_getCString(str);
	clang_disposeString(str);
	return stream;
}

static auto morcelle_type(dls::chaine const &str)
{
	auto ret = dls::tableau<dls::chaine>();
	auto taille_mot = 0;
	auto ptr = &str[0];

	for (auto i = 0; i < str.taille(); ++i) {
		if (str[i] == ' ') {
			if (taille_mot != 0) {
				ret.pousse({ ptr, taille_mot });
				taille_mot = 0;
			}
		}
		else if (str[i] == '*') {
			if (taille_mot != 0) {
				ret.pousse({ ptr, taille_mot });
				taille_mot = 0;
			}

			ptr = &str[i];
			ret.pousse({ ptr, 1 });

			taille_mot = 0;
		}
		else if (str[i] == '&') {
			if (taille_mot != 0) {
				ret.pousse({ ptr, taille_mot });
				taille_mot = 0;
			}

			ptr = &str[i];
			ret.pousse({ ptr, 1 });

			taille_mot = 0;
		}
		else {
			if (taille_mot == 0) {
				ptr = &str[i];
			}

			++taille_mot;
		}
	}

	if (taille_mot != 0) {
		ret.pousse({ ptr, taille_mot });
	}

	return ret;
}

static auto converti_type(CXCursor const &c)
{
	auto flux = std::stringstream();

	//CXTypeKind e;

	switch (clang_getCursorType(c).kind) {
		default:
		{
			flux << "(cas défaut) " << clang_getTypeSpelling(clang_getCursorType(c));
			break;
		}
		case CXType_Int:
		{
			flux << "z32";
			break;
		}
		case CXType_UInt:
		{
			flux << "n32";
			break;
		}
		case CXType_Pointer:
		{
			auto flux_tmp = std::stringstream();
			flux_tmp << clang_getTypeSpelling(clang_getCursorType(c));

			auto chn = flux_tmp.str();

			auto morceaux = morcelle_type(chn);

			for (auto &morceau : morceaux) {
				flux << morceau << ' ';
			}

			break;
		}
	}

	return flux.str();
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		std::cerr << "Utilisation " << argv[0] << " FICHIER\n";
		return 1;
	}

	auto chemin = argv[1];

	CXIndex index = clang_createIndex(0, 0);
	CXTranslationUnit unit = clang_parseTranslationUnit(
				index,
				chemin, nullptr, 0,
				nullptr, 0,
				CXTranslationUnit_None);

	if (unit == nullptr) {
		std::cerr << "Unable to parse translation unit. Quitting.\n";
		exit(-1);
	}

	CXCursor cursor = clang_getTranslationUnitCursor(unit);
	clang_visitChildren(
				cursor,
				[](CXCursor c, CXCursor parent, CXClientData client_data)
	{
		switch (c.kind) {
			default:
			{
				std::cout << "Cursor '" << clang_getCursorSpelling(c) << "' of kind '"
						  << clang_getCursorKindSpelling(clang_getCursorKind(c))
						  << "' of type '" << clang_getTypeSpelling(clang_getCursorType(c)) << "'\n";

				break;
			}
			case CXCursorKind::CXCursor_StructDecl:
			{
				std::cout << "struct ";
				std::cout << clang_getCursorSpelling(c);
				std::cout << " {\n";

				break;
			}
			case CXCursorKind::CXCursor_FieldDecl:
			{
				std::cout << "\t";
				std::cout << clang_getCursorSpelling(c);
				std::cout << " : ";
				std::cout << converti_type(c);
				std::cout << ";\n";

				break;
			}
			case CXCursorKind::CXCursor_TypeRef:
			{
				/* ceci semble être pour quand nous utilisons une structure
				 * comme type ? */
				break;
			}
			case CXCursorKind::CXCursor_TypedefDecl:
			{
				/* pas encore supporter dans le langage */
				break;
			}
		}

		return CXChildVisit_Recurse;
	},
	nullptr);

	clang_disposeTranslationUnit(unit);
	clang_disposeIndex(index);

	return 0;
}
