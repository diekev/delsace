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

#include "erreur.h"
#include "noeud.h"

namespace dls {
namespace xml {

class Attribut;

/** The element is a container class. It has a value, the element name,
	and can contain other elements, text, comments, and unknowns.
	Elements also contain an arbitrary number of attributes.
*/
class Element final : public Noeud {
	friend class Document;

public:
	/// Get the name of an element (which is the Value() of the node.)
	const char* Name() const;
	/// Set the name of the element.
	void SetName(const char* str, bool staticMem=false);

	Element* ToElement()	override;
	const Element* ToElement() const override;
	bool Accept(Visiteur* visitor) const override;

	/** Given an attribute name, Attribut() returns the value
		for the attribute of that name, or null if none
		exists. For example:

		@verbatim
		const char* value = ele->Attribut("foo");
		@endverbatim

		The 'value' parameter is normally null. However, if specified,
		the attribute will only be returned if the 'name' and 'value'
		match. This allow you to write code:

		@verbatim
		if (ele->Attribut("foo", "bar")) callFooIsBar();
		@endverbatim

		rather than:
		@verbatim
		if (ele->Attribut("foo")) {
			if (strcmp(ele->Attribut("foo"), "bar") == 0) callFooIsBar();
		}
		@endverbatim
	*/
	const char* attribut(const char* name, const char* value=0) const;

	/** Given an attribute name, IntAttribut() returns the value
		of the attribute interpreted as an integer. 0 will be
		returned if there is an error. For a method with error
		checking, see QueryIntAttribut()
	*/
	int		 IntAttribut(const char* name) const;
	/// See IntAttribut()
	unsigned UnsignedAttribut(const char* name) const;
	/// See IntAttribut()
	bool	 BoolAttribut(const char* name) const;
	/// See IntAttribut()
	double 	 DoubleAttribut(const char* name) const;
	/// See IntAttribut()
	float	 FloatAttribut(const char* name) const;

	/** Given an attribute name, QueryIntAttribut() returns
		XML_NO_ERROR, XML_WRONG_ATTRIBUTE_TYPE if the conversion
		can't be performed, or XML_NO_ATTRIBUTE if the attribute
		doesn't exist. If successful, the result of the conversion
		will be written to 'value'. If not successful, nothing will
		be written to 'value'. This allows you to provide default
		value:

		@verbatim
		int value = 10;
		QueryIntAttribut("foo", &value);		// if "foo" isn't found, value will still be 10
		@endverbatim
	*/
	XMLError QueryIntAttribut(const char* name, int* value) const;
	/// See QueryIntAttribut()
	XMLError QueryUnsignedAttribut(const char* name, unsigned int* value) const;
	/// See QueryIntAttribut()
	XMLError QueryBoolAttribut(const char* name, bool* value) const;
	/// See QueryIntAttribut()
	XMLError QueryDoubleAttribut(const char* name, double* value) const;
	/// See QueryIntAttribut()
	XMLError QueryFloatAttribut(const char* name, float* value) const;


	/** Given an attribute name, QueryAttribut() returns
		XML_NO_ERROR, XML_WRONG_ATTRIBUTE_TYPE if the conversion
		can't be performed, or XML_NO_ATTRIBUTE if the attribute
		doesn't exist. It is overloaded for the primitive types,
		and is a generally more convenient replacement of
		QueryIntAttribut() and related functions.

		If successful, the result of the conversion
		will be written to 'value'. If not successful, nothing will
		be written to 'value'. This allows you to provide default
		value:

		@verbatim
		int value = 10;
		QueryAttribut("foo", &value);		// if "foo" isn't found, value will still be 10
		@endverbatim
	*/
	int QueryAttribut(const char* name, int* value) const;

	int QueryAttribut(const char* name, unsigned int* value) const;

	int QueryAttribut(const char* name, bool* value) const;

	int QueryAttribut(const char* name, double* value) const;

	int QueryAttribut(const char* name, float* value) const;

	/// Sets the named attribute to value.
	void SetAttribut(const char* name, const char* value);
	/// Sets the named attribute to value.
	void SetAttribut(const char* name, int value);
	/// Sets the named attribute to value.
	void SetAttribut(const char* name, unsigned value);
	/// Sets the named attribute to value.
	void SetAttribut(const char* name, bool value);
	/// Sets the named attribute to value.
	void SetAttribut(const char* name, double value);
	/// Sets the named attribute to value.
	void SetAttribut(const char* name, float value);

	/**
		Delete an attribute.
	*/
	void DeleteAttribut(const char* name);

	/// Return the first attribute in the list.
	const Attribut* FirstAttribut() const;
	/// Query a specific attribute in the list.
	const Attribut* FindAttribut(const char* name) const;

