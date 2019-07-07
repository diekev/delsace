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

 /* Ce fichier est généré automatiquement. NE PAS ÉDITER ! */
 
#include "morceaux.hh"

#include "biblinternes/structures/dico.hh"

static dls::dico<dls::vue_chaine, id_morceau> paires_mots_cles = {
	{ "alignas", id_morceau::ALIGNAS },
	{ "alignof", id_morceau::ALIGNOF },
	{ "and", id_morceau::AND },
	{ "and_eq", id_morceau::AND_EQ },
	{ "asm", id_morceau::ASM },
	{ "atomic_cancel", id_morceau::ATOMIC_CANCEL },
	{ "atomic_commit", id_morceau::ATOMIC_COMMIT },
	{ "atomic_noexcept", id_morceau::ATOMIC_NOEXCEPT },
	{ "audit", id_morceau::AUDIT },
	{ "auto", id_morceau::AUTO },
	{ "axiom", id_morceau::AXIOM },
	{ "bitand", id_morceau::BITAND },
	{ "bitor", id_morceau::BITOR },
	{ "bool", id_morceau::BOOL },
	{ "break", id_morceau::BREAK },
	{ "case", id_morceau::CASE },
	{ "catch", id_morceau::CATCH },
	{ "char", id_morceau::CHAR },
	{ "char16_t", id_morceau::CHAR16_T },
	{ "char32_t", id_morceau::CHAR32_T },
	{ "char8_t", id_morceau::CHAR8_T },
	{ "class", id_morceau::CLASS },
	{ "co_await", id_morceau::CO_AWAIT },
	{ "co_return", id_morceau::CO_RETURN },
	{ "co_yield", id_morceau::CO_YIELD },
	{ "compl", id_morceau::COMPL },
	{ "concept", id_morceau::CONCEPT },
	{ "const", id_morceau::CONST },
	{ "const_cast", id_morceau::CONST_CAST },
	{ "consteval", id_morceau::CONSTEVAL },
	{ "constexpr", id_morceau::CONSTEXPR },
	{ "continue", id_morceau::CONTINUE },
	{ "decltype", id_morceau::DECLTYPE },
	{ "default", id_morceau::DEFAULT },
	{ "delete", id_morceau::DELETE },
	{ "do", id_morceau::DO },
	{ "double", id_morceau::DOUBLE },
	{ "dynamic_cast", id_morceau::DYNAMIC_CAST },
	{ "else", id_morceau::ELSE },
	{ "enum", id_morceau::ENUM },
	{ "explicit", id_morceau::EXPLICIT },
	{ "export", id_morceau::EXPORT },
	{ "extern", id_morceau::EXTERN },
	{ "false", id_morceau::FALSE },
	{ "final", id_morceau::FINAL },
	{ "float", id_morceau::FLOAT },
	{ "for", id_morceau::FOR },
	{ "friend", id_morceau::FRIEND },
	{ "goto", id_morceau::GOTO },
	{ "if", id_morceau::IF },
	{ "import", id_morceau::IMPORT },
	{ "inline", id_morceau::INLINE },
	{ "int", id_morceau::INT },
	{ "long", id_morceau::LONG },
	{ "module", id_morceau::MODULE },
	{ "mutable", id_morceau::MUTABLE },
	{ "namespace", id_morceau::NAMESPACE },
	{ "new", id_morceau::NEW },
	{ "noexcept", id_morceau::NOEXCEPT },
	{ "not", id_morceau::NOT },
	{ "not_eq", id_morceau::NOT_EQ },
	{ "nullptr", id_morceau::NULLPTR },
	{ "operator", id_morceau::OPERATOR },
	{ "or", id_morceau::OR },
	{ "or_eq", id_morceau::OR_EQ },
	{ "override", id_morceau::OVERRIDE },
	{ "private", id_morceau::PRIVATE },
	{ "protected", id_morceau::PROTECTED },
	{ "public", id_morceau::PUBLIC },
	{ "reflexpr", id_morceau::REFLEXPR },
	{ "register", id_morceau::REGISTER },
	{ "reinterpret_cast", id_morceau::REINTERPRET_CAST },
	{ "requires", id_morceau::REQUIRES },
	{ "return", id_morceau::RETURN },
	{ "short", id_morceau::SHORT },
	{ "signed", id_morceau::SIGNED },
	{ "sizeof", id_morceau::SIZEOF },
	{ "static", id_morceau::STATIC },
	{ "static_assert", id_morceau::STATIC_ASSERT },
	{ "static_cast", id_morceau::STATIC_CAST },
	{ "struct", id_morceau::STRUCT },
	{ "switch", id_morceau::SWITCH },
	{ "synchronized", id_morceau::SYNCHRONIZED },
	{ "template", id_morceau::TEMPLATE },
	{ "this", id_morceau::THIS },
	{ "thread_local", id_morceau::THREAD_LOCAL },
	{ "throw", id_morceau::THROW },
	{ "transaction_safe", id_morceau::TRANSACTION_SAFE },
	{ "transaction_safe_dynamic", id_morceau::TRANSACTION_SAFE_DYNAMIC },
	{ "true", id_morceau::TRUE },
	{ "try", id_morceau::TRY },
	{ "typedef", id_morceau::TYPEDEF },
	{ "typeid", id_morceau::TYPEID },
	{ "typename", id_morceau::TYPENAME },
	{ "union", id_morceau::UNION },
	{ "unsigned", id_morceau::UNSIGNED },
	{ "using", id_morceau::USING },
	{ "virtual", id_morceau::VIRTUAL },
	{ "void", id_morceau::VOID },
	{ "volatile", id_morceau::VOLATILE },
	{ "wchar_t", id_morceau::WCHAR_T },
	{ "while", id_morceau::WHILE },
	{ "xor", id_morceau::XOR },
	{ "xor_eq", id_morceau::XOR_EQ },
};

