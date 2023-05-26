/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

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
#endif
