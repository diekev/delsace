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

#include <filesystem>
#include <fstream>
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
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/OperationKinds.h>
#include <clang/AST/Stmt.h>
#pragma GCC diagnostic pop

#include <clang-c/Index.h>

#include "biblinternes/json/json.hh"
#include "biblinternes/outils/conditions.h"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/dico_fixe.hh"
#include "biblinternes/structures/ensemble.hh"
#include "biblinternes/structures/pile.hh"

#include "structures/tableau.hh"

using dls::outils::est_element;

/* À FAIRE :
 * - 'auto'
 * - 'template' (FunctionTemplate, ClassTemplate)
 * - classes : public/protected/private, si supporté dans le langage
 * - les appels de constructeurs ont une paire de parenthèse extra
 * - les structs/unions/enum déclarés avec un typedef n'ont pas le bon type
 *   p.e. typedef struct X { } X_t sera X et non X_t => le petit frère du cursor
 *   possède cette information
 * - assert est mal converti
 * - conversion typage pour les opérateurs 'new' :
 *      int i = new int[a * b];
 *      devient
 *      i : *z32 = loge [a * b]z32
 *      et non
 *      i : []z32 = loge [a * b]z32
 */

std::ostream &operator<<(std::ostream &stream, const CXString &str)
{
    stream << clang_getCString(str);
    clang_disposeString(str);
    return stream;
}

/* En fonction de là où ils apparaissent, les types anonymes sont de la forme :
 * (anonymous at FILE:POS)
 * ou
 * (anonymous {struct|union|enum} at FILE:POS)
 *
 * Donc nous utilions "FILE:POS)" comme « nom » pour les insérer et les trouver
 * dans la liste des typedefs afin de ne pas avoir à ce soucier de la
 * possibilité d'avoir un mot-clé dans la chaine.
 */
static dls::chaine trouve_nom_anonyme(dls::chaine chn)
{
    auto pos_anonymous = chn.trouve("(anonymous");

    if (pos_anonymous == -1) {
        return "";
    }

    auto pos_slash = chn.trouve_premier_de('/');
    return chn.sous_chaine(pos_slash);
}

static auto rassemble_enfants(CXCursor cursor)
{
    auto enfants = kuri::tableau<CXCursor>();

    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData client_data) {
            auto ptr_enfants = static_cast<kuri::tableau<CXCursor> *>(client_data);
            ptr_enfants->ajoute(c);
            return CXChildVisit_Continue;
        },
        &enfants);

    return enfants;
}

void imprime_asa(CXCursor c, int tab, std::ostream &os)
{
    for (auto i = 0; i < tab; ++i) {
        os << ' ' << ' ';
    }

    os << "Cursor '" << clang_getCursorSpelling(c) << "' of kind '"
       << clang_getCursorKindSpelling(clang_getCursorKind(c)) << "' of type '"
       << clang_getTypeSpelling(clang_getCursorType(c)) << "'\n";

    auto enfants = rassemble_enfants(c);

    for (auto enfant : enfants) {
        imprime_asa(enfant, tab + 1, os);
    }
}

static inline const clang::Stmt *getCursorStmt(CXCursor c)
{
    auto est_stmt = !est_element(
        c.kind, CXCursor_ObjCSuperClassRef, CXCursor_ObjCProtocolRef, CXCursor_ObjCClassRef);

    return est_stmt ? static_cast<const clang::Stmt *>(c.data[1]) : nullptr;
}

static inline const clang::Expr *getCursorExpr(CXCursor c)
{
    return clang::dyn_cast_or_null<clang::Expr>(getCursorStmt(c));
}

