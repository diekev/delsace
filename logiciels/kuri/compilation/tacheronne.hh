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

#include "unite_compilation.hh"

#include "../representation_intermediaire/constructrice_ri.hh"

struct Compilatrice;

struct Tacheronne {
	Compilatrice &compilatrice;

	ConstructriceRI constructrice_ri{compilatrice};

	double temps_validation = 0.0;
	double temps_lexage = 0.0;
	double temps_parsage = 0.0;

	Tacheronne(Compilatrice &comp);

	void gere_tache();

	void gere_unite(UniteCompilation unite);

//	// données par thread
//	Lexeuse lexeuse;
//	Syntaxeuse syntaxeuse;
//	ConstructriceRI constructrice_ri;
//	ValideuseSyntaxe valideuse;
//	Coulisse coulisse;

//	// données globales
//	Compilatrice compilatrice;
//	Typeuse typeuse;
//	Operateur operateurs;
//	NormalisatriceNom normalisatrice_nom;
};