static dls::dico<dls::vue_chaine, id_morceau> paires_digraphes = {
	{ "!=", id_morceau::DIFFERENCE },
	{ "%=", id_morceau::MODULO_EGAL },
	{ "&&", id_morceau::ESP_ESP },
	{ "&=", id_morceau::ET_EGAL },
	{ "*/", id_morceau::FIN_COMMENTAIRE_C },
	{ "*=", id_morceau::MULTIPLIE_EGAL },
	{ "+=", id_morceau::PLUS_EGAL },
	{ "-=", id_morceau::MOINS_EGAL },
	{ "/*", id_morceau::DEBUT_COMMENTAIRE_C },
	{ "//", id_morceau::DEBUT_COMMENTAIRE_CPP },
	{ "/=", id_morceau::DIVISE_EGAL },
	{ "::", id_morceau::SCOPE },
	{ "<<", id_morceau::DECALAGE_GAUCHE },
	{ "<=", id_morceau::INFERIEUR_EGAL },
	{ "==", id_morceau::EGALITE },
	{ ">=", id_morceau::SUPERIEUR_EGAL },
	{ ">>", id_morceau::DECALAGE_DROITE },
	{ "^=", id_morceau::OUX_EGAL },
	{ "|=", id_morceau::OU_EGAL },
	{ "||", id_morceau::BARRE_BARRE },
};

static dls::dico<dls::vue_chaine, id_morceau> paires_trigraphes = {
	{ "...", id_morceau::TROIS_POINTS },
	{ "<<=", id_morceau::DEC_DROITE_EGAL },
	{ ">>=", id_morceau::DEC_GAUCHE_EGAL },
};

