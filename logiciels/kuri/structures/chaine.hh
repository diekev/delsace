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

#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/chaine.hh"

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

	void ajoute(char c)
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

}

/*

struct Allocatrice;

struct Allocatrice {
private:
	long m_nombre_allocations = 0;
	long m_taille_allouee = 0;

public:
	template <typename T, typename... Args>
	T *loge(const char *ident, Args &&... args)
	{
		m_nombre_allocations += 1;
		m_taille_allouee += static_cast<long>(sizeof(T));
		return new T(args...);
	}

	template <typename T>
	void deloge(T *&ptr)
	{
		delete ptr;
		ptr = nullptr;
		m_taille_allouee -= static_cast<long>(sizeof(T));
	}

	template <typename T>
	T *loge_tableau(const char *ident, long elements)
	{
		m_nombre_allocations += 1;
		m_taille_allouee += static_cast<long>(sizeof(T)) * elements;
		return new T[elements];
	}

	template <typename T>
	void reloge_tableau(const char *ident, T *&ptr, long ancienne_taille, long nouvelle_taille);

	template <typename T>
	void deloge_tableau(T *&ptr, long taille);

	long nombre_allocation() const
	{
		return m_nombre_allocations;
	}

	long taille_allouee() const
	{
		return m_taille_allouee;
	}
};

namespace kuri {

struct chaine {
private:
	char *m_pointeur = nullptr;
    long m_taille = 0;
    long m_capacite = 0;
    Allocatrice *m_allocatrice = nullptr;

public:
	explicit chaine(Allocatrice &alloc);

	template <unsigned long N>
    chaine(Allocatrice &alloc, const char (&c_str)[N])
        : m_allocatrice(&alloc)
    {
		redimensionne(static_cast<long>(N));
		memcpy(c_str, m_pointeur, N);
    }

	chaine(chaine const &autre);

	chaine(chaine &&autre);

	chaine &operator=(chaine const &autre);

	chaine &operator=(chaine &&autre);

	~chaine();

	void ajoute(char c);

	void reserve(long taille);

	void redimensionne(long taille);

	long taille() const;

	long capacite() const;

	const char *pointeur() const;

	explicit operator bool ();

    void permute(chaine &autre);

	void copie_donnees(chaine const &autre);

	void garantie_capacite(long taille);
};

template <typename T>
struct tableau_statique {
	const T *pointeur = nullptr;
	long taille = 0;

	tableau_statique(const T *pointeur_, long taille_)
		: pointeur(pointeur_)
		, taille(taille_)
	{}
};

struct chaine_statique : public tableau_statique<char> {
	chaine_statique(const char *pointeur_);

	chaine_statique(chaine const &chn);
};

}

*/
