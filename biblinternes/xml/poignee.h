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

namespace dls {
namespace xml {

class Noeud;
class Element;
class Texte;
class Inconnu;
class Declaration;


/**
	A XMLHandle is a class that wraps a node pointer with null checks; this is
	an incredibly useful thing. Note that XMLHandle is not part of the TinyXML-2
	DOM structure. It is a separate utility class.

	Take an example:
	@verbatim
	<Document>
		<Element attributeA = "valueA">
			<Child attributeB = "value1" />
			<Child attributeB = "value2" />
		</Element>
	</Document>
	@endverbatim

	Assuming you want the value of "attributeB" in the 2nd "Child" element, it's very
	easy to write a *lot* of code that looks like:

	@verbatim
	Element* root = document.FirstChildElement("Document");
	if (root)
	{
		Element* element = root->FirstChildElement("Element");
		if (element)
		{
			Element* child = element->FirstChildElement("Child");
			if (child)
			{
				Element* child2 = child->NextSiblingElement("Child");
				if (child2)
				{
					// Finally do something useful.
	@endverbatim

	And that doesn't even cover "else" cases. XMLHandle addresses the verbosity
	of such code. A XMLHandle checks for null pointers so it is perfectly safe
	and correct to use:

	@verbatim
	XMLHandle docHandle(&document);
	Element* child2 = docHandle.FirstChildElement("Document").FirstChildElement("Element").FirstChildElement().NextSiblingElement();
	if (child2)
	{
		// do something useful
	@endverbatim

	Which is MUCH more concise and useful.

	It is also safe to copy handles - internally they are nothing more than node pointers.
	@verbatim
	XMLHandle handleCopy = handle;
	@endverbatim

	See also XMLConstHandle, which is the same as XMLHandle, but operates on const objects.
*/
class PoigneeNoeud {
	Noeud* _node{};

public:
	/// Create a handle from any node (at any depth of the tree.) This can be a null pointer.
	explicit PoigneeNoeud(Noeud* node);
	/// Create a handle from a node.
	explicit PoigneeNoeud(Noeud& node);
	/// Copy constructor
	PoigneeNoeud(const PoigneeNoeud& ref);
	/// Assignment
	PoigneeNoeud& operator=(const PoigneeNoeud& ref);

	/// Get the first child of this handle.
	PoigneeNoeud FirstChild();
	/// Get the first child element of this handle.
	PoigneeNoeud FirstChildElement(const char* name = 0);
	/// Get the last child of this handle.
	PoigneeNoeud LastChild();
	/// Get the last child element of this handle.
	PoigneeNoeud LastChildElement(const char* name = 0);
	/// Get the previous sibling of this handle.
	PoigneeNoeud PreviousSibling();
	/// Get the previous sibling element of this handle.
	PoigneeNoeud PreviousSiblingElement(const char* name = 0);
	/// Get the next sibling of this handle.
	PoigneeNoeud NextSibling();
	/// Get the next sibling element of this handle.
	PoigneeNoeud NextSiblingElement(const char* name = 0);

	/// Safe cast to Noeud. This can return null.
	Noeud* ToNode();
	/// Safe cast to Element. This can return null.
	Element* ToElement();
	/// Safe cast to Texte. This can return null.
	Texte* ToText();
	/// Safe cast to Inconnu. This can return null.
	Inconnu* ToUnknown();
	/// Safe cast to Declaration. This can return null.
	Declaration* ToDeclaration();
};

}  /* namespace xml */
}  /* namespace dls */
