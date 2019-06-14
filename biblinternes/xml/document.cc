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

#include "document.h"

#include <new>  /* Placement new. */

#include "declaration.h"
#include "imprimeur.h"
#include "inconnu.h"
#include "outils.h"
#include "xml.h"  // TIXMLASSERT
#include "visiteur.h"

namespace dls {
namespace xml {

char* Document::Identify(char* p, Noeud** node)
{
	TIXMLASSERT(node);
	TIXMLASSERT(p);
	char* const start = p;
	p = XMLUtil::SkipWhiteSpace(p);
	if(!*p) {
		*node = nullptr;
		TIXMLASSERT(p);
		return p;
	}

	// These strings define the matching patterns:
	static const char* xmlHeader		= { "<?" };
	static const char* commentHeader	= { "<!--" };
	static const char* cdataHeader		= { "<![CDATA[" };
	static const char* dtdHeader		= { "<!" };
	static const char* elementHeader	= { "<" };	// and a header for everything else; check last.

	static const int xmlHeaderLen		= 2;
	static const int commentHeaderLen	= 4;
	static const int cdataHeaderLen		= 9;
	static const int dtdHeaderLen		= 2;
	static const int elementHeaderLen	= 1;

	TIXMLASSERT(sizeof(Commentaire) == sizeof(Inconnu));		// use same memory pool
	TIXMLASSERT(sizeof(Commentaire) == sizeof(Declaration));	// use same memory pool
	Noeud* returnNode = nullptr;
	if (XMLUtil::StringEqual(p, xmlHeader, xmlHeaderLen)) {
		TIXMLASSERT(sizeof(Declaration) == m_ensemble_commentaire.ItemSize());
		returnNode = new (m_ensemble_commentaire.Alloc()) Declaration(this);
		returnNode->_memPool = &m_ensemble_commentaire;
		p += xmlHeaderLen;
	}
	else if (XMLUtil::StringEqual(p, commentHeader, commentHeaderLen)) {
		TIXMLASSERT(sizeof(Commentaire) == m_ensemble_commentaire.ItemSize());
		returnNode = new (m_ensemble_commentaire.Alloc()) Commentaire(this);
		returnNode->_memPool = &m_ensemble_commentaire;
		p += commentHeaderLen;
	}
	else if (XMLUtil::StringEqual(p, cdataHeader, cdataHeaderLen)) {
		TIXMLASSERT(sizeof(Texte) == m_ensemble_texte.ItemSize());
		Texte* text = new (m_ensemble_texte.Alloc()) Texte(this);
		returnNode = text;
		returnNode->_memPool = &m_ensemble_texte;
		p += cdataHeaderLen;
		text->SetCData(true);
	}
	else if (XMLUtil::StringEqual(p, dtdHeader, dtdHeaderLen)) {
		TIXMLASSERT(sizeof(Inconnu) == m_ensemble_commentaire.ItemSize());
		returnNode = new (m_ensemble_commentaire.Alloc()) Inconnu(this);
		returnNode->_memPool = &m_ensemble_commentaire;
		p += dtdHeaderLen;
	}
	else if (XMLUtil::StringEqual(p, elementHeader, elementHeaderLen)) {
		TIXMLASSERT(sizeof(Element) == m_ensemble_element.ItemSize());
		returnNode = new (m_ensemble_element.Alloc()) Element(this);
		returnNode->_memPool = &m_ensemble_element;
		p += elementHeaderLen;
	}
	else {
		TIXMLASSERT(sizeof(Texte) == m_ensemble_texte.ItemSize());
		returnNode = new (m_ensemble_texte.Alloc()) Texte(this);
		returnNode->_memPool = &m_ensemble_texte;
		p = start;	// Back it up, all the text counts.
	}

	TIXMLASSERT(returnNode);
	TIXMLASSERT(p);
	*node = returnNode;
	return p;
}

Noeud *Document::ShallowClone(Document *) const	{
	return nullptr;
}

bool Document::ShallowEqual(const Noeud *) const	{
	return false;
}


bool Document::Accept(Visiteur* visitor) const
{
	TIXMLASSERT(visitor);
	if (visitor->VisitEnter(*this)) {
		for (const Noeud* node=FirstChild(); node; node=node->NextSibling()) {
			if (!node->Accept(visitor)) {
				break;
			}
		}
	}
	return visitor->VisitExit(*this);
}

// Warning: List must match 'enum XMLError'
const char* Document::m_noms_erreurs[XML_ERROR_COUNT] = {
	"XML_SUCCESS",
	"XML_NO_ATTRIBUTE",
	"XML_WRONG_ATTRIBUTE_TYPE",
	"XML_ERROR_FILE_NOT_FOUND",
	"XML_ERROR_FILE_COULD_NOT_BE_OPENED",
	"XML_ERROR_FILE_READ_ERROR",
	"XML_ERROR_ELEMENT_MISMATCH",
	"XML_ERROR_PARSING_ELEMENT",
	"XML_ERROR_PARSING_ATTRIBUTE",
	"XML_ERROR_IDENTIFYING_TAG",
	"XML_ERROR_PARSING_TEXT",
	"XML_ERROR_PARSING_CDATA",
	"XML_ERROR_PARSING_COMMENT",
	"XML_ERROR_PARSING_DECLARATION",
	"XML_ERROR_PARSING_UNKNOWN",
	"XML_ERROR_EMPTY_DOCUMENT",
	"XML_ERROR_MISMATCHED_ELEMENT",
	"XML_ERROR_PARSING",
	"XML_CAN_NOT_CONVERT_TEXT",
	"XML_NO_TEXT_NODE"
};


Document::Document(bool processEntities, Whitespace whitespace)
	: Noeud(nullptr)
	, m_ecris_BOM(false)
	, m_traites_entities(processEntities)
	, m_id_erreur(XML_NO_ERROR)
	, m_espace_blanc(whitespace)
	, m_str_erreur_1(nullptr)
	, m_str_erreur_2(nullptr)
	, m_tampon_char(nullptr)
{
	// avoid VC++ C4355 warning about 'this' in initializer list (C4355 is off by default in VS2012+)
	_document = this;
}

Document::~Document()
{
	Clear();
}

Document *Document::ToDocument()				{
	TIXMLASSERT(this == _document);
	return this;
}

const Document *Document::ToDocument() const	{
	TIXMLASSERT(this == _document);
	return this;
}


void Document::Clear()
{
	DeleteChildren();

#ifdef DEBUG
	const bool hadError = Error();
#endif
	m_id_erreur = XML_NO_ERROR;
	m_str_erreur_1 = nullptr;
	m_str_erreur_2 = nullptr;

	delete [] m_tampon_char;
	m_tampon_char = nullptr;

#if 0
	_textPool.Trace("text");
	_elementPool.Trace("element");
	_commentPool.Trace("comment");
	_attributePool.Trace("attribute");
#endif

#ifdef DEBUG
	if (!hadError) {
		TIXMLASSERT(_elementPool.CurrentAllocs()   == _elementPool.Untracked());
		TIXMLASSERT(_attributePool.CurrentAllocs() == _attributePool.Untracked());
		TIXMLASSERT(_textPool.CurrentAllocs()      == _textPool.Untracked());
		TIXMLASSERT(_commentPool.CurrentAllocs()   == _commentPool.Untracked());
	}
#endif
}


Element* Document::NewElement(const char* name)
{
	TIXMLASSERT(sizeof(Element) == m_ensemble_element.ItemSize());
	Element* ele = new (m_ensemble_element.Alloc()) Element(this);
	ele->_memPool = &m_ensemble_element;
	ele->SetName(name);
	return ele;
}


Commentaire* Document::NewComment(const char* str)
{
	TIXMLASSERT(sizeof(Commentaire) == m_ensemble_commentaire.ItemSize());
	Commentaire* comment = new (m_ensemble_commentaire.Alloc()) Commentaire(this);
	comment->_memPool = &m_ensemble_commentaire;
	comment->SetValue(str);
	return comment;
}


Texte* Document::NewText(const char* str)
{
	TIXMLASSERT(sizeof(Texte) == m_ensemble_texte.ItemSize());
	Texte* text = new (m_ensemble_texte.Alloc()) Texte(this);
	text->_memPool = &m_ensemble_texte;
	text->SetValue(str);
	return text;
}


Declaration* Document::NewDeclaration(const char* str)
{
	TIXMLASSERT(sizeof(Declaration) == m_ensemble_commentaire.ItemSize());
	Declaration* dec = new (m_ensemble_commentaire.Alloc()) Declaration(this);
	dec->_memPool = &m_ensemble_commentaire;
	dec->SetValue(str ? str : "xml version=\"1.0\" encoding=\"UTF-8\"");
	return dec;
}


Inconnu* Document::NewUnknown(const char* str)
{
	TIXMLASSERT(sizeof(Inconnu) == m_ensemble_commentaire.ItemSize());
	Inconnu* unk = new (m_ensemble_commentaire.Alloc()) Inconnu(this);
	unk->_memPool = &m_ensemble_commentaire;
	unk->SetValue(str);
	return unk;
}

static FILE* callfopen(const char* filepath, const char* mode)
{
	TIXMLASSERT(filepath);
	TIXMLASSERT(mode);
#if defined(_MSC_VER) && (_MSC_VER >= 1400) && (!defined WINCE)
	FILE* fp = 0;
	errno_t err = fopen_s(&fp, filepath, mode);
	if (err) {
		return 0;
	}
#else
	FILE* fp = fopen(filepath, mode);
#endif
	return fp;
}

void Document::DeleteNode(Noeud* node)	{
	TIXMLASSERT(node);
	TIXMLASSERT(node->_document == this);
	if (node->_parent) {
		node->_parent->DeleteChild(node);
	}
	else {
		// Isn't in the tree.
		// Use the parent delete.
		// Also, we need to mark it tracked: we 'know'
		// it was never used.
		node->_memPool->SetTracked();
		// Call the static Noeud version:
		Noeud::DeleteNode(node);
	}
}


XMLError Document::LoadFile(const char* filename)
{
	Clear();
	FILE* fp = callfopen(filename, "rb");
	if (!fp) {
		SetError(XML_ERROR_FILE_NOT_FOUND, filename, nullptr);
		return m_id_erreur;
	}
	LoadFile(fp);
	fclose(fp);
	return m_id_erreur;
}

// This is likely overengineered template art to have a check that unsigned long value incremented
// by one still fits into size_t. If size_t type is larger than unsigned long type
// (x86_64-w64-mingw32 target) then the check is redundant and gcc and clang emit
// -Wtype-limits warning. This piece makes the compiler select code with a check when a check
// is useful and code with no check when a check is redundant depending on how size_t and unsigned long
// types sizes relate to each other.
template
<bool = (sizeof(unsigned long) >= sizeof(size_t))>
struct LongFitsIntoSizeTMinusOne {
	static bool Fits(unsigned long value)
	{
		return value < -1ul;
	}
};

template <>
bool LongFitsIntoSizeTMinusOne<false>::Fits(unsigned long /*value*/)
{
	return true;
}

XMLError Document::LoadFile(FILE* fp)
{
	Clear();

	fseek(fp, 0, SEEK_SET);
	if (fgetc(fp) == EOF && ferror(fp) != 0) {
		SetError(XML_ERROR_FILE_READ_ERROR, nullptr, nullptr);
		return m_id_erreur;
	}

	fseek(fp, 0, SEEK_END);
	const long filelength = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if (filelength == -1L) {
		SetError(XML_ERROR_FILE_READ_ERROR, nullptr, nullptr);
		return m_id_erreur;
	}

	if (!LongFitsIntoSizeTMinusOne<>::Fits(static_cast<size_t>(filelength))) {
		// Cannot handle files which won't fit in buffer together with null terminator
		SetError(XML_ERROR_FILE_READ_ERROR, nullptr, nullptr);
		return m_id_erreur;
	}

	if (filelength == 0) {
		SetError(XML_ERROR_EMPTY_DOCUMENT, nullptr, nullptr);
		return m_id_erreur;
	}

	auto const size = static_cast<size_t>(filelength);
	TIXMLASSERT(m_tampon_char == 0);
	m_tampon_char = new char[size+1];
	size_t read = fread(m_tampon_char, 1, size, fp);
	if (read != size) {
		SetError(XML_ERROR_FILE_READ_ERROR, nullptr, nullptr);
		return m_id_erreur;
	}

	m_tampon_char[size] = 0;

	Parse();
	return m_id_erreur;
}


XMLError Document::SaveFile(const char* filename, bool compact)
{
	FILE* fp = callfopen(filename, "w");
	if (!fp) {
		SetError(XML_ERROR_FILE_COULD_NOT_BE_OPENED, filename, nullptr);
		return m_id_erreur;
	}
	SaveFile(fp, compact);
	fclose(fp);
	return m_id_erreur;
}


XMLError Document::SaveFile(FILE* fp, bool compact)
{
	// Clear any error from the last save, otherwise it will get reported
	// for *this* call.
	SetError(XML_NO_ERROR, nullptr, nullptr);
	XMLPrinter stream(fp, compact);
	Print(&stream);
	return m_id_erreur;
}

bool Document::ProcessEntities() const
{
	return m_traites_entities;
}

Whitespace Document::WhitespaceMode() const
{
	return m_espace_blanc;
}

bool Document::HasBOM() const
{
	return m_ecris_BOM;
}

void Document::SetBOM(bool useBOM)
{
	m_ecris_BOM = useBOM;
}

Element *Document::RootElement()
{
	return FirstChildElement();
}

const Element *Document::RootElement() const
{
	return FirstChildElement();
}


XMLError Document::Parse(const char* p, size_t len)
{
	Clear();

	if (len == 0 || !p || !*p) {
		SetError(XML_ERROR_EMPTY_DOCUMENT, nullptr, nullptr);
		return m_id_erreur;
	}
	if (len == -1ul) {
		len = strlen(p);
	}
	TIXMLASSERT(m_tampon_char == 0);
	m_tampon_char = new char[ len+1 ];
	memcpy(m_tampon_char, p, len);
	m_tampon_char[len] = 0;

	Parse();
	if (Error()) {
		// clean up now essentially dangling memory.
		// and the parse fail can put objects in the
		// pools that are dead and inaccessible.
		DeleteChildren();
		m_ensemble_element.Clear();
		m_ensemble_attribut.Clear();
		m_ensemble_texte.Clear();
		m_ensemble_commentaire.Clear();
	}
	return m_id_erreur;
}


void Document::Print(XMLPrinter* streamer) const
{
	if (streamer) {
		Accept(streamer);
	}
	else {
		XMLPrinter stdoutStreamer(stdout);
		Accept(&stdoutStreamer);
	}
}


void Document::SetError(XMLError error, const char* str1, const char* str2)
{
	TIXMLASSERT(error >= 0 && error < XML_ERROR_COUNT);
	m_id_erreur = error;
	m_str_erreur_1 = str1;
	m_str_erreur_2 = str2;
}

bool Document::Error() const {
	return m_id_erreur != XML_NO_ERROR;
}

XMLError Document::ErrorID() const {
	return m_id_erreur;
}

const char* Document::ErrorName() const
{
	TIXMLASSERT(m_id_erreur >= 0 && m_id_erreur < XML_ERROR_COUNT);
	const char* errorName = m_noms_erreurs[m_id_erreur];
	TIXMLASSERT(errorName && errorName[0]);
	return errorName;
}

const char *Document::GetErrorStr1() const {
	return m_str_erreur_1;
}

const char *Document::GetErrorStr2() const {
	return m_str_erreur_2;
}

void Document::PrintError() const
{
	if (Error()) {
		static const int LEN = 20;
		char buf1[LEN] = { 0 };
		char buf2[LEN] = { 0 };

		if (m_str_erreur_1) {
			TIXML_SNPRINTF(buf1, LEN, "%s", m_str_erreur_1);
		}
		if (m_str_erreur_2) {
			TIXML_SNPRINTF(buf2, LEN, "%s", m_str_erreur_2);
		}

		// Should check INT_MIN <= _errorID && _errorId <= INT_MAX, but that
		// causes a clang "always true" -Wtautological-constant-out-of-range-compare warning
		TIXMLASSERT(0 <= m_id_erreur && XML_ERROR_COUNT - 1 <= INT_MAX);
		printf("Document error id=%d '%s' str1=%s str2=%s\n",
			   static_cast<int>(m_id_erreur), ErrorName(), buf1, buf2);
	}
}

void Document::Parse()
{
	TIXMLASSERT(NoChildren()); // Clear() must have been called previously
	TIXMLASSERT(m_tampon_char);
	char* p = m_tampon_char;
	p = XMLUtil::SkipWhiteSpace(p);
	p = const_cast<char*>(XMLUtil::ReadBOM(p, &m_ecris_BOM));
	if (!*p) {
		SetError(XML_ERROR_EMPTY_DOCUMENT, nullptr, nullptr);
		return;
	}
	ParseDeep(p, nullptr);
}

}  /* namespace xml */
}  /* namespace dls */
