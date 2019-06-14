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

/**
 * A class that wraps strings. Normally stores the start and end pointers into
 * the XML file itself, and will apply normalization and entity translation if
 * actually read. Can also store (and memory manage) a traditional char[]
 */
class PaireString {
	enum {
		BESOIN_VIDAGE      = 0x100,
		BESOIN_SUPPRESSION = 0x200
	};

	int m_drapeaux = 0;
	char *m_debut = nullptr;
	char *m_fin = nullptr;

public:
	enum {
		NEEDS_ENTITY_PROCESSING			= 0x01,
		NEEDS_NEWLINE_NORMALIZATION		= 0x02,
		NEEDS_WHITESPACE_COLLAPSING     = 0x04,

		TEXT_ELEMENT               = NEEDS_ENTITY_PROCESSING | NEEDS_NEWLINE_NORMALIZATION,
		TEXT_ELEMENT_LEAVE_ENTITIES		= NEEDS_NEWLINE_NORMALIZATION,
		ATTRIBUTE_NAME		            	= 0,
		ATTRIBUTE_VALUE		            	= NEEDS_ENTITY_PROCESSING | NEEDS_NEWLINE_NORMALIZATION,
		ATTRIBUTE_VALUE_LEAVE_ENTITIES  	= NEEDS_NEWLINE_NORMALIZATION,
		COMMENT				        = NEEDS_NEWLINE_NORMALIZATION
	};

	PaireString() = default;

	/* Pas possible. */
	PaireString(const PaireString &autre) = delete;

	~PaireString();

	/* Pas possible, utiliser TransferTo(). */
	PaireString &operator=(const PaireString &autre) = delete;

	void initialise(char *debut, char *fin, int drapeaux);

	const char *str();

	bool vide() const;

	void initialise_str_interne(const char *str);

	void initialise_str(const char *str, int drapeaux = 0);

	char *analyse_texte(char *texte, const char *etiquette_fin, int drapeaux);

	char *analyse_nom(char *nom);

	void transfers_vers(PaireString *autre);

private:
	void reinitialise();

	void fusionne_espaces_blancs();
};

}  /* namespace xml */
}  /* namespace dls */
