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

#include "commentaire.h"

#include "document.h"
#include "outils.h"
#include "visiteur.h"
#include "xml.h"  // TIXMLASSERT

namespace dls {
namespace xml {

Commentaire::Commentaire(Document *doc)
	: Noeud(doc)
{}

char* Commentaire::ParseDeep(char* p, PaireString*)
{
	// Comment parses as text.
	const char *start = p;
	p = _value.analyse_texte(p, "-->", PaireString::COMMENT);

	if (p == 0) {
		_document->SetError(XML_ERROR_PARSING_COMMENT, start, 0);
	}

	return p;
}

Noeud *Commentaire::ShallowClone(Document *doc) const
{
	if (!doc) {
		doc = _document;
	}

	// fixme: this will always allocate memory. Intern?
	return doc->NewComment(Value());
}

bool Commentaire::ShallowEqual(const Noeud *compare) const
{
	TIXMLASSERT(compare);
	const Commentaire* comment = compare->ToComment();
	return (comment && XMLUtil::StringEqual(comment->Value(), Value()));
}

Commentaire *Commentaire::ToComment()
{
	return this;
}

const Commentaire *Commentaire::ToComment() const
{
	return this;
}

bool Commentaire::Accept(Visiteur *visiteur) const
{
	TIXMLASSERT(visiteur);
	return visiteur->Visit(*this);
}

}  /* namespace xml */
}  /* namespace dls */
