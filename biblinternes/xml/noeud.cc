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

#include "noeud.h"

#include "document.h"
#include "declaration.h"
#include "outils.h"
#include "xml.h"  // TIXMLASSERT

namespace dls {
namespace xml {

Noeud::Noeud(Document* doc)
	: _memPool(nullptr)
	, _document(doc)
	, _parent(nullptr)
	, _firstChild(nullptr)
	, _lastChild(nullptr)
	, _prev(nullptr)
	, _next(nullptr)
{}

Noeud::~Noeud()
{
	DeleteChildren();
	if (_parent) {
		_parent->Unlink(this);
	}
}

const char* Noeud::Value() const
{
	// Catch an edge case: Documents don't have a a Value. Carefully return nullptr.
	if (this->ToDocument())
		return 0;
	return _value.str();
}

void Noeud::SetValue(const char* str, bool staticMem)
{
	if (staticMem) {
		_value.initialise_str_interne(str);
	}
	else {
		_value.initialise_str(str);
	}
}


void Noeud::DeleteChildren()
{
	while(_firstChild) {
		TIXMLASSERT(_lastChild);
		TIXMLASSERT(_firstChild->_document == _document);
		Noeud* node = _firstChild;
		Unlink(node);

		DeleteNode(node);
	}
	_firstChild = _lastChild = 0;
}


void Noeud::Unlink(Noeud* child)
{
	TIXMLASSERT(child);
	TIXMLASSERT(child->_document == _document);
	TIXMLASSERT(child->_parent == this);
	if (child == _firstChild) {
		_firstChild = _firstChild->_next;
	}
	if (child == _lastChild) {
		_lastChild = _lastChild->_prev;
	}

	if (child->_prev) {
		child->_prev->_next = child->_next;
	}
	if (child->_next) {
		child->_next->_prev = child->_prev;
	}
	child->_parent = 0;
}


void Noeud::DeleteChild(Noeud* node)
{
	TIXMLASSERT(node);
	TIXMLASSERT(node->_document == _document);
	TIXMLASSERT(node->_parent == this);
	Unlink(node);
	DeleteNode(node);
}


Noeud* Noeud::InsertEndChild(Noeud* addThis)
{
	TIXMLASSERT(addThis);
	if (addThis->_document != _document) {
		TIXMLASSERT(false);
		return 0;
	}
	InsertChildPreamble(addThis);

	if (_lastChild) {
		TIXMLASSERT(_firstChild);
		TIXMLASSERT(_lastChild->_next == 0);
		_lastChild->_next = addThis;
		addThis->_prev = _lastChild;
		_lastChild = addThis;

		addThis->_next = 0;
	}
	else {
		TIXMLASSERT(_firstChild == 0);
		_firstChild = _lastChild = addThis;

		addThis->_prev = 0;
		addThis->_next = 0;
	}
	addThis->_parent = this;
	return addThis;
}


Noeud* Noeud::InsertFirstChild(Noeud* addThis)
{
	TIXMLASSERT(addThis);
	if (addThis->_document != _document) {
		TIXMLASSERT(false);
		return 0;
	}
	InsertChildPreamble(addThis);

	if (_firstChild) {
		TIXMLASSERT(_lastChild);
		TIXMLASSERT(_firstChild->_prev == 0);

		_firstChild->_prev = addThis;
		addThis->_next = _firstChild;
		_firstChild = addThis;

		addThis->_prev = 0;
	}
	else {
		TIXMLASSERT(_lastChild == 0);
		_firstChild = _lastChild = addThis;

		addThis->_prev = 0;
		addThis->_next = 0;
	}
	addThis->_parent = this;
	return addThis;
}



const Element* Noeud::FirstChildElement(const char* name) const
{
	for(const Noeud* node = _firstChild; node; node = node->_next) {
		const Element* element = node->ToElement();
		if (element) {
			if (!name || XMLUtil::StringEqual(element->Name(), name)) {
				return element;
			}
		}
	}
	return 0;
}


const Element* Noeud::LastChildElement(const char* name) const
{
	for(const Noeud* node = _lastChild; node; node = node->_prev) {
		const Element* element = node->ToElement();
		if (element) {
			if (!name || XMLUtil::StringEqual(element->Name(), name)) {
				return element;
			}
		}
	}
	return 0;
}


const Element* Noeud::NextSiblingElement(const char* name) const
{
	for(const Noeud* node = _next; node; node = node->_next) {
		const Element* element = node->ToElement();
		if (element
				&& (!name || XMLUtil::StringEqual(name, element->Name()))) {
			return element;
		}
	}
	return 0;
}


const Element* Noeud::PreviousSiblingElement(const char* name) const
{
	for(const Noeud* node = _prev; node; node = node->_prev) {
		const Element* element = node->ToElement();
		if (element
				&& (!name || XMLUtil::StringEqual(name, element->Name()))) {
			return element;
		}
	}
	return 0;
}


char* Noeud::ParseDeep(char* p, PaireString* parentEnd)
{
	// This is a recursive method, but thinking about it "at the current level"
	// it is a pretty simple flat list:
	//		<foo/>
	//		<!-- comment -->
	//
	// With a special case:
	//		<foo>
	//		</foo>
	//		<!-- comment -->
	//
	// Where the closing element (/foo) *must* be the next thing after the opening
	// element, and the names must match. BUT the tricky bit is that the closing
	// element will be read by the child.
	//
	// 'endTag' is the end tag for this node, it is returned by a call to a child.
	// 'parentEnd' is the end tag for the parent, which is filled in and returned.

	while(p && *p) {
		Noeud* node = 0;

		p = _document->Identify(p, &node);
		if (node == 0) {
			break;
		}

		PaireString endTag;
		p = node->ParseDeep(p, &endTag);
		if (!p) {
			DeleteNode(node);
			if (!_document->Error()) {
				_document->SetError(XML_ERROR_PARSING, 0, 0);
			}
			break;
		}

		Declaration* decl = node->ToDeclaration();
		if (decl) {
				// A declaration can only be the first child of a document.
				// Set error, if document already has children.
				if (!_document->NoChildren()) {
						_document->SetError(XML_ERROR_PARSING_DECLARATION, decl->Value(), 0);
						DeleteNode(decl);
						break;
				}
		}

		Element* ele = node->ToElement();
		if (ele) {
			// We read the end tag. Return it to the parent.
			if (ele->ClosingType() == Element::CLOSING) {
				if (parentEnd) {
					ele->_value.transfers_vers(parentEnd);
				}
				node->_memPool->SetTracked();   // created and then immediately deleted.
				DeleteNode(node);
				return p;
			}

			// Handle an end tag returned to this level.
			// And handle a bunch of annoying errors.
			bool mismatch = false;
			if (endTag.vide()) {
				if (ele->ClosingType() == Element::OPEN) {
					mismatch = true;
				}
			}
			else {
				if (ele->ClosingType() != Element::OPEN) {
					mismatch = true;
				}
				else if (!XMLUtil::StringEqual(endTag.str(), ele->Name())) {
					mismatch = true;
				}
			}
			if (mismatch) {
				_document->SetError(XML_ERROR_MISMATCHED_ELEMENT, ele->Name(), 0);
				DeleteNode(node);
				break;
			}
		}
		InsertEndChild(node);
	}
	return 0;
}

void Noeud::DeleteNode(Noeud* node)
{
	if (node == 0) {
		return;
	}
	MemPool* pool = node->_memPool;
	node->~Noeud();
	pool->Free(node);
}

void Noeud::InsertChildPreamble(Noeud* insertThis) const
{
	TIXMLASSERT(insertThis);
	TIXMLASSERT(insertThis->_document == _document);

	if (insertThis->_parent)
		insertThis->_parent->Unlink(insertThis);
	else
		insertThis->_memPool->SetTracked();
}


Noeud* Noeud::InsertAfterChild(Noeud* afterThis, Noeud* addThis)
{
	TIXMLASSERT(addThis);
	if (addThis->_document != _document) {
		TIXMLASSERT(false);
		return 0;
	}

	TIXMLASSERT(afterThis);

	if (afterThis->_parent != this) {
		TIXMLASSERT(false);
		return 0;
	}

	if (afterThis->_next == 0) {
		// The last node or the only node.
		return InsertEndChild(addThis);
	}
	InsertChildPreamble(addThis);
	addThis->_prev = afterThis;
	addThis->_next = afterThis->_next;
	afterThis->_next->_prev = addThis;
	afterThis->_next = addThis;
	addThis->_parent = this;
	return addThis;
}

const Document *Noeud::GetDocument() const
{
	TIXMLASSERT(_document);
	return _document;
}

Document *Noeud::GetDocument()
{
	TIXMLASSERT(_document);
	return _document;
}

Element *Noeud::ToElement()
{
	return nullptr;
}

Texte *Noeud::ToText()
{
	return nullptr;
}

Commentaire *Noeud::ToComment()
{
	return nullptr;
}

Document *Noeud::ToDocument()
{
	return nullptr;
}

Declaration *Noeud::ToDeclaration()
{
	return nullptr;
}

Inconnu *Noeud::ToUnknown()
{
	return nullptr;
}

const Element *Noeud::ToElement() const
{
	return nullptr;
}

const Texte *Noeud::ToText() const
{
	return nullptr;
}

const Commentaire *Noeud::ToComment() const
{
	return nullptr;
}

const Document *Noeud::ToDocument() const
{
	return nullptr;
}

const Declaration *Noeud::ToDeclaration() const
{
	return nullptr;
}

const Inconnu *Noeud::ToUnknown() const
{
	return nullptr;
}

const Noeud *Noeud::Parent() const
{
	return _parent;
}

Noeud *Noeud::Parent()
{
	return _parent;
}

bool Noeud::NoChildren() const
{
	return !_firstChild;
}

const Noeud *Noeud::FirstChild() const
{
	return _firstChild;
}

Noeud *Noeud::FirstChild()
{
	return _firstChild;
}

Element *Noeud::FirstChildElement(const char *name)
{
	return const_cast<Element *>(const_cast<const Noeud *>(this)->FirstChildElement(name));
}

const Noeud *Noeud::LastChild() const
{
	return _lastChild;
}

Noeud *Noeud::LastChild()
{
	return _lastChild;
}

Element *Noeud::LastChildElement(const char *name)
{
	return const_cast<Element *>(const_cast<const Noeud *>(this)->LastChildElement(name));
}

const Noeud *Noeud::PreviousSibling() const
{
	return _prev;
}

Noeud *Noeud::PreviousSibling()
{
	return _prev;
}

Element *Noeud::PreviousSiblingElement(const char *name)
{
	return const_cast<Element *>(const_cast<const Noeud*>(this)->PreviousSiblingElement(name));
}

const Noeud *Noeud::NextSibling() const
{
	return _next;
}

Noeud *Noeud::NextSibling()
{
	return _next;
}

Element *Noeud::NextSiblingElement(const char *name)
{
	return const_cast<Element *>(const_cast<const Noeud *>(this)->NextSiblingElement(name));
}

Noeud *Noeud::LinkEndChild(Noeud *addThis)
{
	return InsertEndChild(addThis);
}

}  /* namespace xml */
}  /* namespace dls */
