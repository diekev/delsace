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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/memoire/logeuse_memoire.hh"

namespace kuri {

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
		memoire::reloge_tableau("kuri::tableau", this->pointeur, this->capacite, autre.taille);
		this->taille = autre.taille;
		this->capacite = autre.taille;

		for (auto i = 0; i < autre.taille; ++i) {
			this->pointeur[i] = autre.pointeur[i];
		}
	}

	tableau &operator=(tableau const &autre)
	{
		memoire::reloge_tableau("kuri::tableau", this->pointeur, this->capacite, autre.taille);
		this->taille = autre.taille;
		this->capacite = autre.taille;

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
		for (auto i = 0; i < taille; ++i) {
			this->pointeur[i].~T();
		}

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

	void ajoute(T const &valeur)
	{
		reserve(this->taille + 1);
		this->taille += 1;
		this->pointeur[this->taille - 1] = valeur;
	}

	void ajoute(T &&valeur)
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

	void supprime_dernier()
	{
		if (this->taille == 0) {
			return;
		}

		this->pointeur[this->taille - 1].~T();
		this->taille -= 1;
	}

	void reserve(long nombre)
	{
		if (capacite >= nombre) {
			return;
		}

		if (capacite == 0) {
			if (nombre < 8) {
				nombre = 8;
			}
		}
		else if (nombre < (capacite * 2)) {
			nombre = capacite * 2;
		}

		memoire::reloge_tableau("kuri::tableau", this->pointeur, this->capacite, nombre);
		this->capacite = nombre;
	}

	void redimensionne(long nombre)
	{
		reserve(nombre);
		taille = nombre;
	}

	void reserve_delta(long delta)
	{
		reserve(taille + delta);
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

	T const &derniere() const
	{
		return this->pointeur[this->taille - 1];
	}
};

}
