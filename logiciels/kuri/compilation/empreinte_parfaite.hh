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
	TOTAL_KEYWORDS = 64,
	MIN_WORD_LENGTH = 2,
	MAX_WORD_LENGTH = 16,
	MIN_HASH_VALUE = 3,
	MAX_HASH_VALUE = 73
};

/* maximum key range = 71, duplicates = 0 */

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
		74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
		74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
		74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
		74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
		74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
		48, 74, 44, 74, 38, 74, 74, 74, 74, 74,
		74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
		74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
		74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
		74, 74, 74, 74, 74, 74, 74, 21, 33, 19,
		27,  1, 17, 24, 74,  0, 74, 74, 21, 16,
		1, 10, 10, 74,  8,  6,  2,  4, 28, 74,
		15, 74, 19, 74, 74, 74, 74, 74, 74, 74,
		74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
		74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
		74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
		74, 74, 74, 74, 74, 74, 74, 74,  6, 18,
		74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
		74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
		74, 74, 74, 74, 74, 29, 74, 74, 74, 74,
		74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
		74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
		74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
		74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
		74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
		74, 74, 74, 74, 74, 74
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
	0,  0,  0,  2,  0,  0,  4,  0,  2,  7,  5,  7,  7, 11,
	4,  7,  6,  7,  5,  5,  7,  2,  5, 10,  6,  3,  4,  7,
	8, 16,  8,  3,  9,  6,  7, 15,  4, 14,  4,  5,  4,  4,
	3,  6,  8,  6,  6, 13,  3,  3,  4,  6,  3,  4,  6,  3,
	5,  5,  8,  3,  3,  0,  6,  0,  5,  7,  3, 10,  4,  8,
	3,  0,  0,  7
};

static const struct EntreeTable wordlist[] =
{
{"",GenreLexeme::CHAINE_CARACTERE},
{"",GenreLexeme::CHAINE_CARACTERE},
{"",GenreLexeme::CHAINE_CARACTERE},
{"n8", GenreLexeme::N8},
{"",GenreLexeme::CHAINE_CARACTERE},
{"",GenreLexeme::CHAINE_CARACTERE},
{"eini", GenreLexeme::EINI},
{"",GenreLexeme::CHAINE_CARACTERE},
{"si", GenreLexeme::SI},
{"init_de", GenreLexeme::INIT_DE},
{"tente", GenreLexeme::TENTE},
{"externe", GenreLexeme::EXTERNE},
{"tantque", GenreLexeme::TANTQUE},
{"eini_erreur", GenreLexeme::EINI_ERREUR},
{"rien", GenreLexeme::RIEN},
{"nons\303\273r", GenreLexeme::NONSUR},
{"erreur", GenreLexeme::ERREUR},
{"retiens", GenreLexeme::RETIENS},
{"octet", GenreLexeme::OCTET},
{"union", GenreLexeme::UNION},
{"type_de", GenreLexeme::TYPE_DE},
{"z8", GenreLexeme::Z8},
{"sinon", GenreLexeme::SINON},
{"sansarr\303\252t", GenreLexeme::SANSARRET},
{"struct", GenreLexeme::STRUCT},
{"nul", GenreLexeme::NUL},
{"pour", GenreLexeme::POUR},
{"importe", GenreLexeme::IMPORTE},
{"retourne", GenreLexeme::RETOURNE},
{"type_de_donn\303\251es", GenreLexeme::TYPE_DE_DONNEES},
{"continue", GenreLexeme::CONTINUE},
{"dyn", GenreLexeme::DYN},
{"taille_de", GenreLexeme::TAILLE_DE},
{"saufsi", GenreLexeme::SAUFSI},
{"info_de", GenreLexeme::INFO_DE},
{"pousse_contexte", GenreLexeme::POUSSE_CONTEXTE},
{"empl", GenreLexeme::EMPL},
{"nonatteignable", GenreLexeme::NONATTEIGNABLE},
{"dans", GenreLexeme::DANS},
{"\303\251num", GenreLexeme::ENUM},
{"faux", GenreLexeme::FAUX},
{"fonc", GenreLexeme::FONC},
{"n16", GenreLexeme::N16},
{"corout", GenreLexeme::COROUT},
{"r\303\251p\303\250te", GenreLexeme::REPETE},
{"reloge", GenreLexeme::RELOGE},
{"chaine", GenreLexeme::CHAINE},
{"\303\251num_drapeau", GenreLexeme::ENUM_DRAPEAU},
{"n64", GenreLexeme::N64},
{"r16", GenreLexeme::R16},
{"loge", GenreLexeme::LOGE},
{"pi\303\250ge", GenreLexeme::PIEGE},
{"n32", GenreLexeme::N32},
{"vrai", GenreLexeme::VRAI},
{"charge", GenreLexeme::CHARGE},
{"r64", GenreLexeme::R64},
{"comme", GenreLexeme::COMME},
{"discr", GenreLexeme::DISCR},
{"m\303\251moire", GenreLexeme::MEMOIRE},
{"r32", GenreLexeme::R32},
{"z16", GenreLexeme::Z16},
{"",GenreLexeme::CHAINE_CARACTERE},
{"boucle", GenreLexeme::BOUCLE},
{"",GenreLexeme::CHAINE_CARACTERE},
{"garde", GenreLexeme::GARDE},
{"arr\303\252te", GenreLexeme::ARRETE},
{"z64", GenreLexeme::Z64},
{"op\303\251rateur", GenreLexeme::OPERATEUR},
{"bool", GenreLexeme::BOOL},
{"diff\303\250re", GenreLexeme::DIFFERE},
{"z32", GenreLexeme::Z32},
{"",GenreLexeme::CHAINE_CARACTERE},
{"",GenreLexeme::CHAINE_CARACTERE},
{"d\303\251loge", GenreLexeme::DELOGE}
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
