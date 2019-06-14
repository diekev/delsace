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

#include "../block.h"

#include <iostream>

namespace memori {

class Mallocator {
public:
	Blk allocate(size_t n)
	{
		auto ptr = malloc(n);
		return { ptr, n };
	}

	// Blk allocateAll();

	Blk alignedAllocate(size_t n, unsigned alignment)
	{
		auto ptr = aligned_alloc(alignment, n);
		return { ptr, n };
	}

	void deallocate(Blk b)
	{
		free(b.ptr);
	}

	// void deallocateAll();

	bool expand(Blk &b, size_t delta)
	{
		auto ptr = realloc(b.ptr, b.size + delta);

		if (ptr == nullptr) {
			return false;
		}

		b.ptr = ptr;
		b.size += delta;

		return true;
	}

	void reallocate(Blk &b, size_t new_size)
	{
		b.ptr = realloc(b.ptr, new_size);
		b.size = new_size;
	}

	void alignedReallocate(Blk &b, size_t n, unsigned alignment)
	{
		auto ptr = aligned_alloc(alignment, n);

		if (ptr != nullptr) {
			free(b.ptr);
			b.ptr = ptr;
			b.size = n;
		}
	}

	bool owns(Blk /*b*/)
	{
		return true;
	}
};

struct AllocateurBlender {
	void *allocate(size_t n)
	{
		return malloc(n);
	}

	// Blk allocateAll();

	void *alignedAllocate(size_t n, unsigned alignment)
	{
		return aligned_alloc(alignment, n);
	}

	void deallocate(void *ptr, size_t /* n */)
	{
		free(ptr);
	}

	// void deallocateAll();

	bool expand(void *&ptr, size_t n, size_t delta)
	{
		auto rptr = realloc(ptr, n + delta);

		if (rptr != nullptr) {
			ptr = rptr;
			return true;
		}

		return false;
	}

	void reallocate(void *&ptr,
	                size_t /* old_size */,
                    size_t new_size)
	{
		ptr = realloc(ptr, new_size);
	}

	void alignedReallocate(void *&ptr,
	                       size_t /* old_size */,
	                       size_t new_size,
	                       unsigned alignment)
	{
		auto rptr = alignedAllocate(new_size, alignment);

		if (rptr != nullptr) {
			free(ptr);
			ptr = rptr;
		}
	}

	bool owns(void * /* ptr */, size_t /* n */)
	{
		return true;
	}
};

}  /* namespace memori */
