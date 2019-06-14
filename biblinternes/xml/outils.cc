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

#include "outils.h"

#include "xml.h" // pour TIXMLASSERT

namespace dls {
namespace xml {

// --------- XMLUtil ----------- //

namespace XMLUtil {

const char *SkipWhiteSpace(const char *p)
{
	TIXMLASSERT(p);
	while(IsWhiteSpace(*p)) {
		++p;
	}
	TIXMLASSERT(p);
	return p;
}

char *SkipWhiteSpace(char *p)
{
	return const_cast<char*>(SkipWhiteSpace(const_cast<const char*>(p)));
}

bool IsWhiteSpace(char p)
{
	return !IsUTF8Continuation(p) && isspace(static_cast<unsigned char>(p));
}

const char* ReadBOM(const char* p, bool* bom)
{
	TIXMLASSERT(p);
	TIXMLASSERT(bom);
	*bom = false;
	const unsigned char* pu = reinterpret_cast<const unsigned char*>(p);
	// Check for BOM:
	if (   *(pu+0) == TIXML_UTF_LEAD_0
			&& *(pu+1) == TIXML_UTF_LEAD_1
			&& *(pu+2) == TIXML_UTF_LEAD_2)
	{
		*bom = true;
		p += 3;
	}
	TIXMLASSERT(p);
	return p;
}

void ConvertUTF32ToUTF8(unsigned long input, char* output, int* length)
{
	const unsigned long BYTE_MASK = 0xBF;
	const unsigned long BYTE_MARK = 0x80;
	const unsigned long FIRST_BYTE_MARK[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

	if (input < 0x80) {
		*length = 1;
	}
	else if (input < 0x800) {
		*length = 2;
	}
	else if (input < 0x10000) {
		*length = 3;
	}
	else if (input < 0x200000) {
		*length = 4;
	}
	else {
		*length = 0;    // This code won't convert this correctly anyway.
		return;
	}

	output += *length;

	// Scary scary fall throughs.
	switch (*length) {
		case 4:
			--output;
			*output = static_cast<char>((input | BYTE_MARK) & BYTE_MASK);
			input >>= 6;
			[[fallthrough]];
		case 3:
			--output;
			*output = static_cast<char>((input | BYTE_MARK) & BYTE_MASK);
			input >>= 6;
			[[fallthrough]];
		case 2:
			--output;
			*output = static_cast<char>((input | BYTE_MARK) & BYTE_MASK);
			input >>= 6;
			[[fallthrough]];
		case 1:
			--output;
			*output = static_cast<char>(input | FIRST_BYTE_MARK[*length]);
			break;
		default:
			TIXMLASSERT(false);
	}
}

const char* GetCharacterRef(const char* p, char* value, int* length)
{
	// Presume an entity, and pull it out.
	*length = 0;

	if (*(p+1) == '#' && *(p+2)) {
		unsigned long ucs = 0;
		TIXMLASSERT(sizeof(ucs) >= 4);
		std::ptrdiff_t delta = 0;
		unsigned mult = 1;
		static const char SEMICOLON = ';';

		if (*(p+2) == 'x') {
			// Hexadecimal.
			const char* q = p+3;
			if (!(*q)) {
				return nullptr;
			}

			q = strchr(q, SEMICOLON);

			if (!q) {
				return nullptr;
			}
			TIXMLASSERT(*q == SEMICOLON);

			delta = q-p;
			--q;

			while (*q != 'x') {
				unsigned int digit = 0;

				if (*q >= '0' && *q <= '9') {
					digit = static_cast<unsigned>(*q - '0');
				}
				else if (*q >= 'a' && *q <= 'f') {
					digit = static_cast<unsigned>(*q - 'a' + 10);
				}
				else if (*q >= 'A' && *q <= 'F') {
					digit = static_cast<unsigned>(*q - 'A' + 10);
				}
				else {
					return nullptr;
				}
				TIXMLASSERT(digit >= 0 && digit < 16);
				TIXMLASSERT(digit == 0 || mult <= UINT_MAX / digit);
				const unsigned int digitScaled = mult * digit;
				TIXMLASSERT(ucs <= ULONG_MAX - digitScaled);
				ucs += digitScaled;
				TIXMLASSERT(mult <= UINT_MAX / 16);
				mult *= 16;
				--q;
			}
		}
		else {
			// Decimal.
			const char* q = p+2;
			if (!(*q)) {
				return nullptr;
			}

			q = strchr(q, SEMICOLON);

			if (!q) {
				return nullptr;
			}
			TIXMLASSERT(*q == SEMICOLON);

			delta = q-p;
			--q;

			while (*q != '#') {
				if (*q >= '0' && *q <= '9') {
					auto const digit = static_cast<unsigned int>(*q - '0');
					TIXMLASSERT(digit >= 0 && digit < 10);
					TIXMLASSERT(digit == 0 || mult <= UINT_MAX / digit);
					const unsigned int digitScaled = mult * digit;
					TIXMLASSERT(ucs <= ULONG_MAX - digitScaled);
					ucs += digitScaled;
				}
				else {
					return nullptr;
				}
				TIXMLASSERT(mult <= UINT_MAX / 10);
				mult *= 10;
				--q;
			}
		}
		// convert the UCS to UTF-8
		ConvertUTF32ToUTF8(ucs, value, length);
		return p + delta + 1;
	}
	return p+1;
}

void ToStr(int v, char* buffer, int bufferSize)
{
	TIXML_SNPRINTF(buffer, static_cast<size_t>(bufferSize), "%d", v);
}

void ToStr(unsigned v, char* buffer, int bufferSize)
{
	TIXML_SNPRINTF(buffer, static_cast<size_t>(bufferSize), "%u", v);
}

void ToStr(bool v, char* buffer, int bufferSize)
{
	TIXML_SNPRINTF(buffer, static_cast<size_t>(bufferSize), "%d", v ? 1 : 0);
}

/*
	ToStr() of a number is a very tricky topic.
	https://github.com/leethomason/tinyxml2/issues/106
*/
void ToStr(float v, char* buffer, int bufferSize)
{
	TIXML_SNPRINTF(buffer, static_cast<size_t>(bufferSize), "%.8g", static_cast<double>(v));
}

void ToStr(double v, char* buffer, int bufferSize)
{
	TIXML_SNPRINTF(buffer, static_cast<size_t>(bufferSize), "%.17g", v);
}

bool ToInt(const char* str, int* value)
{
	if (TIXML_SSCANF(str, "%d", value) == 1) {
		return true;
	}

	return false;
}

bool ToUnsigned(const char* str, unsigned *value)
{
	if (TIXML_SSCANF(str, "%u", value) == 1) {
		return true;
	}

	return false;
}

bool ToBool(const char* str, bool* value)
{
	int ival = 0;

	if (ToInt(str, &ival)) {
		*value = (ival==0) ? false : true;
		return true;
	}

	if (StringEqual(str, "true")) {
		*value = true;
		return true;
	}
	else if (StringEqual(str, "false")) {
		*value = false;
		return true;
	}

	return false;
}

bool ToFloat(const char* str, float* value)
{
	if (TIXML_SSCANF(str, "%f", value) == 1) {
		return true;
	}

	return false;
}

bool ToDouble(const char* str, double* value)
{
	if (TIXML_SSCANF(str, "%lf", value) == 1) {
		return true;
	}

	return false;
}

}

}  /* namespace xml */
}  /* namespace dls */
