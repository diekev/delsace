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

static void converti_declaration_struct(CXCursor cursor)
{
	std::cout << "struct ";
	std::cout << clang_getCursorSpelling(cursor);
	std::cout << " {\n";

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
				converti_declaration_struct(c);
				return CXChildVisit_Continue;
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

	std::cout << "}\n";
}

static auto rassemble_enfants(CXCursor cursor)
{
	auto enfants = dls::tableau<CXCursor>();

	clang_visitChildren(
				cursor,
				[](CXCursor c, CXCursor parent, CXClientData client_data)
	{
		auto ptr_enfants = static_cast<dls::tableau<CXCursor> *>(client_data);
		ptr_enfants->pousse(c);
		return CXChildVisit_Continue;
	},
	&enfants);

	return enfants;
}

//https://stackoverflow.com/questions/23227812/get-operator-type-for-cxcursor-binaryoperator
static auto determine_operateur_binaire(
		CXTranslationUnit tu,
		CXCursor cursor,
		std::ostream &os)
{
	CXSourceRange range = clang_getCursorExtent(cursor);
	CXToken *tokens = nullptr;
	unsigned nombre_tokens = 0;
	clang_tokenize(tu, range, &tokens, &nombre_tokens);

	for (unsigned i = 0; i < nombre_tokens; i++) {
		auto loc_tok = clang_getTokenLocation(tu, tokens[i]);
		auto loc_cur = clang_getCursorLocation(cursor);

		if (clang_equalLocations(loc_cur, loc_tok) == 0) {
			CXString s = clang_getTokenSpelling(tu, tokens[i]);
			os << s;
			break;
		}
	}

	clang_disposeTokens(tu, tokens, nombre_tokens);
}

//https://stackoverflow.com/questions/10692015/libclang-get-primitive-value
static auto obtiens_litterale(
		CXTranslationUnit tu,
		CXCursor cursor,
		std::ostream &os)
{
	CXSourceRange range = clang_getCursorExtent(cursor);
	CXToken *tokens = nullptr;
	unsigned nombre_tokens = 0;
	clang_tokenize(tu, range, &tokens, &nombre_tokens);

	os << clang_getTokenSpelling(tu, tokens[0]);

	clang_disposeTokens(tu, tokens, nombre_tokens);
}

static void converti_declaration_expression(CXTranslationUnit trans_unit, CXCursor cursor);

static CXChildVisitResult rappel_visite_enfant(CXCursor c, CXCursor parent, CXClientData client_data)
{
	auto tu = static_cast<CXTranslationUnit>(client_data);

	switch (c.kind) {
		default:
		{
			std::cout << "Cursor '" << clang_getCursorSpelling(c) << "' of kind '"
					  << clang_getCursorKindSpelling(clang_getCursorKind(c))
					  << "' of type '" << clang_getTypeSpelling(clang_getCursorType(c)) << "'\n";

			return CXChildVisit_Recurse;
		}
		case CXCursorKind::CXCursor_DeclStmt:
		{
			//std::cout << "déclaration...\n";
			converti_declaration_expression(tu, c);
			break;
		}
		case CXCursorKind::CXCursor_VarDecl:
		{
			std::cout << clang_getCursorSpelling(c);
			std::cout << " : ";
			std::cout << converti_type(c);
			std::cout << " = ";
			converti_declaration_expression(tu, c);
			break;
		}
		case CXCursorKind::CXCursor_ParmDecl:
		case CXCursorKind::CXCursor_StructDecl:
		case CXCursorKind::CXCursor_FieldDecl:
		case CXCursorKind::CXCursor_TypeRef:
		case CXCursorKind::CXCursor_CompoundStmt:
		case CXCursorKind::CXCursor_TypedefDecl:
		{
			/* ne peut pas en avoir à ce niveau */
			break;
		}
		case CXCursorKind::CXCursor_ReturnStmt:
		{
			std::cout << "retourne ";
			converti_declaration_expression(tu, c);
			break;
		}
		case CXCursorKind::CXCursor_ParenExpr:
		{
			std::cout << "(";
			converti_declaration_expression(tu, c);
			std::cout << ")";
			break;
		}
		case CXCursorKind::CXCursor_IntegerLiteral:
		case CXCursorKind::CXCursor_FloatingLiteral:
		case CXCursorKind::CXCursor_CharacterLiteral:
		case CXCursorKind::CXCursor_StringLiteral:
		{
			obtiens_litterale(tu, c, std::cout);
			break;
		}
		case CXCursorKind::CXCursor_BinaryOperator:
		{
			auto enfants = rassemble_enfants(c);
			assert(enfants.taille() == 2);

			/* NOTE : il nous faut appeler rappel_visite_enfant au lieu de
			 * converti_declaration_expression, car ce dernier visitera les
			 * enfants du curseur passé et non le curseur lui-même. */
			rappel_visite_enfant(enfants[0], c, client_data);

			std::cout << ' ';
			determine_operateur_binaire(tu, c, std::cout);
			std::cout << ' ';

			rappel_visite_enfant(enfants[1], c, client_data);

			break;
		}
		case CXCursorKind::CXCursor_DeclRefExpr:
		{
			/* variable : enfant -> DeclRefExpr */
			std::cout << clang_getCursorSpelling(c);
			break;
		}
		case CXCursorKind::CXCursor_UnexposedExpr:
		{
			converti_declaration_expression(tu, c);
			break;
		}
	}

	return CXChildVisit_Continue;
}

