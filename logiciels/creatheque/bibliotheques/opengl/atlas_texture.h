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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

class AtlasTexture {
	unsigned int m_bindcode = 0;
	int m_format_interne = 0;
	int m_bordure = 0;
	unsigned int m_nombre = 0;
	unsigned int m_format = 0;
	unsigned int m_type = 0;

public:
	explicit AtlasTexture(unsigned int nombre);

	~AtlasTexture();

	void detruit(bool renouvel);

	void attache() const;

	void detache() const;

	void typage(unsigned int type, unsigned int format, int format_interne);

	void filtre_min_mag(int min, int mag) const;

	void rempli(const void *donnees, int taille[3]) const;

	void enveloppage(int envelope) const;

	unsigned int nombre() const;
};
