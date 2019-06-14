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

// alocate data between Prefix and Suffix to check for errors
// e.g.: store file + line where the allocation happens in the prefix
template <class A, class Prefix, class Suffix = void>
class AffixAllocator {
	A m_parent;

public:
	Blk allocate(size_t n)
	{
		auto affix_size = n + sizeof(Prefix) + sizeof(Suffix);
		auto ptr = m_parent.allocate(affix_size);

		return { ptr + sizeof(Prefix), n };
	}

	// Blk allocateAll();

	Blk alignedAllocate(size_t n, unsigned alignment)
	{
		auto affix_size = n + sizeof(Prefix) + sizeof(Suffix);
		auto ptr = m_parent.alignedAllocate(affix_size, alignment);

		return { ptr + sizeof(Prefix), n };
	}

	void deallocate(Blk b)
	{
		m_parent.deallocate(b);
	}

	// void deallocateAll();

	bool expand(Blk &b, size_t delta)
	{
//		auto ptr = realloc(b.ptr, b.size + delta);

//		if (ptr == nullptr) {
//			return false;
//		}

//		b.ptr = ptr;
//		b.size += delta;

//		return true;
	}

	void reallocate(Blk &b, size_t new_size)
	{
		auto affix_size = new_size + sizeof(Prefix) + sizeof(Suffix);
//		b.ptr = realloc(b.ptr, new_size);
		b.size = affix_size;
	}

	void alignedReallocate(Blk &b, size_t n, unsigned alignment)
	{
//		auto ptr = aligned_alloc(alignment, n);

//		if (ptr != nullptr) {
//			free(b.ptr);
//			b.ptr = ptr;
//			b.size = n;
//		}
	}

	bool owns(Blk b)
	{
		return m_parent.owns({b.ptr - sizeof(Prefix), b.size});
	}
};

}  /* namespace memori */