static void converti_declaration_expression(CXTranslationUnit trans_unit, CXCursor cursor)
{
	clang_visitChildren(cursor, rappel_visite_enfant, trans_unit);
}

static auto rassemble_params(CXCursor cursor)
{
	auto enfants = dls::tableau<CXCursor>();

	clang_visitChildren(
				cursor,
				[](CXCursor c, CXCursor parent, CXClientData client_data)
	{
		auto ptr_enfants = static_cast<dls::tableau<CXCursor> *>(client_data);

		if (c.kind != CXCursorKind::CXCursor_ParmDecl) {
			return CXChildVisit_Break;
		}

		ptr_enfants->pousse(c);
		return CXChildVisit_Continue;
	},
	&enfants);

	return enfants;
}

void imprime_asa(CXCursor c, int tab, std::ostream &os)
{
	for (auto i = 0; i < tab; ++ i) {
		std::cout << ' ' << ' ';
	}

	os << "Cursor '" << clang_getCursorSpelling(c) << "' of kind '"
			  << clang_getCursorKindSpelling(clang_getCursorKind(c))
			  << "' of type '" << clang_getTypeSpelling(clang_getCursorType(c)) << "'\n";

	auto enfants = rassemble_enfants(c);

	for (auto enfant : enfants) {
		imprime_asa(enfant, tab + 1, os);
	}
}

static void converti_declaration_fonction(CXTranslationUnit trans_unit, CXCursor cursor)
{
	/* À FAIRE : considère le cas où nous n'avons qu'une déclaration et non une
	 * implémentation, par exemple :
	 *
	 * void foo();
	 * void foo()
	 * {
	 * }
	 *
	 * générera deux fois le code :
	 *
	 * fonc foo() : rien
	 * {
	 * }
	 * fonc foo() : rien
	 * {
	 * }
	 *
	 * au lieu d'une seule fois.
	 */

	std::cout << "fonc ";
	std::cout << clang_getCursorSpelling(cursor);

	// les paramètres sont dans les ParamDecl
	auto params = rassemble_params(cursor);

	auto virgule = "(";

	for (auto param : params) {
		std::cout << virgule;
		std::cout << clang_getCursorSpelling(param);
		std::cout << " : ";
		std::cout << converti_type(param);

		virgule = ", ";
	}

	if (params.est_vide()) {
		std::cout << '(';
	}

	std::cout << ") : rien\n"; // À FAIRE : type fonction

	std::cout << "{\n";

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
				converti_declaration_struct(c);
				break;
			}
			case CXCursorKind::CXCursor_ParmDecl:
			case CXCursorKind::CXCursor_FieldDecl:
			{
				/* ne peut pas en avoir à ce niveau */
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
			case CXCursorKind::CXCursor_CompoundStmt:
			{
				/* NOTE : un CompoundStmt peut contenir pluseurs instructions,
				 * par exemple :
				 * int a = 0;
				 * int b = 5;
				 * retourne a + b;
				 * est un seul, et non trois CompoundStmt
				 */

				//imprime_asa(c, 0, std::cout);

				auto enfants = rassemble_enfants(c);

				for (auto enfant : enfants) {
					rappel_visite_enfant(enfant, c, client_data);
					std::cout << ";\n";
				}

				break;
			}
		}

		return CXChildVisit_Continue;
	},
	trans_unit);

	std::cout << "}\n";
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		std::cerr << "Utilisation " << argv[0] << " FICHIER\n";
		return 1;
	}

	auto chemin = argv[1];

	const char *args[] = {
		"-fparse-all-comments",
	};

	CXIndex index = clang_createIndex(0, 0);
	CXTranslationUnit unit = clang_parseTranslationUnit(
				index,
				chemin,
				args,
				0,
				nullptr,
				0,
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
		auto tu = static_cast<CXTranslationUnit>(client_data);

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
				converti_declaration_struct(c);
				break;
			}
			case CXCursorKind::CXCursor_FieldDecl:
			{
				/* ne peut en avoir à ce niveau là */
				break;
			}
			case CXCursorKind::CXCursor_TypeRef:
			{
				/* ceci semble être pour quand nous utilisons une structure
				 * comme type ? */
				break;
			}
			case CXCursorKind::CXCursor_FunctionDecl:
			{
				converti_declaration_fonction(tu, c);
				break;
			}
			case CXCursorKind::CXCursor_TypedefDecl:
			{
				/* pas encore supporter dans le langage */
				break;
			}
		}

		return CXChildVisit_Continue;
	},
	unit);

	clang_disposeTranslationUnit(unit);
	clang_disposeIndex(index);

	return 0;
}
