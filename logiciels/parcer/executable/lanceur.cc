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

#if 1

#include <clang-c/Index.h>

#include "biblinternes/outils/conditions.h"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

/* À FAIRE :
 * - la résolution des opérateurs via determine_operateur_binaire échoue quand
 *   une expression contient des accès '.' ou '[]'
 * - 'auto'
 * - 'template' (FunctionTemplate, ClassTemplate)
 * - 'class' (ClassDecl)
 * - conversion des types, avec les tailles des tableaux, et les types retours
 *   des fonctions
 * - les noeuds correspondants aux tailles des tableaux sont considérés comme
 *   des noeuds dans les expressions (lors des assignements)
 */

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

static auto determine_operateur_unaire(
		CXTranslationUnit tu,
		CXCursor cursor)
{
	CXSourceRange range = clang_getCursorExtent(cursor);
	CXToken *tokens = nullptr;
	unsigned nombre_tokens = 0;
	clang_tokenize(tu, range, &tokens, &nombre_tokens);

	auto spelling = clang_getTokenSpelling(tu, tokens[0]);

	dls::chaine chn = clang_getCString(spelling);
	clang_disposeString(spelling);

	clang_disposeTokens(tu, tokens, nombre_tokens);

	return chn;
}

//https://stackoverflow.com/questions/10692015/libclang-get-primitive-value
static auto obtiens_litterale(
		CXTranslationUnit tu,
		CXCursor cursor,
		std::ostream &os,
		bool est_bool)
{
	CXSourceRange range = clang_getCursorExtent(cursor);
	CXToken *tokens = nullptr;
	unsigned nombre_tokens = 0;
	clang_tokenize(tu, range, &tokens, &nombre_tokens);

	if (est_bool) {
		CXString s = clang_getTokenSpelling(tu, tokens[0]);
		const char* str = clang_getCString(s);

		os << ((strcmp(str, "true") == 0) ? "vrai" : "faux");

		clang_disposeString(s);
	}
	else {
		os << clang_getTokenSpelling(tu, tokens[0]);
	}

	clang_disposeTokens(tu, tokens, nombre_tokens);
}

static void converti_declaration_expression(CXTranslationUnit trans_unit, CXCursor cursor);
static CXChildVisitResult rappel_visite_enfant(CXCursor c, CXCursor parent, CXClientData client_data);

/* NOTE : un CompoundStmt correspond à un bloc, et peut donc contenir pluseurs
 * instructions, par exemple :
 *
 * int a = 0;
 * int b = 5;
 * retourne a + b;
 *
 * est un CompoundStmt, et non trois différentes instructions.
 */