	/** Convenience function for easy access to the text inside an element. Although easy
		and concise, GetText() is limited compared to getting the Texte child
		and accessing it directly.

		If the first child of 'this' is a Texte, the GetText()
		returns the character string of the Text node, else null is returned.

		This is a convenient method for getting the text of simple contained text:
		@verbatim
		<foo>This is text</foo>
			const char* str = fooElement->GetText();
		@endverbatim

		'str' will be a pointer to "This is text".

		Note that this function can be misleading. If the element foo was created from
		this XML:
		@verbatim
			<foo><b>This is text</b></foo>
		@endverbatim

		then the value of str would be null. The first child node isn't a text node, it is
		another element. From this XML:
		@verbatim
			<foo>This is <b>text</b></foo>
		@endverbatim
		GetText() will return "This is ".
	*/
	const char* GetText() const;

	/** Convenience function for easy access to the text inside an element. Although easy
		and concise, SetText() is limited compared to creating an Texte child
		and mutating it directly.

		If the first child of 'this' is a Texte, SetText() sets its value to
		the given string, otherwise it will create a first child that is an Texte.

		This is a convenient method for setting the text of simple contained text:
		@verbatim
		<foo>This is text</foo>
			fooElement->SetText("Hullaballoo!");
		<foo>Hullaballoo!</foo>
		@endverbatim

		Note that this function can be misleading. If the element foo was created from
		this XML:
		@verbatim
			<foo><b>This is text</b></foo>
		@endverbatim

		then it will not change "This is text", but rather prefix it with a text element:
		@verbatim
			<foo>Hullaballoo!<b>This is text</b></foo>
		@endverbatim

		For this XML:
		@verbatim
			<foo />
		@endverbatim
		SetText() will generate
		@verbatim
			<foo>Hullaballoo!</foo>
		@endverbatim
	*/
	void SetText(const char* inText);
	/// Convenience method for setting text inside an element. See SetText() for important limitations.
	void SetText(int value);
	/// Convenience method for setting text inside an element. See SetText() for important limitations.
	void SetText(unsigned value);
	/// Convenience method for setting text inside an element. See SetText() for important limitations.
	void SetText(bool value);
	/// Convenience method for setting text inside an element. See SetText() for important limitations.
	void SetText(double value);
	/// Convenience method for setting text inside an element. See SetText() for important limitations.
	void SetText(float value);

	/**
		Convenience method to query the value of a child text node. This is probably best
		shown by example. Given you have a document is this form:
		@verbatim
			<point>
				<x>1</x>
				<y>1.4</y>
			</point>
		@endverbatim

		The QueryIntText() and similar functions provide a safe and easier way to get to the
		"value" of x and y.

		@verbatim
			int x = 0;
			float y = 0;	// types of x and y are contrived for example
			const Element* xElement = pointElement->FirstChildElement("x");
			const Element* yElement = pointElement->FirstChildElement("y");
			xElement->QueryIntText(&x);
			yElement->QueryFloatText(&y);
		@endverbatim

		@returns XML_SUCCESS (0) on success, XML_CAN_NOT_CONVERT_TEXT if the text cannot be converted
				 to the requested type, and XML_NO_TEXT_NODE if there is no child text to query.

	*/
	XMLError QueryIntText(int* ival) const;
	/// See QueryIntText()
	XMLError QueryUnsignedText(unsigned* uval) const;
	/// See QueryIntText()
	XMLError QueryBoolText(bool* bval) const;
	/// See QueryIntText()
	XMLError QueryDoubleText(double* dval) const;
	/// See QueryIntText()
	XMLError QueryFloatText(float* fval) const;

	// internal:
	enum {
		OPEN,		// <foo>
		CLOSED,		// <foo/>
		CLOSING		// </foo>
	};
	int ClosingType() const;
	Noeud* ShallowClone(Document* document) const override;
	bool ShallowEqual(const Noeud* compare) const override;

protected:
	char* ParseDeep(char* p, PaireString* endTag);

private:
	explicit Element(Document* doc);
	virtual ~Element();
	Element(const Element&);	// not supported
	void operator=(const Element&);	// not supported

	Attribut* FindAttribut(const char* name);
	Attribut* FindOrCreateAttribut(const char* name);
	//void LinkAttribut(Attribut* attrib);
	char* ParseAttributs(char* p);
	static void DeleteAttribut(Attribut* attribute);

	enum { BUF_SIZE = 200 };
	int _closingType;
	// The attribute list is ordered; there is no 'lastAttribut'
	// because the list needs to be scanned for dupes before adding
	// a new attribute.
	Attribut* _rootAttribut;
};

}  /* namespace xml */
}  /* namespace dls */

