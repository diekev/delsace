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

#include "visiteur.h"

#include <cstdio>

#include "dyn_array.h"
#include "outils.h"

namespace dls {
namespace xml {

/**
	Printing functionality. The XMLPrinter gives you more
	options than the Document::Print() method.

	It can:
	-# Print to memory.
	-# Print to a file you provide.
	-# Print XML without a Document.

	Print to Memory

	@verbatim
	XMLPrinter printer;
	doc.Print(&printer);
	SomeFunction(printer.CStr());
	@endverbatim

	Print to a File

	You provide the file pointer.
	@verbatim
	XMLPrinter printer(fp);
	doc.Print(&printer);
	@endverbatim

	Print without a Document

	When loading, an XML parser is very useful. However, sometimes
	when saving, it just gets in the way. The code is often set up
	for streaming, and constructing the DOM is just overhead.

	The Printer supports the streaming case. The following code
	prints out a trivially simple XML file without ever creating
	an XML document.

	@verbatim
	XMLPrinter printer(fp);
	printer.OpenElement("foo");
	printer.PushAttribut("foo", "bar");
	printer.CloseElement();
	@endverbatim
*/
class XMLPrinter final : public Visiteur {
public:
	/** Construct the printer. If the FILE* is specified,
		this will print to the FILE. Else it will print
		to memory, and the result is available in CStr().
		If 'compact' is set to true, then output is created
		with only required whitespace and newlines.
	*/
	XMLPrinter(FILE* file=nullptr, bool compact = false, int depth = 0);
	virtual ~XMLPrinter()	{}

	XMLPrinter(XMLPrinter const&) = default;
	XMLPrinter &operator=(XMLPrinter const&) = default;

	/** If streaming, write the BOM and declaration. */
	void PushHeader(bool writeBOM, bool writeDeclaration);
	/** If streaming, start writing an element.
		The element must be closed with CloseElement()
	*/
	void OpenElement(const char* name, bool compactMode=false);
	/// If streaming, add an attribute to an open element.
	void PushAttribut(const char* name, const char* valeur);
	void PushAttribut(const char* name, int valeur);
	void PushAttribut(const char* name, unsigned valeur);
	void PushAttribut(const char* name, bool valeur);
	void PushAttribut(const char* name, double valeur);
	/// If streaming, close the Element.
	virtual void CloseElement(bool compactMode=false);

	/// Add a text node.
	void PushText(const char* text, bool cdata=false);
	/// Add a text node from an integer.
	void PushText(int valeur);
	/// Add a text node from an unsigned.
	void PushText(unsigned valeur);
	/// Add a text node from a bool.
	void PushText(bool valeur);
	/// Add a text node from a float.
	void PushText(float valeur);
	/// Add a text node from a double.
	void PushText(double valeur);

	/// Add a comment
	void PushComment(const char* comment);

	void PushDeclaration(const char* valeur);
	void PushUnknown(const char* valeur);

	virtual bool VisitEnter(const Document& /*doc*/);
	virtual bool VisitExit(const Document& /*doc*/)			{
		return true;
	}

	virtual bool VisitEnter(const Element& element, const Attribut* attribute);
	virtual bool VisitExit(const Element& element);

	virtual bool Visit(const Texte& text);
	virtual bool Visit(const Commentaire& comment);
	virtual bool Visit(const Declaration& declaration);
	virtual bool Visit(const Inconnu& unknown);

	/**
		If in print to memory mode, return a pointer to
		the XML file in memory.
	*/
	const char* CStr() const {
		return _buffer.Mem();
	}
	/**
		If in print to memory mode, return the size
		of the XML file in memory. (Note the size returned
		includes the terminating null.)
	*/
	int CStrSize() const {
		return _buffer.Size();
	}
	/**
		If in print to memory mode, reset the buffer to the
		beginning.
	*/
	void ClearBuffer() {
		_buffer.Clear();
		_buffer.Push(0);
	}

protected:
	virtual bool CompactMode(const Element&)	{ return _compactMode; }

	/** Prints out the space before an element. You may override to change
		the space and tabs used. A PrintSpace() override should call Print().
	*/
	virtual void PrintSpace(int depth);
	void Print(const char* format, ...);

	void SealElementIfJustOpened();
	bool _elementJustOpened{};
	DynArray< const char*, 10 > _stack{};

private:
	void PrintString(const char*, bool restrictedEntitySet);	// prints out, after detecting entities.

	bool _firstElement{};
	FILE* _fp{};
	int _depth{};
	int _textDepth{};
	bool _processEntities{};
	bool _compactMode{};

	enum {
		ENTITY_RANGE = 64,
		BUF_SIZE = 200
	};
	bool _entityFlag[ENTITY_RANGE];
	bool _restrictedEntityFlag[ENTITY_RANGE];

	DynArray< char, 20 > _buffer{};
};

}  /* namespace xml */
}  /* namespace dls */

