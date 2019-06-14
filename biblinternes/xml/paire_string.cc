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

#include "paire_string.h"

#include "outils.h"
#include "xml.h" // pour TIXMLASSERT

namespace dls {
namespace xml {

PaireString::~PaireString()
{
	reinitialise();
}

void PaireString::initialise(char *debut, char *fin, int drapeaux)
{
	reinitialise();
	m_debut = debut;
	m_fin = fin;
	m_drapeaux = drapeaux | BESOIN_VIDAGE;
}

void PaireString::transfers_vers(PaireString *autre)
{
	if (this == autre) {
		return;
	}

	// This in effect implements the assignment operator by "moving"
	// ownership (as in auto_ptr).

	TIXMLASSERT(autre->m_drapeaux == 0);
	TIXMLASSERT(autre->m_debut == nullptr);
	TIXMLASSERT(autre->m_fin == nullptr);

	autre->reinitialise();

	autre->m_drapeaux = m_drapeaux;
	autre->m_debut = m_debut;
	autre->m_fin = m_fin;

	m_drapeaux = 0;
	m_debut = nullptr;
	m_fin = nullptr;
}

void PaireString::reinitialise()
{
	if (m_drapeaux & BESOIN_SUPPRESSION) {
		delete [] m_debut;
	}

	m_drapeaux = 0;
	m_debut = nullptr;
	m_fin = nullptr;
}


void PaireString::initialise_str(const char *str, int drapeaux)
{
	TIXMLASSERT(str);
	reinitialise();
	size_t len = strlen(str);
	TIXMLASSERT(m_debut == 0);
	m_debut = new char[ len+1 ];
	memcpy(m_debut, str, len+1);
	m_fin = m_debut + len;
	m_drapeaux = drapeaux | BESOIN_SUPPRESSION;
}

char *PaireString::analyse_texte(char *texte, const char *etiquette_fin, int drapeaux)
{
	TIXMLASSERT(etiquette_fin && *etiquette_fin);

	char *start = texte;
	char  endChar = *etiquette_fin;
	size_t length = strlen(etiquette_fin);

	// Inner loop of text parsing.
	while (*texte) {
		if (*texte == endChar && strncmp(texte, etiquette_fin, length) == 0) {
			initialise(start, texte, drapeaux);
			return texte + length;
		}

		++texte;
	}

	return nullptr;
}

char *PaireString::analyse_nom(char *nom)
{
	if (!nom || !(*nom)) {
		return nullptr;
	}

	if (!XMLUtil::IsNameStartChar(static_cast<unsigned char>(*nom))) {
		return nullptr;
	}

	char *start = nom;
	++nom;

	while (*nom && XMLUtil::IsNameChar(static_cast<unsigned char>(*nom))) {
		++nom;
	}

	initialise(start, nom, 0);
	return nom;
}

void PaireString::fusionne_espaces_blancs()
{
	// Adjusting _start would cause undefined behavior on delete[]
	TIXMLASSERT((m_drapeaux & BESOIN_SUPPRESSION) == 0);
	// Trim leading space.
	m_debut = XMLUtil::SkipWhiteSpace(m_debut);

	if (*m_debut) {
		char *p = m_debut;	// the read pointer
		char *q = m_debut;	// the write pointer

		while(*p) {
			if (XMLUtil::IsWhiteSpace(*p)) {
				p = XMLUtil::SkipWhiteSpace(p);
				if (*p == 0) {
					break;    // don't write to q; this trims the trailing space.
				}
				*q = ' ';
				++q;
			}
			*q = *p;
			++q;
			++p;
		}
		*q = 0;
	}
}

const char *PaireString::str()
{
	TIXMLASSERT(m_debut);
	TIXMLASSERT(m_fin);
	if (m_drapeaux & BESOIN_VIDAGE) {
		*m_fin = 0;
		m_drapeaux ^= BESOIN_VIDAGE;

		if (m_drapeaux) {
			char *p = m_debut;	// the read pointer
			char *q = m_debut;	// the write pointer

			while(p < m_fin) {
				if ((m_drapeaux & NEEDS_NEWLINE_NORMALIZATION) && *p == CR) {
					// CR-LF pair becomes LF
					// CR alone becomes LF
					// LF-CR becomes LF
					if (*(p+1) == LF) {
						p += 2;
					}
					else {
						++p;
					}
					*q++ = LF;
				}
				else if ((m_drapeaux & NEEDS_NEWLINE_NORMALIZATION) && *p == LF) {
					if (*(p+1) == CR) {
						p += 2;
					}
					else {
						++p;
					}
					*q++ = LF;
				}
				else if ((m_drapeaux & NEEDS_ENTITY_PROCESSING) && *p == '&') {
					// Entities handled by tinyXML2:
					// - special entities in the entity table [in/out]
					// - numeric character reference [in]
					//   &#20013; or &#x4e2d;

					if (*(p+1) == '#') {
						const int buflen = 10;
						char buf[buflen] = { 0 };
						int len = 0;
						char *adjusted = const_cast<char*>(XMLUtil::GetCharacterRef(p, buf, &len));
						if (adjusted == nullptr) {
							*q = *p;
							++p;
							++q;
						}
						else {
							TIXMLASSERT(0 <= len && len <= buflen);
							TIXMLASSERT(q + len <= adjusted);
							p = adjusted;
							memcpy(q, buf, static_cast<size_t>(len));
							q += len;
						}
					}
					else {
						bool entityFound = false;
						for(int i = 0; i < NUM_ENTITIES; ++i) {
							const Entity& entity = entities[i];
							if (strncmp(p + 1, entity.pattern, static_cast<size_t>(entity.length)) == 0
									&& *(p + entity.length + 1) == ';') {
								// Found an entity - convert.
								*q = entity.value;
								++q;
								p += entity.length + 2;
								entityFound = true;
								break;
							}
						}
						if (!entityFound) {
							// fixme: treat as error?
							++p;
							++q;
						}
					}
				}
				else {
					*q = *p;
					++p;
					++q;
				}
			}
			*q = 0;
		}

		// The loop below has plenty going on, and this
		// is a less useful mode. Break it out.
		if (m_drapeaux & NEEDS_WHITESPACE_COLLAPSING) {
			fusionne_espaces_blancs();
		}

		m_drapeaux = (m_drapeaux & BESOIN_SUPPRESSION);
	}

	TIXMLASSERT(m_debut);
	return m_debut;
}

bool PaireString::vide() const
{
	return m_debut == m_fin;
}

void PaireString::initialise_str_interne(const char *str)
{
	reinitialise();
	m_debut = const_cast<char *>(str);
}

}  /* namespace xml */
}  /* namespace dls */