static void converti_compound_stmt(CXCursor c, CXClientData client_data)
{
	using dls::outils::est_element;

	auto enfants = rassemble_enfants(c);

	for (auto enfant : enfants) {
		rappel_visite_enfant(enfant, c, client_data);

		/* il est possible que le point-virgule ne soit pas désiré, par exemple
		 * après le '}' à la fin de certains blocs (if, for, etc.) */
		auto besoin_point_virgule = !est_element(
					enfant.kind,
					CXCursorKind::CXCursor_IfStmt,
					CXCursorKind::CXCursor_WhileStmt,
					CXCursorKind::CXCursor_ForStmt);

		if (besoin_point_virgule) {
			std::cout << ';';
		}

		std::cout << '\n';
	}
}

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
		case CXCursorKind::CXCursor_CallExpr:
		{
			std::cout << clang_getCursorSpelling(c);

			auto enfants = rassemble_enfants(c);
			auto virgule = "(";

			/* le premier enfant est UnexposedExpr pour savoir le type du
			 * pointeur de fonction */
			for (auto i = 1; i < enfants.taille(); ++i) {
				std::cout << virgule;
				rappel_visite_enfant(enfants[i], c, client_data);
				virgule = ", ";
			}

			/* pour les constructeurs implicites, il n'y a pas de premier enfant */
			if (enfants.taille() <= 1) {
				std::cout << '(';
			}

			std::cout << ')';

			break;
		}
		case CXCursorKind::CXCursor_DeclStmt:
		{
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
		case CXCursorKind::CXCursor_IfStmt:
		{
			auto enfants = rassemble_enfants(c);

			std::cout << "si ";
			rappel_visite_enfant(enfants[0], c, client_data);

			std::cout << " {\n";
			converti_compound_stmt(enfants[1], client_data);
			std::cout << "}";

			if (enfants.taille() == 3) {
				std::cout << "\nsinon ";
				if (enfants[2].kind == CXCursorKind::CXCursor_IfStmt) {
					rappel_visite_enfant(enfants[2], c, client_data);
				}
				else {
					std::cout << "{\n";
					converti_compound_stmt(enfants[2], client_data);
					std::cout << "}";
				}
			}

			break;
		}
		case CXCursorKind::CXCursor_WhileStmt:
		{
			auto enfants = rassemble_enfants(c);

			std::cout << "tantque ";
			rappel_visite_enfant(enfants[0], c, client_data);

			std::cout << " {\n";
			converti_compound_stmt(enfants[1], client_data);
			std::cout << "}";

			break;
		}
		case CXCursorKind::CXCursor_DoStmt:
		{
			auto enfants = rassemble_enfants(c);

			std::cout << "répète {\n";
			converti_compound_stmt(enfants[0], client_data);
			std::cout << "} tantque ";
			rappel_visite_enfant(enfants[1], c, client_data);

			break;
		}
		case CXCursorKind::CXCursor_ForStmt:
		{
			/* Transforme :
			 * for (int i = 0; i < 10; ++i) {
			 *		...
			 * }
			 *
			 * en :
			 *
			 * i = 0;
			 *
			 * boucle {
			 *		si i < 10 {
			 *			arrête;
			 *		}
			 *
			 *		...
			 *		++i;
			 * }
			 */
			auto enfants = rassemble_enfants(c);

			/* int i = 0 */
			rappel_visite_enfant(enfants[0], c, client_data);
			std::cout << ";\n";

			std::cout << "boucle {\n";

			/* i < 10 */
			std::cout << "si ";
			rappel_visite_enfant(enfants[1], c, client_data);
			std::cout << " {\narrête;\n}\n";

			/* ... */
			converti_compound_stmt(enfants[3], client_data);

			/* ++i */
			rappel_visite_enfant(enfants[2], c, client_data);
			std::cout << ";\n";

			std::cout << "}";

			break;
		}
		case CXCursorKind::CXCursor_BreakStmt:
		{
			std::cout << "arrête";
			break;
		}
		case CXCursorKind::CXCursor_ContinueStmt:
		{
			std::cout << "continue";
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
			obtiens_litterale(tu, c, std::cout, false);
			break;
		}
		case CXCursorKind::CXCursor_CXXBoolLiteralExpr:
		{
			obtiens_litterale(tu, c, std::cout, true);
			break;
		}
		case CXCursorKind::CXCursor_ArraySubscriptExpr:
		{
			auto enfants = rassemble_enfants(c);
			assert(enfants.taille() == 2);

			rappel_visite_enfant(enfants[0], c, client_data);
			std::cout << '[';
			rappel_visite_enfant(enfants[1], c, client_data);
			std::cout << ']';

			break;
		}
		case CXCursorKind::CXCursor_MemberRefExpr:
		{
			auto enfants = rassemble_enfants(c);
			assert(enfants.taille() == 1);

			rappel_visite_enfant(enfants[0], c, client_data);
			std::cout << '.';
			std::cout << clang_getCursorSpelling(c);

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
		case CXCursorKind::CXCursor_UnaryOperator:
		{
			auto enfants = rassemble_enfants(c);
			assert(enfants.taille() == 1);

			auto chn = determine_operateur_unaire(tu, c);

			if (chn == "++") {
				rappel_visite_enfant(enfants[0], c, client_data);
				std::cout << " += 1";
			}
			else if (chn == "--") {
				rappel_visite_enfant(enfants[0], c, client_data);
				std::cout << " -= 1";
			}
			else {
				std::cout << chn;
				rappel_visite_enfant(enfants[0], c, client_data);
			}

			break;
		}
		case CXCursorKind::CXCursor_DeclRefExpr:
		{
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

static void converti_declaration_fonction(CXTranslationUnit trans_unit, CXCursor cursor)
{
	auto enfants = rassemble_enfants(cursor);

	if (enfants.est_vide()) {
		/* Nous avons une déclaration */
		return;
	}

	if (enfants.back().kind != CXCursorKind::CXCursor_CompoundStmt) {
		/* Nous avons une déclaration */
		return;
	}

	std::cout << "fonc ";
	std::cout << clang_getCursorSpelling(cursor);

	auto virgule = "(";

	for (auto i = 0; i < enfants.taille() - 1; ++i) {
		auto param = enfants[i];

		std::cout << virgule;
		std::cout << clang_getCursorSpelling(param);
		std::cout << " : ";
		std::cout << converti_type(param);

		virgule = ", ";
	}

	/* Il n'y a pas de paramètres. */
	if (enfants.taille() == 1) {
		std::cout << '(';
	}

	std::cout << ") : rien\n";

	std::cout << "{\n";

	converti_compound_stmt(enfants.back(), trans_unit);

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
#else

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "clang/AST/RecursiveASTVisitor.h"

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

class ExampleVisitor : public clang::RecursiveASTVisitor<ExampleVisitor>
{
private:
	clang::ASTContext *astContext;
public:
	ExampleVisitor(ExampleVisitor const &) = default;
	ExampleVisitor &operator=(ExampleVisitor const &) = default;

	virtual ~ExampleVisitor() = default;

	explicit
	ExampleVisitor(clang::CompilerInstance* CI, clang::StringRef file):
		astContext(&(CI->getASTContext()))
	{

	}

	virtual bool VisitTypeDecl(clang::Decl *d)
	{
		d->dump();
		return true;
	}

	bool VisitCXXRecordDecl(clang::CXXRecordDecl *Declaration)
	{
		// For debugging, dumping the AST nodes will show which nodes are already
		// being visited.
	//	Declaration->dump();

		// The return value indicates whether we want the visitation to proceed.
		// Return false to stop the traversal of the AST.
		return true;
	}
};
#pragma GCC diagnostic pop

class ExampleASTConsumer : public clang::ASTConsumer
{
private:
	ExampleVisitor *visitor;
public:

	ExampleASTConsumer(ExampleASTConsumer const &) = default;
	ExampleASTConsumer &operator=(ExampleASTConsumer const &) = default;

	explicit ExampleASTConsumer(clang::CompilerInstance *CI, clang::StringRef file) :
		visitor(new ExampleVisitor(CI,file))
	{}

	~ExampleASTConsumer()
	{
		delete visitor;
	}

	virtual void HandleTranslationUnit(clang::ASTContext &Context) {
		// de cette façon, on applique le visiteur sur l'ensemble de la translation unit
		visitor->TraverseDecl(Context.getTranslationUnitDecl());
	}
};

class ExampleFrontendAction : public clang::ASTFrontendAction {
public:
	virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI, clang::StringRef file)
	{
		return std::make_unique<ExampleASTConsumer>(&CI, file);
	}
};

int main(int argc, const char **argv)
{
	llvm::cl::OptionCategory MyToolCategory("My tool options");

	clang::tooling::CommonOptionsParser op(argc, argv, MyToolCategory);
	clang::tooling::ClangTool Tool(op.getCompilations(), op.getSourcePathList());
	int result = Tool.run(clang::tooling::newFrontendActionFactory<ExampleFrontendAction>().get());
	return result;
}
#endif