static auto determine_operateur_binaire(CXCursor cursor,
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

static auto est_operateur_unaire(CXString const &str)
{
    auto c_str = clang_getCString(str);

    const char *operateurs[] = {"+", "-", "++", "--", "!", "&", "~", "*"};

    for (auto op : operateurs) {
        if (strcmp(c_str, op) == 0) {
            return true;
        }
    }

    return false;
}

static auto determine_operateur_unaire(CXCursor cursor, CXTranslationUnit trans_unit)
{
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens = nullptr;
    unsigned nombre_tokens = 0;
    clang_tokenize(trans_unit, range, &tokens, &nombre_tokens);

    if (tokens == nullptr) {
        return dls::chaine();
    }

    auto spelling = clang_getTokenSpelling(trans_unit, tokens[0]);

    /* les opérateurs post-fix sont après */
    if (!est_operateur_unaire(spelling)) {
        clang_disposeString(spelling);

        spelling = clang_getTokenSpelling(trans_unit, tokens[nombre_tokens - 1]);
    }

    dls::chaine chn = clang_getCString(spelling);
    clang_disposeString(spelling);

    clang_disposeTokens(trans_unit, tokens, nombre_tokens);

    return chn;
}

static auto determine_expression_unaire(CXCursor cursor, CXTranslationUnit trans_unit)
{
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens = nullptr;
    unsigned nombre_tokens = 0;
    clang_tokenize(trans_unit, range, &tokens, &nombre_tokens);

    if (tokens == nullptr) {
        return dls::chaine();
    }

    auto spelling = clang_getTokenSpelling(trans_unit, tokens[0]);

    dls::chaine chn = clang_getCString(spelling);
    clang_disposeString(spelling);

    clang_disposeTokens(trans_unit, tokens, nombre_tokens);

    return chn;
}

// https://stackoverflow.com/questions/10692015/libclang-get-primitive-value
static auto obtiens_litterale(CXCursor cursor, CXTranslationUnit trans_unit, std::ostream &os)
{
    auto expr = getCursorExpr(cursor);

    if (expr != nullptr) {
        if (cursor.kind == CXCursorKind::CXCursor_IntegerLiteral) {
            auto op = clang::cast<clang::IntegerLiteral>(expr);
            os << op->getValue().getLimitedValue();
            return;
        }
    }

    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens = nullptr;
    unsigned nombre_tokens = 0;
    clang_tokenize(trans_unit, range, &tokens, &nombre_tokens);

    if (tokens == nullptr) {
        os << clang_getCursorSpelling(cursor);
        return;
    }

    if (cursor.kind == CXCursorKind::CXCursor_CXXBoolLiteralExpr) {
//        CXString s = clang_getTokenSpelling(trans_unit, tokens[0]);
//        const char *str = clang_getCString(s);
//        clang_disposeString(s);
    }
    else if (cursor.kind == CXCursorKind::CXCursor_FloatingLiteral) {
//        /* il faut se débarasser du 'f' final */
//        auto s = clang_getTokenSpelling(trans_unit, tokens[0]);
//        const char *str = clang_getCString(s);
//        clang_disposeString(s);
    }
    else {
        os << clang_getTokenSpelling(trans_unit, tokens[0]);
    }

    clang_disposeTokens(trans_unit, tokens, nombre_tokens);
}

static auto converti_chaine(CXString string)
{
    auto c_str = clang_getCString(string);
    auto chaine = dls::chaine(c_str);
    clang_disposeString(string);
    return chaine;
}

//static auto determine_nom_anomyme(CXCursor cursor, dico_typedefs &typedefs, int &nombre_anonyme)
//{
//    auto spelling = clang_getCursorSpelling(cursor);
//    auto c_str = clang_getCString(spelling);

//    if (strcmp(c_str, "") != 0) {
//        auto chn = dls::chaine(c_str);
//        clang_disposeString(spelling);
//        return chn;
//    }

//    clang_disposeString(spelling);

//    /* le type peut avoir l'information : typedef struct {} TYPE */
//    spelling = clang_getTypeSpelling(clang_getCursorType(cursor));
//    auto chn_spelling = converti_chaine(spelling);

//    if (chn_spelling != "") {
//        auto nom_anonymous = trouve_nom_anonyme(chn_spelling);

//        if (nom_anonymous != "") {
//            auto nom = "anonyme" + dls::vers_chaine(nombre_anonyme++);
//            typedefs.insere({nom_anonymous, {nom}});
//            return nom;
//        }

//        return chn_spelling;
//    }

//    return "anonyme" + dls::vers_chaine(nombre_anonyme++);
//}

static auto compare_token(CXToken token, CXTranslationUnit trans_unit, const char *str)
{
    auto spelling = clang_getTokenSpelling(trans_unit, token);
    auto c_str = clang_getCString(spelling);
    auto ok = strcmp(c_str, str) == 0;
    clang_disposeString(spelling);
    return ok;
}

static auto trouve_decalage(CXToken *tokens,
                            unsigned nombre_tokens,
                            int decalage,
                            CXTranslationUnit trans_unit)
{
    if (compare_token(tokens[decalage], trans_unit, ";")) {
        return 0;
    }

    auto dec = 0;

    for (auto i = decalage; i < nombre_tokens; ++i) {
        if (compare_token(tokens[i], trans_unit, ";")) {
            break;
        }

        dec += 1;
    }

    return dec;
}

//static auto tokens_typedef(CXCursor cursor, CXTranslationUnit trans_unit, dico_typedefs &dico)
//{
//    CXSourceRange range = clang_getCursorExtent(cursor);
//    CXToken *tokens = nullptr;
//    unsigned nombre_tokens = 0;
//    clang_tokenize(trans_unit, range, &tokens, &nombre_tokens);

//    if (tokens == nullptr) {
//        clang_disposeTokens(trans_unit, tokens, nombre_tokens);
//        return;
//    }

//    /* il y a des cas où le token pour typedef se trouve être caché derrière un
//     * define donc la range pointent sur tout le code entre le define et son
//     * utilisation, ce qui peut représenter plusieurs lignes, donc valide le
//     * typedef
//     * À FAIRE : il manque les typedefs pour les structures et union connues
//     */
//    for (auto i = 1u; i < nombre_tokens - 1; ++i) {
//        auto spelling = clang_getTokenSpelling(trans_unit, tokens[i]);

//        auto chn = converti_chaine(spelling);

//        auto est_mot_cle = est_element(chn,
//                                       "struct",
//                                       "enum",
//                                       "union",
//                                       "*",
//                                       "&",
//                                       "unsigned",
//                                       "char",
//                                       "short",
//                                       "int",
//                                       "long",
//                                       "float",
//                                       "double",
//                                       "void",
//                                       ";");

//        if (est_mot_cle) {
//            continue;
//        }

//        if (dico.trouve(chn) != dico.fin()) {
//            continue;
//        }

//        clang_disposeTokens(trans_unit, tokens, nombre_tokens);
//        return;
//    }

//    auto nom = converti_chaine(clang_getTokenSpelling(trans_unit, tokens[nombre_tokens - 1]));
//    auto morceaux = kuri::tableau<dls::chaine>();

//    for (auto i = 1u; i < nombre_tokens - 1; ++i) {
//        auto spelling = clang_getTokenSpelling(trans_unit, tokens[i]);
//        morceaux.ajoute(converti_chaine(spelling));
//    }

//    dico.insere({nom, morceaux});

//    clang_disposeTokens(trans_unit, tokens, nombre_tokens);

//    return;
//}

//static auto tokens_typealias(CXCursor cursor, CXTranslationUnit trans_unit, dico_typedefs &dico)
//{
//    CXSourceRange range = clang_getCursorExtent(cursor);
//    CXToken *tokens = nullptr;
//    unsigned nombre_tokens = 0;
//    clang_tokenize(trans_unit, range, &tokens, &nombre_tokens);

//    if (tokens == nullptr) {
//        clang_disposeTokens(trans_unit, tokens, nombre_tokens);
//        return;
//    }

//    auto nom = converti_chaine(clang_getTokenSpelling(trans_unit, tokens[1]));
//    auto morceaux = kuri::tableau<dls::chaine>();

//    for (auto i = 3u; i < nombre_tokens; ++i) {
//        auto spelling = clang_getTokenSpelling(trans_unit, tokens[i]);
//        morceaux.ajoute(converti_chaine(spelling));
//    }

//    dico.insere({nom, morceaux});

//    clang_disposeTokens(trans_unit, tokens, nombre_tokens);
//}

struct EnfantsBoucleFor {
    CXCursor const *enfant_init = nullptr;
    CXCursor const *enfant_comp = nullptr;
    CXCursor const *enfant_inc = nullptr;
    CXCursor const *enfant_bloc = nullptr;
};

static EnfantsBoucleFor determine_enfants_for(CXCursor cursor,
                                              CXTranslationUnit trans_unit,
                                              kuri::tableau<CXCursor> const &enfants)
{
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens = nullptr;
    unsigned nombre_tokens = 0;
    clang_tokenize(trans_unit, range, &tokens, &nombre_tokens);

    if (tokens == nullptr) {
        return {};
    }

    auto res = EnfantsBoucleFor{};
    auto decalage_enfant = 0;

    auto decalage = 2;

    auto dec = trouve_decalage(tokens, nombre_tokens, decalage, trans_unit);

    if (dec == 0) {
        decalage += 1;
    }
    else {
        res.enfant_init = &enfants[decalage_enfant++];
        decalage += dec + 1;
    }

    dec = trouve_decalage(tokens, nombre_tokens, decalage, trans_unit);

    if (dec == 0) {
        decalage += 1;
    }
    else {
        res.enfant_comp = &enfants[decalage_enfant++];
        decalage += dec + 1;
    }

    if (!compare_token(tokens[decalage], trans_unit, ")")) {
        res.enfant_inc = &enfants[decalage_enfant++];
    }

    clang_disposeTokens(trans_unit, tokens, nombre_tokens);

    res.enfant_bloc = &enfants[decalage_enfant];

    return res;
}

struct Convertisseuse {
    std::filesystem::path fichier_source{};
    std::filesystem::path fichier_entete{};

    int profondeur = 0;
    /* pour les structures, unions, et énumérations anonymes */
    int nombre_anonymes = 0;

    dls::pile<dls::chaine> noms_structure{};

   // dico_typedefs typedefs{};

    dls::ensemble<CXCursorKind> cursors_non_pris_en_charges{};

    auto imprime_commentaire(CXCursor cursor, std::ostream &os)
    {
//        auto comment = clang_Cursor_getBriefCommentText(cursor);
//        auto c_str = clang_getCString(comment);

//        if (c_str != nullptr) {
//            auto chn = dls::chaine(c_str);

//            if (chn != "") {
//                imprime_tab(os);
//                os << "// " << chn << '\n';
//            }
//        }

//        clang_disposeString(comment);
    }

    void convertis(CXCursor cursor, CXTranslationUnit trans_unit)
    {
        ++profondeur;

        switch (cursor.kind) {
            default:
            {
                cursors_non_pris_en_charges.insere(clang_getCursorKind(cursor));

                std::cerr << "Cursor '" << clang_getCursorSpelling(cursor) << "' of kind '"
                            << clang_getCursorKindSpelling(clang_getCursorKind(cursor))
                            << "' of type '" << clang_getTypeSpelling(clang_getCursorType(cursor))
                            << "'\n";

                break;
            }
            case CXCursorKind::CXCursor_Namespace:
            case CXCursorKind::CXCursor_TranslationUnit:
            {
                /* À FAIRE : conversion correcte des espaces de nom, cela
                 * demandera peut-être de savoir comment bien déclarer les
                 * modules et espaces de noms dans Kuri. */
//                auto enfants = rassemble_enfants(cursor);

//                for (auto enfant : enfants) {
//                    auto loc = clang_getCursorLocation(enfant);
//                    CXFile file;
//                    unsigned line;
//                    unsigned column;
//                    unsigned offset;
//                    clang_getExpansionLocation(loc, &file, &line, &column, &offset);

//                    auto nom_fichier = clang_getFileName(file);
//                    auto nom_fichier_c = std::filesystem::path(clang_getCString(nom_fichier));
//                    clang_disposeString(nom_fichier);

//                    if (nom_fichier_c != fichier_source && nom_fichier_c != fichier_entete) {
//                        continue;
//                    }

//                    convertis(enfant, trans_unit, flux_sortie);

//                    /* variable globale */
//                    if (enfant.kind == CXCursorKind::CXCursor_VarDecl) {
//                        flux_sortie << "\n";
//                    }
//                }

                break;
            }
            case CXCursorKind::CXCursor_StructDecl:
            case CXCursorKind::CXCursor_ClassDecl:
            {
//                auto enfants = rassemble_enfants(cursor);

//                auto enfants_filtres = kuri::tableau<CXCursor>();

//                for (auto enfant : enfants) {
//                    if (enfant.kind == CXCursorKind::CXCursor_VisibilityAttr) {
//                        continue;
//                    }

//                    enfants_filtres.ajoute(enfant);
//                }

//                /* S'il n'y a pas d'enfants, nous avons une déclaration, donc ignore. */
//                if (!enfants_filtres.est_vide()) {
//                    imprime_commentaire(cursor, flux_sortie);
//                    converti_enfants(enfants_filtres, trans_unit, flux_sortie);
//                }

                break;
            }
            case CXCursorKind::CXCursor_UnionDecl:
            {
//                imprime_commentaire(cursor, flux_sortie);
//                imprime_tab(flux_sortie);
//                converti_enfants(cursor, trans_unit, flux_sortie);
                break;
            }
            case CXCursorKind::CXCursor_FieldDecl:
            {
//                imprime_commentaire(cursor, flux_sortie);
//                imprime_tab(flux_sortie);
//                //flux_sortie << clang_getCursorSpelling(cursor);
//                flux_sortie << " : ";
//                flux_sortie << converti_type(cursor, typedefs);
//                flux_sortie << '\n';
                break;
            }
            case CXCursorKind::CXCursor_EnumDecl:
            {
//                imprime_commentaire(cursor, flux_sortie);
//                imprime_tab(flux_sortie);
//                flux_sortie << determine_nom_anomyme(cursor, typedefs, nombre_anonymes);

//                auto type = clang_getEnumDeclIntegerType(cursor);
//                flux_sortie << " :: énum " << converti_type(type, typedefs);

//                flux_sortie << " {\n";
//                converti_enfants(cursor, trans_unit, flux_sortie);

//                imprime_tab(flux_sortie);
//                flux_sortie << "}\n\n";

                break;
            }
            case CXCursorKind::CXCursor_EnumConstantDecl:
            {
//                imprime_commentaire(cursor, flux_sortie);
//                imprime_tab(flux_sortie);
//                //flux_sortie << clang_getCursorSpelling(cursor);

//                auto enfants = rassemble_enfants(cursor);

//                if (!enfants.est_vide()) {
//                    flux_sortie << " := ";
//                    converti_enfants(enfants, trans_unit, flux_sortie);
//                }

//                flux_sortie << "\n";

                break;
            }
            case CXCursorKind::CXCursor_TypeRef:
            {
                /* pour les constructeurs entre autres */
 //               flux_sortie << clang_getTypeSpelling(clang_getCursorType(cursor));
                break;
            }
            case CXCursorKind::CXCursor_FunctionDecl:
            {
  //               converti_declaration_fonction(cursor, trans_unit, false, flux_sortie);
                break;
            }
            case CXCursorKind::CXCursor_Constructor:
            case CXCursorKind::CXCursor_Destructor:
            case CXCursorKind::CXCursor_CXXMethod:
            {
 //               converti_declaration_fonction(cursor, trans_unit, true, flux_sortie);
                break;
            }
            case CXCursorKind::CXCursor_CXXThisExpr:
            {
               // flux_sortie << "this";
                break;
            }
            case CXCursorKind::CXCursor_CXXAccessSpecifier:
            {
#if 0
    auto acces = clang_getCXXAccessSpecifier(cursor);

    switch (acces) {
     case CX_CXXInvalidAccessSpecifier:
     {
      break;
     }
     case CX_CXXPublic:
     {
      break;
     }
     case CX_CXXProtected:
     {
      break;
     }
     case CX_CXXPrivate:
     {
      break;
     }
    }
#endif
                break;
            }
            case CXCursorKind::CXCursor_TypedefDecl:
            {
//                tokens_typedef(cursor, trans_unit, typedefs);
                break;
            }
            case CXCursorKind::CXCursor_TypeAliasDecl:
            {
//                tokens_typealias(cursor, trans_unit, typedefs);
                break;
            }
            case CXCursorKind::CXCursor_CallExpr:
            {
//                auto enfants = rassemble_enfants(cursor);

//                /* le premier enfant nous fournis soit le nom de la fonction,
//                 * soit l'expression this.nom si la fonction est appelée depuis
//                 * une méthode de classe */
//                if (enfants.taille() > 0) {
//                    convertis(enfants[0], trans_unit, flux_sortie);
//                }
//                else {
//                    //flux_sortie << clang_getCursorSpelling(cursor);
//                }

//                auto virgule = "(";

//                for (auto i = 1; i < enfants.taille(); ++i) {
//                    flux_sortie << virgule;
//                    convertis(enfants[i], trans_unit, flux_sortie);
//                    virgule = ", ";
//                }

//                /* pour les constructeurs implicites, il n'y a pas de premier enfant */
//                if (enfants.taille() <= 1) {
//                    flux_sortie << '(';
//                }

//                flux_sortie << ')';

                break;
            }
            case CXCursorKind::CXCursor_DeclStmt:
            {
//                imprime_commentaire(cursor, flux_sortie);

//                /* Un DeclStmt peut être :
//                 * soit int x = 0;
//                 * soit int x = 0, y = 0, z = 0;
//                 *
//                 * Dans le deuxième cas, la virgule n'est pas considérée comme
//                 * un opérateur binaire, et les différentes expressions sont
//                 * les filles du DeclStmt. Donc pour proprement tenir en compte
//                 * ce cas, on rassemble et converti les enfants en insérant un
//                 * point-virgule quand nécessaire.
//                 */
//                auto enfants = rassemble_enfants(cursor);

//                for (auto i = 0; i < enfants.taille(); ++i) {
//                    convertis(enfants[i], trans_unit, flux_sortie);

//                    if (enfants.taille() > 1 && i < enfants.taille() - 1) {
//                        flux_sortie << '\n';
//                        --profondeur;
//                        imprime_tab(flux_sortie);
//                        ++profondeur;
//                    }
//                }

                break;
            }
            case CXCursorKind::CXCursor_VarDecl:
            {
//                imprime_commentaire(cursor, flux_sortie);

//                auto enfants = rassemble_enfants(cursor);
//                auto cxtype = clang_getCursorType(cursor);

//                auto nombre_enfants = enfants.taille();
//                auto decalage = 0;

//                if (cxtype.kind == CXTypeKind::CXType_ConstantArray) {
//                    /* le premier enfant est la taille du tableau */
//                    nombre_enfants -= 1;
//                    decalage += 1;
//                }

//                /* les variables déclarées comme étant des pointeurs de
//                 * fonctions ont les types des arguments comme enfants */
//                for (auto const &enfant : enfants) {
//                    switch (enfant.kind) {
//                        default:
//                        {
//                            break;
//                        }
//                        /* pour certaines déclarations dans les codes C, le premier
//                         * enfant semble être une référence vers le type
//                         * (p.e. struct Vecteur) */
//                        case CXCursorKind::CXCursor_TypeRef:
//                        case CXCursorKind::CXCursor_ParmDecl:
//                        case CXCursorKind::CXCursor_VisibilityAttr:
//                        {
//                            decalage += 1;
//                            nombre_enfants -= 1;
//                        }
//                    }
//                }

//                if (nombre_enfants == 0) {
//                    /* nous avons une déclaration simple (int x;) */
//                    flux_sortie << "dyn ";
//                    //flux_sortie << clang_getCursorSpelling(cursor);
//                    flux_sortie << " : ";
//                    flux_sortie << converti_type(cursor, typedefs);
//                }
//                else {
//                    //flux_sortie << clang_getCursorSpelling(cursor);
//                    flux_sortie << " : ";
//                    flux_sortie << converti_type(cursor, typedefs);
//                    flux_sortie << " = ";

//                    for (auto i = decalage; i < enfants.taille(); ++i) {
//                        convertis(enfants[i], trans_unit, flux_sortie);
//                    }
//                }

                break;
            }
            case CXCursorKind::CXCursor_InitListExpr:
            {
//                auto enfants = rassemble_enfants(cursor);

//                auto virgule = "[ ";

//                for (auto enfant : enfants) {
//                    flux_sortie << virgule;
//                    convertis(enfant, trans_unit, flux_sortie);
//                    virgule = ", ";
//                }

//                flux_sortie << " ]";
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

//                auto enfants = rassemble_enfants(cursor);
//                auto debut = true;

//                for (auto enfant : enfants) {
//                    auto besoin_nouvelle_ligne = est_element(enfant.kind,
//                                                             CXCursorKind::CXCursor_IfStmt,
//                                                             CXCursorKind::CXCursor_WhileStmt,
//                                                             CXCursorKind::CXCursor_ForStmt,
//                                                             CXCursorKind::CXCursor_DoStmt,
//                                                             CXCursorKind::CXCursor_ReturnStmt);

//                    if (besoin_nouvelle_ligne && !debut) {
//                        flux_sortie << '\n';
//                    }

//                    debut = false;

//                    imprime_tab(flux_sortie);
//                    convertis(enfant, trans_unit, flux_sortie);

//                    flux_sortie << '\n';
//                }

                break;
            }
            case CXCursorKind::CXCursor_IfStmt:
            {
//                auto enfants = rassemble_enfants(cursor);

//                flux_sortie << "si ";
//                convertis(enfants[0], trans_unit, flux_sortie);

//                flux_sortie << " {\n";
//                auto non_compound = enfants[1].kind != CXCursorKind::CXCursor_CompoundStmt;

//                if (non_compound) {
//                    imprime_tab(flux_sortie);
//                }

//                convertis(enfants[1], trans_unit, flux_sortie);

//                if (non_compound) {
//                    flux_sortie << '\n';
//                }

//                --profondeur;
//                imprime_tab(flux_sortie);
//                ++profondeur;
//                flux_sortie << "}";

//                if (enfants.taille() == 3) {
//                    flux_sortie << "\n";
//                    --profondeur;
//                    imprime_tab(flux_sortie);
//                    ++profondeur;
//                    flux_sortie << "sinon ";
//                    if (enfants[2].kind == CXCursorKind::CXCursor_IfStmt) {
//                        convertis(enfants[2], trans_unit, flux_sortie);
//                    }
//                    else {
//                        flux_sortie << "{\n";
//                        non_compound = enfants[2].kind != CXCursorKind::CXCursor_CompoundStmt;

//                        if (non_compound) {
//                            imprime_tab(flux_sortie);
//                        }

//                        convertis(enfants[2], trans_unit, flux_sortie);

//                        if (non_compound) {
//                            flux_sortie << '\n';
//                        }

//                        --profondeur;
//                        imprime_tab(flux_sortie);
//                        ++profondeur;
//                        flux_sortie << "}";
//                    }
//                }

                break;
            }
            case CXCursorKind::CXCursor_WhileStmt:
            {
//                auto enfants = rassemble_enfants(cursor);

//                flux_sortie << "tantque ";
//                convertis(enfants[0], trans_unit, flux_sortie);

//                flux_sortie << " {\n";
//                --profondeur;
//                convertis(enfants[1], trans_unit, flux_sortie);
//                imprime_tab(flux_sortie);
//                ++profondeur;

//                flux_sortie << "}";

                break;
            }
            case CXCursorKind::CXCursor_DoStmt:
            {
//                auto enfants = rassemble_enfants(cursor);

//                flux_sortie << "répète {\n";
//                --profondeur;
//                convertis(enfants[0], trans_unit, flux_sortie);
//                imprime_tab(flux_sortie);
//                ++profondeur;

//                flux_sortie << "} tantque ";
//                convertis(enfants[1], trans_unit, flux_sortie);

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
                 * i = 0				 *
                 * tantque i < 10 {
                 *		...
                 *		i += 1
                 * }
                 */
//                auto enfants = rassemble_enfants(cursor);
//                auto enfants_for = determine_enfants_for(cursor, trans_unit, enfants);

//                /* int i = 0 */
//                if (enfants_for.enfant_init) {
//                    flux_sortie << "dyn ";
//                    convertis(*enfants_for.enfant_init, trans_unit, flux_sortie);
//                    flux_sortie << '\n';
//                }

//                /* i < 10 */
//                if (enfants_for.enfant_comp) {
//                    --profondeur;
//                    imprime_tab(flux_sortie);
//                    ++profondeur;
//                    flux_sortie << "tantque ";
//                    convertis(*enfants_for.enfant_comp, trans_unit, flux_sortie);
//                    flux_sortie << " {\n";
//                }
//                else {
//                    --profondeur;
//                    imprime_tab(flux_sortie);
//                    ++profondeur;
//                    flux_sortie << "boucle {\n";
//                }

//                /* ... */
//                if (enfants_for.enfant_bloc) {
//                    convertis(*enfants_for.enfant_bloc, trans_unit, flux_sortie);
//                }

//                /* ++i */
//                if (enfants_for.enfant_inc) {
//                    imprime_tab(flux_sortie);
//                    convertis(*enfants_for.enfant_inc, trans_unit, flux_sortie);
//                    flux_sortie << '\n';
//                }

//                --profondeur;
//                imprime_tab(flux_sortie);
//                ++profondeur;
//                flux_sortie << "}";

                break;
            }
            case CXCursorKind::CXCursor_BreakStmt:
            {
                //flux_sortie << "arrête";
                break;
            }
            case CXCursorKind::CXCursor_ContinueStmt:
            {
                //flux_sortie << "continue";
                break;
            }
            case CXCursorKind::CXCursor_ReturnStmt:
            {
                //flux_sortie << "retourne ";
//                converti_enfants(cursor, trans_unit, flux_sortie);
                break;
            }
            case CXCursorKind::CXCursor_SwitchStmt:
            {
                auto enfants = rassemble_enfants(cursor);

                //flux_sortie << "discr ";
                //convertis(enfants[0], trans_unit, flux_sortie);
                //flux_sortie << " {\n";
                //convertis(enfants[1], trans_unit, flux_sortie);

                --profondeur;
//                imprime_tab(flux_sortie);
                ++profondeur;
                //flux_sortie << "}\n";

                break;
            }
            case CXCursorKind::CXCursor_DefaultStmt:
            {
                /* À FAIRE : gestion propre du cas défaut, le langage possède un
                 * 'sinon' qu'il faudra utiliser correctement */
//                converti_enfants(cursor, trans_unit, flux_sortie);
                break;
            }
            case CXCursorKind::CXCursor_CaseStmt:
            {
                /* L'arbre des cas est ainsi :
                 * case
                 *	valeur
                 *	case | bloc
                 *
                 * Le case | bloc dépend de si on a plusieurs cas successif avec
                 * le même bloc, p.e. :
                 *
                 * case 0:
                 * case 1:
                 * case 2:
                 * {
                 *	...
                 * }
                 *
                 * donc nous rassemblons tous les enfants récursivement jusqu'à
                 * ce que nous ayons un bloc, qui sera l'enfant restant
                 */

//                auto enfants = rassemble_enfants(cursor);

//                /* valeur */
//                convertis(enfants[0], trans_unit, flux_sortie);

//                auto cas_similaires = kuri::tableau<CXCursor>();

//                /* case | bloc */
//                auto enfant = enfants[1];

//                while (enfant.kind == CXCursorKind::CXCursor_CaseStmt) {
//                    auto petits_enfants = rassemble_enfants(enfant);
//                    cas_similaires.ajoute(petits_enfants[0]);
//                    enfant = petits_enfants[1];
//                }

//                for (auto const &cas : cas_similaires) {
//                    flux_sortie << ", ";
//                    convertis(cas, trans_unit, flux_sortie);
//                }

//                flux_sortie << " {\n";
//                convertis(enfant, trans_unit, flux_sortie);

//                --profondeur;
//                imprime_tab(flux_sortie);
//                ++profondeur;
//                flux_sortie << "}\n";

                break;
            }
            case CXCursorKind::CXCursor_ParenExpr:
            {
//                flux_sortie << "(";
//                converti_enfants(cursor, trans_unit, flux_sortie);
//                flux_sortie << ")";
                break;
            }
            case CXCursorKind::CXCursor_IntegerLiteral:
            case CXCursorKind::CXCursor_CharacterLiteral:
            case CXCursorKind::CXCursor_StringLiteral:
            case CXCursorKind::CXCursor_FloatingLiteral:
            case CXCursorKind::CXCursor_CXXBoolLiteralExpr:
            {
//                obtiens_litterale(cursor, trans_unit, flux_sortie);
                break;
            }
            case CXCursorKind::CXCursor_GNUNullExpr:
            case CXCursorKind::CXCursor_CXXNullPtrLiteralExpr:
            {
                //flux_sortie << "nul";
                break;
            }
            case CXCursorKind::CXCursor_ArraySubscriptExpr:
            {
//                auto enfants = rassemble_enfants(cursor);
//                assert(enfants.taille() == 2);

//                convertis(enfants[0], trans_unit, flux_sortie);
//                flux_sortie << '[';
//                convertis(enfants[1], trans_unit, flux_sortie);
//                flux_sortie << ']';

                break;
            }
            case CXCursorKind::CXCursor_MemberRefExpr:
            {
//                auto enfants = rassemble_enfants(cursor);

//                if (enfants.taille() == 1) {
//                    convertis(enfants[0], trans_unit, flux_sortie);
//                    flux_sortie << '.';
//                    //flux_sortie << clang_getCursorSpelling(cursor);
//                }
//                else {
//                    /* this implicit */
//                    flux_sortie << "this.";
//                    //flux_sortie << clang_getCursorSpelling(cursor);
//                }

                break;
            }
            case CXCursorKind::CXCursor_BinaryOperator:
            case CXCursorKind::CXCursor_CompoundAssignOperator:
            {
//                auto enfants = rassemble_enfants(cursor);
//                assert(enfants.taille() == 2);

//                convertis(enfants[0], trans_unit, flux_sortie);

//                flux_sortie << ' ';
//                determine_operateur_binaire(cursor, trans_unit, flux_sortie);
//                flux_sortie << ' ';

//                convertis(enfants[1], trans_unit, flux_sortie);

                break;
            }
            case CXCursorKind::CXCursor_UnaryOperator:
            {
//                auto enfants = rassemble_enfants(cursor);
//                assert(enfants.taille() == 1);

//                auto chn = determine_operateur_unaire(cursor, trans_unit);

//                if (chn == "++") {
//                    convertis(enfants[0], trans_unit, flux_sortie);
//                    flux_sortie << " += 1";
//                }
//                else if (chn == "--") {
//                    convertis(enfants[0], trans_unit, flux_sortie);
//                    flux_sortie << " -= 1";
//                }
//                else if (chn == "*") {
//                    flux_sortie << "mémoire(";
//                    convertis(enfants[0], trans_unit, flux_sortie);
//                    flux_sortie << ")";
//                }
//                else {
//                    if (chn == "&") {
//                        flux_sortie << '@';
//                    }
//                    else {
//                        flux_sortie << chn;
//                    }

//                    convertis(enfants[0], trans_unit, flux_sortie);
//                }

                break;
            }
            case CXCursorKind::CXCursor_ConditionalOperator:
            {
//                auto enfants = rassemble_enfants(cursor);
//                assert(enfants.taille() == 3);

//                flux_sortie << "si ";
//                convertis(enfants[0], trans_unit, flux_sortie);
//                flux_sortie << " { ";
//                convertis(enfants[1], trans_unit, flux_sortie);
//                flux_sortie << " } sinon { ";
//                convertis(enfants[2], trans_unit, flux_sortie);
//                flux_sortie << " } ";

                break;
            }
            case CXCursorKind::CXCursor_DeclRefExpr:
            {
                //flux_sortie << clang_getCursorSpelling(cursor);
                break;
            }
            case CXCursorKind::CXCursor_UnexposedExpr:
            {
//                converti_enfants(cursor, trans_unit, flux_sortie);
                break;
            }
            case CXCursorKind::CXCursor_UnexposedDecl:
            {
//                converti_enfants(cursor, trans_unit, flux_sortie);
                break;
            }
            case CXCursorKind::CXCursor_CStyleCastExpr:
            case CXCursorKind::CXCursor_CXXFunctionalCastExpr:
            case CXCursorKind::CXCursor_CXXStaticCastExpr:
            case CXCursorKind::CXCursor_CXXDynamicCastExpr:
            case CXCursorKind::CXCursor_CXXConstCastExpr:
            case CXCursorKind::CXCursor_CXXReinterpretCastExpr:
            {
//                auto enfants = rassemble_enfants(cursor);

//                if (enfants.taille() == 1) {
//                    flux_sortie << "transtype(";
//                    convertis(enfants[0], trans_unit, flux_sortie);
//                    flux_sortie << " : " << converti_type(cursor, typedefs) << ')';
//                }
//                else if (enfants.taille() == 2) {
//                    /* par exemple :
//                     * - static_cast<decltype(a)>(b)
//                     * - (typeof(a))(b)
//                     */
//                    flux_sortie << "transtype(";
//                    convertis(enfants[1], trans_unit, flux_sortie);
//                    flux_sortie << " : " << converti_type(enfants[0], typedefs) << ')';
//                }

                break;
            }
            case CXCursorKind::CXCursor_UnaryExpr:
            {
                auto chn = determine_expression_unaire(cursor, trans_unit);

                if (chn == "sizeof") {
                    //flux_sortie << converti_type_sizeof(cursor, trans_unit, typedefs);
                }
                else {
                    //converti_enfants(cursor, trans_unit, flux_sortie);
                }

                break;
            }
            case CXCursorKind::CXCursor_StmtExpr:
            {
                //converti_enfants(cursor, trans_unit, flux_sortie);
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
            case CXCursorKind::CXCursor_NullStmt:
            {
                /* les lignes ne consistant que d'un ';' */
               // flux_sortie << '\n';
                break;
            }
            case CXCursorKind::CXCursor_CXXNewExpr:
            {
//                auto enfants = rassemble_enfants(cursor);
//                auto cxtype = clang_getCursorType(cursor);

//                flux_sortie << "loge ";

//                if (enfants.est_vide()) {
//                    //flux_sortie << converti_type(cxtype, typedefs, true);
//                }
//                else {
//                    /* tableau */
//                    //convertis(enfants[0], trans_unit);
//                    //flux_sortie << converti_type(cxtype, typedefs, true);
//                }

                break;
            }
            case CXCursorKind::CXCursor_CXXDeleteExpr:
            {
                converti_enfants(cursor, trans_unit);
                break;
            }
            case CXCursorKind::CXCursor_CXXForRangeStmt:
            {
                auto enfants = rassemble_enfants(cursor);

                //flux_sortie << clang_getCursorSpelling(enfants[0]);
                //flux_sortie << clang_getCursorSpelling(enfants[1]);
                //convertis(enfants[2], trans_unit);

                break;
            }
            case CXCursorKind::CXCursor_NamespaceRef:
            {
                //flux_sortie << clang_getCursorSpelling(cursor) << '.';
                break;
            }
            case CXCursorKind::CXCursor_TemplateRef:
            {
                //flux_sortie << clang_getCursorSpelling(cursor) << '.';
                break;
            }
        }

        --profondeur;
    }

    void converti_enfants(CXCursor cursor, CXTranslationUnit trans_unit)
    {
        auto enfants = rassemble_enfants(cursor);
        converti_enfants(enfants, trans_unit);
    }

    void converti_enfants(kuri::tableau<CXCursor> const &enfants,
                          CXTranslationUnit trans_unit)
    {
        for (auto enfant : enfants) {
            convertis(enfant, trans_unit);
        }
    }

    void converti_declaration_fonction(CXCursor cursor,
                                       CXTranslationUnit trans_unit,
                                       bool est_methode_cpp)
    {
        auto enfants = rassemble_enfants(cursor);

        bool est_declaration = enfants.est_vide();
        CXCursor enfant_bloc;

        if (!est_declaration) {
            est_declaration = enfants.derniere().kind != CXCursorKind::CXCursor_CompoundStmt;

            if (!est_declaration) {
                /* Nous n'avons pas une déclaration */
                enfant_bloc = enfants.derniere();
                enfants.supprime_dernier();
            }
        }

        //imprime_commentaire(cursor, flux_sortie);

        if (clang_Cursor_isFunctionInlined(cursor)) {
        }

        //flux_sortie << clang_getCursorSpelling(cursor);

//        auto virgule = "(";

//        for (auto i = 0; i < enfants.taille(); ++i) {
//            auto param = enfants[i];

//            if (est_methode_cpp && param.kind == CXCursorKind::CXCursor_TypeRef) {
//                virgule = ", ";
//                continue;
//            }

//            /* les premiers enfants peuvent être des infos sur la fonctions */
//            if (param.kind != CXCursorKind::CXCursor_ParmDecl) {
//                continue;
//            }

//            //clang_getCursorSpelling(param);

//            virgule = ", ";
//        }

        if (est_declaration) {
            /* Nous avons une déclaration */
            return;
        }

        convertis(enfant_bloc, trans_unit);
    }
};

struct Configuration {
    dls::chaine fichier{};
    dls::chaine fichier_sortie{};
    kuri::tableau<dls::chaine> args{};
    kuri::tableau<dls::chaine> inclusions{};
};

static auto analyse_configuration(const char *chemin)
{
    auto config = Configuration{};
    auto obj = json::compile_script(chemin);

    if (obj == nullptr) {
        std::cerr << "La compilation du script a renvoyé un objet nul !\n";
        return config;
    }

    if (obj->type != tori::type_objet::DICTIONNAIRE) {
        std::cerr << "La compilation du script n'a pas produit de dictionnaire !\n";
        return config;
    }

    auto dico = tori::extrait_dictionnaire(obj.get());

    auto obj_fichier = cherche_chaine(dico, "fichier");

    if (obj_fichier == nullptr) {
        return config;
    }

    config.fichier = obj_fichier->valeur;

    auto obj_args = cherche_tableau(dico, "args");

    if (obj_args != nullptr) {
        for (auto objet : obj_args->valeur) {
            if (objet->type != tori::type_objet::CHAINE) {
                std::cerr << "args : l'objet n'est pas une chaine !\n";
                continue;
            }

            auto obj_chaine = extrait_chaine(objet.get());
            config.args.ajoute(obj_chaine->valeur);
        }
    }

    auto obj_inclusions = cherche_tableau(dico, "inclusions");

    if (obj_inclusions != nullptr) {
        for (auto objet : obj_inclusions->valeur) {
            if (objet->type != tori::type_objet::CHAINE) {
                std::cerr << "inclusions : l'objet n'est pas une chaine !\n";
                continue;
            }

            auto obj_chaine = extrait_chaine(objet.get());
            config.inclusions.ajoute(obj_chaine->valeur);
        }
    }

    auto obj_sortie = cherche_chaine(dico, "sortie");

    if (obj_sortie != nullptr) {
        config.fichier_sortie = obj_sortie->valeur;
    }

    return config;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Utilisation " << argv[0] << " CONFIG.json\n";
        return 1;
    }

    auto config = analyse_configuration(argv[1]);

    if (config.fichier == "") {
        return 1;
    }

    auto args = kuri::tableau<const char *>();

    for (auto const &arg : config.args) {
        args.ajoute(arg.c_str());
    }

    for (auto const &inclusion : config.inclusions) {
        args.ajoute("-I");
        args.ajoute(inclusion.c_str());
    }

    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit unit = clang_parseTranslationUnit(index,
                                                        config.fichier.c_str(),
                                                        args.donnees(),
                                                        static_cast<int>(args.taille()),
                                                        nullptr,
                                                        0,
                                                        CXTranslationUnit_None);

    if (unit == nullptr) {
        std::cerr << "Unable to parse translation unit. Quitting.\n";
        exit(-1);
    }

    auto fichier_source = std::filesystem::path(config.fichier.c_str());
    auto fichier_entete = fichier_source;
    fichier_entete = fichier_entete.replace_extension(".hh");

    if (!std::filesystem::exists(fichier_entete)) {
        fichier_entete = fichier_entete.replace_extension(".h");

        if (!std::filesystem::exists(fichier_entete)) {
            fichier_entete = "";
        }
    }

    auto nombre_diagnostics = clang_getNumDiagnostics(unit);

    if (nombre_diagnostics != 0) {
        std::cerr << "Il y a " << nombre_diagnostics << " diagnostics\n";

        for (auto i = 0u; i < nombre_diagnostics; ++i) {
            auto diag = clang_getDiagnostic(unit, i);
            std::cerr << clang_getDiagnosticSpelling(diag) << '\n';
            clang_disposeDiagnostic(diag);
        }

        exit(-1);
    }

    CXCursor cursor = clang_getTranslationUnitCursor(unit);

    auto convertisseuse = Convertisseuse();
    convertisseuse.fichier_source = fichier_source;
    convertisseuse.fichier_entete = fichier_entete;

    if (config.fichier_sortie != "") {
        std::ofstream fichier(config.fichier_sortie.c_str());
        convertisseuse.convertis(cursor, unit);
    }
    else {
        convertisseuse.convertis(cursor, unit);
    }

    if (convertisseuse.cursors_non_pris_en_charges.taille() != 0) {
        std::cerr << "Les cursors non pris en charges sont :\n";

        for (auto kind : convertisseuse.cursors_non_pris_en_charges) {
            std::cerr << '\t' << clang_getCursorKindSpelling(kind) << " (" << kind << ')' << '\n';
        }
    }

    // imprime_asa(cursor, 0, std::cout);

    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);

    return 0;
}