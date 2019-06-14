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

#include "poignee.h"

#include "element.h"
#include "noeud.h"

namespace dls {
namespace xml {

PoigneeNoeud::PoigneeNoeud(Noeud *node)
{
	_node = node;
}

PoigneeNoeud::PoigneeNoeud(Noeud &node)
{
	_node = &node;
}

PoigneeNoeud::PoigneeNoeud(const PoigneeNoeud &ref)
{
	_node = ref._node;
}

PoigneeNoeud &PoigneeNoeud::operator=(const PoigneeNoeud &ref)
{
	_node = ref._node;
	return *this;
}

PoigneeNoeud PoigneeNoeud::FirstChild()
{
	return PoigneeNoeud(_node ? _node->FirstChild() : nullptr);
}

PoigneeNoeud PoigneeNoeud::FirstChildElement(const char *name)
{
	return PoigneeNoeud(_node ? _node->FirstChildElement(name) : nullptr);
}

PoigneeNoeud PoigneeNoeud::LastChild()
{
	return PoigneeNoeud(_node ? _node->LastChild() : nullptr);
}

PoigneeNoeud PoigneeNoeud::LastChildElement(const char *name)
{
	return PoigneeNoeud(_node ? _node->LastChildElement(name) : nullptr);
}

PoigneeNoeud PoigneeNoeud::PreviousSibling()
{
	return PoigneeNoeud(_node ? _node->PreviousSibling() : nullptr);
}

PoigneeNoeud PoigneeNoeud::PreviousSiblingElement(const char *name)
{
	return PoigneeNoeud(_node ? _node->PreviousSiblingElement(name) : nullptr);
}

PoigneeNoeud PoigneeNoeud::NextSibling()
{
	return PoigneeNoeud(_node ? _node->NextSibling() : nullptr);
}

PoigneeNoeud PoigneeNoeud::NextSiblingElement(const char *name)
{
	return PoigneeNoeud(_node ? _node->NextSiblingElement(name) : nullptr);
}

Noeud *PoigneeNoeud::ToNode()
{
	return _node;
}

Element *PoigneeNoeud::ToElement()
{
	return ((_node == nullptr) ? nullptr : _node->ToElement());
}

Texte *PoigneeNoeud::ToText()
{
	return ((_node == nullptr) ? nullptr : _node->ToText());
}

Inconnu *PoigneeNoeud::ToUnknown()
{
	return ((_node == nullptr) ? nullptr : _node->ToUnknown());
}

Declaration *PoigneeNoeud::ToDeclaration()
{
	return ((_node == nullptr) ? nullptr : _node->ToDeclaration());
}

}  /* namespace xml */
}  /* namespace dls */
