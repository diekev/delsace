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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <clang/AST/OperationKinds.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#pragma GCC diagnostic pop

#include <clang-c/Index.h>

#include "biblinternes/outils/conditions.h"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico_fixe.hh"
#include "biblinternes/structures/pile.hh"
#include "biblinternes/structures/tableau.hh"

using dls::outils::est_element;

/* À FAIRE :
 * - 'auto'
 * - 'template' (FunctionTemplate, ClassTemplate)
 * - 'class' (ClassDecl)
 * - 'new', 'delete'
 * - conversion des types, avec les tailles des tableaux, typedefs
 * - ctors/dtors
 * - les structs et unions anonymes ayant pourtant un typedef ne peuvent être
 *   converties car l'arbre syntactique n'a pas cette l'information à la fin du
 *   typedef => typedef struct { } nom_t; « nom_t » est perdu.
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

static dls::chaine converti_type(CXType const &cxtype)
{
	static auto dico_type = dls::cree_dico(
				dls::paire{ CXType_Void, dls::vue_chaine("rien") },
				dls::paire{ CXType_Bool, dls::vue_chaine("bool") },
				dls::paire{ CXType_Char_U, dls::vue_chaine("n8") },
				dls::paire{ CXType_UChar, dls::vue_chaine("n8") },
				dls::paire{ CXType_UShort, dls::vue_chaine("n16") },
				dls::paire{ CXType_UInt, dls::vue_chaine("n32") },
				dls::paire{ CXType_ULong, dls::vue_chaine("n64") },
				dls::paire{ CXType_ULongLong, dls::vue_chaine("n128") },
				dls::paire{ CXType_Char_S, dls::vue_chaine("z8") },
				dls::paire{ CXType_SChar, dls::vue_chaine("z8") },
				dls::paire{ CXType_Short, dls::vue_chaine("z16") },
				dls::paire{ CXType_Int, dls::vue_chaine("z32") },
				dls::paire{ CXType_Long, dls::vue_chaine("z64") },
				dls::paire{ CXType_LongLong, dls::vue_chaine("z128") },
				dls::paire{ CXType_Float, dls::vue_chaine("r32") },
				dls::paire{ CXType_Double, dls::vue_chaine("r64") },
				dls::paire{ CXType_LongDouble, dls::vue_chaine("r128") });

	auto type = cxtype.kind;

	auto plg_type = dico_type.trouve(type);

	if (!plg_type.est_finie()) {
		return plg_type.front().second;
	}

	static auto dico_type_chn = dls::cree_dico(
				dls::paire{ dls::vue_chaine("void"), dls::vue_chaine("rien") },
				dls::paire{ dls::vue_chaine("bool"), dls::vue_chaine("bool") },
				dls::paire{ dls::vue_chaine("uchar"), dls::vue_chaine("n8") },
				dls::paire{ dls::vue_chaine("ushort"), dls::vue_chaine("n16") },
				dls::paire{ dls::vue_chaine("uint"), dls::vue_chaine("n32") },
				dls::paire{ dls::vue_chaine("ulong"), dls::vue_chaine("n64") },
				dls::paire{ dls::vue_chaine("char"), dls::vue_chaine("z8") },
				dls::paire{ dls::vue_chaine("short"), dls::vue_chaine("z16") },
				dls::paire{ dls::vue_chaine("int"), dls::vue_chaine("z32") },
				dls::paire{ dls::vue_chaine("long"), dls::vue_chaine("z64") },
				dls::paire{ dls::vue_chaine("float"), dls::vue_chaine("r32") },
				dls::paire{ dls::vue_chaine("double"), dls::vue_chaine("r64") });

	auto flux = std::stringstream();

	switch (type) {
		default:
		{
			flux << "(cas défaut) " << type << " : " << clang_getTypeSpelling(cxtype);
			break;
		}
		case CXType_Invalid:
		{
			std::cout << "invalide";
			break;
		}
		case CXType_Typedef:
		{
			// cxtype = clang_getTypedefDeclUnderlyingType(c); // seulement pour les déclarations des typedefs
			// flux << clang_getTypeSpelling(cxtype);
			flux << clang_getTypedefName(cxtype);
			break;
		}
		case CXType_Record:          /* p.e. struct Vecteur */
		case CXType_ConstantArray:   /* p.e. float [4] */
		case CXType_IncompleteArray: /* p.e. float [] */
		case CXType_Pointer:         /* p.e. float * */
		case CXType_LValueReference: /* p.e. float & */
		case CXType_Elaborated:      /* p.e. struct Vecteur */
		{
			auto flux_tmp = std::stringstream();
			flux_tmp << clang_getTypeSpelling(cxtype);

			auto chn = flux_tmp.str();

			auto morceaux = morcelle_type(chn);
			auto pile_morceaux = dls::pile<dls::chaine>();

			for (auto i = 0; i < morceaux.taille(); ++i) {
				auto &morceau = morceaux[i];

				if (morceau == "struct") {
					continue;
				}

				if (morceau == "const") {
					continue;
				}

				if (morceau == "unsigned") {
					if (pile_morceaux.est_vide()) {
						if (i + 1 >= morceaux.taille() - 1) {
							pile_morceaux.empile("uint");
						}
						else {
							auto morceau_suiv = morceaux[i + 1];
							pile_morceaux.empile("u" + morceau_suiv);

							i += 1;
						}
					}
					else {
						auto morceau_prev = pile_morceaux.depile();
						pile_morceaux.empile("u" + morceau_prev);
					}

					continue;
				}

				pile_morceaux.empile(morceau);
			}

			while (!pile_morceaux.est_vide()) {
				auto morceau = pile_morceaux.depile();

				auto plg_type_chn = dico_type_chn.trouve(morceau);

				if (!plg_type_chn.est_finie()) {
					flux << plg_type_chn.front().second;
				}
				else {
					flux << morceau;
				}
			}

			break;
		}
	}

	return flux.str();
}

