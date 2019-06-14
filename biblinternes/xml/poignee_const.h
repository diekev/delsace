/*
  ***** *BEGIN GPL LICENSE BLOCK *****
 *
  *This program is free software; you can redistribute it and/or
  *modify it under the terms of the GNU General Public License
  *as published by the Free Software Foundation; either version 2
  *of the License, or (at your option) any later version.
 *
  *This program is distributed in the hope that it will be useful,
  *but WITHOUT ANY WARRANTY; without even the implied warranty of
  *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  *GNU General Public License for more details.
 *
  *You should have received a copy of the GNU General Public License
  *along with this program; if not, write to the Free Software  Foundation,
  *Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
  *The Original Code is Copyright (C) 2017 Kévin Dietrich.
  *All rights reserved.
 *
  ***** *END GPL LICENSE BLOCK *****
 *
 */

#pragma once

namespace dls {
namespace xml {

class Declaration;
class Element;
class Noeud;
class Texte;
class Inconnu;

/**
	A variant of the XMLHandle class for working with const Noeuds and Documents. It is the
	same in all regards, except for the 'const' qualifiers. See XMLHandle for API.
*/
class PoigneeConst {
	const Noeud *m_noeud{};

public:
	explicit PoigneeConst(const Noeud *node);
	explicit PoigneeConst(const Noeud& node);
	PoigneeConst(const PoigneeConst& ref);

	PoigneeConst& operator=(const PoigneeConst& ref);

	const PoigneeConst premier_enfant() const;
	const PoigneeConst premier_element_enfant(const char *name = nullptr) const;

	const PoigneeConst dernier_enfant()	const;
	const PoigneeConst dernier_element_enfant(const char *name = nullptr) const;

	const PoigneeConst adelphe_precedent() const;
	const PoigneeConst element_adelphe_precedent(const char *name = nullptr) const;

	const PoigneeConst adelphe_suivant() const;
	const PoigneeConst element_adelphe_suivant(const char *name = nullptr) const;

	const Noeud *noeud() const;
};

const Element *extrait_element(const PoigneeConst *poignee);

const Texte *extrait_texte(const PoigneeConst *poignee);

const Inconnu *extrait_inconnu(const PoigneeConst *poignee);

const Declaration *extrait_declaration(const PoigneeConst *poignee);

}  /* namespace xml */
}  /* namespace dls */
