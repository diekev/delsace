﻿/*
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
#include <atomic>
#include <cassert>
#include <string>

namespace memoire {

template <typename T>
inline constexpr long calcule_memoire(long nombre)
{
	return static_cast<long>(sizeof(T)) * nombre;
}

struct logeuse_memoire {
	std::atomic_long memoire_consommee = 0;
	std::atomic_long memoire_allouee = 0;
	std::atomic_long nombre_allocations = 0;
	std::atomic_long nombre_reallocations = 0;
	std::atomic_long nombre_deallocations = 0;

	logeuse_memoire() = default;

	~logeuse_memoire();

	logeuse_memoire(logeuse_memoire const &) = delete;
	logeuse_memoire(logeuse_memoire &&) = delete;

	logeuse_memoire &operator=(logeuse_memoire const &) = delete;
	logeuse_memoire &operator=(logeuse_memoire &&) = delete;

	template <typename T, typename... Args>
	[[nodiscard]] T *loge(const char *message, Args &&... args)
	{
		auto ptr = static_cast<T *>(loge_generique(message, calcule_memoire<T>(1)));

		if (ptr == nullptr) {
			throw std::bad_alloc();
		}

		new (ptr) T(args...);

		return ptr;
	}

	template <typename T>
	[[nodiscard]] T *loge_tableau(const char *message, long nombre)
	{
		assert(nombre >= 0);

		auto ptr = static_cast<T *>(loge_generique(message, calcule_memoire<T>(nombre)));

		if (ptr == nullptr) {
			throw std::bad_alloc();
		}

		return ptr;
	}

	template <typename T>
	void reloge_tableau(const char *message, T *&ptr, long ancienne_taille, long nouvelle_taille)
	{
		assert(ancienne_taille >= 0);
		assert(nouvelle_taille >= 0);

		if constexpr (std::is_trivially_copyable_v<T>) {
			ptr = static_cast<T *>(reloge_generique(message, ptr, calcule_memoire<T>(ancienne_taille), calcule_memoire<T>(nouvelle_taille)));
		}
		else {
			auto res = static_cast<T *>(loge_generique(message, calcule_memoire<T>(nouvelle_taille)));

			for (auto i = 0; i < nouvelle_taille; ++i) {
				new (&res[i]) T();
			}

			for (auto i = 0; i < ancienne_taille; ++i) {
				res[i] = std::move(ptr[i]);
			}

			deloge_generique(message, ptr, calcule_memoire<T>(ancienne_taille));
			ptr = res;
		}
	}

	template <typename T>
	void deloge(const char *message, T *&ptr)
	{
		if (ptr == nullptr) {
			return;
		}

		ptr->~T();

		deloge_generique(message, ptr, calcule_memoire<T>(1));
		ptr = nullptr;
	}

	template <typename T>
	void deloge_tableau(const char *message, T *&ptr, long nombre)
	{
		assert(nombre >= 0);

		deloge_generique(message, ptr, calcule_memoire<T>(nombre));
		ptr = nullptr;
	}

	static inline logeuse_memoire &instance()
	{
		return m_instance;
	}

private:
	static logeuse_memoire m_instance;

	inline void ajoute_memoire(long taille)
	{
		this->memoire_allouee += taille;
		this->memoire_consommee = std::max(this->memoire_allouee.load(), this->memoire_consommee.load());
	}

	inline void enleve_memoire(long taille)
	{
		this->memoire_allouee -= taille;
	}

	void *loge_generique(const char *message, long taille);

	void *reloge_generique(const char *message, void *ptr, long ancienne_taille, long nouvelle_taille);

	void deloge_generique(const char *message, void *ptr, long taille);
};

/**
 * Retourne la quantité en octets de mémoire allouée au moment de l'appel.
 */
[[nodiscard]] long allouee();

/**
 * Retourne la quantité en octets de mémoire consommée au moment de l'appel.
 */
[[nodiscard]] long consommee();

[[nodiscard]] long nombre_allocations();

[[nodiscard]] long nombre_reallocations();

[[nodiscard]] long nombre_deallocations();

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
[[nodiscard]] std::string formate_taille(long octets);

template <typename T, typename... Args>
[[nodiscard]] T *loge(const char *message, Args &&...args)
{
	auto &logeuse = logeuse_memoire::instance();
	return logeuse.loge<T>(message, args...);
}

template <typename T>
void reloge_tableau(const char *message, T *&ptr, long ancienne_taille, long nouvelle_taille)
{
	auto &logeuse = logeuse_memoire::instance();
	logeuse.reloge_tableau(message, ptr, ancienne_taille, nouvelle_taille);
}

template <typename T>
[[nodiscard]] T *loge_tableau(const char *message, long nombre)
{
	auto &logeuse = logeuse_memoire::instance();
	return logeuse.loge_tableau<T>(message, nombre);
}

template <typename T>
void deloge(const char *message, T *&ptr)
{
	auto &logeuse = logeuse_memoire::instance();
	logeuse.deloge<T>(message, ptr);
}

template <typename T>
void deloge_tableau(const char *message, T *&ptr, long nombre)
{
	auto &logeuse = logeuse_memoire::instance();
	logeuse.deloge_tableau<T>(message, ptr, nombre);
}

}  /* namespace memoire */
