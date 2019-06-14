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

namespace memori {

// - Sizes <= threshold go to SmallAllocator
// - All else go to LargeAllocator
// - no need for owns (-> compare alloc len to threshold)
template <size_t THRESHOLD, class SmallAllocator, class LargeAllocator>
struct Segregator : private SmallAllocator, private LargeAllocator {
	Blk allocate(size_t n)
	{
		if (n <= THRESHOLD) {
			return SmallAllocator::allocate(n);
		}

		return LargeAllocator::allocate(n);
	}

	Blk alignedAllocate(size_t n, unsigned alignment)
	{
		if (n <= THRESHOLD) {
			return SmallAllocator::alignedAllocate(n, alignment);
		}

		return LargeAllocator::alignedAllocate(n, alignment);
	}

//	Blk allocateAll()
//	{
//		if (b.size <= THRESHOLD) {
//			SmallAllocator::allocateAll();
//			return;
//		}

//		LargeAllocator::allocateAll();
//	}

	void deallocate(Blk b)
	{
		if (b.size <= THRESHOLD) {
			SmallAllocator::deallocate(b);
			return;
		}

		LargeAllocator::deallocate(b);
	}

//	void deallocateAll()
//	{
//		if (b.size <= THRESHOLD) {
//			SmallAllocator::deallocateAll();
//			return;
//		}

//		LargeAllocator::deallocateAll();
//	}

	bool expand(Blk &b, size_t delta)
	{
		if (b.size <= THRESHOLD) {
			SmallAllocator::expand(b, delta);
			return true;
		}

		return LargeAllocator::expand(b, delta);
	}

	void reallocate(Blk &b, size_t new_size)
	{
		if (new_size <= THRESHOLD) {
			SmallAllocator::reallocate(b, new_size);
			return;
		}

		LargeAllocator::reallocate(b, new_size);
	}

	void alignedReallocate(Blk &b, size_t n, unsigned alignment)
	{
		if (n <= THRESHOLD) {
			SmallAllocator::alignedReallocate(b, n, alignment);
			return;
		}

		LargeAllocator::alignedReallocate(b, n, alignment);
	}

	bool owns(Blk /*b*/)
	{
		return true;
	}
};

}  /* namespace memori */
