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

#include "biblinternes/structures/flux_chaine.hh"
#include "biblinternes/structures/chaine.hh"
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

	void imprime_dans_flux(std::ostream &flux);

	void ajoute_tampon();

	int nombre_tampons() const;

	int nombre_tampons_alloues() const;

	long taille_chaine() const;

	dls::chaine chaine() const;
};

template <typename T>
Enchaineuse &operator << (Enchaineuse &enchaineuse, T const &valeur)
{
	dls::flux_chaine flux;
	flux << valeur;

	for (auto c : flux.chn()) {
		enchaineuse.pousse_caractere(c);
	}

	return enchaineuse;
}

template <size_t N>
Enchaineuse &operator << (Enchaineuse &enchaineuse, const char (&c)[N])
{
	enchaineuse.pousse(c, static_cast<long>(N));
	return enchaineuse;
}

Enchaineuse &operator << (Enchaineuse &enchaineuse, dls::vue_chaine_compacte const &chn);

Enchaineuse &operator << (Enchaineuse &enchaineuse, dls::chaine const &chn);

Enchaineuse &operator << (Enchaineuse &enchaineuse, const char *chn);