static dls::dico<char, id_morceau> paires_caracteres_speciaux = {
	{ '!', id_morceau::EXCLAMATION },
	{ '"', id_morceau::GUILLEMET },
	{ '#', id_morceau::DIESE },
	{ '%', id_morceau::POURCENT },
	{ '&', id_morceau::ESPERLUETTE },
	{ '\'', id_morceau::APOSTROPHE },
	{ '(', id_morceau::PARENTHESE_OUVRANTE },
	{ ')', id_morceau::PARENTHESE_FERMANTE },
	{ '*', id_morceau::FOIS },
	{ '+', id_morceau::PLUS },
	{ ',', id_morceau::VIRGULE },
	{ '-', id_morceau::MOINS },
	{ '.', id_morceau::POINT },
	{ '/', id_morceau::DIVISE },
	{ ':', id_morceau::DOUBLE_POINTS },
	{ ';', id_morceau::POINT_VIRGULE },
	{ '<', id_morceau::INFERIEUR },
	{ '=', id_morceau::EGAL },
	{ '>', id_morceau::SUPERIEUR },
	{ '@', id_morceau::AROBASE },
	{ '[', id_morceau::CROCHET_OUVRANT },
	{ ']', id_morceau::CROCHET_FERMANT },
	{ '^', id_morceau::CHAPEAU },
	{ '{', id_morceau::ACCOLADE_OUVRANTE },
	{ '|', id_morceau::BARRE },
	{ '}', id_morceau::ACCOLADE_FERMANTE },
	{ '~', id_morceau::TILDE },
};

