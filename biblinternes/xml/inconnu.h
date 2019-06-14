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

/** Any tag that TinyXML-2 doesn't recognize is saved as an
	unknown. It is a tag of text, but should not be modified.
	It will be written back to the XML, unchanged, when the file
	is saved.

	DTD tags get thrown into Inconnus.
*/
class Inconnu final : public Noeud {
	friend class Document;

protected:
	explicit Inconnu(Document* doc);
	~Inconnu() = default;

	char* ParseDeep(char*, PaireString* endTag) override;

public:
	Inconnu(const Inconnu&) = delete;
	Inconnu& operator=(const Inconnu&) = delete;

	Inconnu *ToUnknown() override;
	const Inconnu *ToUnknown() const override;

	bool Accept(Visiteur* visitor) const override;

	Noeud* ShallowClone(Document* document) const override;
	bool ShallowEqual(const Noeud* compare) const override;
};

}  /* namespace xml */
}  /* namespace dls */

