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

#ifdef __cplusplus
extern "C" {
#endif

/* Structures contenant des fonctions de rappels pour déléguer certaines choses au programme client
 * comme l'allocation de mémoire. */
struct ContexteKuri {
    void *(*loge_memoire)(struct ContexteKuri *ctx, unsigned long taille);
    void *(*reloge_memoire)(struct ContexteKuri *ctx,
                            void *ancien_pointeur,
                            unsigned long ancienne_taille,
                            unsigned long nouvelle_taille);
    void (*deloge_memoire)(struct ContexteKuri *ctx, void *ancien_pointeur, unsigned long taille);
};

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
template <typename T, typename... Args>
T *kuri_loge(ContexteKuri *ctx_kuri, Args &&... args)
{
  T *ptr = static_cast<T *>(ctx_kuri->loge_memoire(ctx_kuri, sizeof (T)));
  new (ptr) T(args...);
  return ptr;
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
