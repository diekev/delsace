/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "markup.h"
#include "texture-atlas.h"
#include "texture-font.h"

typedef void *(*malloc_func_t)(uint64_t);
typedef void *(*calloc_func_t)(uint64_t, uint64_t);
typedef void *(*realloc_func_t)(void *, uint64_t, uint64_t);
typedef void (*free_func_t)(void *, uint64_t);

int FTGL_set_allocators(malloc_func_t malloc_func,
                        calloc_func_t calloc_func,
                        realloc_func_t realloc_func,
                        free_func_t free_func);

void *FTGL_malloc(uint64_t size);

void *FTGL_calloc(uint64_t nmemb, uint64_t size);

void *FTGL_realloc(void *ptr, uint64_t old_size, uint64_t new_size);

void FTGL_free(void *ptr, uint64_t size);

#ifdef __cplusplus
}
#endif
