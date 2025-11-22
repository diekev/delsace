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

#include "logeuse_memoire.hh"

#include <iostream>
#include <mutex>
#include <sstream>

namespace mémoire {

static void imprime_blocs_memoire();

logeuse_mémoire logeuse_mémoire::m_instance = logeuse_mémoire{};

#undef IMPRIME_ERREUR_MEMOIRE

logeuse_mémoire::~logeuse_mémoire()
{
#ifdef IMPRIME_ERREUR_MEMOIRE
    if (mémoire_allouee != 0) {
        std::cerr << "Fuite de mémoire ou désynchronisation : " << formate_taille(mémoire_allouee)
                  << '\n';

        imprime_blocs_memoire();
    }
#endif
}

/* ************************************************************************** */

#define PROTEGE_MEMOIRE

struct LienLocal {
    LienLocal *suivant, *precedent;
};

struct ListeChainee {
    void *premier, *dernier;
};

static std::mutex g_mutex;
static volatile struct ListeChainee _g_liste_mem;
static volatile struct ListeChainee *g_liste_mem = &_g_liste_mem;

static void ajoute_lien(volatile ListeChainee *liste, void *vlien)
{
    struct LienLocal *lien = static_cast<LienLocal *>(vlien);

    lien->suivant = nullptr;
    lien->precedent = static_cast<LienLocal *>(liste->dernier);

    if (liste->dernier) {
        static_cast<LienLocal *>(liste->dernier)->suivant = lien;
    }
    if (liste->premier == nullptr) {
        liste->premier = lien;
    }
    liste->dernier = lien;
}

static void enleve_lien(volatile ListeChainee *liste, void *vlien)
{
    LienLocal *lien = static_cast<LienLocal *>(vlien);

    if (lien->suivant) {
        lien->suivant->precedent = lien->precedent;
    }
    if (lien->precedent) {
        lien->precedent->suivant = lien->suivant;
    }

    if (liste->dernier == lien) {
        liste->dernier = lien->precedent;
    }
    if (liste->premier == lien) {
        liste->premier = lien->suivant;
    }
}

inline int64_t taille_alignee(int64_t taille)
{
    return (taille + 3) & (~3);
}

struct EntêteMémoire {
    const char *message = nullptr;
    int64_t taille = 0;

