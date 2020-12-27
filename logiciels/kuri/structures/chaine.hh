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
