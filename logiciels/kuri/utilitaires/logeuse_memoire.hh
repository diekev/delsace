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

#include <atomic>
#include <cassert>
#include <string>

namespace mémoire {

template <typename T>
inline constexpr int64_t calcule_memoire(int64_t nombre)
{
    return static_cast<int64_t>(sizeof(T)) * nombre;
}

struct logeuse_mémoire {
    std::atomic_int64_t mémoire_consommee = 0;
    std::atomic_int64_t mémoire_allouee = 0;
    std::atomic_int64_t nombre_allocations = 0;
    std::atomic_int64_t nombre_reallocations = 0;
    std::atomic_int64_t nombre_deallocations = 0;

    logeuse_mémoire() = default;

    ~logeuse_mémoire();

    logeuse_mémoire(logeuse_mémoire const &) = delete;
    logeuse_mémoire(logeuse_mémoire &&) = delete;

    logeuse_mémoire &operator=(logeuse_mémoire const &) = delete;
    logeuse_mémoire &operator=(logeuse_mémoire &&) = delete;

    template <typename T, typename... Args>
    [[nodiscard]] T *loge(const char *message, Args &&...args)
    {
        auto ptr = static_cast<T *>(loge_generique(message, calcule_memoire<T>(1)));

        if (ptr == nullptr) {
            throw std::bad_alloc();
        }

        new (ptr) T(args...);

        return ptr;
    }

    template <typename T>
    [[nodiscard]] T *loge_tableau(const char *message, int64_t nombre)
    {
        assert(nombre >= 0);

        auto ptr = static_cast<T *>(loge_generique(message, calcule_memoire<T>(nombre)));

        if (ptr == nullptr) {
            throw std::bad_alloc();
        }

        return ptr;
    }

    template <typename T>
    void reloge_tableau(const char *message,
                        T *&ptr,
                        int64_t ancienne_taille,
                        int64_t nouvelle_taille)
    {
        assert(ancienne_taille >= 0);
        assert(nouvelle_taille >= 0);

        if constexpr (std::is_trivially_copyable_v<T>) {
            ptr = static_cast<T *>(reloge_generique(message,
                                                    ptr,
                                                    calcule_memoire<T>(ancienne_taille),
                                                    calcule_memoire<T>(nouvelle_taille)));
        }
        else {
            auto res = static_cast<T *>(
                loge_generique(message, calcule_memoire<T>(nouvelle_taille)));

            for (auto i = 0; i < nouvelle_taille; ++i) {
                new (&res[i]) T();
            }

            for (auto i = 0; i < ancienne_taille; ++i) {
                res[i] = std::move(ptr[i]);
            }

            déloge_generique(message, ptr, calcule_memoire<T>(ancienne_taille));
            ptr = res;
        }
    }

    template <typename T>
    void déloge(const char *message, T *&ptr)
    {
        if (ptr == nullptr) {
            return;
        }

        ptr->~T();

        déloge_generique(message, ptr, calcule_memoire<T>(1));
        ptr = nullptr;
    }

    template <typename T>
    void déloge_tableau(const char *message, T *&ptr, int64_t nombre)
    {
        assert(nombre >= 0);

        déloge_generique(message, ptr, calcule_memoire<T>(nombre));
        ptr = nullptr;
    }

    static inline logeuse_mémoire &instance()
    {
        return m_instance;
    }

  private:
    static logeuse_mémoire m_instance;

    inline void ajoute_memoire(int64_t taille)
    {
        this->mémoire_allouee += taille;
        auto allouée = this->mémoire_allouee.load();
        auto consommée = this->mémoire_consommee.load();
        if (allouée > consommée) {
            this->mémoire_consommee = allouée;
        }
    }

    inline void enleve_memoire(int64_t taille)
    {
        this->mémoire_allouee -= taille;
    }

    void *loge_generique(const char *message, int64_t taille);

    void *reloge_generique(const char *message,
                           void *ptr,
                           int64_t ancienne_taille,
                           int64_t nouvelle_taille);

    void déloge_generique(const char *message, void *ptr, int64_t taille);
};

/**
 * Retourne la quantité en octets de mémoire allouée au moment de l'appel.
 */
[[nodiscard]] int64_t allouee();

/**
 * Retourne la quantité en octets de mémoire consommée au moment de l'appel.
 */
[[nodiscard]] int64_t consommee();

[[nodiscard]] int64_t nombre_allocations();

[[nodiscard]] int64_t nombre_reallocations();

[[nodiscard]] int64_t nombre_deallocations();

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
[[nodiscard]] std::string formate_taille(int64_t octets);

template <typename T, typename... Args>
[[nodiscard]] T *loge(const char *message, Args &&...args)
{
    auto &logeuse = logeuse_mémoire::instance();
    return logeuse.loge<T>(message, args...);
}

template <typename T>
void reloge_tableau(const char *message, T *&ptr, int64_t ancienne_taille, int64_t nouvelle_taille)
{
    auto &logeuse = logeuse_mémoire::instance();
    logeuse.reloge_tableau(message, ptr, ancienne_taille, nouvelle_taille);
}

template <typename T>
[[nodiscard]] T *loge_tableau(const char *message, int64_t nombre)
{
    auto &logeuse = logeuse_mémoire::instance();
    return logeuse.loge_tableau<T>(message, nombre);
}

template <typename T>
void déloge(const char *message, T *&ptr)
{
    auto &logeuse = logeuse_mémoire::instance();
    logeuse.déloge<T>(message, ptr);
}

template <typename T>
void déloge_tableau(const char *message, T *&ptr, int64_t nombre)
{
    auto &logeuse = logeuse_mémoire::instance();
    logeuse.déloge_tableau<T>(message, ptr, nombre);
}

}  // namespace mémoire
