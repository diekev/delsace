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

#include <algorithm>
#include <cassert>
#include <string>

namespace memoire {

struct logeuse_memoire {
	size_t memoire_consommee = 0;
	size_t memoire_allouee = 0;

	~logeuse_memoire();

	logeuse_memoire(logeuse_memoire const &) = delete;
	logeuse_memoire(logeuse_memoire &&) = delete;

	logeuse_memoire &operator=(logeuse_memoire const &) = delete;
	logeuse_memoire &operator=(logeuse_memoire &&) = delete;

	template <typename T, typename... Args>
	[[nodiscard]] T *loge(Args &&... args)
	{
		auto ptr = new T(args...);

		this->memoire_allouee += sizeof(T);
		this->memoire_consommee = std::max(this->memoire_allouee, this->memoire_consommee);

		return ptr;
	}

	template <typename T>
	[[nodiscard]] T *loge_tableau(long nombre)
	{
		assert(nombre >= 0);

		auto ptr = new T[nombre];

		this->memoire_allouee += sizeof(T) * static_cast<size_t>(nombre);
		this->memoire_consommee = std::max(this->memoire_allouee, this->memoire_consommee);

		return ptr;
	}

	template <typename T>
	void deloge(T *&ptr)
	{
		delete ptr;
		ptr = nullptr;

		this->memoire_allouee -= sizeof(T);
	}

	template <typename T>
	void deloge_tableau(T *&ptr, long nombre)
	{
		assert(nombre >= 0);

		delete [] ptr;
		ptr = nullptr;

		this->memoire_allouee -= sizeof(T) * static_cast<size_t>(nombre);
	}

	static logeuse_memoire &instance();

private:
	static logeuse_memoire m_instance;
};

/**
 * Retourne la quantité en octets de mémoire allouée au moment de l'appel.
 */
[[nodiscard]] size_t allouee();

/**
 * Retourne la quantité en octets de mémoire consommée au moment de l'appel.
 */
[[nodiscard]] size_t consommee();

/**
 * Convertit le nombre d'octet passé en paramètre en une chaine contenant :
 * - si la taille est inférieure à 1 Ko : la taille en octets + " o"
 * - si la taille est inférieure à 1 Mo : la taille en kiloctets + " Ko"
 * - si la taille est inférieure à 1 Go : la taille en mégaoctets + " Mo"
 * - sinon, rétourne la taille en gigaoctets + " Go"
 *
 * par exemple:
 * 8564 -> "8 Ko"
 * 16789432158 -> "15 Go"
 */
[[nodiscard]] std::string formate_taille(size_t octets);

template <typename T, typename... Args>
[[nodiscard]] T *loge(Args &&...args)
{
	auto &logeuse = logeuse_memoire::instance();
	return logeuse.loge<T>(args...);
}

template <typename T>
[[nodiscard]] T *loge_tableau(long nombre)
{
	auto &logeuse = logeuse_memoire::instance();
	return logeuse.loge_tableau<T>(nombre);
}

template <typename T>
void deloge(T *&ptr)
{
	auto &logeuse = logeuse_memoire::instance();
	logeuse.deloge<T>(ptr);
}

template <typename T>
void deloge_tableau(T *&ptr, long nombre)
{
	auto &logeuse = logeuse_memoire::instance();
	logeuse.deloge_tableau<T>(ptr, nombre);
}

}  /* namespace memoire */
