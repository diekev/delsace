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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <cstdlib>

namespace memori {

struct Blk {
	void *ptr;
	size_t size;
};

struct MEMHead {
	size_t len;
};

static size_t mem_aligned_4(size_t n)
{
	return (n + 3) & ~static_cast<size_t>(3);
}

static inline void *ptr_from_memhead(MEMHead * const ptr)
{
	return ptr + 1;
}

static inline MEMHead *memhead_from_ptr(void *ptr)
{
	return static_cast<MEMHead *>(ptr) - 1;
}

}  /* namespace memori */
