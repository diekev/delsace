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
#include "biblinternes/structures/chaine.hh"
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

	explicit chaine(const char *c_str)
		: pointeur(const_cast<char *>(c_str))
	{
		while (*c_str++ != '\0') {
			taille += 1;
		}
	}

	chaine(dls::chaine const &chn)
		: pointeur(const_cast<char *>(chn.c_str()))
		, taille(chn.taille())
	{}

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

	void pousse(char c)
	{
		memoire::reloge_tableau("chaine", this->pointeur, this->taille, this->taille + 1);
		pousse_reserve(c);
	}

	void pousse_reserve(char c)
	{
		this->pointeur[this->taille] = c;
		this->taille += 1;
	}

	void reserve(long nouvelle_taille)
	{
		if (nouvelle_taille <= this->taille) {
			return;
		}

		memoire::reloge_tableau("chaine", this->pointeur, this->taille, this->taille + nouvelle_taille);
	}
};

chaine copie_chaine(chaine const &autre);

void detruit_chaine(chaine &chn);

bool operator == (kuri::chaine const &chn1, kuri::chaine const &chn2);

bool operator != (kuri::chaine const &chn1, kuri::chaine const &chn2);

std::ostream &operator<<(std::ostream &os, kuri::chaine const &chn);

template <typename T>
struct tableau {
	T *pointeur = nullptr;
	long taille = 0;
	long capacite = 0;

	tableau() = default;

	explicit tableau(long taille_initiale)
		: pointeur(memoire::loge_tableau<T>("kuri::tableau", taille_initiale))
		, taille(taille_initiale)
		, capacite(taille_initiale)
	{}

	tableau(tableau const &autre)
	{
		memoire::reloge_tableau("kuri::tableau", this->pointeur, this->taille, autre.taille);
		this->taille = autre.taille;
		this->capacite = autre.capacite;

		for (auto i = 0; i < autre.taille; ++i) {
			this->pointeur[i] = autre.pointeur[i];
		}
	}

	tableau &operator=(tableau const &autre)
	{
		memoire::reloge_tableau("kuri::tableau", this->pointeur, this->taille, autre.taille);
		this->taille = autre.taille;
		this->capacite = autre.capacite;

		for (auto i = 0; i < autre.taille; ++i) {
			this->pointeur[i] = autre.pointeur[i];
		}

		return *this;
	}

	tableau(tableau &&autre)
	{
		std::swap(this->pointeur, autre.pointeur);
		std::swap(this->taille, autre.taille);
		std::swap(this->capacite, autre.capacite);
	}

	tableau &operator=(tableau &&autre)
	{
		std::swap(this->pointeur, autre.pointeur);
		std::swap(this->taille, autre.taille);
		std::swap(this->capacite, autre.capacite);

		return *this;
	}

	~tableau()
	{
		memoire::deloge_tableau("kuri::tableau", this->pointeur, this->capacite);
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
		reserve(this->taille + 1);
		this->taille += 1;
		this->pointeur[this->taille - 1] = valeur;
	}

	void pousse(T &&valeur)
	{
		reserve(this->taille + 1);
		this->taille += 1;
		this->pointeur[this->taille - 1] = std::move(valeur);
	}

	void pousse_front(T const &valeur)
	{
		reserve(this->taille + 1);
		this->taille += 1;

		for (auto i = this->taille - 1; i >= 1; --i) {
			this->pointeur[i] = this->pointeur[i - 1];
		}

		this->pointeur[0] = valeur;
	}

	void reserve(long nombre)
	{
		if (capacite >= nombre) {
			return;
		}

		memoire::reloge_tableau("kuri::tableau", this->pointeur, this->capacite, nombre);
		this->capacite = nombre;
	}

	void reserve_delta(long delta)
	{
		if (capacite >= (taille + delta)) {
			return;
		}

		memoire::reloge_tableau("kuri::tableau", this->pointeur, this->capacite, this->capacite + delta);
		this->capacite += delta;
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

	T &a(long index)
	{
		return this->pointeur[index];
	}

	T const &a(long index)  const
	{
		return this->pointeur[index];
	}
};

template <typename T>
void pousse(tableau<T> *tabl, T valeur)
{
	memoire::reloge_tableau("kuri::tableau", tabl->pointeur, tabl->taille, tabl->taille + 1);
	tabl->taille += 1;
	tabl->pointeur[tabl->taille - 1] = valeur;
}

/* Structure pour passer les lexèmes aux métaprogrammes, via compilatrice_lèxe_fichier
 */
struct Lexeme {
	int genre;
	chaine texte;
};

}