const char *chaine_identifiant(id_morceau id)
{
	switch (id) {
		case id_morceau::EXCLAMATION:
			return "id_morceau::EXCLAMATION";
		case id_morceau::GUILLEMET:
			return "id_morceau::GUILLEMET";
		case id_morceau::DIESE:
			return "id_morceau::DIESE";
		case id_morceau::POURCENT:
			return "id_morceau::POURCENT";
		case id_morceau::ESPERLUETTE:
			return "id_morceau::ESPERLUETTE";
		case id_morceau::APOSTROPHE:
			return "id_morceau::APOSTROPHE";
		case id_morceau::PARENTHESE_OUVRANTE:
			return "id_morceau::PARENTHESE_OUVRANTE";
		case id_morceau::PARENTHESE_FERMANTE:
			return "id_morceau::PARENTHESE_FERMANTE";
		case id_morceau::FOIS:
			return "id_morceau::FOIS";
		case id_morceau::PLUS:
			return "id_morceau::PLUS";
		case id_morceau::VIRGULE:
			return "id_morceau::VIRGULE";
		case id_morceau::MOINS:
			return "id_morceau::MOINS";
		case id_morceau::POINT:
			return "id_morceau::POINT";
		case id_morceau::DIVISE:
			return "id_morceau::DIVISE";
		case id_morceau::DOUBLE_POINTS:
			return "id_morceau::DOUBLE_POINTS";
		case id_morceau::POINT_VIRGULE:
			return "id_morceau::POINT_VIRGULE";
		case id_morceau::INFERIEUR:
			return "id_morceau::INFERIEUR";
		case id_morceau::EGAL:
			return "id_morceau::EGAL";
		case id_morceau::SUPERIEUR:
			return "id_morceau::SUPERIEUR";
		case id_morceau::AROBASE:
			return "id_morceau::AROBASE";
		case id_morceau::CROCHET_OUVRANT:
			return "id_morceau::CROCHET_OUVRANT";
		case id_morceau::CROCHET_FERMANT:
			return "id_morceau::CROCHET_FERMANT";
		case id_morceau::CHAPEAU:
			return "id_morceau::CHAPEAU";
		case id_morceau::ACCOLADE_OUVRANTE:
			return "id_morceau::ACCOLADE_OUVRANTE";
		case id_morceau::BARRE:
			return "id_morceau::BARRE";
		case id_morceau::ACCOLADE_FERMANTE:
			return "id_morceau::ACCOLADE_FERMANTE";
		case id_morceau::TILDE:
			return "id_morceau::TILDE";
		case id_morceau::DIFFERENCE:
			return "id_morceau::DIFFERENCE";
		case id_morceau::MODULO_EGAL:
			return "id_morceau::MODULO_EGAL";
		case id_morceau::ESP_ESP:
			return "id_morceau::ESP_ESP";
		case id_morceau::ET_EGAL:
			return "id_morceau::ET_EGAL";
		case id_morceau::FIN_COMMENTAIRE_C:
			return "id_morceau::FIN_COMMENTAIRE_C";
		case id_morceau::MULTIPLIE_EGAL:
			return "id_morceau::MULTIPLIE_EGAL";
		case id_morceau::PLUS_EGAL:
			return "id_morceau::PLUS_EGAL";
		case id_morceau::MOINS_EGAL:
			return "id_morceau::MOINS_EGAL";
		case id_morceau::DEBUT_COMMENTAIRE_C:
			return "id_morceau::DEBUT_COMMENTAIRE_C";
		case id_morceau::DEBUT_COMMENTAIRE_CPP:
			return "id_morceau::DEBUT_COMMENTAIRE_CPP";
		case id_morceau::DIVISE_EGAL:
			return "id_morceau::DIVISE_EGAL";
		case id_morceau::SCOPE:
			return "id_morceau::SCOPE";
		case id_morceau::DECALAGE_GAUCHE:
			return "id_morceau::DECALAGE_GAUCHE";
		case id_morceau::INFERIEUR_EGAL:
			return "id_morceau::INFERIEUR_EGAL";
		case id_morceau::EGALITE:
			return "id_morceau::EGALITE";
		case id_morceau::SUPERIEUR_EGAL:
			return "id_morceau::SUPERIEUR_EGAL";
		case id_morceau::DECALAGE_DROITE:
			return "id_morceau::DECALAGE_DROITE";
		case id_morceau::OUX_EGAL:
			return "id_morceau::OUX_EGAL";
		case id_morceau::OU_EGAL:
			return "id_morceau::OU_EGAL";
		case id_morceau::BARRE_BARRE:
			return "id_morceau::BARRE_BARRE";
		case id_morceau::TROIS_POINTS:
			return "id_morceau::TROIS_POINTS";
		case id_morceau::DEC_DROITE_EGAL:
			return "id_morceau::DEC_DROITE_EGAL";
		case id_morceau::DEC_GAUCHE_EGAL:
			return "id_morceau::DEC_GAUCHE_EGAL";
		case id_morceau::ALIGNAS:
			return "id_morceau::ALIGNAS";
		case id_morceau::ALIGNOF:
			return "id_morceau::ALIGNOF";
		case id_morceau::AND:
			return "id_morceau::AND";
		case id_morceau::AND_EQ:
			return "id_morceau::AND_EQ";
		case id_morceau::ASM:
			return "id_morceau::ASM";
		case id_morceau::ATOMIC_CANCEL:
			return "id_morceau::ATOMIC_CANCEL";
		case id_morceau::ATOMIC_COMMIT:
			return "id_morceau::ATOMIC_COMMIT";
		case id_morceau::ATOMIC_NOEXCEPT:
			return "id_morceau::ATOMIC_NOEXCEPT";
		case id_morceau::AUDIT:
			return "id_morceau::AUDIT";
		case id_morceau::AUTO:
			return "id_morceau::AUTO";
		case id_morceau::AXIOM:
			return "id_morceau::AXIOM";
		case id_morceau::BITAND:
			return "id_morceau::BITAND";
		case id_morceau::BITOR:
			return "id_morceau::BITOR";
		case id_morceau::BOOL:
			return "id_morceau::BOOL";
		case id_morceau::BREAK:
			return "id_morceau::BREAK";
		case id_morceau::CASE:
			return "id_morceau::CASE";
		case id_morceau::CATCH:
			return "id_morceau::CATCH";
		case id_morceau::CHAR:
			return "id_morceau::CHAR";
		case id_morceau::CHAR16_T:
			return "id_morceau::CHAR16_T";
		case id_morceau::CHAR32_T:
			return "id_morceau::CHAR32_T";
		case id_morceau::CHAR8_T:
			return "id_morceau::CHAR8_T";
		case id_morceau::CLASS:
			return "id_morceau::CLASS";
		case id_morceau::CO_AWAIT:
			return "id_morceau::CO_AWAIT";
		case id_morceau::CO_RETURN:
			return "id_morceau::CO_RETURN";
		case id_morceau::CO_YIELD:
			return "id_morceau::CO_YIELD";
		case id_morceau::COMPL:
			return "id_morceau::COMPL";
		case id_morceau::CONCEPT:
			return "id_morceau::CONCEPT";
		case id_morceau::CONST:
			return "id_morceau::CONST";
		case id_morceau::CONST_CAST:
			return "id_morceau::CONST_CAST";
		case id_morceau::CONSTEVAL:
			return "id_morceau::CONSTEVAL";
		case id_morceau::CONSTEXPR:
			return "id_morceau::CONSTEXPR";
		case id_morceau::CONTINUE:
			return "id_morceau::CONTINUE";
		case id_morceau::DECLTYPE:
			return "id_morceau::DECLTYPE";
		case id_morceau::DEFAULT:
			return "id_morceau::DEFAULT";
		case id_morceau::DELETE:
			return "id_morceau::DELETE";
		case id_morceau::DO:
			return "id_morceau::DO";
		case id_morceau::DOUBLE:
			return "id_morceau::DOUBLE";
		case id_morceau::DYNAMIC_CAST:
			return "id_morceau::DYNAMIC_CAST";
		case id_morceau::ELSE:
			return "id_morceau::ELSE";
		case id_morceau::ENUM:
			return "id_morceau::ENUM";
		case id_morceau::EXPLICIT:
			return "id_morceau::EXPLICIT";
		case id_morceau::EXPORT:
			return "id_morceau::EXPORT";
		case id_morceau::EXTERN:
			return "id_morceau::EXTERN";
		case id_morceau::FALSE:
			return "id_morceau::FALSE";
		case id_morceau::FINAL:
			return "id_morceau::FINAL";
		case id_morceau::FLOAT:
			return "id_morceau::FLOAT";
		case id_morceau::FOR:
			return "id_morceau::FOR";
		case id_morceau::FRIEND:
			return "id_morceau::FRIEND";
		case id_morceau::GOTO:
			return "id_morceau::GOTO";
		case id_morceau::IF:
			return "id_morceau::IF";
		case id_morceau::IMPORT:
			return "id_morceau::IMPORT";
		case id_morceau::INLINE:
			return "id_morceau::INLINE";
		case id_morceau::INT:
			return "id_morceau::INT";
		case id_morceau::LONG:
			return "id_morceau::LONG";
		case id_morceau::MODULE:
			return "id_morceau::MODULE";
		case id_morceau::MUTABLE:
			return "id_morceau::MUTABLE";
		case id_morceau::NAMESPACE:
			return "id_morceau::NAMESPACE";
		case id_morceau::NEW:
			return "id_morceau::NEW";
		case id_morceau::NOEXCEPT:
			return "id_morceau::NOEXCEPT";
		case id_morceau::NOT:
			return "id_morceau::NOT";
		case id_morceau::NOT_EQ:
			return "id_morceau::NOT_EQ";
		case id_morceau::NULLPTR:
			return "id_morceau::NULLPTR";
		case id_morceau::OPERATOR:
			return "id_morceau::OPERATOR";
		case id_morceau::OR:
			return "id_morceau::OR";
		case id_morceau::OR_EQ:
			return "id_morceau::OR_EQ";
		case id_morceau::OVERRIDE:
			return "id_morceau::OVERRIDE";
		case id_morceau::PRIVATE:
			return "id_morceau::PRIVATE";
		case id_morceau::PROTECTED:
			return "id_morceau::PROTECTED";
		case id_morceau::PUBLIC:
			return "id_morceau::PUBLIC";
		case id_morceau::REFLEXPR:
			return "id_morceau::REFLEXPR";
		case id_morceau::REGISTER:
			return "id_morceau::REGISTER";
		case id_morceau::REINTERPRET_CAST:
			return "id_morceau::REINTERPRET_CAST";
		case id_morceau::REQUIRES:
			return "id_morceau::REQUIRES";
		case id_morceau::RETURN:
			return "id_morceau::RETURN";
		case id_morceau::SHORT:
			return "id_morceau::SHORT";
		case id_morceau::SIGNED:
			return "id_morceau::SIGNED";
		case id_morceau::SIZEOF:
			return "id_morceau::SIZEOF";
		case id_morceau::STATIC:
			return "id_morceau::STATIC";
		case id_morceau::STATIC_ASSERT:
			return "id_morceau::STATIC_ASSERT";
		case id_morceau::STATIC_CAST:
			return "id_morceau::STATIC_CAST";
		case id_morceau::STRUCT:
			return "id_morceau::STRUCT";
		case id_morceau::SWITCH:
			return "id_morceau::SWITCH";
		case id_morceau::SYNCHRONIZED:
			return "id_morceau::SYNCHRONIZED";
		case id_morceau::TEMPLATE:
			return "id_morceau::TEMPLATE";
		case id_morceau::THIS:
			return "id_morceau::THIS";
		case id_morceau::THREAD_LOCAL:
			return "id_morceau::THREAD_LOCAL";
		case id_morceau::THROW:
			return "id_morceau::THROW";
		case id_morceau::TRANSACTION_SAFE:
			return "id_morceau::TRANSACTION_SAFE";
		case id_morceau::TRANSACTION_SAFE_DYNAMIC:
			return "id_morceau::TRANSACTION_SAFE_DYNAMIC";
		case id_morceau::TRUE:
			return "id_morceau::TRUE";
		case id_morceau::TRY:
			return "id_morceau::TRY";
		case id_morceau::TYPEDEF:
			return "id_morceau::TYPEDEF";
		case id_morceau::TYPEID:
			return "id_morceau::TYPEID";
		case id_morceau::TYPENAME:
			return "id_morceau::TYPENAME";
		case id_morceau::UNION:
			return "id_morceau::UNION";
		case id_morceau::UNSIGNED:
			return "id_morceau::UNSIGNED";
		case id_morceau::USING:
			return "id_morceau::USING";
		case id_morceau::VIRTUAL:
			return "id_morceau::VIRTUAL";
		case id_morceau::VOID:
			return "id_morceau::VOID";
		case id_morceau::VOLATILE:
			return "id_morceau::VOLATILE";
		case id_morceau::WCHAR_T:
			return "id_morceau::WCHAR_T";
		case id_morceau::WHILE:
			return "id_morceau::WHILE";
		case id_morceau::XOR:
			return "id_morceau::XOR";
		case id_morceau::XOR_EQ:
			return "id_morceau::XOR_EQ";
		case id_morceau::NOMBRE_REEL:
			return "id_morceau::NOMBRE_REEL";
		case id_morceau::NOMBRE_ENTIER:
			return "id_morceau::NOMBRE_ENTIER";
		case id_morceau::NOMBRE_HEXADECIMAL:
			return "id_morceau::NOMBRE_HEXADECIMAL";
		case id_morceau::NOMBRE_OCTAL:
			return "id_morceau::NOMBRE_OCTAL";
		case id_morceau::NOMBRE_BINAIRE:
			return "id_morceau::NOMBRE_BINAIRE";
		case id_morceau::PLUS_UNAIRE:
			return "id_morceau::PLUS_UNAIRE";
		case id_morceau::MOINS_UNAIRE:
			return "id_morceau::MOINS_UNAIRE";
		case id_morceau::CHAINE_CARACTERE:
			return "id_morceau::CHAINE_CARACTERE";
		case id_morceau::CHAINE_LITTERALE:
			return "id_morceau::CHAINE_LITTERALE";
		case id_morceau::CARACTERE:
			return "id_morceau::CARACTERE";
		case id_morceau::POINTEUR:
			return "id_morceau::POINTEUR";
		case id_morceau::TABLEAU:
			return "id_morceau::TABLEAU";
		case id_morceau::REFERENCE:
			return "id_morceau::REFERENCE";
		case id_morceau::INCONNU:
			return "id_morceau::INCONNU";
		case id_morceau::CARACTERE_BLANC:
			return "id_morceau::CARACTERE_BLANC";
		case id_morceau::COMMENTAIRE:
			return "id_morceau::COMMENTAIRE";
	};

	return "ERREUR";
}

