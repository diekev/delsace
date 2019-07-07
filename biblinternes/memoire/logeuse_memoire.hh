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
#include <atomic>
#include <cassert>
#include <string>

#undef DEBOGUE_MEMOIRE

#ifdef DEBOGUE_MEMOIRE
#	include "biblinternes/structures/dico.hh"
#endif

namespace memoire {

struct logeuse_memoire {
	std::atomic_long memoire_consommee = 0;
	std::atomic_long memoire_allouee = 0;

#ifdef DEBOGUE_MEMOIRE
	/* XXX - il est possible d'avoir une situation de concurence sur la table */
	dls::dico<const char *, std::atomic_long> tableau_allocation;
#endif

	~logeuse_memoire();

	logeuse_memoire(logeuse_memoire const &) = delete;
	logeuse_memoire(logeuse_memoire &&) = delete;

	logeuse_memoire &operator=(logeuse_memoire const &) = delete;
	logeuse_memoire &operator=(logeuse_memoire &&) = delete;

	void ajoute_memoire(const char *message, long taille);

	void enleve_memoire(const char *message, long taille);

	template <typename T, typename... Args>
	[[nodiscard]] T *loge(const char *message, Args &&... args)
	{
		auto ptr = new T(args...);

		if (ptr == nullptr) {
			throw std::bad_alloc();
		}

		ajoute_memoire(message, static_cast<long>(sizeof(T)));

		return ptr;
	}

	template <typename T>
	[[nodiscard]] T *loge_tableau(const char *message, long nombre)
	{
		assert(nombre >= 0);

		auto ptr = static_cast<T *>(malloc(sizeof(T) * static_cast<size_t>(nombre)));

		if (ptr == nullptr) {
			throw std::bad_alloc();
		}

		ajoute_memoire(message, static_cast<long>(sizeof(T)) * nombre);

		return ptr;
	}

	template <typename T>
	void reloge_tableau(const char *message, T *&ptr, long ancienne_taille, long nouvelle_taille)
	{
		assert(ancienne_taille >= 0);
		assert(nouvelle_taille >= 0);

		if constexpr (std::is_trivially_copyable_v<T>) {
			ptr = static_cast<T *>(realloc(ptr, static_cast<size_t>(nouvelle_taille)));
		}
		else {
			auto res = static_cast<T *>(malloc(sizeof(T) * static_cast<size_t>(nouvelle_taille)));

			for (auto i = 0; i < ancienne_taille; ++i) {
				res[i] = ptr[i];
			}

			free(ptr);
			ptr = res;
		}

		ajoute_memoire(message, static_cast<long>(sizeof(T)) * (nouvelle_taille - ancienne_taille));
	}

	template <typename T>
	void deloge(const char *message, T *&ptr)
	{
		if (ptr == nullptr) {
			return;
		}

		delete ptr;
		ptr = nullptr;

		enleve_memoire(message, static_cast<long>(sizeof(T)));
	}

	template <typename T>
	void deloge_tableau(const char *message, T *&ptr, long nombre)
	{
		assert(nombre >= 0);

		if (ptr == nullptr) {
			return;
		}

		free(ptr);
		ptr = nullptr;

		enleve_memoire(message, static_cast<long>(sizeof(T)) * nombre);
	}

	static logeuse_memoire &instance();

private:
	static logeuse_memoire m_instance;
};

/**
 * Retourne la quantité en octets de mémoire allouée au moment de l'appel.
 */
[[nodiscard]] long allouee();

/**
 * Retourne la quantité en octets de mémoire consommée au moment de l'appel.
 */
[[nodiscard]] long consommee();

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
