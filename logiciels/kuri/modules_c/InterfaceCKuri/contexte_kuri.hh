/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Structures contenant des fonctions de rappels pour déléguer certaines choses au programme client
 * comme l'allocation de mémoire. */
struct ContexteKuri {
    void *(*loge_memoire)(struct ContexteKuri *ctx, uint64_t taille);
    void *(*reloge_memoire)(struct ContexteKuri *ctx,
                            void *ancien_pointeur,
                            uint64_t ancienne_taille,
                            uint64_t nouvelle_taille);
    void (*deloge_memoire)(struct ContexteKuri *ctx, void *ancien_pointeur, uint64_t taille);
};

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
template <typename T, typename... Args>
T *kuri_loge(ContexteKuri *ctx_kuri, Args &&...args)
{
    T *ptr = static_cast<T *>(ctx_kuri->loge_memoire(ctx_kuri, sizeof(T)));
    new (ptr) T(args...);
    return ptr;
}

template <typename T>
T *kuri_loge_tableau(ContexteKuri *ctx_kuri, uint64_t taille)
{
    T *ptr = static_cast<T *>(ctx_kuri->loge_memoire(ctx_kuri, sizeof(T) * taille));
    return ptr;
}

template <typename T>
T *kuri_reloge(ContexteKuri *ctx, T *objet, uint64_t nouvelle_taille)
{
    void *nouveau_pointeur = ctx->reloge_memoire(ctx, objet, sizeof(T), nouvelle_taille);
    return static_cast<T *>(nouveau_pointeur);
}

template <typename T>
void kuri_deloge(ContexteKuri *ctx_kuri, T *ptr)
{
    if (!ptr) {
        return;
    }

    ptr->~T();
    ctx_kuri->deloge_memoire(ctx_kuri, ptr, sizeof(T));
}

template <typename T>
void kuri_déloge_tableau(ContexteKuri *ctx_kuri, T *tableau, uint64_t taille)
{
    if (!tableau) {
        return;
    }
    ctx_kuri->deloge_memoire(ctx_kuri, tableau, sizeof(T) * taille);
}
#else
inline void *kuri_loge(struct ContexteKuri *ctx_kuri, uint64_t taille)
{
    return ctx_kuri->loge_memoire(ctx_kuri, taille);
}

inline void *kuri_loge_zero(struct ContexteKuri *ctx_kuri, uint64_t taille)
{
    void *result = ctx_kuri->loge_memoire(ctx_kuri, taille);
    if (result == NULL) {
        return NULL;
    }
    memset(result, 0, taille);
    return result;
}

inline void *kuri_loge_tableau(struct ContexteKuri *ctx_kuri,
                               uint64_t taille,
                               uint64_t taille_élément)
{
    return ctx_kuri->loge_memoire(ctx_kuri, taille_élément * taille);
}

inline void *kuri_loge_tableau_zero(struct ContexteKuri *ctx_kuri,
                                    uint64_t taille,
                                    uint64_t taille_élément)
{
    void *result = ctx_kuri->loge_memoire(ctx_kuri, taille_élément * taille);
    if (result == NULL) {
        return NULL;
    }
    memset(result, 0, taille_élément * taille);
    return result;
}

inline void *kuri_reloge_tableau(struct ContexteKuri *ctx_kuri,
                                 void *pointer,
                                 uint64_t ancienne_taille,
                                 uint64_t nouvelle_taille,
                                 uint64_t taille_élément)
{
    return ctx_kuri->reloge_memoire(
        ctx_kuri, poniter, taille_élément * ancienne_taille, taille_élément * nouvelle_taille);
}

inline void kuri_deloge(struct ContexteKuri *ctx_kuri, void *pointer, uint64_t taille)
{
    ctx_kuri->deloge_memoire(ctx_kuri, pointer, taille)
}

inline void kuri_deloge_tableau(struct ContexteKuri *ctx_kuri,
                                void *pointer,
                                uint64_t taille,
                                uint64_t taille_élément)
{
    ctx_kuri->deloge_memoire(ctx_kuri, pointer, taille_élément * taille)
}
#endif
