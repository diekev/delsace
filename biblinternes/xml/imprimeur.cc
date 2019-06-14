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

#include "imprimeur.h"

#include <cstdarg>

#include "document.h"
#include "declaration.h"
#include "inconnu.h"

static inline int TIXML_VSCPRINTF(const char* format, va_list va)
{
	/* À FAIRE */
#ifdef __GNUC__
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wsuggest-attribute=format"
#endif
	int len = vsnprintf(0, 0, format, va);
#ifdef __GNUC__
#	pragma GCC diagnostic pop
#endif
	TIXMLASSERT(len >= 0);
	return len;
}

namespace dls {
namespace xml {

XMLPrinter::XMLPrinter(FILE* file, bool compact, int depth) :
	_elementJustOpened(false),
	_firstElement(true),
	_fp(file),
	_depth(depth),
	_textDepth(-1),
	_processEntities(true),
	_compactMode(compact)
{
	for(int i=0; i<ENTITY_RANGE; ++i) {
		_entityFlag[i] = false;
		_restrictedEntityFlag[i] = false;
	}
	for(int i=0; i<NUM_ENTITIES; ++i) {
		const char entityValue = entities[i].value;
		TIXMLASSERT(0 <= entityValue && entityValue < ENTITY_RANGE);
		_entityFlag[ static_cast<unsigned char>(entityValue) ] = true;
	}
	_restrictedEntityFlag[static_cast<unsigned char>('&')] = true;
	_restrictedEntityFlag[static_cast<unsigned char>('<')] = true;
	_restrictedEntityFlag[static_cast<unsigned char>('>')] = true;	// not required, but consistency is nice
	_buffer.Push(0);
}

void XMLPrinter::Print(const char* format, ...)
{
	va_list     va;
	va_start(va, format);

	if (_fp) {
		/* À FAIRE */
#ifdef __GNUC__
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wsuggest-attribute=format"
#endif
		vfprintf(_fp, format, va);
#ifdef __GNUC__
#	pragma GCC diagnostic pop
#endif
	}
	else {
		const int len = TIXML_VSCPRINTF(format, va);
		// Close out and re-start the va-args
		va_end(va);
		TIXMLASSERT(len >= 0);
		va_start(va, format);
		TIXMLASSERT(_buffer.Size() > 0 && _buffer[_buffer.Size() - 1] == 0);
		char* p = _buffer.PushArr(len) - 1;	// back up over the null terminator.

		/* À FAIRE */
#ifdef __GNUC__
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wsuggest-attribute=format"
#endif
		TIXML_VSNPRINTF(p, static_cast<size_t>(len+1), format, va);
#ifdef __GNUC__
#	pragma GCC diagnostic pop
#endif
	}
	va_end(va);
}


void XMLPrinter::PrintSpace(int depth)
{
	for(int i=0; i<depth; ++i) {
		Print("    ");
	}
}


void XMLPrinter::PrintString(const char* p, bool restricted)
{
	// Look for runs of bytes between entities to print.
	const char* q = p;

	if (_processEntities) {
		const bool* flag = restricted ? _restrictedEntityFlag : _entityFlag;
		while (*q) {
			TIXMLASSERT(p <= q);
			// Remember, char is sometimes signed. (How many times has that bitten me?)
			if (*q > 0 && *q < ENTITY_RANGE) {
				// Check for entities. If one is found, flush
				// the stream up until the entity, write the
				// entity, and keep looking.
				if (flag[static_cast<unsigned char>(*q)]) {
					while (p < q) {
						auto const delta = static_cast<size_t>(q - p);
						// %.*s accepts type int as "precision"
						auto const toPrint = (INT_MAX < delta) ? INT_MAX : static_cast<int>(delta);
						Print("%.*s", toPrint, p);
						p += toPrint;
					}
					bool entityPatternPrinted = false;
					for(int i=0; i<NUM_ENTITIES; ++i) {
						if (entities[i].value == *q) {
							Print("&%s;", entities[i].pattern);
							entityPatternPrinted = true;
							break;
						}
					}
					if (!entityPatternPrinted) {
						// TIXMLASSERT(entityPatternPrinted) causes gcc -Wunused-but-set-variable in release
						TIXMLASSERT(false);
					}
					++p;
				}
			}
			++q;
			TIXMLASSERT(p <= q);
		}
	}
	// Flush the remaining string. This will be the entire
	// string if an entity wasn't found.
	TIXMLASSERT(p <= q);
	if (!_processEntities || (p < q)) {
		Print("%s", p);
	}
}


void XMLPrinter::PushHeader(bool writeBOM, bool writeDec)
{
	if (writeBOM) {
		static const unsigned char bom[] = { TIXML_UTF_LEAD_0, TIXML_UTF_LEAD_1, TIXML_UTF_LEAD_2, 0 };
		Print("%s", bom);
	}
	if (writeDec) {
		PushDeclaration("xml version=\"1.0\"");
	}
}


void XMLPrinter::OpenElement(const char* name, bool compactMode)
{
	SealElementIfJustOpened();
	_stack.Push(name);

	if (_textDepth < 0 && !_firstElement && !compactMode) {
		Print("\n");
	}
	if (!compactMode) {
		PrintSpace(_depth);
	}

	Print("<%s", name);
	_elementJustOpened = true;
	_firstElement = false;
	++_depth;
}


void XMLPrinter::PushAttribut(const char* name, const char* value)
{
	TIXMLASSERT(_elementJustOpened);
	Print(" %s=\"", name);
	PrintString(value, false);
	Print("\"");
}


void XMLPrinter::PushAttribut(const char* name, int v)
{
	char buf[BUF_SIZE];
	XMLUtil::ToStr(v, buf, BUF_SIZE);
	PushAttribut(name, buf);
}


void XMLPrinter::PushAttribut(const char* name, unsigned v)
{
	char buf[BUF_SIZE];
	XMLUtil::ToStr(v, buf, BUF_SIZE);
	PushAttribut(name, buf);
}


void XMLPrinter::PushAttribut(const char* name, bool v)
{
	char buf[BUF_SIZE];
	XMLUtil::ToStr(v, buf, BUF_SIZE);
	PushAttribut(name, buf);
}


void XMLPrinter::PushAttribut(const char* name, double v)
{
	char buf[BUF_SIZE];
	XMLUtil::ToStr(v, buf, BUF_SIZE);
	PushAttribut(name, buf);
}


void XMLPrinter::CloseElement(bool compactMode)
{
	--_depth;
	const char* name = _stack.Pop();

	if (_elementJustOpened) {
		Print("/>");
	}
	else {
		if (_textDepth < 0 && !compactMode) {
			Print("\n");
			PrintSpace(_depth);
		}
		Print("</%s>", name);
	}

	if (_textDepth == _depth) {
		_textDepth = -1;
	}
	if (_depth == 0 && !compactMode) {
		Print("\n");
	}
	_elementJustOpened = false;
}


void XMLPrinter::SealElementIfJustOpened()
{
	if (!_elementJustOpened) {
		return;
	}
	_elementJustOpened = false;
	Print(">");
}


void XMLPrinter::PushText(const char* text, bool cdata)
{
	_textDepth = _depth-1;

	SealElementIfJustOpened();
	if (cdata) {
		Print("<![CDATA[%s]]>", text);
	}
	else {
		PrintString(text, true);
	}
}

void XMLPrinter::PushText(int value)
{
	char buf[BUF_SIZE];
	XMLUtil::ToStr(value, buf, BUF_SIZE);
	PushText(buf, false);
}


void XMLPrinter::PushText(unsigned value)
{
	char buf[BUF_SIZE];
	XMLUtil::ToStr(value, buf, BUF_SIZE);
	PushText(buf, false);
}


void XMLPrinter::PushText(bool value)
{
	char buf[BUF_SIZE];
	XMLUtil::ToStr(value, buf, BUF_SIZE);
	PushText(buf, false);
}


void XMLPrinter::PushText(float value)
{
	char buf[BUF_SIZE];
	XMLUtil::ToStr(value, buf, BUF_SIZE);
	PushText(buf, false);
}


void XMLPrinter::PushText(double value)
{
	char buf[BUF_SIZE];
	XMLUtil::ToStr(value, buf, BUF_SIZE);
	PushText(buf, false);
}


void XMLPrinter::PushComment(const char* comment)
{
	SealElementIfJustOpened();
	if (_textDepth < 0 && !_firstElement && !_compactMode) {
		Print("\n");
		PrintSpace(_depth);
	}
	_firstElement = false;
	Print("<!--%s-->", comment);
}


void XMLPrinter::PushDeclaration(const char* value)
{
	SealElementIfJustOpened();
	if (_textDepth < 0 && !_firstElement && !_compactMode) {
		Print("\n");
		PrintSpace(_depth);
	}
	_firstElement = false;
	Print("<?%s?>", value);
}


void XMLPrinter::PushUnknown(const char* value)
{
	SealElementIfJustOpened();
	if (_textDepth < 0 && !_firstElement && !_compactMode) {
		Print("\n");
		PrintSpace(_depth);
	}
	_firstElement = false;
	Print("<!%s>", value);
}


bool XMLPrinter::VisitEnter(const Document& doc)
{
	_processEntities = doc.ProcessEntities();
	if (doc.HasBOM()) {
		PushHeader(true, false);
	}
	return true;
}

bool XMLPrinter::VisitEnter(const Element& element, const Attribut* attribute)
{
	const Element* parentElem = 0;
	if (element.Parent()) {
		parentElem = element.Parent()->ToElement();
	}
	const bool compactMode = parentElem ? CompactMode(*parentElem) : _compactMode;
	OpenElement(element.Name(), compactMode);
	while (attribute) {
		PushAttribut(attribute->nom(), attribute->valeur());
		attribute = attribute->suivant();
	}
	return true;
}

bool XMLPrinter::VisitExit(const Element& element)
{
	CloseElement(CompactMode(element));
	return true;
}

bool XMLPrinter::Visit(const Texte& text)
{
	PushText(text.Value(), text.CData());
	return true;
}

bool XMLPrinter::Visit(const Commentaire& comment)
{
	PushComment(comment.Value());
	return true;
}

bool XMLPrinter::Visit(const Declaration& declaration)
{
	PushDeclaration(declaration.Value());
	return true;
}


bool XMLPrinter::Visit(const Inconnu& unknown)
{
	PushUnknown(unknown.Value());
	return true;
}

}  /* namespace xml */
}  /* namespace dls */

