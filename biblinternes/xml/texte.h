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

namespace dls {
namespace xml {

class Visiteur;

/** XML text.

	Note that a text node can have child element nodes, for example:
	@verbatim
	<root>This is <b>bold</b></root>
	@endverbatim

	A text node can have 2 ways to output the next. "normal" output
	and CDATA. It will default to the mode it was parsed from the XML file and
	you generally want to leave it alone, but you can change the output mode with
	SetCData() and query it with CData().
*/
class Texte final : public Noeud {
	friend class XMLBase;
	friend class Document;
public:
	bool Accept(Visiteur* visitor) const;

	Texte* ToText() override;
	const Texte* ToText() const override;

	/// Declare whether this should be CDATA or standard text.
	void SetCData(bool isCData);
	/// Returns true if this is a CDATA text element.
	bool CData() const;

	Noeud* ShallowClone(Document* document) const override;
	bool ShallowEqual(const Noeud* compare) const override;

protected:
	explicit Texte(Document* doc);
	~Texte() = default;

	char* ParseDeep(char*, PaireString* endTag) override;

private:
	bool _isCData;

	Texte(const Texte&);	// not supported
	Texte& operator=(const Texte&);	// not supported
};

}  /* namespace xml */
}  /* namespace dls */
