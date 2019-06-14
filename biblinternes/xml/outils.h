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

#include <cstring>
#include <ctype.h>

// Bunch of unicode info at:
//		http://www.unicode.org/faq/utf_bom.html
//	ef bb bf (Microsoft "lead bytes") - designates UTF-8

static const unsigned char TIXML_UTF_LEAD_0 = 0xefU;
static const unsigned char TIXML_UTF_LEAD_1 = 0xbbU;
static const unsigned char TIXML_UTF_LEAD_2 = 0xbfU;

static const char LINE_FEED				= static_cast<char>(0x0a);			// all line endings are normalized to LF
static const char LF = LINE_FEED;
static const char CARRIAGE_RETURN		= static_cast<char>(0x0d);			// CR gets filtered out
static const char CR = CARRIAGE_RETURN;
static const char SINGLE_QUOTE			= '\'';
static const char DOUBLE_QUOTE			= '\"';

namespace dls {
namespace xml {

#define TIXML_SNPRINTF	snprintf
#define TIXML_VSNPRINTF	vsnprintf
#define TIXML_SSCANF   sscanf

static const int NUM_ENTITIES = 5;

struct Entity {
	const char* pattern;
	int length;
	char value;
};

static const Entity entities[NUM_ENTITIES] = {
	{ "quot", 4,	DOUBLE_QUOTE },
	{ "amp", 3,		'&'  },
	{ "apos", 4,	SINGLE_QUOTE },
	{ "lt",	2, 		'<'	 },
	{ "gt",	2,		'>'	 }
};

/*
	Utility functionality.
*/
namespace XMLUtil {

const char *SkipWhiteSpace(const char* p);

char *SkipWhiteSpace(char* p);

// Anything in the high order range of UTF-8 is assumed to not be whitespace. This isn't
// correct, but simple, and usually works.
bool IsWhiteSpace(char p);

inline bool IsNameStartChar(unsigned char ch)
{
	if (ch >= 128) {
		// This is a heuristic guess in attempt to not implement Unicode-aware isalpha()
		return true;
	}

	if (isalpha(ch)) {
		return true;
	}

	return ch == ':' || ch == '_';
}

inline bool IsNameChar(unsigned char ch)
{
	return IsNameStartChar(ch)
			|| isdigit(ch)
			|| ch == '.'
			|| ch == '-';
}

inline bool StringEqual(const char* p, const char* q, int nChar=__INT_MAX__)
{
	if (p == q) {
		return true;
	}

	return strncmp(p, q, static_cast<size_t>(nChar)) == 0;
}

inline bool IsUTF8Continuation(char p)
{
	return (p & 0x80) != 0;
}

const char* ReadBOM(const char* p, bool* hasBOM);
// p is the starting location,
// the UTF-8 value of the entity will be placed in value, and length filled in.
const char* GetCharacterRef(const char* p, char* value, int* length);
void ConvertUTF32ToUTF8(unsigned long input, char* output, int* length);

// converts primitive types to strings
void ToStr(int v, char* buffer, int bufferSize);
void ToStr(unsigned v, char* buffer, int bufferSize);
void ToStr(bool v, char* buffer, int bufferSize);
void ToStr(float v, char* buffer, int bufferSize);
void ToStr(double v, char* buffer, int bufferSize);

// converts strings to primitive types
bool	ToInt(const char* str, int* value);
bool ToUnsigned(const char* str, unsigned* value);
bool	ToBool(const char* str, bool* value);
bool	ToFloat(const char* str, float* value);
bool ToDouble(const char* str, double* value);

}

}  /* namespace xml */
}  /* namespace dls */
