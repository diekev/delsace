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

#include "texte.h"

#include "document.h"
#include "outils.h"
#include "visiteur.h"

namespace dls {
namespace xml {

Texte::Texte(Document *doc)
	: Noeud(doc)
	, _isCData(false)
{}

char* Texte::ParseDeep(char* p, PaireString*)
{
	const char* start = p;
	if (this->CData()) {
		p = _value.analyse_texte(p, "]]>", PaireString::NEEDS_NEWLINE_NORMALIZATION);
		if (!p) {
			_document->SetError(XML_ERROR_PARSING_CDATA, start, 0);
		}
		return p;
	}
	else {
		int flags = _document->ProcessEntities() ? PaireString::TEXT_ELEMENT : PaireString::TEXT_ELEMENT_LEAVE_ENTITIES;
		if (_document->WhitespaceMode() == COLLAPSE_WHITESPACE) {
			flags |= PaireString::NEEDS_WHITESPACE_COLLAPSING;
		}

		p = _value.analyse_texte(p, "<", flags);
		if (p && *p) {
			return p-1;
		}
		if (!p) {
			_document->SetError(XML_ERROR_PARSING_TEXT, start, 0);
		}
	}

	return 0;
}

Noeud* Texte::ShallowClone(Document* doc) const
{
	if (!doc) {
		doc = _document;
	}

	Texte* text = doc->NewText(Value());	// fixme: this will always allocate memory. Intern?
	text->SetCData(this->CData());
	return text;
}

bool Texte::ShallowEqual(const Noeud* compare) const
{
	const Texte* text = compare->ToText();
	return (text && XMLUtil::StringEqual(text->Value(), Value()));
}

bool Texte::Accept(Visiteur *visitor) const
{
	TIXMLASSERT(visitor);
	return visitor->Visit(*this);
}

Texte *Texte::ToText()
{
	return this;
}

const Texte *Texte::ToText() const
{
	return this;
}

void Texte::SetCData(bool isCData)
{
	_isCData = isCData;
}

bool Texte::CData() const
{
	return _isCData;
}

}  /* namespace xml */
}  /* namespace dls */
