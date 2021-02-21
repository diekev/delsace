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

#include "biblinternes/outils/definitions.h"
#include "biblinternes/memoire/logeuse_memoire.hh"

namespace kuri {

template <typename T, typename TypeIndex = long>
struct tableau {
private:
	T *pointeur = nullptr;
	TypeIndex taille_ = 0;
	TypeIndex capacite = 0;

public:
	tableau() = default;

	explicit tableau(TypeIndex taille_initiale)
		: pointeur(memoire::loge_tableau<T>("kuri::tableau", taille_initiale))
		, taille_(taille_initiale)
		, capacite(taille_initiale)
	{}

	tableau(std::initializer_list<T> &&liste)
		: tableau(static_cast<TypeIndex>(liste.size()))
	{
		for (auto &&elem : liste) {
			ajoute(elem);
		}
	}

	tableau(tableau const &autre)
	{
		memoire::reloge_tableau("kuri::tableau", this->pointeur, this->capacite, autre.taille_);
		this->taille_ = autre.taille_;
		this->capacite = autre.taille_;

		for (auto i = 0; i < autre.taille_; ++i) {
			this->pointeur[i] = autre.pointeur[i];
		}
	}

	tableau &operator=(tableau const &autre)
	{
		memoire::reloge_tableau("kuri::tableau", this->pointeur, this->capacite, autre.taille_);
		this->taille_ = autre.taille_;
		this->capacite = autre.taille_;

		for (auto i = 0; i < autre.taille_; ++i) {
			this->pointeur[i] = autre.pointeur[i];
		}

		return *this;
	}

	tableau(tableau &&autre)
	{
		permute(autre);
	}

	tableau &operator=(tableau &&autre)
	{
		permute(autre);
		return *this;
	}

	~tableau()
	{
		for (auto i = 0; i < taille_; ++i) {
			this->pointeur[i].~T();
		}

		memoire::deloge_tableau("kuri::tableau", this->pointeur, this->capacite);
	}

	T &operator[](TypeIndex i)
	{
		assert(i >= 0 && i < this->taille_);
		return this->pointeur[i];
	}

	T const &operator[](TypeIndex i) const
	{
		assert(i >= 0 && i < this->taille_);
		return this->pointeur[i];
	}

	void ajoute(T const &valeur)
	{
		reserve(this->taille_ + 1);
		this->taille_ += 1;
		this->pointeur[this->taille_ - 1] = valeur;
	}

	void ajoute(T &&valeur)
	{
		reserve(this->taille_ + 1);
		this->taille_ += 1;
		this->pointeur[this->taille_ - 1] = std::move(valeur);
	}

	void pousse_front(T const &valeur)
	{
		reserve(this->taille_ + 1);
		this->taille_ += 1;

		for (auto i = this->taille_ - 1; i >= 1; --i) {
			this->pointeur[i] = this->pointeur[i - 1];
		}

		this->pointeur[0] = valeur;
	}

	void supprime_dernier()
	{
		if (this->taille_ == 0) {
			return;
		}

		this->pointeur[this->taille_ - 1].~T();
		this->taille_ -= 1;
	}

	void reserve(TypeIndex nombre)
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

	T *donnees()
	{
		return pointeur;
	}

	T const *donnees() const
	{
		return pointeur;
	}

	TypeIndex taille_memoire() const
	{
		return taille_ * static_cast<TypeIndex>(taille_de(T));
	}

	void efface()
	{
		taille_ = 0;
	}

	void redimensionne(TypeIndex nombre)
	{
		reserve(nombre);
		taille_ = nombre;
	}

	void reserve_delta(TypeIndex delta)
	{
		reserve(taille_ + delta);
	}

	TypeIndex taille() const
	{
		return taille_;
	}

	bool est_vide() const
	{
		return this->taille_ == 0;
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
		return this->begin() + this->taille_;
	}

	T const *end() const
	{
		return this->begin() + this->taille_;
	}

	T &a(TypeIndex index)
	{
		return this->pointeur[index];
	}

	T const &a(TypeIndex index)  const
	{
		return this->pointeur[index];
	}

	T const &derniere() const
	{
		return this->pointeur[this->taille_ - 1];
	}

	void permute(tableau &autre)
	{
		std::swap(pointeur, autre.pointeur);
		std::swap(capacite, autre.capacite);
		std::swap(taille_, autre.taille_);
	}
};

}
