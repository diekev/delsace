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
#include "biblinternes/structures/vue_chaine_compacte.hh"

/**
 * Ces structures sont les mêmes que celles définies par le langage (tableaux
 * via « []TYPE », et chaine via « chaine ») ; elles sont donc la même
 * définition que celles du langage. Elles sont utilisées pour pouvoir passer
 * des messages sainement entre la compilatrice et les métaprogrammes. Par
 * sainement, on entend que l'interface binaire de l'application doit être la
 * même.
 */

namespace kuri {

#define POUR(x) for (auto &it : (x))

struct chaine {
	char *pointeur = nullptr;
	long taille = 0;

	chaine() = default;

	COPIE_CONSTRUCT(chaine);

	chaine(const char *c_str)
		: pointeur(const_cast<char *>(c_str))
	{
		while (*c_str++ != '\0') {
			taille += 1;
		}
	}

	chaine(dls::vue_chaine_compacte const &chn)
		: pointeur(const_cast<char *>(chn.pointeur()))
		, taille(chn.taille())
	{}

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

	char *begin()
	{
		return this->pointeur;
	}

	char const *begin() const
	{
		return this->pointeur;
	}

	char *end()
	{
		return this->begin() + this->taille;
	}

	char const *end() const
	{
		return this->begin() + this->taille;
	}
};

chaine copie_chaine(chaine &autre);

void detruit_chaine(chaine &chn);

bool operator == (kuri::chaine const &chn1, kuri::chaine const &chn2);

bool operator != (kuri::chaine const &chn1, kuri::chaine const &chn2);

std::ostream &operator<<(std::ostream &os, kuri::chaine const &chn);

template <typename T>
struct tableau {
	T *pointeur = nullptr;
	long taille = 0;

	tableau() = default;

	tableau(tableau const &autre)
	{
		memoire::reloge_tableau("kuri::tableau", this->pointeur, this->taille, autre.taille);
		this->taille = autre.taille;

		for (auto i = 0; i < autre.taille; ++i) {
			this->pointeur[i] = autre.pointeur[i];
		}
	}

	tableau &operator=(tableau const &autre)
	{
		memoire::reloge_tableau("kuri::tableau", this->pointeur, this->taille, autre.taille);
		this->taille = autre.taille;

		for (auto i = 0; i < autre.taille; ++i) {
			this->pointeur[i] = autre.pointeur[i];
		}

		return *this;
	}

	tableau(tableau &&autre)
	{
		std::swap(this->pointeur, autre.pointeur);
		std::swap(this->taille, autre.taille);
	}

	tableau &operator=(tableau &&autre)
	{
		std::swap(this->pointeur, autre.pointeur);
		std::swap(this->taille, autre.taille);
	}

	~tableau()
	{
		memoire::deloge_tableau("kuri::tableau", this->pointeur, this->taille);
	}

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

	void pousse(T const &valeur)
	{
		memoire::reloge_tableau("kuri::tableau", this->pointeur, this->taille, this->taille + 1);
		this->taille += 1;
		this->pointeur[this->taille - 1] = valeur;
	}

	void pousse_front(T const &valeur)
	{
		memoire::reloge_tableau("kuri::tableau", this->pointeur, this->taille, this->taille + 1);
		this->taille += 1;

		for (auto i = this->taille - 1; i >= 1; --i) {
			this->pointeur[i] = this->pointeur[i - 1];
		}

		this->pointeur[0] = valeur;
	}

	bool est_vide() const
	{
		return this->taille == 0;
	}

	T *begin()
	{
		return this->pointeur;
	}

	T const *begin() const
	{
		return this->pointeur;
	}

	T *end()
	{
		return this->begin() + this->taille;
	}

	T const *end() const
	{
		return this->begin() + this->taille;
	}
};

template <typename T>
void pousse(tableau<T> *tabl, T valeur)
{
	memoire::reloge_tableau("kuri::tableau", tabl->pointeur, tabl->taille, tabl->taille + 1);
	tabl->taille += 1;
	tabl->pointeur[tabl->taille - 1] = valeur;
}

}
