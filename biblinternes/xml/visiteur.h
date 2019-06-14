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

class Attribut;
class Commentaire;
class Declaration;
class Document;
class Element;
class Texte;
class Inconnu;

/**
	Implements the interface to the "Visitor pattern" (see the Accept() method.)
	If you call the Accept() method, it requires being passed a XMLVisitor
	class to handle callbacks. For nodes that contain other nodes (Document, Element)
	you will get called with a VisitEnter/VisitExit pair. Nodes that are always leafs
	are simply called with Visit().

	If you return 'true' from a Visit method, recursive parsing will continue. If you return
	false, <b>no children of this node or its siblings</b> will be visited.

	All flavors of Visit methods have a default implementation that returns 'true' (continue
	visiting). You need to only override methods that are interesting to you.

	Generally Accept() is called on the Document, although all nodes support visiting.

	You should never change the document from a callback.

	@sa Noeud::Accept()
*/
class Visiteur {
public:
	virtual ~Visiteur() = default;

	/// Visit a document.
	virtual bool VisitEnter(const Document& /*doc*/);

	/// Visit a document.
	virtual bool VisitExit(const Document& /*doc*/);

	/// Visit an element.
	virtual bool VisitEnter(const Element& /*element*/, const Attribut* /*firstAttribut*/);

	/// Visit an element.
	virtual bool VisitExit(const Element& /*element*/);

	/// Visit a declaration.
	virtual bool Visit(const Declaration& /*declaration*/);

	/// Visit a text node.
	virtual bool Visit(const Texte& /*text*/);

	/// Visit a comment node.
	virtual bool Visit(const Commentaire& /*comment*/);

	/// Visit an unknown node.
	virtual bool Visit(const Inconnu& /*unknown*/);
};

}  /* namespace xml */
}  /* namespace dls */