static constexpr auto TAILLE_MAX_MOT_CLE = 24;

static bool tables_caracteres[256] = {};
static id_morceau tables_identifiants[256] = {};
static bool tables_digraphes[256] = {};
static bool tables_trigraphes[256] = {};
static bool tables_mots_cles[256] = {};

void construit_tables_caractere_speciaux()
{
	for (int i = 0; i < 256; ++i) {
		tables_caracteres[i] = false;
		tables_digraphes[i] = false;
		tables_trigraphes[i] = false;
		tables_mots_cles[i] = false;
		tables_identifiants[i] = id_morceau::INCONNU;
	}

	for (const auto &iter : paires_caracteres_speciaux) {
		tables_caracteres[int(iter.first)] = true;
		tables_identifiants[int(iter.first)] = iter.second;
	}

	for (const auto &iter : paires_digraphes) {
		tables_digraphes[int(iter.first[0])] = true;
	}

	for (const auto &iter : paires_trigraphes) {
		tables_trigraphes[int(iter.first[0])] = true;
	}

	for (const auto &iter : paires_mots_cles) {
		tables_mots_cles[static_cast<unsigned char>(iter.first[0])] = true;
	}
}

bool est_caractere_special(char c, id_morceau &i)
{
	if (!tables_caracteres[static_cast<int>(c)]) {
		return false;
	}

	i = tables_identifiants[static_cast<int>(c)];
	return true;
}

