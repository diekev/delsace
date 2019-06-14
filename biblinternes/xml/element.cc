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

#include "element.h"

#include <new>

#include "attribut.h"
#include "document.h"
#include "erreur.h"
#include "outils.h"
#include "xml.h" // TIXMLASSERT
#include "visiteur.h"

namespace dls {
namespace xml {

Element::Element(Document* doc) : Noeud(doc),
	_closingType(0),
	_rootAttribut(0)
{
}

Element::~Element()
{
	while(_rootAttribut) {
		Attribut* next = _rootAttribut->m_next;
		DeleteAttribut(_rootAttribut);
		_rootAttribut = next;
	}
}

Attribut *Element::FindAttribut(const char *name)
{
	return const_cast<Attribut *>(const_cast<const Element*>(this)->FindAttribut(name));
}

const Attribut* Element::FindAttribut(const char *name) const
{
	for(Attribut* a = _rootAttribut; a; a = a->m_next) {
		if (XMLUtil::StringEqual(a->nom(), name)) {
			return a;
		}
	}
	return nullptr;
}

const char *Element::attribut(const char *name, const char *value) const
{
	const Attribut *a = FindAttribut(name);

	if (!a) {
		return nullptr;
	}

	if (!value || XMLUtil::StringEqual(a->valeur(), value)) {
		return a->valeur();
	}

	return nullptr;
}

int Element::IntAttribut(const char *name) const
{
	int i=0;
	QueryIntAttribut(name, &i);
	return i;
}

unsigned Element::UnsignedAttribut(const char *name) const
{
	unsigned i=0;
	QueryUnsignedAttribut(name, &i);
	return i;
}

bool Element::BoolAttribut(const char *name) const
{
	bool b=false;
	QueryBoolAttribut(name, &b);
	return b;
}

double Element::DoubleAttribut(const char *name) const
{
	double d=0;
	QueryDoubleAttribut(name, &d);
	return d;
}

float Element::FloatAttribut(const char *name) const
{
	float f=0;
	QueryFloatAttribut(name, &f);
	return f;
}

XMLError Element::QueryIntAttribut(const char *name, int *value) const
{
	const Attribut* a = FindAttribut(name);
	if (!a) {
		return XML_NO_ATTRIBUTE;
	}
	return a->requiers_valeur_int(value);
}

XMLError Element::QueryUnsignedAttribut(const char *name, unsigned int *value) const
{
	const Attribut* a = FindAttribut(name);
	if (!a) {
		return XML_NO_ATTRIBUTE;
	}
	return a->requiers_valeur_unsigned(value);
}

XMLError Element::QueryBoolAttribut(const char *name, bool *value) const
{
	const Attribut* a = FindAttribut(name);
	if (!a) {
		return XML_NO_ATTRIBUTE;
	}
	return a->requiers_valeur_bool(value);
}

XMLError Element::QueryDoubleAttribut(const char *name, double *value) const
{
	const Attribut* a = FindAttribut(name);
	if (!a) {
		return XML_NO_ATTRIBUTE;
	}
	return a->requiers_valeurr_double(value);
}

XMLError Element::QueryFloatAttribut(const char *name, float *value) const
{
	const Attribut* a = FindAttribut(name);
	if (!a) {
		return XML_NO_ATTRIBUTE;
	}
	return a->requiers_valeur_float(value);
}

int Element::QueryAttribut(const char *name, int *value) const
{
	return QueryIntAttribut(name, value);
}

int Element::QueryAttribut(const char *name, unsigned int *value) const
{
	return QueryUnsignedAttribut(name, value);
}

int Element::QueryAttribut(const char *name, bool *value) const
{
	return QueryBoolAttribut(name, value);
}

int Element::QueryAttribut(const char *name, double *value) const
{
	return QueryDoubleAttribut(name, value);
}

int Element::QueryAttribut(const char *name, float *value) const
{
	return QueryFloatAttribut(name, value);
}

void Element::SetAttribut(const char *name, const char *value)
{
	Attribut* a = FindOrCreateAttribut(name);
	a->ajourne_valeur(value);
}

void Element::SetAttribut(const char *name, int value)
{
	Attribut* a = FindOrCreateAttribut(name);
	a->ajourne_valeur(value);
}

void Element::SetAttribut(const char *name, unsigned value)
{
	Attribut* a = FindOrCreateAttribut(name);
	a->ajourne_valeur(value);
}

void Element::SetAttribut(const char *name, bool value)
{
	Attribut* a = FindOrCreateAttribut(name);
	a->ajourne_valeur(value);
}

void Element::SetAttribut(const char *name, double value)
{
	Attribut* a = FindOrCreateAttribut(name);
	a->ajourne_valeur(value);
}

void Element::SetAttribut(const char *name, float value)
{
	Attribut* a = FindOrCreateAttribut(name);
	a->ajourne_valeur(value);
}

const char *Element::GetText() const
{
	if (FirstChild() && FirstChild()->ToText()) {
		return FirstChild()->Value();
	}

	return nullptr;
}

void Element::SetText(const char *inText)
{
	if (FirstChild() && FirstChild()->ToText()) {
		FirstChild()->SetValue(inText);
	}
	else {
		Texte*	theText = GetDocument()->NewText(inText);
		InsertFirstChild(theText);
	}
}


void Element::SetText(int v)
{
	char buf[BUF_SIZE];
	XMLUtil::ToStr(v, buf, BUF_SIZE);
	SetText(buf);
}


void Element::SetText(unsigned v)
{
	char buf[BUF_SIZE];
	XMLUtil::ToStr(v, buf, BUF_SIZE);
	SetText(buf);
}


void Element::SetText(bool v)
{
	char buf[BUF_SIZE];
	XMLUtil::ToStr(v, buf, BUF_SIZE);
	SetText(buf);
}


void Element::SetText(float v)
{
	char buf[BUF_SIZE];
	XMLUtil::ToStr(v, buf, BUF_SIZE);
	SetText(buf);
}


void Element::SetText(double v)
{
	char buf[BUF_SIZE];
	XMLUtil::ToStr(v, buf, BUF_SIZE);
	SetText(buf);
}


XMLError Element::QueryIntText(int* ival) const
{
	if (FirstChild() && FirstChild()->ToText()) {
		const char *t = FirstChild()->Value();
		if (XMLUtil::ToInt(t, ival)) {
			return XML_SUCCESS;
		}
		return XML_CAN_NOT_CONVERT_TEXT;
	}
	return XML_NO_TEXT_NODE;
}


XMLError Element::QueryUnsignedText(unsigned* uval) const
{
	if (FirstChild() && FirstChild()->ToText()) {
		const char *t = FirstChild()->Value();
		if (XMLUtil::ToUnsigned(t, uval)) {
			return XML_SUCCESS;
		}
		return XML_CAN_NOT_CONVERT_TEXT;
	}
	return XML_NO_TEXT_NODE;
}


XMLError Element::QueryBoolText(bool* bval) const
{
	if (FirstChild() && FirstChild()->ToText()) {
		const char *t = FirstChild()->Value();
		if (XMLUtil::ToBool(t, bval)) {
			return XML_SUCCESS;
		}
		return XML_CAN_NOT_CONVERT_TEXT;
	}
	return XML_NO_TEXT_NODE;
}


XMLError Element::QueryDoubleText(double* dval) const
{
	if (FirstChild() && FirstChild()->ToText()) {
		const char *t = FirstChild()->Value();
		if (XMLUtil::ToDouble(t, dval)) {
			return XML_SUCCESS;
		}
		return XML_CAN_NOT_CONVERT_TEXT;
	}
	return XML_NO_TEXT_NODE;
}


XMLError Element::QueryFloatText(float* fval) const
{
	if (FirstChild() && FirstChild()->ToText()) {
		const char *t = FirstChild()->Value();
		if (XMLUtil::ToFloat(t, fval)) {
			return XML_SUCCESS;
		}
		return XML_CAN_NOT_CONVERT_TEXT;
	}
	return XML_NO_TEXT_NODE;
}

int Element::ClosingType() const
{
	return _closingType;
}

Attribut* Element::FindOrCreateAttribut(const char *name)
{
	Attribut* last = nullptr;
	Attribut* attrib = nullptr;

	for(attrib = _rootAttribut; attrib; last = attrib, attrib = attrib->m_next) {
		if (XMLUtil::StringEqual(attrib->nom(), name)) {
			break;
		}
	}

	if (!attrib) {
		TIXMLASSERT(sizeof(Attribut) == _document->m_ensemble_attribut.ItemSize());
		attrib = new (_document->m_ensemble_attribut.Alloc()) Attribut();
		attrib->_memPool = &_document->m_ensemble_attribut;

		if (last) {
			last->m_next = attrib;
		}
		else {
			_rootAttribut = attrib;
		}

		attrib->nom(name);
		attrib->_memPool->SetTracked(); // always created and linked.
	}

	return attrib;
}

void Element::DeleteAttribut(const char *name)
{
	Attribut* prev = nullptr;
	for(Attribut* a=_rootAttribut; a; a=a->m_next) {
		if (XMLUtil::StringEqual(name, a->nom())) {
			if (prev) {
				prev->m_next = a->m_next;
			}
			else {
				_rootAttribut = a->m_next;
			}
			DeleteAttribut(a);
			break;
		}
		prev = a;
	}
}

const Attribut *Element::FirstAttribut() const {
	return _rootAttribut;
}


char *Element::ParseAttributs(char *p)
{
	const char *start = p;
	Attribut* prevAttribut = nullptr;

	// Read the attributes.
	while(p) {
		p = XMLUtil::SkipWhiteSpace(p);
		if (!(*p)) {
			_document->SetError(XML_ERROR_PARSING_ELEMENT, start, Name());
			return nullptr;
		}

		// attribute.
		if (XMLUtil::IsNameStartChar(static_cast<unsigned char>(*p))) {
			TIXMLASSERT(sizeof(Attribut) == _document->m_ensemble_attribut.ItemSize());
			Attribut* attrib = new (_document->m_ensemble_attribut.Alloc()) Attribut();
			attrib->_memPool = &_document->m_ensemble_attribut;
			attrib->_memPool->SetTracked();

			p = attrib->analyse_profonde(p, _document->ProcessEntities());
			if (!p || attribut(attrib->nom())) {
				DeleteAttribut(attrib);
				_document->SetError(XML_ERROR_PARSING_ATTRIBUTE, start, p);
				return nullptr;
			}
			// There is a minor bug here: if the attribute in the source xml
			// document is duplicated, it will not be detected and the
			// attribute will be doubly added. However, tracking the 'prevAttribut'
			// avoids re-scanning the attribute list. Preferring performance for
			// now, may reconsider in the future.
			if (prevAttribut) {
				prevAttribut->m_next = attrib;
			}
			else {
				_rootAttribut = attrib;
			}
			prevAttribut = attrib;
		}
		// end of the tag
		else if (*p == '>') {
			++p;
			break;
		}
		// end of the tag
		else if (*p == '/' && *(p+1) == '>') {
			_closingType = CLOSED;
			return p+2;	// done; sealed element.
		}
		else {
			_document->SetError(XML_ERROR_PARSING_ELEMENT, start, p);
			return nullptr;
		}
	}
	return p;
}

void Element::DeleteAttribut(Attribut* attribute)
{
	if (attribute == nullptr) {
		return;
	}
	MemPool* pool = attribute->_memPool;
	attribute->~Attribut();
	pool->Free(attribute);
}

//
//	<ele></ele>
//	<ele>foo<b>bar</b></ele>
//
char *Element::ParseDeep(char *p, PaireString* strPair)
{
	// Read the element name.
	p = XMLUtil::SkipWhiteSpace(p);

	// The closing element is the </element> form. It is
	// parsed just like a regular element then deleted from
	// the DOM.
	if (*p == '/') {
		_closingType = CLOSING;
		++p;
	}

	p = _value.analyse_nom(p);
	if (_value.vide()) {
		return nullptr;
	}

	p = ParseAttributs(p);
	if (!p || !*p || _closingType) {
		return p;
	}

	p = Noeud::ParseDeep(p, strPair);
	return p;
}

Noeud* Element::ShallowClone(Document* doc) const
{
	if (!doc) {
		doc = _document;
	}
	Element* element = doc->NewElement(Value());					// fixme: this will always allocate memory. Intern?
	for(const Attribut* a=FirstAttribut(); a; a=a->suivant()) {
		element->SetAttribut(a->nom(), a->valeur());					// fixme: this will always allocate memory. Intern?
	}
	return element;
}


bool Element::ShallowEqual(const Noeud* compare) const
{
	TIXMLASSERT(compare);
	const Element* other = compare->ToElement();
	if (other && XMLUtil::StringEqual(other->Name(), Name())) {

		const Attribut* a=FirstAttribut();
		const Attribut* b=other->FirstAttribut();

		while (a && b) {
			if (!XMLUtil::StringEqual(a->valeur(), b->valeur())) {
				return false;
			}
			a = a->suivant();
			b = b->suivant();
		}
		if (a || b) {
			// different count
			return false;
		}
		return true;
	}
	return false;
}


const char *Element::Name() const
{
	return Value();
}

void Element::SetName(const char *str, bool staticMem)
{
	SetValue(str, staticMem);
}

Element *Element::ToElement()
{
	return this;
}

const Element *Element::ToElement() const
{
	return this;
}

bool Element::Accept(Visiteur* visitor) const
{
	TIXMLASSERT(visitor);
	if (visitor->VisitEnter(*this, _rootAttribut)) {
		for (const Noeud* node=FirstChild(); node; node=node->NextSibling()) {
			if (!node->Accept(visitor)) {
				break;
			}
		}
	}
	return visitor->VisitExit(*this);
}

}  /* namespace xml */
}  /* namespace dls */
