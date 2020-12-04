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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "lexemes.hh"

/* C++ code produced by gperf version 3.1 */
/* Command-line: gperf -m100 empreinte_parfaite.txt  */
/* Computed positions: -k'1,3-4' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
	&& ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
	&& (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
	&& ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
	&& ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
	&& ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
	&& ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
	&& ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
	&& ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
	&& ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
	&& ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
	&& ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
	&& ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
	&& ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
	&& ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
	&& ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
	&& ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
	&& ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
	&& ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
	&& ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
	&& ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
	&& ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
	&& ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

struct EntreeTable {  const char *nom; GenreLexeme genre;  };
enum
{
	TOTAL_KEYWORDS = 62,
	MIN_WORD_LENGTH = 2,
	MAX_WORD_LENGTH = 16,
	MIN_HASH_VALUE = 2,
	MAX_HASH_VALUE = 78
};

/* maximum key range = 77, duplicates = 0 */

struct EmpreinteParfaite
{
private:
	static inline unsigned int calcule_empreinte (const char *str, size_t len);
public:
	static GenreLexeme lexeme_pour_chaine(const char *str, size_t len);
};

inline unsigned int
EmpreinteParfaite::calcule_empreinte (const char *str, size_t len)
{
	static const unsigned char asso_values[] =
	{
		79, 79, 79, 79, 79, 79, 79, 79, 79, 79,
		79, 79, 79, 79, 79, 79, 79, 79, 79, 79,
		79, 79, 79, 79, 79, 79, 79, 79, 79, 79,
		79, 79, 79, 79, 79, 79, 79, 79, 79, 79,
		79, 79, 79, 79, 79, 79, 79, 79, 79, 79,
		50, 79, 63, 79,  3, 79, 79, 79, 79, 79,
		79, 79, 79, 79, 79, 79, 79, 79, 79, 79,
		79, 79, 79, 79, 79, 79, 79, 79, 79, 79,
		79, 79, 79, 79, 79, 79, 79, 79, 79, 79,
		79, 79, 79, 79, 79, 79, 79, 12, 33, 18,
		0,  3, 33, 24, 79,  0, 79, 79, 18, 27,
		0,  9, 18, 79, 12, 21,  3,  3, 54, 79,
		27, 79,  3, 79, 79, 79, 79, 79, 79, 79,
		79, 79, 79, 79, 79, 79, 79, 79, 79, 79,
		79, 79, 79, 79, 79, 79, 79, 79, 79, 79,
		79, 79, 79, 79, 79, 79, 79, 79, 79, 79,
		79, 79, 79, 79, 79, 79, 79, 79,  0,  9,
		79, 79, 79, 79, 79, 79, 79, 79, 79, 79,
		79, 79, 79, 79, 79, 79, 79, 79, 79, 79,
		79, 79, 79, 79, 79, 30, 79, 79, 79, 79,
		79, 79, 79, 79, 79, 79, 79, 79, 79, 79,
		79, 79, 79, 79, 79, 79, 79, 79, 79, 79,
		79, 79, 79, 79, 79, 79, 79, 79, 79, 79,
		79, 79, 79, 79, 79, 79, 79, 79, 79, 79,
		79, 79, 79, 79, 79, 79, 79, 79, 79, 79,
		79, 79, 79, 79, 79, 79
	};
	unsigned int hval = static_cast<unsigned int>(len);

	switch (hval)
	{
		default:
			hval += asso_values[static_cast<unsigned char>(str[3])];
			/*FALLTHROUGH*/
		case 3:
			hval += asso_values[static_cast<unsigned char>(str[2])];
			/*FALLTHROUGH*/
		case 2:
		case 1:
			hval += asso_values[static_cast<unsigned char>(str[0])];
			break;
	}
	return hval;
}

static const unsigned char lengthtable[] =
{
	0,  0,  2,  3,  0,  2,  3,  4,  0,  3,  7,  5,  0,  7,
	11,  0,  7,  5,  3,  4,  5,  3,  7,  2,  6,  4, 14,  0,
	7,  8,  9,  7,  8,  0,  7,  5,  6,  4,  5,  0, 16,  5,
	6,  4,  5,  6, 13,  8,  6,  7,  8,  0, 10,  3,  6,  4,
	3, 15, 10,  0,  6,  7,  0,  6,  4,  3,  3,  4,  0,  3,
	4,  8,  0,  0,  8,  0,  0,  5,  3
};

