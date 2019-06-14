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

#include "paire_string.h"

namespace dls {
namespace xml {

class Element;
class Texte;
class Commentaire;
class Document;
class Declaration;
class Inconnu;
class Visiteur;
class MemPool;

/** Noeud is a base class for every object that is in the
	XML Document Object Model (DOM), except Attributs.
	Nodes have siblings, a parent, and children which can
	be navigated. A node is always in a Document.
	The type of a Noeud can be queried, and it can
	be cast to its more defined type.

	A Document allocates memory for all its Nodes.
	When the Document gets deleted, all its Nodes
	will also be deleted.

	@verbatim
	A Document can contain:	Element	(container or leaf)
							Comment (leaf)
							Unknown (leaf)
							Declaration(leaf)

	An Element can contain:	Element (container or leaf)
							Text	(leaf)
							Attributs (not on tree)
							Comment (leaf)
							Unknown (leaf)

	@endverbatim
*/
class Noeud {
	friend class Document;
	friend class Element;

	MemPool*		_memPool{};

protected:
	explicit Noeud(Document*);
	virtual ~Noeud();

	virtual char* ParseDeep(char*, PaireString *);

	Document*	_document{};
	Noeud*		_parent{};
	mutable PaireString	_value{};

	Noeud*		_firstChild{};
	Noeud*		_lastChild{};

	Noeud*		_prev{};
	Noeud*		_next{};

public:

	/// Get the Document that owns this Noeud.
	const Document* GetDocument() const;
	/// Get the Document that owns this Noeud.
	Document* GetDocument();

	/// Safely cast to an Element, or null.
	virtual Element*		ToElement();
	/// Safely cast to Text, or null.
	virtual Texte*		ToText();
	/// Safely cast to a Comment, or null.
	virtual Commentaire*		ToComment();
	/// Safely cast to a Document, or null.
	virtual Document*	ToDocument();
	/// Safely cast to a Declaration, or null.
	virtual Declaration*	ToDeclaration();
	/// Safely cast to an Unknown, or null.
	virtual Inconnu*		ToUnknown();

	virtual const Element*		ToElement() const;
	virtual const Texte*			ToText() const;
	virtual const Commentaire*		ToComment() const;
	virtual const Document*		ToDocument() const;
	virtual const Declaration*	ToDeclaration() const;
	virtual const Inconnu*		ToUnknown() const;

	/** The meaning of 'value' changes for the specific type.
		@verbatim
		Document:	empty (NULL is returned, not an empty string)
		Element:	name of the element
		Comment:	the comment text
		Unknown:	the tag contents
		Text:		the text string
		@endverbatim
	*/
	const char* Value() const;

	/** Set the Value of an XML node.
		@sa Value()
	*/
	void SetValue(const char* val, bool staticMem=false);

	/// Get the parent of this node on the DOM.
	const Noeud*	Parent() const;

	Noeud* Parent();

	/// Returns true if this node has no children.
	bool NoChildren() const;

	/// Get the first child node, or null if none exists.
	const Noeud*  FirstChild() const;

	Noeud*		FirstChild();

	/** Get the first child element, or optionally the first child
		element with the specified name.
	*/
	const Element* FirstChildElement(const char* name = 0) const;

	Element* FirstChildElement(const char* name = 0);

	/// Get the last child node, or null if none exists.
	const Noeud*	LastChild() const;

	Noeud*		LastChild();

	/** Get the last child element or optionally the last child
		element with the specified name.
	*/
	const Element* LastChildElement(const char* name = 0) const;

	Element* LastChildElement(const char* name = 0);

	/// Get the previous (left) sibling node of this node.
	const Noeud*	PreviousSibling() const;

	Noeud*	PreviousSibling();

	/// Get the previous (left) sibling element of this node, with an optionally supplied name.
	const Element*	PreviousSiblingElement(const char* name = 0) const ;

	Element*	PreviousSiblingElement(const char* name = 0);

	/// Get the next (right) sibling node of this node.
	const Noeud*	NextSibling() const;

	Noeud*	NextSibling();

	/// Get the next (right) sibling element of this node, with an optionally supplied name.
	const Element*	NextSiblingElement(const char* name = 0) const;

	Element*	NextSiblingElement(const char* name = 0);

	/**
		Add a child node as the last (right) child.
		If the child node is already part of the document,
		it is moved from its old location to the new location.
		Returns the addThis argument or 0 if the node does not
		belong to the same document.
	*/
	Noeud* InsertEndChild(Noeud* addThis);

	Noeud* LinkEndChild(Noeud* addThis);
	/**
		Add a child node as the first (left) child.
		If the child node is already part of the document,
		it is moved from its old location to the new location.
		Returns the addThis argument or 0 if the node does not
		belong to the same document.
	*/
	Noeud* InsertFirstChild(Noeud* addThis);
	/**
		Add a node after the specified child node.
		If the child node is already part of the document,
		it is moved from its old location to the new location.
		Returns the addThis argument or 0 if the afterThis node
		is not a child of this node, or if the node does not
		belong to the same document.
	*/
	Noeud* InsertAfterChild(Noeud* afterThis, Noeud* addThis);

	/**
		Delete all the children of this node.
	*/
	void DeleteChildren();

	/**
		Delete a child of this node.
	*/
	void DeleteChild(Noeud* node);

	/**
		Make a copy of this node, but not its children.
		You may pass in a Document pointer that will be
		the owner of the new Node. If the 'document' is
		null, then the node returned will be allocated
		from the current Document. (this->GetDocument())

		Note: if called on a Document, this will return null.
	*/
	virtual Noeud* ShallowClone(Document* document) const = 0;

	/**
		Test if 2 nodes are the same, but don't test children.
		The 2 nodes do not need to be in the same Document.

		Note: if called on a Document, this will return false.
	*/
	virtual bool ShallowEqual(const Noeud* compare) const = 0;

	/** Accept a hierarchical visit of the nodes in the TinyXML-2 DOM. Every node in the
		XML tree will be conditionally visited and the host will be called back
		via the XMLVisitor interface.

		This is essentially a SAX interface for TinyXML-2. (Note however it doesn't re-parse
		the XML for the callbacks, so the performance of TinyXML-2 is unchanged by using this
		interface versus any other.)

		The interface has been based on ideas from:

		- http://www.saxproject.org/
		- http://c2.com/cgi/wiki?HierarchicalVisitorPattern

		Which are both good references for "visiting".

		An example of using Accept():
		@verbatim
		XMLPrinter printer;
		tinyxmlDoc.Accept(&printer);
		const char* xmlcstr = printer.CStr();
		@endverbatim
	*/
	virtual bool Accept(Visiteur* visitor) const = 0;

private:
	void Unlink(Noeud* child);
	static void DeleteNode(Noeud* node);
	void InsertChildPreamble(Noeud* insertThis) const;

	Noeud(const Noeud&);	// not supported
	Noeud& operator=(const Noeud&);	// not supported
};

}  /* namespace xml */
}  /* namespace dls */