id_morceau id_digraphe(const dls::vue_chaine &chaine)
{
	if (!tables_digraphes[int(chaine[0])]) {
		return id_morceau::INCONNU;
	}

	auto iterateur = paires_digraphes.trouve(chaine);

	if (iterateur != paires_digraphes.fin()) {
		return (*iterateur).second;
	}

	return id_morceau::INCONNU;
}

id_morceau id_trigraphe(const dls::vue_chaine &chaine)
{
	if (!tables_trigraphes[int(chaine[0])]) {
		return id_morceau::INCONNU;
	}

	auto iterateur = paires_trigraphes.trouve(chaine);

	if (iterateur != paires_trigraphes.fin()) {
		return (*iterateur).second;
	}

	return id_morceau::INCONNU;
}

id_morceau id_chaine(const dls::vue_chaine &chaine)
{
	if (chaine.taille() == 1 || chaine.taille() > TAILLE_MAX_MOT_CLE) {
		return id_morceau::CHAINE_CARACTERE;
	}

	if (!tables_mots_cles[static_cast<unsigned char>(chaine[0])]) {
		return id_morceau::CHAINE_CARACTERE;
	}

	auto iterateur = paires_mots_cles.trouve(chaine);

	if (iterateur != paires_mots_cles.fin()) {
		return (*iterateur).second;
	}

	return id_morceau::CHAINE_CARACTERE;
}
