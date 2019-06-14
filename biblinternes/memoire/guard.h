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

#include "block.h"

namespace memori {

static void print_error(size_t expected, size_t given)
{
	std::cerr << "Allocation length mismatch! Expected: " << expected
	          << ", got: " << given << ".\n";
}

template <typename Allocateur>
class Garde {
	Allocateur m_parent{};

public:
	void *allocate(size_t n)
	{
		const auto len = mem_aligned_4(n);

		auto mmh = static_cast<MEMHead *>(m_parent.allocate(len + sizeof(MEMHead)));

		if (mmh != nullptr) {
			mmh->len = len;
			return ptr_from_memhead(mmh);
		}

		return nullptr;
	}

	void deallocate(void *&ptr, size_t n)
	{
		const auto len = mem_aligned_4(n);
		auto mmh = memhead_from_ptr(ptr);

		if (mmh->len != len) {
			print_error(mmh->len, len);
		}

		m_parent.deallocate(mmh, n);
	}

	// Blk allocateAll();

	// void deallocateAll();

	void *alignedAllocate(size_t n, unsigned alignment)
	{
		return aligned_alloc(alignment, n);
	}

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
	                size_t old_size,
                    size_t new_size)
	{
		if (ptr == nullptr) {
			ptr = allocate(new_size);
			return;
		}

		auto mmh = memhead_from_ptr(ptr);

		if (mmh->len != old_size) {
			print_error(mmh->len, old_size);
		}

		m_parent.reallocate(ptr, old_size, new_size);
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

	bool owns(void *ptr, size_t n)
	{
		return m_parent.owns(ptr, n);
	}
};

}  /* namespace memori */