static dls::chaine converti_type(CXCursor const &c, bool est_fonction = false)
{
	auto cxtype = est_fonction ? clang_getCursorResultType(c) : clang_getCursorType(c);
	return converti_type(cxtype);
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

static inline const clang::Stmt *getCursorStmt(CXCursor c)
{
	auto est_stmt = !est_element(
				c.kind,
				CXCursor_ObjCSuperClassRef,
				CXCursor_ObjCProtocolRef,
				CXCursor_ObjCClassRef);

	return est_stmt ? static_cast<const clang::Stmt *>(c.data[1]) : nullptr;
}

static inline const clang::Expr *getCursorExpr(CXCursor c)
{
	return clang::dyn_cast_or_null<clang::Expr>(getCursorStmt(c));
}

static auto determine_operateur_binaire(
		CXCursor cursor,
		CXTranslationUnit trans_unit,
		std::ostream &os)
{
	/* Méthode tirée de
	 * https://www.mail-archive.com/cfe-commits@cs.uiuc.edu/msg95414.html
	 * https://github.com/llvm-mirror/clang/blob/master/tools/libclang/CXCursor.cpp
	 * https://github.com/pybee/sealang/blob/f4c1b0a9f3203912b6367d8de4ab7508517e60ef/sealang/sealang.cpp
	 */
	auto expr = getCursorExpr(cursor);

	if (expr != nullptr) {
		auto op = clang::cast<clang::BinaryOperator>(expr);
		os << op->getOpcodeStr().str();

		return;
	}

	/* Si la méthode au-dessus échoue, utilise celle-ci tirée de
	 * https://stackoverflow.com/questions/23227812/get-operator-type-for-cxcursor-binaryoperator
	 */

	CXSourceRange range = clang_getCursorExtent(cursor);
	CXToken *tokens = nullptr;
	unsigned nombre_tokens = 0;
	clang_tokenize(trans_unit, range, &tokens, &nombre_tokens);

	for (unsigned i = 0; i < nombre_tokens; i++) {
		auto loc_tok = clang_getTokenLocation(trans_unit, tokens[i]);
		auto loc_cur = clang_getCursorLocation(cursor);

		if (clang_equalLocations(loc_cur, loc_tok) == 0) {
			CXString s = clang_getTokenSpelling(trans_unit, tokens[i]);
			os << s;
			break;
		}
	}

	clang_disposeTokens(trans_unit, tokens, nombre_tokens);
}

static auto determine_operateur_unaire(
		CXCursor cursor,
		CXTranslationUnit trans_unit)
{
	CXSourceRange range = clang_getCursorExtent(cursor);
	CXToken *tokens = nullptr;
	unsigned nombre_tokens = 0;
	clang_tokenize(trans_unit, range, &tokens, &nombre_tokens);

	auto spelling = clang_getTokenSpelling(trans_unit, tokens[0]);

	dls::chaine chn = clang_getCString(spelling);
	clang_disposeString(spelling);

	clang_disposeTokens(trans_unit, tokens, nombre_tokens);

	return chn;
}

//https://stackoverflow.com/questions/10692015/libclang-get-primitive-value
static auto obtiens_litterale(
		CXCursor cursor,
		CXTranslationUnit trans_unit,
		std::ostream &os,
		bool est_bool,
		bool est_float = false)
{
	CXSourceRange range = clang_getCursorExtent(cursor);
	CXToken *tokens = nullptr;
	unsigned nombre_tokens = 0;
	clang_tokenize(trans_unit, range, &tokens, &nombre_tokens);

	if (est_bool) {
		CXString s = clang_getTokenSpelling(trans_unit, tokens[0]);
		const char* str = clang_getCString(s);

		os << ((strcmp(str, "true") == 0) ? "vrai" : "faux");

		clang_disposeString(s);
	}
	else if (est_float) {
		/* il faut se débarasser du 'f' final */
		auto s = clang_getTokenSpelling(trans_unit, tokens[0]);
		const char* str = clang_getCString(s);
		auto len = strlen(str);

		if (str[len - 1] == 'f') {
			len = len - 1;
		}

		for (auto i = 0u; i < len; ++i) {
			os << str[i];
		}

		clang_disposeString(s);
	}
	else {
		os << clang_getTokenSpelling(trans_unit, tokens[0]);
	}

	clang_disposeTokens(trans_unit, tokens, nombre_tokens);
}

struct Convertisseuse {
	int profondeur = 0;
	/* pour les énumérations anonymes */
	int nombre_enums = 0;

	void convertis(CXCursor cursor, CXTranslationUnit trans_unit)
	{
		++profondeur;

		switch (cursor.kind) {
			default:
			{
				std::cout << "Cursor '" << clang_getCursorSpelling(cursor) << "' of kind '"
						  << clang_getCursorKindSpelling(clang_getCursorKind(cursor))
						  << "' of type '" << clang_getTypeSpelling(clang_getCursorType(cursor)) << "'\n";

				break;
			}
			case CXCursorKind::CXCursor_TranslationUnit:
			{
				converti_enfants(cursor, trans_unit);
				break;
			}
			case CXCursorKind::CXCursor_StructDecl:
			{
				auto enfants = rassemble_enfants(cursor);

				/* S'il n'y a pas d'enfants, nous avons une déclaration, donc ignore. */
				if (!enfants.est_vide()) {
					imprime_tab();
					std::cout << "struct ";
					std::cout << clang_getCursorSpelling(cursor);
					std::cout << " {\n";
					converti_enfants(enfants, trans_unit);

					imprime_tab();
					std::cout << "}\n\n";
				}

				break;
			}
			case CXCursorKind::CXCursor_UnionDecl:
			{
				imprime_tab();
				std::cout << "union ";
				std::cout << clang_getCursorSpelling(cursor);
				std::cout << " nonsûr {\n";
				converti_enfants(cursor, trans_unit);

				imprime_tab();
				std::cout << "}\n\n";

				break;
			}
			case CXCursorKind::CXCursor_FieldDecl:
			{
				imprime_tab();
				std::cout << clang_getCursorSpelling(cursor);
				std::cout << " : ";
				std::cout << converti_type(cursor);
				std::cout << '\n';
				break;
			}
			case CXCursorKind::CXCursor_EnumDecl:
			{
				imprime_tab();
				std::cout << "énum ";

				auto str = clang_getCursorSpelling(cursor);
				auto c_str = clang_getCString(str);

				if (strcmp(c_str, "") == 0) {
					std::cout << "anomyme" << nombre_enums++;
				}
				else {
					std::cout << c_str;
				}

				clang_disposeString(str);

				auto type = clang_getEnumDeclIntegerType(cursor);
				std::cout << " : " << converti_type(type);

				std::cout << " {\n";
				converti_enfants(cursor, trans_unit);

				imprime_tab();
				std::cout << "}\n\n";

				break;
			}
			case CXCursorKind::CXCursor_EnumConstantDecl:
			{
				imprime_tab();
				std::cout << clang_getCursorSpelling(cursor);

				auto enfants = rassemble_enfants(cursor);

				if (!enfants.est_vide()) {
					std::cout << " = ";
					converti_enfants(enfants, trans_unit);
				}

				std::cout << ",\n";

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
				converti_declaration_fonction(cursor, trans_unit);
				break;
			}
			case CXCursorKind::CXCursor_TypedefDecl:
			{
				/* pas encore supporter dans le langage */
				break;
			}
			case CXCursorKind::CXCursor_CallExpr:
			{
				std::cout << clang_getCursorSpelling(cursor);

				auto enfants = rassemble_enfants(cursor);
				auto virgule = "(";

				/* le premier enfant est UnexposedExpr pour savoir le type du
				 * pointeur de fonction */
				for (auto i = 1; i < enfants.taille(); ++i) {
					std::cout << virgule;
					convertis(enfants[i], trans_unit);
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
				/* Un DeclStmt peut être :
				 * soit int x = 0;
				 * soit int x = 0, y = 0, z = 0;
				 *
				 * Dans le deuxième cas, la virgule n'est pas considérée comme
				 * un opérateur binaire, et les différentes expressions sont
				 * les filles du DeclStmt. Donc pour proprement tenir en compte
				 * ce cas, on rassemble et converti les enfants en insérant un
				 * point-virgule quand nécessaire.
				 */
				auto enfants = rassemble_enfants(cursor);

				for (auto i = 0; i < enfants.taille(); ++i) {
					convertis(enfants[i], trans_unit);

					if (enfants.taille() > 1 && i < enfants.taille() - 1) {
						std::cout << '\n';
						--profondeur;
						imprime_tab();
						++profondeur;
					}
				}

				break;
			}
			case CXCursorKind::CXCursor_VarDecl:
			{
				auto enfants = rassemble_enfants(cursor);
				auto cxtype = clang_getCursorType(cursor);

				auto nombre_enfants = enfants.taille();
				auto decalage = 0;

				if (cxtype.kind == CXTypeKind::CXType_ConstantArray) {
					/* le premier enfant est la taille du tableau */
					nombre_enfants -= 1;
					decalage += 1;
				}

				if (nombre_enfants == 0) {
					/* nous avons une déclaration simple (int x;) */
					std::cout << "dyn ";
					std::cout << clang_getCursorSpelling(cursor);
					std::cout << " : ";
					std::cout << converti_type(cursor);
				}
				else {
					std::cout << clang_getCursorSpelling(cursor);
					std::cout << " : ";
					std::cout << converti_type(cursor);
					std::cout << " = ";

					for (auto i = decalage; i < enfants.taille(); ++i) {
						convertis(enfants[i], trans_unit);
					}
				}

				break;
			}
			case CXCursorKind::CXCursor_InitListExpr:
			{
				auto enfants = rassemble_enfants(cursor);

				auto virgule = "[ ";

				for (auto enfant : enfants) {
					std::cout << virgule;
					convertis(enfant, trans_unit);
					virgule = ", ";
				}

				std::cout << " ]";
				break;
			}
			case CXCursorKind::CXCursor_ParmDecl:
			{
				/* ne peut pas en avoir à ce niveau */
				break;
			}
			case CXCursorKind::CXCursor_CompoundStmt:
			{
				/* NOTE : un CompoundStmt correspond à un bloc, et peut donc contenir pluseurs
				 * instructions, par exemple :
				 *
				 * int a = 0;
				 * int b = 5;
				 * retourne a + b;
				 *
				 * est un CompoundStmt, et non trois différentes instructions.
				 */

				auto enfants = rassemble_enfants(cursor);

				for (auto enfant : enfants) {
					auto besoin_nouvelle_ligne = est_element(
								enfant.kind,
								CXCursorKind::CXCursor_IfStmt,
								CXCursorKind::CXCursor_WhileStmt,
								CXCursorKind::CXCursor_ForStmt,
								CXCursorKind::CXCursor_DoStmt,
								CXCursorKind::CXCursor_ReturnStmt);

					if (besoin_nouvelle_ligne) {
						std::cout << '\n';
					}

					imprime_tab();
					convertis(enfant, trans_unit);

					std::cout << '\n';
				}

				break;
			}
			case CXCursorKind::CXCursor_IfStmt:
			{
				auto enfants = rassemble_enfants(cursor);

				std::cout << "si ";
				convertis(enfants[0], trans_unit);

				std::cout << " {\n";
				auto non_compound = enfants[1].kind != CXCursorKind::CXCursor_CompoundStmt;

				if (non_compound) {
					imprime_tab();
				}

				convertis(enfants[1], trans_unit);

				if (non_compound) {
					std::cout << '\n';
				}

				--profondeur;
				imprime_tab();
				++profondeur;
				std::cout << "}";

				if (enfants.taille() == 3) {
					std::cout << "\n";
					--profondeur;
					imprime_tab();
					++profondeur;
					std::cout << "sinon ";
					if (enfants[2].kind == CXCursorKind::CXCursor_IfStmt) {
						convertis(enfants[2], trans_unit);
					}
					else {
						std::cout << "{\n";
						non_compound = enfants[2].kind != CXCursorKind::CXCursor_CompoundStmt;

						if (non_compound) {
							imprime_tab();
						}

						convertis(enfants[2], trans_unit);

						if (non_compound) {
							std::cout << '\n';
						}

						--profondeur;
						imprime_tab();
						++profondeur;
						std::cout << "}";
					}
				}

				break;
			}
			case CXCursorKind::CXCursor_WhileStmt:
			{
				auto enfants = rassemble_enfants(cursor);

				std::cout << "tantque ";
				convertis(enfants[0], trans_unit);

				std::cout << " {\n";
				--profondeur;
				convertis(enfants[1], trans_unit);
				imprime_tab();
				++profondeur;

				std::cout << "}";

				break;
			}
			case CXCursorKind::CXCursor_DoStmt:
			{
				auto enfants = rassemble_enfants(cursor);

				std::cout << "répète {\n";
				--profondeur;
				convertis(enfants[0], trans_unit);
				imprime_tab();
				++profondeur;

				std::cout << "} tantque ";
				convertis(enfants[1], trans_unit);

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
				 *			arrête
				 *		}
				 *
				 *		...
				 *		++i;
				 * }
				 */
				auto enfants = rassemble_enfants(cursor);

				/* int i = 0 */
				std::cout << "dyn ";
				convertis(enfants[0], trans_unit);
				std::cout << '\n';

				--profondeur;
				imprime_tab();
				++profondeur;
				std::cout << "boucle {\n";

				/* i < 10 */
				imprime_tab();
				std::cout << "si !(";
				convertis(enfants[1], trans_unit);
				std::cout << ") {\n";
				++profondeur;
				imprime_tab();
				std::cout << "arrête\n";
				--profondeur;

				imprime_tab();
				std::cout << "}\n";

				/* ... */
				convertis(enfants[3], trans_unit);

				/* ++i */
				imprime_tab();
				convertis(enfants[2], trans_unit);
				std::cout << '\n';

				--profondeur;
				imprime_tab();
				++profondeur;
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
				converti_enfants(cursor, trans_unit);
				break;
			}
			case CXCursorKind::CXCursor_ParenExpr:
			{
				std::cout << "(";
				converti_enfants(cursor, trans_unit);
				std::cout << ")";
				break;
			}
			case CXCursorKind::CXCursor_IntegerLiteral:
			case CXCursorKind::CXCursor_CharacterLiteral:
			case CXCursorKind::CXCursor_StringLiteral:
			{
				obtiens_litterale(cursor, trans_unit, std::cout, false);
				break;
			}
			case CXCursorKind::CXCursor_FloatingLiteral:
			{
				obtiens_litterale(cursor, trans_unit, std::cout, false, true);
				break;
			}
			case CXCursorKind::CXCursor_CXXBoolLiteralExpr:
			{
				obtiens_litterale(cursor, trans_unit, std::cout, true);
				break;
			}
			case CXCursorKind::CXCursor_GNUNullExpr:
			case CXCursorKind::CXCursor_CXXNullPtrLiteralExpr:
			{
				std::cout << "nul";
				break;
			}
			case CXCursorKind::CXCursor_ArraySubscriptExpr:
			{
				auto enfants = rassemble_enfants(cursor);
				assert(enfants.taille() == 2);

				convertis(enfants[0], trans_unit);
				std::cout << '[';
				convertis(enfants[1], trans_unit);
				std::cout << ']';

				break;
			}
			case CXCursorKind::CXCursor_MemberRefExpr:
			{
				auto enfants = rassemble_enfants(cursor);
				assert(enfants.taille() == 1);

				convertis(enfants[0], trans_unit);
				std::cout << '.';
				std::cout << clang_getCursorSpelling(cursor);

				break;
			}
			case CXCursorKind::CXCursor_BinaryOperator:
			{
				auto enfants = rassemble_enfants(cursor);
				assert(enfants.taille() == 2);

				convertis(enfants[0], trans_unit);

				std::cout << ' ';
				determine_operateur_binaire(cursor, trans_unit, std::cout);
				std::cout << ' ';

				convertis(enfants[1], trans_unit);

				break;
			}
			case CXCursorKind::CXCursor_UnaryOperator:
			{
				auto enfants = rassemble_enfants(cursor);
				assert(enfants.taille() == 1);

				auto chn = determine_operateur_unaire(cursor, trans_unit);

				if (chn == "++") {
					convertis(enfants[0], trans_unit);
					std::cout << " += 1";
				}
				else if (chn == "--") {
					convertis(enfants[0], trans_unit);
					std::cout << " -= 1";
				}
				else {
					if (chn == "&") {
						std::cout << '@';
					}
					else {
						std::cout << chn;
					}

					convertis(enfants[0], trans_unit);
				}

				break;
			}
			case CXCursorKind::CXCursor_ConditionalOperator:
			{
				auto enfants = rassemble_enfants(cursor);
				assert(enfants.taille() == 3);

				std::cout << "si ";
				convertis(enfants[0], trans_unit);
				std::cout << " { ";
				convertis(enfants[1], trans_unit);
				std::cout << " } sinon { ";
				convertis(enfants[2], trans_unit);
				std::cout << " } ";

				break;
			}
			case CXCursorKind::CXCursor_DeclRefExpr:
			{
				std::cout << clang_getCursorSpelling(cursor);
				break;
			}
			case CXCursorKind::CXCursor_UnexposedExpr:
			{
				converti_enfants(cursor, trans_unit);
				break;
			}
			case CXCursorKind::CXCursor_UnexposedDecl:
			{
				converti_enfants(cursor, trans_unit);
				break;
			}
			case CXCursorKind::CXCursor_CStyleCastExpr:
			{
				auto enfants = rassemble_enfants(cursor);
				assert(enfants.taille() == 1);

				std::cout << "transtype(";
				convertis(enfants[0], trans_unit);
				std::cout << " : " << converti_type(cursor) << ')';

				break;
			}
			case CXCursorKind::CXCursor_CXXFinalAttr:
			case CXCursorKind::CXCursor_CXXOverrideAttr:
			case CXCursorKind::CXCursor_PackedAttr:
			case CXCursorKind::CXCursor_VisibilityAttr:
			{
				/* ignore pour le moment les attributs */
				break;
			}
		}

		--profondeur;
	}

	void imprime_tab()
	{
		for (auto i = 0; i < profondeur - 2; ++i) {
			std::cout << '\t';
		}
	}

	void converti_enfants(CXCursor cursor, CXTranslationUnit trans_unit)
	{
		auto enfants = rassemble_enfants(cursor);
		converti_enfants(enfants, trans_unit);
	}

	void converti_enfants(dls::tableau<CXCursor> const &enfants, CXTranslationUnit trans_unit)
	{
		for (auto enfant : enfants) {
			convertis(enfant, trans_unit);
		}
	}

	void converti_declaration_fonction(CXCursor cursor, CXTranslationUnit trans_unit)
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

		if (clang_Cursor_isFunctionInlined(cursor)) {
			std::cout << "#!enligne ";
		}

		std::cout << "fonc ";
		std::cout << clang_getCursorSpelling(cursor);

		auto virgule = "(";

		for (auto i = 0; i < enfants.taille() - 1; ++i) {
			auto param = enfants[i];

			/* les premiers enfants peuvent être des infos sur la fonctions */
			if (param.kind != CXCursorKind::CXCursor_ParmDecl) {
				continue;
			}

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

		std::cout << ") : " << converti_type(cursor, true) << '\n';

		std::cout << "{\n";

		convertis(enfants.back(), trans_unit);

		std::cout << "}\n\n";
	}
};

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

	auto convertisseuse = Convertisseuse();
	convertisseuse.convertis(cursor, unit);

	clang_disposeTranslationUnit(unit);
	clang_disposeIndex(index);

	return 0;
}
