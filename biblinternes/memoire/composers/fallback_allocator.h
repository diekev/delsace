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

template <typename Primary, typename FallBack>
struct FallBackAllocator : private Primary, private FallBack {
	Blk allocate(size_t n)
	{
		Blk r = Primary::allocate(n);

		if (!r.ptr) {
			r = FallBack::allocate(n);
		}

		return r;
	}

	Blk alignedAllocate(size_t n, unsigned alignment)
	{
		Blk r = Primary::alignedAllocate(n, alignment);

		if (!r.ptr) {
			r = FallBack::alignedAllocate(n, alignment);
		}

		return r;
	}

	void deallocate(Blk b)
	{
		if (Primary::owns(b)) {
			return Primary::deallocate(b);
		}

		return FallBack::deallocate(b);
	}

	bool expand(Blk &b, size_t delta)
	{
		if (Primary::owns(b)) {
			return Primary::expand(b, delta);
		}

		return FallBack::expand(b, delta);
	}

	void reallocate(Blk &b, size_t new_size)
	{
		if (Primary::owns(b)) {
			return Primary::reallocate(b, new_size);
		}

		return FallBack::reallocate(b, new_size);
	}

	void alignedReallocate(Blk &b, size_t n, unsigned alignment)
	{
		if (Primary::owns(b)) {
			return Primary::alignedReallocate(b, n, alignment);
		}

		return FallBack::alignedReallocate(b, n, alignment);
	}

	bool owns(Blk b)
	{
		return Primary::owns(b) || FallBack::owns(b);
	}
};

}  /* namespace memori */
