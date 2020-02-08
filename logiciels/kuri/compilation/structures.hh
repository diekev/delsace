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

#include "biblinternes/memoire/logeuse_gardee.hh"
#include "biblinternes/outils/definitions.h"

/**
 * Ces structures sont les mêmes que celles définies par le langage (tableaux
 * via « []TYPE », et chaine via « chaine ») ; elles sont donc la même
 * définition que celles du langage. Elles sont utilisées pour pouvoir passer
 * des messages sainement entre la compilatrice et les métaprogrammes. Par
 * sainement, on entend que l'interface binaire de l'application doit être la
 * même.
 */

namespace kuri {

struct chaine {
	char *pointeur = nullptr;
	long taille = 0;

	chaine() = default;

	COPIE_CONSTRUCT(chaine);

	char &operator[](long i)
	{
		assert(i >= 0 && i < this->taille);
		return this->pointeur[i];
	}

	char const &operator[](long i) const
	{
		assert(i >= 0 && i < this->taille);
		return this->pointeur[i];
	}
};

template <typename T>
struct tableau {
	T *pointeur = nullptr;
	long taille = 0;

	tableau() = default;

	~tableau()
	{
		memoire::deloge_tableau("kuri::tableau", this->pointeur, this->taille);
	}

	COPIE_CONSTRUCT(tableau);

	T &operator[](long i)
	{
		assert(i >= 0 && i < this->taille);
		return this->pointeur[i];
	}

	T const &operator[](long i) const
	{
		assert(i >= 0 && i < this->taille);
		return this->pointeur[i];
	}
};

template <typename T>
void pousse(tableau<T> *tabl, T valeur)
{
	memoire::reloge_tableau("kuri::tableau", tabl->pointeur, tabl->taille, tabl->taille + 1);
	tabl->pointeur[tabl->taille - 1] = valeur;
}

#define POUR(x) for (int i = 0; i < (x).taille; ++i)

}
