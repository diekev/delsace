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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/langage/unicode.hh"
#include "biblinternes/structures/tableau.hh"

struct ChaineUTF8 {
	dls::chaine chaine{};
	long m_taille = 0;

	ChaineUTF8(char const *chn);

	ChaineUTF8(dls::chaine const &chn);

	long taille() const;

	void calcule_taille();
};

std::ostream &operator<<(std::ostream &os, ChaineUTF8 const &chaine);

enum class Alignement {
	GAUCHE,
	DROITE,
};

struct Tableau {
	struct Ligne {
		dls::tableau<ChaineUTF8> colonnes{};
	};

	dls::tableau<Ligne> lignes{};
	dls::tableau<Alignement> alignements{};
	long nombre_colonnes = 0;

	Tableau(std::initializer_list<ChaineUTF8> const &titres);

	void alignement(int idx, Alignement a);

	void ajoute_ligne(std::initializer_list<ChaineUTF8> const &valeurs);
};

void imprime_tableau(Tableau &tableau);
