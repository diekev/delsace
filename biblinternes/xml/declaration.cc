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

#include "declaration.h"

#include "document.h"
#include "erreur.h"
#include "outils.h"
#include "visiteur.h"
#include "xml.h"  // TIXMLASSERT

namespace dls {
namespace xml {

Declaration::Declaration(Document *doc)
	: Noeud(doc)
{}

char* Declaration::ParseDeep(char *p, PaireString*)
{
	// Declaration parses as text.
	const char* start = p;
	p = _value.analyse_texte(p, "?>", PaireString::NEEDS_NEWLINE_NORMALIZATION);

	if (p == 0) {
		_document->SetError(XML_ERROR_PARSING_DECLARATION, start, 0);
	}

	return p;
}

Noeud* Declaration::ShallowClone(Document *doc) const
{
	if (!doc) {
		doc = _document;
	}

	// fixme: this will always allocate memory. Intern?
	return doc->NewDeclaration(Value());
}

bool Declaration::ShallowEqual(const Noeud *compare) const
{
	TIXMLASSERT(compare);
	const Declaration* declaration = compare->ToDeclaration();
	return (declaration && XMLUtil::StringEqual(declaration->Value(), Value()));
}

Declaration *Declaration::ToDeclaration()
{
	return this;
}

const Declaration *Declaration::ToDeclaration() const
{
	return this;
}

bool Declaration::Accept(Visiteur *visiteur) const
{
	TIXMLASSERT(visiteur);
	return visiteur->Visit(*this);
}

}  /* namespace xml */
}  /* namespace dls */