    EntêteMémoire *suivant = nullptr;
    EntêteMémoire *precedent = nullptr;
};

inline void *pointeur_depuis_entête(EntêteMémoire *entête)
{
    return entête + 1;
}

inline EntêteMémoire *entête_depuis_pointeur(void *pointeur)
{
    /* le pointeur pointe sur le bloc, recule d'une EntêteMémoire pour retrouvre l'entête */
    return static_cast<EntêteMémoire *>(pointeur) - 1;
}

#define ENTETE_DEPUIS_LIEN(x)                                                                     \
    (reinterpret_cast<EntêteMémoire *>((static_cast<char *>(x)) -                                 \
                                       offsetof(EntêteMémoire, suivant)))
static void imprime_blocs_memoire()
{
    auto premier = g_liste_mem->premier;

    while (premier != g_liste_mem->dernier) {
        auto entête = ENTETE_DEPUIS_LIEN(premier);
        std::cerr << entête->message << ", " << entête->taille << '\n';
        premier = entête->suivant;
    }
}

void *logeuse_mémoire::loge_generique(const char *message, int64_t taille)
{
#ifndef PROTEGE_MEMOIRE
    return malloc(static_cast<size_t>(taille));
#else
    /* aligne la taille désirée */
    taille = taille_alignee(taille);

    EntêteMémoire *entête = static_cast<EntêteMémoire *>(
        malloc(static_cast<size_t>(taille) + sizeof(EntêteMémoire)));
    entête->message = message;
    entête->taille = taille;

    {
        std::unique_lock l(g_mutex);
        ajoute_lien(g_liste_mem, &entête->suivant);
    }

    ajoute_memoire(taille);
    nombre_allocations += 1;

    /* saute l'entête et retourne le bloc de la taille désirée */
    return pointeur_depuis_entête(entête);
#endif
}

void *logeuse_mémoire::reloge_generique(const char *message,
                                        void *ptr,
                                        int64_t ancienne_taille,
                                        int64_t nouvelle_taille)
{
#ifndef PROTEGE_MEMOIRE
    return realloc(ptr, static_cast<size_t>(nouvelle_taille));
#else

    assert(ptr || ancienne_taille == 0);

    /* calcule la taille alignée correspondante à l'allocation */
    ancienne_taille = taille_alignee(ancienne_taille);

    /* calcule la taille alignée correspondante à l'allocation */
    nouvelle_taille = taille_alignee(nouvelle_taille);

    EntêteMémoire *entête = static_cast<EntêteMémoire *>(ptr);

    /* il est possible d'utiliser reloge avec un pointeur nul, ce qui agit comme un loge */
    if (entête) {
        /* le pointeur pointe sur le bloc, recule d'une EntêteMémoire pour retrouvre l'entête */
        entête = entête_depuis_pointeur(entête);

        if (entête->taille != ancienne_taille) {
            std::cerr << "Désynchronisation pour le bloc '" << entête->message << "' ('" << message
                      << "') lors du relogement !\n";
            std::cerr << "La taille du logement était de " << entête->taille
                      << ", mais la taille reçue est de " << ancienne_taille << " !\n";
        }
    }

    /* enlève le lien au cas où realloc change le pointeur d'adresse */
    if (ptr) {
        std::unique_lock l(g_mutex);
        enleve_lien(g_liste_mem, &entête->suivant);
    }

    entête = static_cast<EntêteMémoire *>(
        realloc(entête, sizeof(EntêteMémoire) + static_cast<size_t>(nouvelle_taille)));
    entête->taille = nouvelle_taille;
    entête->message = message;

    {
        std::unique_lock l(g_mutex);
        ajoute_lien(g_liste_mem, &entête->suivant);
    }

    ajoute_memoire(nouvelle_taille - ancienne_taille);
    nombre_allocations += 1;
    nombre_reallocations += 1;

    /* saute l'entête et retourne le bloc de la taille désirée */
    return pointeur_depuis_entête(entête);
#endif
}

void logeuse_mémoire::déloge_generique(const char *message, void *ptr, int64_t taille)
{
#ifndef PROTEGE_MEMOIRE
    free(ptr);
#else
    if (ptr == nullptr) {
        return;
    }

    /* calcule la taille alignée correspondante à l'allocation */
    taille = taille_alignee(taille);

    EntêteMémoire *entête = entête_depuis_pointeur(ptr);

    if (entête->taille != taille) {
        std::cerr << "Désynchronisation pour le bloc '" << entête->message << "' ('" << message
                  << "') lors du délogement !\n";
        std::cerr << "La taille du logement était de " << entête->taille
                  << ", mais la taille reçue est de " << taille << " !\n";
    }

    {
        std::unique_lock l(g_mutex);
        enleve_lien(g_liste_mem, &entête->suivant);
    }
    free(entête);

    enleve_memoire(taille);
    nombre_deallocations += 1;
#endif
}

/* ************************************************************************** */

int64_t allouee()
{
    auto &logeuse = logeuse_mémoire::instance();
    return logeuse.mémoire_allouee.load();
}

int64_t consommee()
{
    auto &logeuse = logeuse_mémoire::instance();
    return logeuse.mémoire_consommee.load();
}

int64_t nombre_allocations()
{
    auto &logeuse = logeuse_mémoire::instance();
    return logeuse.nombre_allocations.load();
}

int64_t nombre_reallocations()
{
    auto &logeuse = logeuse_mémoire::instance();
    return logeuse.nombre_reallocations.load();
}

int64_t nombre_deallocations()
{
    auto &logeuse = logeuse_mémoire::instance();
    return logeuse.nombre_deallocations.load();
}

std::string formate_taille(int64_t octets)
{
    std::stringstream ss;

    if (octets < 0) {
        octets = -octets;
        ss << "-";
    }

    if (octets < 1024) {
        ss << octets << " o";
    }
    else if (octets < (1024 * 1024)) {
        ss << octets / (1024) << " Ko";
    }
    else if (octets < (1024 * 1024 * 1024)) {
        ss << octets / (1024 * 1024) << " Mo";
    }
    else {
        ss << octets / (1024 * 1024 * 1024) << " Go";
    }

    return ss.str();
}

}  // namespace mémoire
