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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/structures/vue_chaine.hh"

struct Enchaineuse {
	static constexpr auto TAILLE_TAMPON = 16 * 1024;

	struct Tampon {
		char donnees[TAILLE_TAMPON];
		int occupe = 0;
		Tampon *suivant = nullptr;
	};

	Tampon m_tampon_base{};
	Tampon *tampon_courant = nullptr;

	Enchaineuse();

	Enchaineuse(Enchaineuse const &) = delete;
	Enchaineuse &operator=(Enchaineuse const &) = delete;

	~Enchaineuse();

	void pousse(dls::vue_chaine const &chn);

	void pousse(const char *c_str, long N);

	void pousse_caractere(char c);

private:
	void ajoute_tampon();
};