static const struct EntreeTable wordlist[] =
{
{"",GenreLexeme::CHAINE_CARACTERE},
{"",GenreLexeme::CHAINE_CARACTERE},
{"n8", GenreLexeme::N8},
{"dyn", GenreLexeme::DYN},
{"",GenreLexeme::CHAINE_CARACTERE},
{"z8", GenreLexeme::Z8},
{"n16", GenreLexeme::N16},
{"eini", GenreLexeme::EINI},
{"",GenreLexeme::CHAINE_CARACTERE},
{"z16", GenreLexeme::Z16},
{"init_de", GenreLexeme::INIT_DE},
{"tente", GenreLexeme::TENTE},
{"",GenreLexeme::CHAINE_CARACTERE},
{"tantque", GenreLexeme::TANTQUE},
{"eini_erreur", GenreLexeme::EINI_ERREUR},
{"",GenreLexeme::CHAINE_CARACTERE},
{"externe", GenreLexeme::EXTERNE},
{"union", GenreLexeme::UNION},
{"r16", GenreLexeme::R16},
{"rien", GenreLexeme::RIEN},
{"octet", GenreLexeme::OCTET},
{"nul", GenreLexeme::NUL},
{"retiens", GenreLexeme::RETIENS},
{"si", GenreLexeme::SI},
{"erreur", GenreLexeme::ERREUR},
{"dans", GenreLexeme::DANS},
{"nonatteignable", GenreLexeme::NONATTEIGNABLE},
{"",GenreLexeme::CHAINE_CARACTERE},
{"nons\303\273r", GenreLexeme::NONSUR},
{"continue", GenreLexeme::CONTINUE},
{"taille_de", GenreLexeme::TAILLE_DE},
{"type_de", GenreLexeme::TYPE_DE},
{"retourne", GenreLexeme::RETOURNE},
{"",GenreLexeme::CHAINE_CARACTERE},
{"importe", GenreLexeme::IMPORTE},
{"sinon", GenreLexeme::SINON},
{"chaine", GenreLexeme::CHAINE},
{"pour", GenreLexeme::POUR},
{"\303\251num", GenreLexeme::ENUM},
{"",GenreLexeme::CHAINE_CARACTERE},
{"type_de_donn\303\251es", GenreLexeme::TYPE_DE_DONNEES},
{"garde", GenreLexeme::GARDE},
{"struct", GenreLexeme::STRUCT},
{"empl", GenreLexeme::EMPL},
{"discr", GenreLexeme::DISCR},
{"corout", GenreLexeme::COROUT},
{"\303\251num_drapeau", GenreLexeme::ENUM_DRAPEAU},
{"r\303\251p\303\250te", GenreLexeme::REPETE},
{"charge", GenreLexeme::CHARGE},
{"info_de", GenreLexeme::INFO_DE},
{"d\303\251finis", GenreLexeme::DEFINIS},
{"",GenreLexeme::CHAINE_CARACTERE},
{"sansarr\303\252t", GenreLexeme::SANSARRET},
{"n32", GenreLexeme::N32},
{"pi\303\250ge", GenreLexeme::PIEGE},
{"fonc", GenreLexeme::FONC},
{"z32", GenreLexeme::Z32},
{"pousse_contexte", GenreLexeme::POUSSE_CONTEXTE},
{"op\303\251rateur", GenreLexeme::OPERATEUR},
{"",GenreLexeme::CHAINE_CARACTERE},
{"boucle", GenreLexeme::BOUCLE},
{"arr\303\252te", GenreLexeme::ARRETE},
{"",GenreLexeme::CHAINE_CARACTERE},
{"saufsi", GenreLexeme::SAUFSI},
{"bool", GenreLexeme::BOOL},
{"r32", GenreLexeme::R32},
{"n64", GenreLexeme::N64},
{"faux", GenreLexeme::FAUX},
{"",GenreLexeme::CHAINE_CARACTERE},
{"z64", GenreLexeme::Z64},
{"vrai", GenreLexeme::VRAI},
{"m\303\251moire", GenreLexeme::MEMOIRE},
{"",GenreLexeme::CHAINE_CARACTERE},
{"",GenreLexeme::CHAINE_CARACTERE},
{"diff\303\250re", GenreLexeme::DIFFERE},
{"",GenreLexeme::CHAINE_CARACTERE},
{"",GenreLexeme::CHAINE_CARACTERE},
{"comme", GenreLexeme::COMME},
{"r64", GenreLexeme::R64}
};

GenreLexeme
EmpreinteParfaite::lexeme_pour_chaine (const char *str, size_t len)
{
	if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
	{
		unsigned int key = calcule_empreinte (str, len);

		if (key <= MAX_HASH_VALUE)
			if (len == lengthtable[key])
			{
				const char *s = wordlist[key].nom;

				if (*str == *s && !memcmp (str + 1, s + 1, len - 1))
					return wordlist[key].genre;
			}
	}
	return GenreLexeme::CHAINE_CARACTERE;
}

GenreLexeme lexeme_pour_chaine(dls::vue_chaine_compacte chn)
{
	return EmpreinteParfaite::lexeme_pour_chaine(chn.pointeur(), static_cast<size_t>(chn.taille()));
}
