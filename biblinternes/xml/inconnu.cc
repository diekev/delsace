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

#include "inconnu.h"

#include "document.h"
#include "outils.h"
#include "visiteur.h"

namespace dls {
namespace xml {

Inconnu::Inconnu(Document* doc) : Noeud(doc)
{
}

char* Inconnu::ParseDeep(char* p, PaireString*)
{
	// Unknown parses as text.
	const char* start = p;

	p = _value.analyse_texte(p, ">", PaireString::NEEDS_NEWLINE_NORMALIZATION);
	if (!p) {
		_document->SetError(XML_ERROR_PARSING_UNKNOWN, start, nullptr);
	}
	return p;
}

Noeud* Inconnu::ShallowClone(Document* doc) const
{
	if (!doc) {
		doc = _document;
	}
	Inconnu* text = doc->NewUnknown(Value());	// fixme: this will always allocate memory. Intern?
	return text;
}

bool Inconnu::ShallowEqual(const Noeud* compare) const
{
	TIXMLASSERT(compare);
	const Inconnu* unknown = compare->ToUnknown();
	return (unknown && XMLUtil::StringEqual(unknown->Value(), Value()));
}

Inconnu *Inconnu::ToUnknown()
{
	return this;
}

const Inconnu *Inconnu::ToUnknown() const
{
	return this;
}

bool Inconnu::Accept(Visiteur* visitor) const
{
	TIXMLASSERT(visitor);
	return visitor->Visit(*this);
}

}  /* namespace xml */
}  /* namespace dls */
