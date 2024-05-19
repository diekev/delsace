/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wclass-memaccess"
#    pragma GCC diagnostic ignored "-Wshadow"
#    pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#    pragma GCC diagnostic ignored "-Wold-style-cast"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/OperationKinds.h>
#include <clang/AST/Stmt.h>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include <clang-c/Index.h>

#include "biblinternes/json/json.hh"
#include "biblinternes/outils/conditions.h"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/dico_fixe.hh"

#include "structures/chaine.hh"
#include "structures/chemin_systeme.hh"
#include "structures/enchaineuse.hh"
#include "structures/ensemble.hh"
#include "structures/pile.hh"
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
 *      i : [..]z32 = loge [a * b]z32
 * - gestion correcte des typedefs, notamment pour typedef struct XXX { ... } XXX;
 */

std::ostream &operator<<(std::ostream &stream, const CXString &str)
{
    stream << clang_getCString(str);
    clang_disposeString(str);
    return stream;
}

static auto morcelle_type(dls::chaine const &str)
{
    auto ret = kuri::tableau<dls::chaine>();
    auto taille_mot = 0;
    auto ptr = &str[0];

    for (auto i = 0; i < str.taille(); ++i) {
        if (str[i] == ' ') {
            if (taille_mot != 0) {
                ret.ajoute({ptr, taille_mot});
                taille_mot = 0;
            }
        }
        else if (dls::outils::est_element(str[i], '*', '&', '(', ')', ',')) {
            if (taille_mot != 0) {
                ret.ajoute({ptr, taille_mot});
                taille_mot = 0;
            }

            ptr = &str[i];
            ret.ajoute({ptr, 1});

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
        ret.ajoute({ptr, taille_mot});
    }

    return ret;
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

using dico_typedefs = dls::dico_desordonne<dls::chaine, kuri::tableau<dls::chaine>>;

static dls::chaine converti_type(kuri::tableau<dls::chaine> const &morceaux,
                                 dico_typedefs const &typedefs,
                                 bool dereference = false)
{
    static auto dico_type_chn = dls::cree_dico(
        dls::paire{dls::vue_chaine("void"), dls::vue_chaine("rien")},
        dls::paire{dls::vue_chaine("bool"), dls::vue_chaine("bool")},
        dls::paire{dls::vue_chaine("uchar"), dls::vue_chaine("n8")},
        dls::paire{dls::vue_chaine("ushort"), dls::vue_chaine("n16")},
        dls::paire{dls::vue_chaine("uint"), dls::vue_chaine("n32")},
        dls::paire{dls::vue_chaine("ulong"), dls::vue_chaine("n64")},
        dls::paire{dls::vue_chaine("char"), dls::vue_chaine("z8")},
        dls::paire{dls::vue_chaine("short"), dls::vue_chaine("z16")},
        dls::paire{dls::vue_chaine("int"), dls::vue_chaine("z32")},
        dls::paire{dls::vue_chaine("long"), dls::vue_chaine("z64")},
        // voir autre commentaire dans le fichier, hack pour traduire vers r16
        dls::paire{dls::vue_chaine("r16"), dls::vue_chaine("r16")},
        dls::paire{dls::vue_chaine("float"), dls::vue_chaine("r32")},
        dls::paire{dls::vue_chaine("double"), dls::vue_chaine("r64")});

    auto pile_morceaux = kuri::pile<dls::chaine>();

    for (auto i = 0; i < morceaux.taille(); ++i) {
        auto &morceau = morceaux[i];

        if (morceau == "struct") {
            continue;
        }

        if (morceau == "union") {
            continue;
        }

        if (morceau == "enum") {
            continue;
        }

        if (morceau == "const") {
            continue;
        }

        if (morceau == "signed") {
            continue;
        }

        if (morceau == "unsigned") {
            if (pile_morceaux.est_vide()) {
                if (i >= morceaux.taille() - 1) {
                    pile_morceaux.empile("uint32_t");
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

    if (dereference) {
        pile_morceaux.depile();
    }

    auto flux = std::stringstream();

    while (!pile_morceaux.est_vide()) {
        auto morceau = pile_morceaux.depile();

        auto plg_type_chn = dico_type_chn.trouve(morceau);

        if (!plg_type_chn.est_finie()) {
            flux << plg_type_chn.front().second;
        }
        else {
            auto iter_typedef = typedefs.trouve(morceau);

            if (iter_typedef != typedefs.fin()) {
                flux << converti_type(iter_typedef->second, typedefs, dereference);
            }
            else {
                flux << morceau;
            }
        }
    }

    return flux.str();
}

static dls::chaine converti_type(CXType const &cxtype,
                                 dico_typedefs const &typedefs,
                                 bool dereference = false)
{
    static auto dico_type = dls::cree_dico(dls::paire{CXType_Void, dls::vue_chaine("rien")},
                                           dls::paire{CXType_Bool, dls::vue_chaine("bool")},
                                           dls::paire{CXType_Char_U, dls::vue_chaine("n8")},
                                           dls::paire{CXType_UChar, dls::vue_chaine("n8")},
                                           dls::paire{CXType_UShort, dls::vue_chaine("n16")},
                                           dls::paire{CXType_UInt, dls::vue_chaine("n32")},
                                           dls::paire{CXType_ULong, dls::vue_chaine("n64")},
                                           dls::paire{CXType_ULongLong, dls::vue_chaine("n128")},
                                           dls::paire{CXType_Char_S, dls::vue_chaine("z8")},
                                           dls::paire{CXType_SChar, dls::vue_chaine("z8")},
                                           dls::paire{CXType_Short, dls::vue_chaine("z16")},
                                           dls::paire{CXType_Int, dls::vue_chaine("z32")},
                                           dls::paire{CXType_Long, dls::vue_chaine("z64")},
                                           dls::paire{CXType_LongLong, dls::vue_chaine("z128")},
                                           dls::paire{CXType_Float, dls::vue_chaine("r32")},
                                           dls::paire{CXType_Double, dls::vue_chaine("r64")},
                                           dls::paire{CXType_LongDouble, dls::vue_chaine("r128")});

    auto type = cxtype.kind;

    auto plg_type = dico_type.trouve(type);

    if (!plg_type.est_finie()) {
        return plg_type.front().second;
    }

    auto flux = std::stringstream();

    switch (type) {
        default:
        {
            flux << "(cas défaut) " << type << " : " << clang_getTypeSpelling(cxtype);
            break;
        }
        case CXType_Invalid:
        {
            flux << "invalide";
            break;
        }
        case CXType_Auto:
        case CXType_Enum:
        case CXType_Typedef:
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

            /* pour les types anonymes */
            auto nom_anonymous = trouve_nom_anonyme(chn);

            if (nom_anonymous != "") {
                auto iter_typedefs = typedefs.trouve(nom_anonymous);

                if (iter_typedefs != typedefs.fin()) {
                    return iter_typedefs->second[0];
                }

                return nom_anonymous;
            }

            auto morceaux = morcelle_type(chn);

            if (type == CXTypeKind::CXType_Pointer) {
                /* vérifie s'il y a pointeur de fonction */

                auto est_pointeur_fonc = false;

                for (auto const &m : morceaux) {
                    if (m == "(") {
                        est_pointeur_fonc = true;
                        break;
                    }
                }

                if (est_pointeur_fonc) {
                    auto type_retour = kuri::tableau<dls::chaine>();
                    auto decalage = 0;

                    for (auto i = 0; i < morceaux.taille(); ++i) {
                        auto const &m = morceaux[i];

                        if (m == "(") {
                            break;
                        }

                        type_retour.ajoute(m);
                        decalage++;
                    }

                    flux << " fonc (";

                    auto type_param = kuri::tableau<dls::chaine>();

                    for (auto i = decalage + 4; i < morceaux.taille(); ++i) {
                        auto const &m = morceaux[i];

                        if (m == ")" || m == ",") {
                            flux << converti_type(type_param, typedefs, dereference);
                            flux << m;
                            type_param.efface();
                        }
                        else {
                            type_param.ajoute(m);
                        }
                    }

                    flux << "(";
                    flux << converti_type(type_retour, typedefs, dereference);
                    flux << ")";

                    return flux.str();
                }
            }

            return converti_type(morceaux, typedefs, dereference);
        }
    }

    return flux.str();
}

static dls::chaine converti_type(CXCursor const &c,
                                 dico_typedefs const &typedefs,
                                 bool est_fonction = false)
{
    auto cxtype = est_fonction ? clang_getCursorResultType(c) : clang_getCursorType(c);
    return converti_type(cxtype, typedefs);
}

static dls::chaine converti_type_sizeof(CXCursor cursor,
                                        CXTranslationUnit trans_unit,
                                        dico_typedefs const &typedefs)
{
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens = nullptr;
    unsigned nombre_tokens = 0;
    clang_tokenize(trans_unit, range, &tokens, &nombre_tokens);

    if (tokens == nullptr) {
        clang_disposeTokens(trans_unit, tokens, nombre_tokens);
        return dls::chaine();
    }

    auto donnees_types = kuri::tableau<dls::chaine>();

    for (auto i = 2u; i < nombre_tokens - 1; ++i) {
        auto spelling = clang_getTokenSpelling(trans_unit, tokens[i]);
        auto c_str = clang_getCString(spelling);

        donnees_types.ajoute(c_str);

        clang_disposeString(spelling);
    }

    clang_disposeTokens(trans_unit, tokens, nombre_tokens);

    return converti_type(donnees_types, typedefs);
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
        CXString s = clang_getTokenSpelling(trans_unit, tokens[0]);
        const char *str = clang_getCString(s);

        os << ((strcmp(str, "true") == 0) ? "vrai" : "faux");

        clang_disposeString(s);
    }
    else if (cursor.kind == CXCursorKind::CXCursor_FloatingLiteral) {
        /* il faut se débarasser du 'f' final */
        auto s = clang_getTokenSpelling(trans_unit, tokens[0]);
        const char *str = clang_getCString(s);
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

static auto converti_chaine(CXString string)
{
    auto c_str = clang_getCString(string);
    auto chaine = dls::chaine(c_str);
    clang_disposeString(string);
    return chaine;
}

static auto determine_nom_anomyme(CXCursor cursor, dico_typedefs &typedefs, int &nombre_anonyme)
{
    auto spelling = clang_getCursorSpelling(cursor);
    auto c_str = clang_getCString(spelling);

    if (strcmp(c_str, "") != 0) {
        auto chn = dls::chaine(c_str);
        clang_disposeString(spelling);
        return chn;
    }

    clang_disposeString(spelling);

    /* le type peut avoir l'information : typedef struct {} TYPE */
    spelling = clang_getTypeSpelling(clang_getCursorType(cursor));
    auto chn_spelling = converti_chaine(spelling);

    if (chn_spelling != "") {
        auto nom_anonymous = trouve_nom_anonyme(chn_spelling);

        if (nom_anonymous != "") {
            auto nom = "anonyme" + dls::vers_chaine(nombre_anonyme++);
            kuri::tableau<dls::chaine, int64_t> tabl;
            tabl.ajoute(nom);
            typedefs.insere({nom_anonymous, tabl});
            return nom;
        }

        return chn_spelling;
    }

    return "anonyme" + dls::vers_chaine(nombre_anonyme++);
}

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

template <typename T>
struct TableauStatique {
    const T *donnees = nullptr;
    size_t taille = 0;

  public:
    const T &operator[](size_t index)
    {
        return donnees[index];
    }

    bool est_vide() const
    {
        return !donnees;
    }

    T const *begin() const
    {
        return donnees;
    }

    T const *end() const
    {
        return donnees + taille;
    }
};

static TableauStatique<CXToken> tokenise(CXTranslationUnit trans_unit, CXCursor cursor)
{
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens = nullptr;
    unsigned nombre_tokens = 0;
    clang_tokenize(trans_unit, range, &tokens, &nombre_tokens);

    // À FAIRE clang_disposeTokens(trans_unit, tokens, nombre_tokens);

    return {tokens, static_cast<size_t>(nombre_tokens)};
}

using TypeDonneesType = kuri::tableau<dls::chaine>;
struct TypedefTypeFonction {
    dls::chaine nom_typedef = "";
    TypeDonneesType type_retour{};
    kuri::tableau<TypeDonneesType> type_parametres{};

    void imprime(std::ostream &os, dico_typedefs const &typedefs)
    {
        os << nom_typedef << " :: fonc ";

        auto virgule = "(";

        if (type_parametres.est_vide()) {
            os << virgule;
        }
        else {
            POUR (type_parametres) {
                os << virgule << converti_type(it, typedefs);
                virgule = ", ";
            }
        }

        os << ")";

        os << "(";
        os << converti_type(type_retour, typedefs);
        os << ");\n\n";
    }
};

/* Une parseuse pour comprendre les typedefs.
 * Pour l'instant, ne gère que les typedefs pour les pointeurs de fonctions.
 */
struct ParseuseTypedef {
    CXTranslationUnit m_trans_unit{};
    TableauStatique<CXToken> m_tokens{};

    std::optional<TypedefTypeFonction> parse()
    {
        auto d = m_tokens.begin();
        auto f = m_tokens.end();

        if (apparie(*d, "typedef")) {
            d++;
        }

        return parse_typedef_fonction(d, f);
    }

    std::optional<TypedefTypeFonction> parse_typedef_fonction(const CXToken *d, const CXToken *f)
    {
        auto résultat = TypedefTypeFonction{};

        // d'abord le type de retour
        while (d != f) {
            /* Nous avons le début du nom. */
            if (apparie(*d, "(")) {
                break;
            }

            résultat.type_retour.ajoute(converti_chaine(clang_getTokenSpelling(m_trans_unit, *d)));
            d++;
        }

        /* Retourne si à la fin, ou si le type retour est vide. */
        if (d == f || résultat.type_retour.est_vide()) {
            return {};
        }

        // Vérification
        if (!apparie(*d++, "(")) {
            return {};
        }

        if (!apparie(*d++, "*")) {
            return {};
        }

        résultat.nom_typedef = converti_chaine(clang_getTokenSpelling(m_trans_unit, *d++));

        if (résultat.nom_typedef == "") {
            return {};
        }

        if (!apparie(*d++, ")")) {
            return {};
        }

        /* Paramètres. */
        if (!apparie(*d++, "(")) {
            return {};
        }

        auto type_courant = TypeDonneesType{};

        while (d != f) {
            if (apparie(*d, ")")) {
                if (!type_courant.est_vide()) {
                    résultat.type_parametres.ajoute(type_courant);
                }
                break;
            }

            if (apparie(*d, ",")) {
                if (type_courant.est_vide()) {
                    return {};
                }

                résultat.type_parametres.ajoute(type_courant);
                type_courant = TypeDonneesType{};
                d++;
                continue;
            }

            type_courant.ajoute(converti_chaine(clang_getTokenSpelling(m_trans_unit, *d)));
            d++;
        }

        return résultat;
    }

    bool apparie(CXToken token, const char *chaine)
    {
        auto spelling = clang_getTokenSpelling(m_trans_unit, token);
        return converti_chaine(spelling) == chaine;
    }
};

static auto tokens_typedef(CXCursor cursor,
                           CXTranslationUnit trans_unit,
                           dico_typedefs &dico,
                           std::ostream &flux_sortie)
{
    auto tokens_ = tokenise(trans_unit, cursor);
    auto parseuse = ParseuseTypedef{trans_unit, tokens_};
    auto type_fonction_optionnel = parseuse.parse();

    if (type_fonction_optionnel.has_value()) {
        auto donnees = type_fonction_optionnel.value();
        donnees.imprime(flux_sortie, dico);
        // À FAIRE : meilleure manière de gérer ce cas
        clang_disposeTokens(trans_unit,
                            const_cast<CXToken *>(tokens_.donnees),
                            static_cast<unsigned>(parseuse.m_tokens.taille));
        return;
    }

    // À FAIRE : meilleure manière de gérer ce cas
    clang_disposeTokens(trans_unit,
                        const_cast<CXToken *>(tokens_.donnees),
                        static_cast<unsigned>(parseuse.m_tokens.taille));

    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens = nullptr;
    unsigned nombre_tokens = 0;
    clang_tokenize(trans_unit, range, &tokens, &nombre_tokens);

    if (tokens == nullptr) {
        clang_disposeTokens(trans_unit, tokens, nombre_tokens);
        return;
    }

    /* il y a des cas où le token pour typedef se trouve être caché derrière un
     * define donc la range pointent sur tout le code entre le define et son
     * utilisation, ce qui peut représenter plusieurs lignes, donc valide le
     * typedef
     * À FAIRE : il manque les typedefs pour les structures et union connues
     */
    for (auto i = 1u; i < nombre_tokens - 1; ++i) {
        auto spelling = clang_getTokenSpelling(trans_unit, tokens[i]);

        auto chn = converti_chaine(spelling);

        auto est_mot_cle = est_element(chn,
                                       "struct",
                                       "enum",
                                       "union",
                                       "*",
                                       "&",
                                       "unsigned",
                                       "char",
                                       "short",
                                       "int",
                                       "long",
                                       "float",
                                       "double",
                                       "void",
                                       ";");

        if (est_mot_cle) {
            continue;
        }

        if (dico.trouve(chn) != dico.fin()) {
            continue;
        }

        clang_disposeTokens(trans_unit, tokens, nombre_tokens);
        return;
    }

    auto nom = converti_chaine(clang_getTokenSpelling(trans_unit, tokens[nombre_tokens - 1]));
    auto morceaux = kuri::tableau<dls::chaine>();

    for (auto i = 1u; i < nombre_tokens - 1; ++i) {
        auto spelling = clang_getTokenSpelling(trans_unit, tokens[i]);
        morceaux.ajoute(converti_chaine(spelling));
    }

    dico.insere({nom, morceaux});

    clang_disposeTokens(trans_unit, tokens, nombre_tokens);

    return;
}

static auto tokens_typealias(CXCursor cursor, CXTranslationUnit trans_unit, dico_typedefs &dico)
{
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens = nullptr;
    unsigned nombre_tokens = 0;
    clang_tokenize(trans_unit, range, &tokens, &nombre_tokens);

    if (tokens == nullptr) {
        clang_disposeTokens(trans_unit, tokens, nombre_tokens);
        return;
    }

    auto nom = converti_chaine(clang_getTokenSpelling(trans_unit, tokens[1]));
    auto morceaux = kuri::tableau<dls::chaine>();

    for (auto i = 3u; i < nombre_tokens; ++i) {
        auto spelling = clang_getTokenSpelling(trans_unit, tokens[i]);
        morceaux.ajoute(converti_chaine(spelling));
        std::cerr << morceaux.dernier_élément();
    }
    std::cerr << '\n';

    dico.insere({nom, morceaux});

    clang_disposeTokens(trans_unit, tokens, nombre_tokens);
}

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

static dls::chaine donne_préfixe_valeur_énum(dls::chaine const &nom_énum)
{
    auto résultat = nom_énum + "_";

    for (auto &c : résultat) {
        if (c >= 'a' && c <= 'z') {
            c = c - 'a' + 'A';
        }
    }

    return résultat;
}

static dls::chaine donne_nom_constante_énum_sans_préfixe(dls::chaine const &nom_constante,
                                                         dls::chaine const &préfixe_énum)
{
    if (nom_constante.taille() < préfixe_énum.taille()) {
        return nom_constante;
    }

    auto préfixe_potentiel = nom_constante.sous_chaine(0, préfixe_énum.taille());
    if (préfixe_potentiel != préfixe_énum) {
        return nom_constante;
    }

    auto résultat = nom_constante.sous_chaine(préfixe_potentiel.taille());
    if (résultat.taille() == 0) {
        return nom_constante;
    }

    return résultat;
}

struct Convertisseuse {
    kuri::chemin_systeme fichier_source{};
    kuri::chemin_systeme fichier_entete{};
    kuri::chemin_systeme dossier_source{};

    int profondeur = 0;
    /* pour les structures, unions, et énumérations anonymes */
    int nombre_anonymes = 0;

    kuri::pile<dls::chaine> noms_structure{};

    dico_typedefs typedefs{};

    kuri::ensemble<CXCursorKind> cursors_non_pris_en_charges{};

    kuri::ensemble<kuri::chaine> modules_importes{};

    dls::chaine pour_bibliothèque{};
    kuri::tableau<kuri::chaine> dépendances_biblinternes{};
    kuri::tableau<kuri::chaine> dépendances_qt{};

    dls::chaine m_préfixe_énum_courant{};

    void ajoute_typedef(dls::chaine &&nom_typedef, dls::chaine &&nom_type)
    {
        kuri::tableau<dls::chaine> tabl;
        tabl.ajoute(nom_type);
        typedefs.insere({nom_typedef, tabl});
    }

    auto imprime_commentaire(CXCursor cursor, std::ostream &os)
    {
        auto comment = clang_Cursor_getRawCommentText(cursor);
        auto c_str = clang_getCString(comment);

        if (c_str != nullptr) {
            auto chn = dls::chaine(c_str);

            if (chn != "") {
                imprime_tab(os);
                os << chn << '\n';
            }
        }

        clang_disposeString(comment);
    }

    void convertis(CXTranslationUnit trans_unit, std::ostream &flux_sortie)
    {
        if (pour_bibliothèque != "") {
            flux_sortie << "lib" << pour_bibliothèque << " :: #bibliothèque \""
                        << pour_bibliothèque << "\"\n\n";

            for (auto &dép : dépendances_biblinternes) {
                flux_sortie << "#dépendance_bibliothèque lib" << pour_bibliothèque << " " << dép
                            << "\n";
            }
            if (!dépendances_biblinternes.est_vide()) {
                flux_sortie << "\n";
            }
            for (auto &dép : dépendances_qt) {
                flux_sortie << "libQt5" << dép << " :: #bibliothèque \"Qt5" << dép << "\"\n";
                flux_sortie << "#dépendance_bibliothèque lib" << pour_bibliothèque << " libQt5"
                            << dép << "\n";
            }
            if (!dépendances_qt.est_vide()) {
                flux_sortie << "libQt5" << "Core" << " :: #bibliothèque \"Qt5" << "Core" << "\"\n";
                flux_sortie << "#dépendance_bibliothèque lib" << pour_bibliothèque << " libQt5"
                            << "Core" << "\n";
                flux_sortie << "\n";
                flux_sortie << "libQt5" << "Gui" << " :: #bibliothèque \"Qt5" << "Gui" << "\"\n";
                flux_sortie << "#dépendance_bibliothèque lib" << pour_bibliothèque << " libQt5"
                            << "Gui" << "\n";
                flux_sortie << "\n";
                flux_sortie << "libqt_entetes" << " :: #bibliothèque \"qt_entetes\"\n";
                flux_sortie << "#dépendance_bibliothèque lib" << pour_bibliothèque
                            << " libqt_entetes" << "\n";
                flux_sortie << "\n";
            }
        }

        dossier_source = fichier_entete.chemin_parent();

        CXCursor cursor = clang_getTranslationUnitCursor(trans_unit);
        // imprime_asa(cursor, 0, std::cout);
        convertis(cursor, trans_unit, flux_sortie);
    }

    void convertis(CXCursor cursor, CXTranslationUnit trans_unit, std::ostream &flux_sortie)
    {
        ++profondeur;

        switch (cursor.kind) {
            default:
            {
                cursors_non_pris_en_charges.insère(clang_getCursorKind(cursor));

                flux_sortie << "Cursor '" << clang_getCursorSpelling(cursor) << "' of kind '"
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
                auto enfants = rassemble_enfants(cursor);

                for (auto enfant : enfants) {
                    auto loc = clang_getCursorLocation(enfant);
                    CXFile file;
                    unsigned line;
                    unsigned column;
                    unsigned offset;
                    clang_getExpansionLocation(loc, &file, &line, &column, &offset);

                    auto nom_fichier = clang_getFileName(file);
                    auto nom_fichier_c = kuri::chemin_systeme(clang_getCString(nom_fichier));
                    clang_disposeString(nom_fichier);

                    if (nom_fichier_c.chemin_parent() != kuri::chaine_statique(dossier_source)) {
                        continue;
                    }

                    //  À FAIRE: option pour controler ceci.
                    //                    if (nom_fichier_c != fichier_source && nom_fichier_c !=
                    //                    fichier_entete) {
                    //                        continue;
                    //                    }

                    convertis(enfant, trans_unit, flux_sortie);

                    /* variable globale */
                    if (enfant.kind == CXCursorKind::CXCursor_VarDecl) {
                        flux_sortie << "\n";
                    }
                }

                break;
            }
            case CXCursorKind::CXCursor_StructDecl:
            case CXCursorKind::CXCursor_ClassDecl:
            {
                auto enfants = rassemble_enfants(cursor);

                auto enfants_filtres = kuri::tableau<CXCursor>();

                for (auto enfant : enfants) {
                    if (enfant.kind == CXCursorKind::CXCursor_VisibilityAttr) {
                        continue;
                    }

                    enfants_filtres.ajoute(enfant);
                }

                if (!enfants_filtres.est_vide()) {
                    imprime_commentaire(cursor, flux_sortie);

                    auto nom = determine_nom_anomyme(cursor, typedefs, nombre_anonymes);
                    imprime_tab(flux_sortie);
                    flux_sortie << nom;
                    flux_sortie << " :: struct {\n";

                    noms_structure.empile(nom);

                    converti_enfants(enfants_filtres, trans_unit, flux_sortie);

                    noms_structure.depile();

                    imprime_tab(flux_sortie);
                    flux_sortie << "}\n\n";
                }
                else {
                    imprime_commentaire(cursor, flux_sortie);
                    auto nom = determine_nom_anomyme(cursor, typedefs, nombre_anonymes);
                    // À FAIRE : paramétrise ceci
                    if (nom == "AdaptriceMaillage" || nom == "Interruptrice" ||
                        nom == "ContexteEvaluation") {
                        if (!modules_importes.possède("Géométrie3D")) {
                            flux_sortie << "importe Géométrie3D\n\n";
                            modules_importes.insère("Géométrie3D");
                        }
                    }
                    else if (nom != "ContexteKuri") {
                        imprime_tab(flux_sortie);
                        flux_sortie << nom;
                        flux_sortie << " :: struct #externe;\n\n";
                    }
                }

                break;
            }
            case CXCursorKind::CXCursor_UnionDecl:
            {
                imprime_commentaire(cursor, flux_sortie);
                imprime_tab(flux_sortie);
                flux_sortie << determine_nom_anomyme(cursor, typedefs, nombre_anonymes);
                flux_sortie << " :: union nonsûr {\n";
                converti_enfants(cursor, trans_unit, flux_sortie);

                imprime_tab(flux_sortie);
                flux_sortie << "}\n\n";

                break;
            }
            case CXCursorKind::CXCursor_FieldDecl:
            {
                imprime_commentaire(cursor, flux_sortie);
                imprime_tab(flux_sortie);
                flux_sortie << clang_getCursorSpelling(cursor);
                flux_sortie << " : ";
                flux_sortie << converti_type(cursor, typedefs);
                flux_sortie << '\n';
                break;
            }
            case CXCursorKind::CXCursor_EnumDecl:
            {
                imprime_commentaire(cursor, flux_sortie);
                imprime_tab(flux_sortie);
                auto nom_énum = determine_nom_anomyme(cursor, typedefs, nombre_anonymes);

                auto type = clang_getEnumDeclIntegerType(cursor);
                flux_sortie << nom_énum << " :: énum " << converti_type(type, typedefs);

                flux_sortie << " {\n";
                m_préfixe_énum_courant = donne_préfixe_valeur_énum(nom_énum);
                converti_enfants(cursor, trans_unit, flux_sortie);
                m_préfixe_énum_courant = "";

                imprime_tab(flux_sortie);
                flux_sortie << "}\n\n";

                break;
            }
            case CXCursorKind::CXCursor_EnumConstantDecl:
            {
                imprime_commentaire(cursor, flux_sortie);
                imprime_tab(flux_sortie);

                auto spelling = clang_getCursorSpelling(cursor);
                auto nom_constante = converti_chaine(spelling);
                nom_constante = donne_nom_constante_énum_sans_préfixe(nom_constante,
                                                                      m_préfixe_énum_courant);

                flux_sortie << nom_constante;

                auto enfants = rassemble_enfants(cursor);

                if (!enfants.est_vide()) {
                    flux_sortie << " :: ";
                    converti_enfants(enfants, trans_unit, flux_sortie);
                }

                flux_sortie << "\n";

                break;
            }
            case CXCursorKind::CXCursor_TypeRef:
            {
                /* pour les constructeurs entre autres */
                flux_sortie << clang_getTypeSpelling(clang_getCursorType(cursor));
                break;
            }
            case CXCursorKind::CXCursor_FunctionDecl:
            {
                converti_declaration_fonction(cursor, trans_unit, false, flux_sortie);
                break;
            }
            case CXCursorKind::CXCursor_Constructor:
            case CXCursorKind::CXCursor_Destructor:
            case CXCursorKind::CXCursor_CXXMethod:
            {
                converti_declaration_fonction(cursor, trans_unit, true, flux_sortie);
                break;
            }
            case CXCursorKind::CXCursor_CXXThisExpr:
            {
                flux_sortie << "this";
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
                tokens_typedef(cursor, trans_unit, typedefs, flux_sortie);
                break;
            }
            case CXCursorKind::CXCursor_TypeAliasDecl:
            {
                tokens_typealias(cursor, trans_unit, typedefs);
                break;
            }
            case CXCursorKind::CXCursor_CallExpr:
            {
                auto enfants = rassemble_enfants(cursor);

                /* le premier enfant nous fournis soit le nom de la fonction,
                 * soit l'expression this.nom si la fonction est appelée depuis
                 * une méthode de classe */
                if (enfants.taille() > 0) {
                    convertis(enfants[0], trans_unit, flux_sortie);
                }
                else {
                    flux_sortie << clang_getCursorSpelling(cursor);
                }

                auto virgule = "(";

                for (auto i = 1; i < enfants.taille(); ++i) {
                    flux_sortie << virgule;
                    convertis(enfants[i], trans_unit, flux_sortie);
                    virgule = ", ";
                }

                /* pour les constructeurs implicites, il n'y a pas de premier enfant */
                if (enfants.taille() <= 1) {
                    flux_sortie << '(';
                }

                flux_sortie << ')';

                break;
            }
            case CXCursorKind::CXCursor_DeclStmt:
            {
                imprime_commentaire(cursor, flux_sortie);

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
                    convertis(enfants[i], trans_unit, flux_sortie);

                    if (enfants.taille() > 1 && i < enfants.taille() - 1) {
                        flux_sortie << '\n';
                        --profondeur;
                        imprime_tab(flux_sortie);
                        ++profondeur;
                    }
                }

                break;
            }
            case CXCursorKind::CXCursor_VarDecl:
            {
                imprime_commentaire(cursor, flux_sortie);

                auto enfants = rassemble_enfants(cursor);
                auto cxtype = clang_getCursorType(cursor);

                auto nombre_enfants = enfants.taille();
                auto decalage = 0;

                if (cxtype.kind == CXTypeKind::CXType_ConstantArray) {
                    /* le premier enfant est la taille du tableau */
                    nombre_enfants -= 1;
                    decalage += 1;
                }

                /* les variables déclarées comme étant des pointeurs de
                 * fonctions ont les types des arguments comme enfants */
                for (auto const &enfant : enfants) {
                    switch (enfant.kind) {
                        default:
                        {
                            break;
                        }
                            /* pour certaines déclarations dans les codes C, le premier
                             * enfant semble être une référence vers le type
                             * (p.e. struct Vecteur) */
                        case CXCursorKind::CXCursor_TypeRef:
                        case CXCursorKind::CXCursor_ParmDecl:
                        case CXCursorKind::CXCursor_VisibilityAttr:
                        {
                            decalage += 1;
                            nombre_enfants -= 1;
                        }
                    }
                }

                if (nombre_enfants == 0) {
                    /* nous avons une déclaration simple (int x;) */
                    flux_sortie << clang_getCursorSpelling(cursor);
                    flux_sortie << " : ";
                    flux_sortie << converti_type(cursor, typedefs);
                }
                else {
                    flux_sortie << clang_getCursorSpelling(cursor);
                    flux_sortie << " : ";
                    flux_sortie << converti_type(cursor, typedefs);
                    flux_sortie << " = ";

                    for (auto i = decalage; i < enfants.taille(); ++i) {
                        convertis(enfants[i], trans_unit, flux_sortie);
                    }
                }

                break;
            }
            case CXCursorKind::CXCursor_InitListExpr:
            {
                auto enfants = rassemble_enfants(cursor);

                auto virgule = "[ ";

                for (auto enfant : enfants) {
                    flux_sortie << virgule;
                    convertis(enfant, trans_unit, flux_sortie);
                    virgule = ", ";
                }

                flux_sortie << " ]";
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
                auto debut = true;

                for (auto enfant : enfants) {
                    auto besoin_nouvelle_ligne = est_element(enfant.kind,
                                                             CXCursorKind::CXCursor_IfStmt,
                                                             CXCursorKind::CXCursor_WhileStmt,
                                                             CXCursorKind::CXCursor_ForStmt,
                                                             CXCursorKind::CXCursor_DoStmt,
                                                             CXCursorKind::CXCursor_ReturnStmt);

                    if (besoin_nouvelle_ligne && !debut) {
                        flux_sortie << '\n';
                    }

                    debut = false;

                    imprime_tab(flux_sortie);
                    convertis(enfant, trans_unit, flux_sortie);

                    flux_sortie << '\n';
                }

                break;
            }
            case CXCursorKind::CXCursor_IfStmt:
            {
                auto enfants = rassemble_enfants(cursor);

                flux_sortie << "si ";
                convertis(enfants[0], trans_unit, flux_sortie);

                flux_sortie << " {\n";
                auto non_compound = enfants[1].kind != CXCursorKind::CXCursor_CompoundStmt;

                if (non_compound) {
                    imprime_tab(flux_sortie);
                }

                convertis(enfants[1], trans_unit, flux_sortie);

                if (non_compound) {
                    flux_sortie << '\n';
                }

                --profondeur;
                imprime_tab(flux_sortie);
                ++profondeur;
                flux_sortie << "}";

                if (enfants.taille() == 3) {
                    flux_sortie << "\n";
                    --profondeur;
                    imprime_tab(flux_sortie);
                    ++profondeur;
                    flux_sortie << "sinon ";
                    if (enfants[2].kind == CXCursorKind::CXCursor_IfStmt) {
                        convertis(enfants[2], trans_unit, flux_sortie);
                    }
                    else {
                        flux_sortie << "{\n";
                        non_compound = enfants[2].kind != CXCursorKind::CXCursor_CompoundStmt;

                        if (non_compound) {
                            imprime_tab(flux_sortie);
                        }

                        convertis(enfants[2], trans_unit, flux_sortie);

                        if (non_compound) {
                            flux_sortie << '\n';
                        }

                        --profondeur;
                        imprime_tab(flux_sortie);
                        ++profondeur;
                        flux_sortie << "}";
                    }
                }

                break;
            }
            case CXCursorKind::CXCursor_WhileStmt:
            {
                auto enfants = rassemble_enfants(cursor);

                flux_sortie << "tantque ";
                convertis(enfants[0], trans_unit, flux_sortie);

                flux_sortie << " {\n";
                --profondeur;
                convertis(enfants[1], trans_unit, flux_sortie);
                imprime_tab(flux_sortie);
                ++profondeur;

                flux_sortie << "}";

                break;
            }
            case CXCursorKind::CXCursor_DoStmt:
            {
                auto enfants = rassemble_enfants(cursor);

                flux_sortie << "répète {\n";
                --profondeur;
                convertis(enfants[0], trans_unit, flux_sortie);
                imprime_tab(flux_sortie);
                ++profondeur;

                flux_sortie << "} tantque ";
                convertis(enfants[1], trans_unit, flux_sortie);

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
                auto enfants = rassemble_enfants(cursor);
                auto enfants_for = determine_enfants_for(cursor, trans_unit, enfants);

                /* int i = 0 */
                if (enfants_for.enfant_init) {
                    flux_sortie << "dyn ";
                    convertis(*enfants_for.enfant_init, trans_unit, flux_sortie);
                    flux_sortie << '\n';
                }

                /* i < 10 */
                if (enfants_for.enfant_comp) {
                    --profondeur;
                    imprime_tab(flux_sortie);
                    ++profondeur;
                    flux_sortie << "tantque ";
                    convertis(*enfants_for.enfant_comp, trans_unit, flux_sortie);
                    flux_sortie << " {\n";
                }
                else {
                    --profondeur;
                    imprime_tab(flux_sortie);
                    ++profondeur;
                    flux_sortie << "boucle {\n";
                }

                /* ... */
                if (enfants_for.enfant_bloc) {
                    convertis(*enfants_for.enfant_bloc, trans_unit, flux_sortie);
                }

                /* ++i */
                if (enfants_for.enfant_inc) {
                    imprime_tab(flux_sortie);
                    convertis(*enfants_for.enfant_inc, trans_unit, flux_sortie);
                    flux_sortie << '\n';
                }

                --profondeur;
                imprime_tab(flux_sortie);
                ++profondeur;
                flux_sortie << "}";

                break;
            }
            case CXCursorKind::CXCursor_BreakStmt:
            {
                flux_sortie << "arrête";
                break;
            }
            case CXCursorKind::CXCursor_ContinueStmt:
            {
                flux_sortie << "continue";
                break;
            }
            case CXCursorKind::CXCursor_ReturnStmt:
            {
                flux_sortie << "retourne ";
                converti_enfants(cursor, trans_unit, flux_sortie);
                break;
            }
            case CXCursorKind::CXCursor_SwitchStmt:
            {
                auto enfants = rassemble_enfants(cursor);

                flux_sortie << "discr ";
                convertis(enfants[0], trans_unit, flux_sortie);
                flux_sortie << " {\n";
                convertis(enfants[1], trans_unit, flux_sortie);

                --profondeur;
                imprime_tab(flux_sortie);
                ++profondeur;
                flux_sortie << "}\n";

                break;
            }
            case CXCursorKind::CXCursor_DefaultStmt:
            {
                /* À FAIRE : gestion propre du cas défaut, le langage possède un
                 * 'sinon' qu'il faudra utiliser correctement */
                converti_enfants(cursor, trans_unit, flux_sortie);
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

                auto enfants = rassemble_enfants(cursor);

                /* valeur */
                convertis(enfants[0], trans_unit, flux_sortie);

                auto cas_similaires = kuri::tableau<CXCursor>();

                /* case | bloc */
                auto enfant = enfants[1];

                while (enfant.kind == CXCursorKind::CXCursor_CaseStmt) {
                    auto petits_enfants = rassemble_enfants(enfant);
                    cas_similaires.ajoute(petits_enfants[0]);
                    enfant = petits_enfants[1];
                }

                for (auto const &cas : cas_similaires) {
                    flux_sortie << ", ";
                    convertis(cas, trans_unit, flux_sortie);
                }

                flux_sortie << " {\n";
                convertis(enfant, trans_unit, flux_sortie);

                --profondeur;
                imprime_tab(flux_sortie);
                ++profondeur;
                flux_sortie << "}\n";

                break;
            }
            case CXCursorKind::CXCursor_ParenExpr:
            {
                flux_sortie << "(";
                converti_enfants(cursor, trans_unit, flux_sortie);
                flux_sortie << ")";
                break;
            }
            case CXCursorKind::CXCursor_IntegerLiteral:
            case CXCursorKind::CXCursor_CharacterLiteral:
            case CXCursorKind::CXCursor_StringLiteral:
            case CXCursorKind::CXCursor_FloatingLiteral:
            case CXCursorKind::CXCursor_CXXBoolLiteralExpr:
            {
                obtiens_litterale(cursor, trans_unit, flux_sortie);
                break;
            }
            case CXCursorKind::CXCursor_GNUNullExpr:
            case CXCursorKind::CXCursor_CXXNullPtrLiteralExpr:
            {
                flux_sortie << "nul";
                break;
            }
            case CXCursorKind::CXCursor_ArraySubscriptExpr:
            {
                auto enfants = rassemble_enfants(cursor);
                assert(enfants.taille() == 2);

                convertis(enfants[0], trans_unit, flux_sortie);
                flux_sortie << '[';
                convertis(enfants[1], trans_unit, flux_sortie);
                flux_sortie << ']';

                break;
            }
            case CXCursorKind::CXCursor_MemberRefExpr:
            {
                auto enfants = rassemble_enfants(cursor);

                if (enfants.taille() == 1) {
                    convertis(enfants[0], trans_unit, flux_sortie);
                    flux_sortie << '.';
                    flux_sortie << clang_getCursorSpelling(cursor);
                }
                else {
                    /* this implicit */
                    flux_sortie << "this.";
                    flux_sortie << clang_getCursorSpelling(cursor);
                }

                break;
            }
            case CXCursorKind::CXCursor_BinaryOperator:
            case CXCursorKind::CXCursor_CompoundAssignOperator:
            {
                auto enfants = rassemble_enfants(cursor);
                assert(enfants.taille() == 2);

                convertis(enfants[0], trans_unit, flux_sortie);

                flux_sortie << ' ';
                determine_operateur_binaire(cursor, trans_unit, flux_sortie);
                flux_sortie << ' ';

                convertis(enfants[1], trans_unit, flux_sortie);

                break;
            }
            case CXCursorKind::CXCursor_UnaryOperator:
            {
                auto enfants = rassemble_enfants(cursor);
                assert(enfants.taille() == 1);

                auto chn = determine_operateur_unaire(cursor, trans_unit);

                if (chn == "++") {
                    convertis(enfants[0], trans_unit, flux_sortie);
                    flux_sortie << " += 1";
                }
                else if (chn == "--") {
                    convertis(enfants[0], trans_unit, flux_sortie);
                    flux_sortie << " -= 1";
                }
                else if (chn == "*") {
                    flux_sortie << "mémoire(";
                    convertis(enfants[0], trans_unit, flux_sortie);
                    flux_sortie << ")";
                }
                else {
                    if (chn == "&") {
                        flux_sortie << '@';
                    }
                    else {
                        flux_sortie << chn;
                    }

                    convertis(enfants[0], trans_unit, flux_sortie);
                }

                break;
            }
            case CXCursorKind::CXCursor_ConditionalOperator:
            {
                auto enfants = rassemble_enfants(cursor);
                assert(enfants.taille() == 3);

                flux_sortie << "si ";
                convertis(enfants[0], trans_unit, flux_sortie);
                flux_sortie << " { ";
                convertis(enfants[1], trans_unit, flux_sortie);
                flux_sortie << " } sinon { ";
                convertis(enfants[2], trans_unit, flux_sortie);
                flux_sortie << " } ";

                break;
            }
            case CXCursorKind::CXCursor_DeclRefExpr:
            {
                flux_sortie << clang_getCursorSpelling(cursor);
                break;
            }
            case CXCursorKind::CXCursor_UnexposedExpr:
            {
                converti_enfants(cursor, trans_unit, flux_sortie);
                break;
            }
            case CXCursorKind::CXCursor_UnexposedDecl:
            {
                converti_enfants(cursor, trans_unit, flux_sortie);
                break;
            }
            case CXCursorKind::CXCursor_CStyleCastExpr:
            case CXCursorKind::CXCursor_CXXFunctionalCastExpr:
            case CXCursorKind::CXCursor_CXXStaticCastExpr:
            case CXCursorKind::CXCursor_CXXDynamicCastExpr:
            case CXCursorKind::CXCursor_CXXConstCastExpr:
            case CXCursorKind::CXCursor_CXXReinterpretCastExpr:
            {
                auto enfants = rassemble_enfants(cursor);

                if (enfants.taille() == 1) {
                    flux_sortie << "transtype(";
                    convertis(enfants[0], trans_unit, flux_sortie);
                    flux_sortie << " : " << converti_type(cursor, typedefs) << ')';
                }
                else if (enfants.taille() == 2) {
                    /* par exemple :
                     * - static_cast<decltype(a)>(b)
                     * - (typeof(a))(b)
                     */
                    flux_sortie << "transtype(";
                    convertis(enfants[1], trans_unit, flux_sortie);
                    flux_sortie << " : " << converti_type(enfants[0], typedefs) << ')';
                }

                break;
            }
            case CXCursorKind::CXCursor_UnaryExpr:
            {
                auto chn = determine_expression_unaire(cursor, trans_unit);

                if (chn == "sizeof") {
                    flux_sortie << "taille_de(";
                    flux_sortie << converti_type_sizeof(cursor, trans_unit, typedefs);
                    flux_sortie << ")";
                }
                else {
                    converti_enfants(cursor, trans_unit, flux_sortie);
                }

                break;
            }
            case CXCursorKind::CXCursor_StmtExpr:
            {
                converti_enfants(cursor, trans_unit, flux_sortie);
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
                flux_sortie << '\n';
                break;
            }
            case CXCursorKind::CXCursor_CXXNewExpr:
            {
                auto enfants = rassemble_enfants(cursor);
                auto cxtype = clang_getCursorType(cursor);

                flux_sortie << "loge ";

                if (enfants.est_vide()) {
                    flux_sortie << converti_type(cxtype, typedefs, true);
                }
                else {
                    /* tableau */
                    flux_sortie << '[';
                    convertis(enfants[0], trans_unit, flux_sortie);
                    flux_sortie << ']';
                    flux_sortie << converti_type(cxtype, typedefs, true);
                }

                break;
            }
            case CXCursorKind::CXCursor_CXXDeleteExpr:
            {
                flux_sortie << "déloge ";
                converti_enfants(cursor, trans_unit, flux_sortie);
                break;
            }
            case CXCursorKind::CXCursor_CXXForRangeStmt:
            {
                auto enfants = rassemble_enfants(cursor);

                flux_sortie << "pour ";
                flux_sortie << clang_getCursorSpelling(enfants[0]);
                flux_sortie << " dans ";
                flux_sortie << clang_getCursorSpelling(enfants[1]);
                flux_sortie << " {\n";
                convertis(enfants[2], trans_unit, flux_sortie);
                imprime_tab(flux_sortie);
                flux_sortie << "}\n";

                break;
            }
            case CXCursorKind::CXCursor_NamespaceRef:
            {
                flux_sortie << clang_getCursorSpelling(cursor) << '.';
                break;
            }
            case CXCursorKind::CXCursor_TemplateRef:
            {
                flux_sortie << clang_getCursorSpelling(cursor) << '.';
                break;
            }
        }

        --profondeur;
    }

    void imprime_tab(std::ostream &flux_sortie)
    {
        for (auto i = 0; i < profondeur - 2; ++i) {
            flux_sortie << "    ";
        }
    }

    void converti_enfants(CXCursor cursor, CXTranslationUnit trans_unit, std::ostream &flux_sortie)
    {
        auto enfants = rassemble_enfants(cursor);
        converti_enfants(enfants, trans_unit, flux_sortie);
    }

    void converti_enfants(kuri::tableau<CXCursor> const &enfants,
                          CXTranslationUnit trans_unit,
                          std::ostream &flux_sortie)
    {
        for (auto enfant : enfants) {
            convertis(enfant, trans_unit, flux_sortie);
        }
    }

    void converti_declaration_fonction(CXCursor cursor,
                                       CXTranslationUnit trans_unit,
                                       bool est_methode_cpp,
                                       std::ostream &flux_sortie)
    {
        auto enfants = rassemble_enfants(cursor);

        bool est_declaration = enfants.est_vide();
        CXCursor enfant_bloc;

        if (!est_declaration) {
            est_declaration = enfants.dernier_élément().kind !=
                              CXCursorKind::CXCursor_CompoundStmt;

            if (!est_declaration) {
                /* Nous n'avons pas une déclaration */
                enfant_bloc = enfants.dernier_élément();
                enfants.supprime_dernier();
            }
        }

        imprime_commentaire(cursor, flux_sortie);

        if (clang_Cursor_isFunctionInlined(cursor)) {
            flux_sortie << "#enligne ";
        }

        flux_sortie << clang_getCursorSpelling(cursor);
        flux_sortie << " :: fonc ";

        auto virgule = "(";
        auto nombre_parametres = 0;

        if (!noms_structure.est_vide()) {
            flux_sortie << virgule;
            flux_sortie << "this : *" << noms_structure.haut();
            virgule = ", ";
            ++nombre_parametres;
        }

        for (auto i = 0; i < enfants.taille(); ++i) {
            auto param = enfants[i];

            if (est_methode_cpp && param.kind == CXCursorKind::CXCursor_TypeRef) {
                flux_sortie << virgule;
                flux_sortie << "this : &";
                flux_sortie << converti_type(param, typedefs);
                virgule = ", ";
                ++nombre_parametres;
                continue;
            }

            /* les premiers enfants peuvent être des infos sur la fonctions */
            if (param.kind != CXCursorKind::CXCursor_ParmDecl) {
                continue;
            }

            flux_sortie << virgule;
            flux_sortie << clang_getCursorSpelling(param);
            flux_sortie << " : ";
            flux_sortie << converti_type(param, typedefs);

            virgule = ", ";
            ++nombre_parametres;
        }

        /* Il n'y a pas de paramètres. */
        if (nombre_parametres == 0) {
            flux_sortie << '(';
        }

        flux_sortie << ") -> " << converti_type(cursor, typedefs, true);

        if (est_declaration) {
            /* Nous avons une déclaration */
            flux_sortie << " #externe";
            if (pour_bibliothèque != "") {
                flux_sortie << " lib" << pour_bibliothèque;
            }
            flux_sortie << '\n';
            flux_sortie << '\n';
            return;
        }

        flux_sortie << '\n';

        flux_sortie << "{\n";

        convertis(enfant_bloc, trans_unit, flux_sortie);

        flux_sortie << "}\n\n";
    }
};

struct Configuration {
    dls::chaine fichier{};
    dls::chaine fichier_sortie{};
    kuri::tableau<dls::chaine> args{};
    kuri::tableau<dls::chaine> inclusions{};
    dls::chaine nom_bibliothèque{};
    /* Dépendances sur les bibliothèques internes ; celles installées dans modules/Kuri. */
    kuri::tableau<kuri::chaine> dépendances_biblinternes{};
    /* Dépendances sur les bibliothèques internes ; celles installées dans modules/Kuri. */
    kuri::tableau<kuri::chaine> dépendances_qt{};
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

void imprime_ligne(std::string tampon, uint32_t ligne, uint32_t colonne, uint32_t decalage)
{
    int ligne_courante = 1;
    size_t position_ligne = 0;

    while (ligne_courante != ligne) {
        if (tampon[position_ligne] == '\n') {
            ligne_courante += 1;
        }

        position_ligne += 1;
    }

    size_t position_fin_ligne = position_ligne;

    while (ligne_courante != ligne + 1) {
        if (tampon[position_fin_ligne] == '\n') {
            ligne_courante += 1;
        }

        position_fin_ligne += 1;
    }

    std::cerr << ligne << ":" << colonne << " |"
              << std::string(&tampon[position_ligne], position_fin_ligne - position_ligne);
}

static std::optional<Configuration> valide_configuration(Configuration config)
{
    if (!kuri::chemin_systeme::existe(config.fichier.c_str())) {
        std::cerr << "Le fichier \"" << config.fichier << "\" n'existe pas !\n";
        return {};
    }

    return config;
}

static std::optional<Configuration> crée_config_depuis_json(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Utilisation " << argv[0] << " CONFIG.json\n";
        return {};
    }

    auto config = analyse_configuration(argv[1]);

    if (config.fichier == "") {
        return {};
    }

    return valide_configuration(config);
}

static bool commence_par(kuri::chaine_statique chn1, kuri::chaine_statique chn2)
{
    if (chn1.taille() < chn2.taille()) {
        return false;
    }

    auto sous_chaine = chn1.sous_chaine(0, chn2.taille());
    return sous_chaine == chn2;
}

static std::optional<Configuration> crée_config_pour_metaprogramme(int argc, char **argv)
{
    if (argc < 7) {
        std::cerr
            << "Utilisation " << argv[0]
            << " FICHIER_SOURCE -b NOM_BIBLIOTHEQUE -o FICHIER_SORTIE.kuri -l BIBLIOTHEQUES\n";
        return {};
    }

    auto config = Configuration{};
    config.fichier = argv[1];

    if (std::string(argv[2]) != "-b") {
        std::cerr << "Utilisation " << argv[0]
                  << " FICHIER_SOURCE -b NOM_BIBLIOTHEQUE -o FICHIER_SORTIE.kuri\n";
        std::cerr << "Attendu '-b' après le nom du fichier d'entrée !\n";
        return {};
    }

    if (std::string(argv[4]) != "-o") {
        std::cerr << "Utilisation " << argv[0]
                  << " FICHIER_SOURCE -b NOM_BIBLIOTHEQUE -o FICHIER_SORTIE.kuri\n";
        std::cerr << "Attendu '-o' après le nom de la bibliothèque !\n";
        return {};
    }

    if (std::string(argv[6]) != "-l") {
        std::cerr << "Utilisation " << argv[0]
                  << " FICHIER_SOURCE -b NOM_BIBLIOTHEQUE -o FICHIER_SORTIE.kuri\n";
        std::cerr << "Attendu '-l' après le nom du fichier !\n";
        return {};
    }

    config.nom_bibliothèque = argv[3];
    config.fichier_sortie = argv[5];

    /* À FAIRE : termine ceci. */
#if 0
    for (int i = 7; i < argc; i++) {
        auto chn = kuri::chaine(argv[i]);
        if (commence_par(chn, "dls::")) {
            chn = chn.sous_chaine(5);

            if (chn == "algorithmes_image") {
                continue;
            }

            if (chn == "memoire") {
                chn = "mémoire";
            }

            config.dépendances_biblinternes.ajoute(enchaine("libdls_", chn));

            //    std::cerr << "biblinterne : " << chn << '\n';

            // std::cerr << "lib" << chn << " :: #bibliothèque \"bib_" << chn << "\"\n";
            //            std::cerr << "#dépendance_bibliothèque " << config.nom_bibliothèque << "
            //            lib" << chn
            //                      << "\n";
        }
        else if (commence_par(chn, "Qt5::")) {
            // chn = chn.sous_chaine(5);
            // config.dépendances_qt.ajoute(chn);
        }
        else {
            std::cerr << argv[i] << '\n';
        }
    }
#endif

    return valide_configuration(config);
}

static kuri::tableau<const char *> parse_arguments_depuis_config(Configuration const &config)
{
    auto args = kuri::tableau<const char *>();

    for (auto const &arg : config.args) {
        args.ajoute(arg.c_str());
    }

    for (auto const &inclusion : config.inclusions) {
        args.ajoute("-I");
        args.ajoute(inclusion.c_str());
    }

    return args;
}

int main(int argc, char **argv)
{
    auto config_optionnelle = crée_config_pour_metaprogramme(argc, argv);

    if (!config_optionnelle.has_value()) {
        return 1;
    }

    auto config = config_optionnelle.value();
    auto args = parse_arguments_depuis_config(config);

    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit unit = clang_parseTranslationUnit(
        index,
        config.fichier.c_str(),
        args.données(),
        static_cast<int>(args.taille()),
        nullptr,
        0,
        CXTranslationUnit_None | CXTranslationUnit_IncludeBriefCommentsInCodeCompletion);

    if (unit == nullptr) {
        std::cerr << "Unable to parse translation unit. Quitting.\n";
        exit(-1);
    }

    auto fichier_source = kuri::chemin_systeme(config.fichier.c_str());
    auto fichier_entete = fichier_source;
    fichier_entete = fichier_entete.remplace_extension(".hh");

    if (!kuri::chemin_systeme::existe(fichier_entete)) {
        fichier_entete = fichier_entete.remplace_extension(".h");

        if (!kuri::chemin_systeme::existe(fichier_entete)) {
            fichier_entete = "";
        }
    }

    auto nombre_diagnostics = clang_getNumDiagnostics(unit);

    if (nombre_diagnostics != 0) {
        std::cerr << "Il y a " << nombre_diagnostics << " diagnostics\n";

        for (auto i = 0u; i < nombre_diagnostics; ++i) {
            auto diag = clang_getDiagnostic(unit, i);
            std::cerr << clang_getDiagnosticSpelling(diag) << '\n';

            auto loc = clang_getDiagnosticLocation(diag);

            CXFile file = clang_getFile(unit, config.fichier.c_str());

            size_t taille = 0;
            auto tampon = clang_getFileContents(unit, file, &taille);

            uint32_t ligne = 0;
            uint32_t colonne = 0;
            uint32_t decalage = 0;
            auto filename = clang_getFileName(file);
            std::cerr << filename << ":\n";
            clang_getExpansionLocation(loc, &file, &ligne, &colonne, &decalage);
            imprime_ligne(std::string(tampon, taille), ligne, colonne, decalage);
            clang_disposeDiagnostic(diag);
        }

        exit(-1);
    }

    auto convertisseuse = Convertisseuse();
    convertisseuse.fichier_source = fichier_source;
    convertisseuse.fichier_entete = fichier_entete;
    convertisseuse.pour_bibliothèque = config.nom_bibliothèque;
    convertisseuse.dépendances_biblinternes = config.dépendances_biblinternes;
    convertisseuse.dépendances_qt = config.dépendances_qt;
    convertisseuse.ajoute_typedef("size_t", "ulong");
    convertisseuse.ajoute_typedef("std::size_t", "ulong");
    convertisseuse.ajoute_typedef("uint8_t", "uchar");
    convertisseuse.ajoute_typedef("uint16_t", "ushort");
    convertisseuse.ajoute_typedef("uint32_t", "uint");
    convertisseuse.ajoute_typedef("uint64_t", "ulong");
    convertisseuse.ajoute_typedef("int8_t", "char");
    convertisseuse.ajoute_typedef("int16_t", "short");
    convertisseuse.ajoute_typedef("int32_t", "int");
    convertisseuse.ajoute_typedef("int64_t", "long");
    /* Hack afin de convertir les types half vers notre langage, ceci empêche d'y ajouter les
     * typedefs devant être utilisés afin de faire compiler le code C puisque ni half ni r16
     * n'existent en C. */
    convertisseuse.ajoute_typedef("r16", "r16");
    convertisseuse.ajoute_typedef("half", "r16");

    if (config.fichier_sortie != "") {
        std::ofstream fichier(config.fichier_sortie.c_str());
        convertisseuse.convertis(unit, fichier);
    }
    else {
        convertisseuse.convertis(unit, std::cerr);
    }

    if (convertisseuse.cursors_non_pris_en_charges.taille() != 0) {
        std::cerr << "Les cursors non pris en charges sont :\n";

        convertisseuse.cursors_non_pris_en_charges.pour_chaque_element([&](auto kind) {
            std::cerr << '\t' << clang_getCursorKindSpelling(kind) << " (" << kind << ')' << '\n';
        });
    }

    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);

    return 0;
}
