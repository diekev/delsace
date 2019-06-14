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

#include <cassert>

namespace memori {

/* Does not perform any allocations, always returns nullptr.
 * To be put at the end of a chain of allocators, or to debug allocations (the
 * blocks to be freed are supposed to be nullptr). */
class NullAllocator {
public:
	Blk allocate(size_t /* n */)
	{
		return { nullptr, 0 };
	}

	Blk allocateAll()
	{
		return { nullptr, 0 };
	}

	Blk alignedAllocate(size_t /* n */, unsigned /* alignment */)
	{
		return { nullptr, 0 };
	}

	void deallocate(Blk b)
	{
		assert(b.ptr == nullptr);
	}

	void deallocateAll() {}

	bool expand(Blk &b, size_t /* delta */)
	{
		assert(b.ptr == nullptr && b.size == 0);
		return true;
	}

	void reallocate(Blk &b, size_t /* new_size */)
	{
		assert(b.ptr == nullptr && b.size == 0);
	}

	void alignedReallocate(Blk &b, size_t /* n */, unsigned /* alignment */)
	{
		assert(b.ptr == nullptr && b.size == 0);
	}

	bool owns(Blk /* b */)
	{
		return true;
	}
};

}  /* namespace memori */
