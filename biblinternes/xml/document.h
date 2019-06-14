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

#include "noeud.h"

#include "attribut.h"
#include "commentaire.h"
#include "element.h"
#include "ensemble_memoire.h"
#include "erreur.h"
#include "texte.h"

namespace dls {
namespace xml {

enum Whitespace {
	PRESERVE_WHITESPACE,
	COLLAPSE_WHITESPACE
};

/** A Document binds together all the functionality.
	It can be saved, loaded, and printed to the screen.
	All Nodes are connected and allocated to a Document.
	If the Document is deleted, all its Nodes are also deleted.
*/
class Document : public Noeud {
	friend class Element;

	bool m_ecris_BOM{};
	bool m_traites_entities{};
	XMLError m_id_erreur{};
	Whitespace m_espace_blanc{};
	const char *m_str_erreur_1{};
	const char *m_str_erreur_2{};
	char *m_tampon_char{};

	MemPoolT<sizeof(Attribut)> m_ensemble_attribut{};
	MemPoolT<sizeof(Commentaire)> m_ensemble_commentaire{};
	MemPoolT<sizeof(Element)> m_ensemble_element{};
	MemPoolT<sizeof(Texte)> m_ensemble_texte{};

	static const char* m_noms_erreurs[XML_ERROR_COUNT];

public:
	/// constructor
	Document(bool processEntities = true, Whitespace = PRESERVE_WHITESPACE);
	~Document();

	Document(const Document&) = delete;
	void operator=(const Document&) = delete;

	Document* ToDocument() override;
	const Document* ToDocument() const override;

	/**
		Parse an XML file from a character string.
		Returns XML_NO_ERROR (0) on success, or
		an errorID.

		You may optionally pass in the 'nBytes', which is
		the number of bytes which will be parsed. If not
		specified, TinyXML-2 will assume 'xml' points to a
		null terminated string.
	*/
	XMLError Parse(const char* xml, size_t nBytes = -1ul);

	/**
		Load an XML file from disk.
		Returns XML_NO_ERROR (0) on success, or
		an errorID.
	*/
	XMLError LoadFile(const char* filename);

	/**
		Load an XML file from disk. You are responsible
		for providing and closing the FILE*.

		NOTE: The file should be opened as binary ("rb")
		not text in order for TinyXML-2 to correctly
		do newline normalization.

		Returns XML_NO_ERROR (0) on success, or
		an errorID.
	*/
	XMLError LoadFile(FILE*);

	/**
		Save the XML file to disk.
		Returns XML_NO_ERROR (0) on success, or
		an errorID.
	*/
	XMLError SaveFile(const char* filename, bool compact = false);

	/**
		Save the XML file to disk. You are responsible
		for providing and closing the FILE*.

		Returns XML_NO_ERROR (0) on success, or
		an errorID.
	*/
	XMLError SaveFile(FILE* fp, bool compact = false);

	bool ProcessEntities() const;
	Whitespace WhitespaceMode() const;

	/**
		Returns true if this document has a leading Byte Order Mark of UTF8.
	*/
	bool HasBOM() const;
	/** Sets whether to write the BOM when writing the file.
	*/
	void SetBOM(bool useBOM);

	/** Return the root element of DOM. Equivalent to FirstChildElement().
		To get the first node, use FirstChild().
	*/
	Element* RootElement();
	const Element* RootElement() const;

	/** Print the Document. If the Printer is not provided, it will
		print to stdout. If you provide Printer, this can print to a file:
		@verbatim
		XMLPrinter printer(fp);
		doc.Print(&printer);
		@endverbatim

		Or you can use a printer to print to memory:
		@verbatim
		XMLPrinter printer;
		doc.Print(&printer);
		// printer.CStr() has a const char* to the XML
		@endverbatim
	*/
	void Print(XMLPrinter* streamer=0) const;
	bool Accept(Visiteur* visitor) const override;

	/**
		Create a new Element associated with
		this Document. The memory for the Element
		is managed by the Document.
	*/
	Element* NewElement(const char* name);
	/**
		Create a new Comment associated with
		this Document. The memory for the Comment
		is managed by the Document.
	*/
	Commentaire* NewComment(const char* comment);
	/**
		Create a new Text associated with
		this Document. The memory for the Text
		is managed by the Document.
	*/
	Texte* NewText(const char* text);
	/**
		Create a new Declaration associated with
		this Document. The memory for the object
		is managed by the Document.

		If the 'text' param is null, the standard
		declaration is used.:
		@verbatim
			<?xml version="1.0" encoding="UTF-8"?>
		@endverbatim
	*/
	Declaration* NewDeclaration(const char* text=0);
	/**
		Create a new Unknown associated with
		this Document. The memory for the object
		is managed by the Document.
	*/
	Inconnu* NewUnknown(const char* text);

	/**
		Delete a node associated with this document.
		It will be unlinked from the DOM.
	*/
	void DeleteNode(Noeud* node);

	void SetError(XMLError error, const char* str1, const char* str2);

	/// Return true if there was an error parsing the document.
	bool Error() const;
	/// Return the errorID.
	XMLError  ErrorID() const;
	const char* ErrorName() const;

	/// Return a possibly helpful diagnostic location or string.
	const char* GetErrorStr1() const;
	/// Return a possibly helpful secondary diagnostic location or string.
	const char* GetErrorStr2() const;
	/// If there is an error, print it to stdout.
	void PrintError() const;

	/// Clear the document, resetting it to the initial state.
	void Clear();

	// internal
	char* Identify(char* p, Noeud** node);

	Noeud* ShallowClone(Document* /*document*/) const override;
	bool ShallowEqual(const Noeud* /*compare*/) const override;

private:
	void Parse();
};

}  /* namespace xml */
}  /* namespace dls */
