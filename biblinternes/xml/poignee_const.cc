/*
  ****** BEGIN GPL LICENSE BLOCK *****
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
  ****** END GPL LICENSE BLOCK *****
 *
 */

#include "poignee_const.h"

#include "declaration.h"
#include "element.h"
#include "inconnu.h"
#include "noeud.h"
#include "texte.h"

namespace dls {
namespace xml {

PoigneeConst::PoigneeConst(const Noeud *node)
{
	m_noeud = node;
}

PoigneeConst::PoigneeConst(const Noeud &node)
{
	m_noeud = &node;
}

PoigneeConst::PoigneeConst(const PoigneeConst &ref)
{
	m_noeud = ref.m_noeud;
}

PoigneeConst &PoigneeConst::operator=(const PoigneeConst &ref)
{
	m_noeud = ref.m_noeud;
	return *this;
}

const PoigneeConst PoigneeConst::premier_enfant() const
{
	return PoigneeConst(m_noeud ? m_noeud->FirstChild() : nullptr);
}

const PoigneeConst PoigneeConst::premier_element_enfant(const char *name) const
{
	return PoigneeConst(m_noeud ? m_noeud->FirstChildElement(name) : nullptr);
}

const PoigneeConst PoigneeConst::dernier_enfant() const
{
	return PoigneeConst(m_noeud ? m_noeud->LastChild() : nullptr);
}

const PoigneeConst PoigneeConst::dernier_element_enfant(const char *name) const
{
	return PoigneeConst(m_noeud ? m_noeud->LastChildElement(name) : nullptr);
}

const PoigneeConst PoigneeConst::adelphe_precedent() const
{
	return PoigneeConst(m_noeud ? m_noeud->PreviousSibling() : nullptr);
}

const PoigneeConst PoigneeConst::element_adelphe_precedent(const char *name) const
{
	return PoigneeConst(m_noeud ? m_noeud->PreviousSiblingElement(name) : nullptr);
}

const PoigneeConst PoigneeConst::adelphe_suivant() const
{
	return PoigneeConst(m_noeud ? m_noeud->NextSibling() : nullptr);
}

const PoigneeConst PoigneeConst::element_adelphe_suivant(const char *name) const
{
	return PoigneeConst(m_noeud ? m_noeud->NextSiblingElement(name) : nullptr);
}

const Noeud *PoigneeConst::noeud() const
{
	return m_noeud;
}

const Element *extrait_element(const PoigneeConst *poignee)
{
	return dynamic_cast<const Element *>(poignee->noeud());
}

const Texte *extrait_texte(const PoigneeConst *poignee)
{
	return dynamic_cast<const Texte *>(poignee->noeud());
}

const Inconnu *extrait_inconnu(const PoigneeConst *poignee)
{
	return dynamic_cast<const Inconnu *>(poignee->noeud());
}

const Declaration *extrait_declaration(const PoigneeConst *poignee)
{
	return dynamic_cast<const Declaration *>(poignee->noeud());
}

}  /* namespace xml */
}  /* namespace dls */
