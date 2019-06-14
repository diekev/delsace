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

bool is_aligned(void *ptr, size_t alignment)
{
	return ((size_t(ptr) & (alignment - 1)) == 0);
}

template <size_t STACK_SIZE>
class StackAllocator {
	char m_d[STACK_SIZE];
	char *m_ptr;

	static constexpr size_t alignement = 8;

	auto roundToAlign(size_t n) -> size_t
	{
		auto remainder = n % alignement;

		if (remainder == 0) {
			return n;
		}

		return n + alignement - remainder;
	}

public:
	StackAllocator()
	    : m_ptr(m_d)
	{}

	Blk allocate(size_t n)
	{
		auto n1 = roundToAlign(n);

		if (n1 > (m_d + STACK_SIZE) - m_ptr) {
			return { nullptr, 0 };
		}

		Blk result = { m_ptr, n };
		m_ptr += n1;
		return result;
	}

	Blk allocateAll()
	{
		Blk r = { m_ptr, STACK_SIZE };
		m_ptr += STACK_SIZE;
		return r;
	}

	Blk alignedAllocate(size_t n, unsigned alignment)
	{
		if (is_aligned(m_ptr, alignment)) {
			return allocate(n);
		}

		auto ptr_delta = roundToAlign(size_t(m_ptr));
		auto n1 = roundToAlign(n);

		if (n1 > (m_d + STACK_SIZE) - (m_ptr + ptr_delta)) {
			return { nullptr, 0 };
		}

		m_ptr += ptr_delta;
		Blk result = { m_ptr, n };
		m_ptr += n1;
		return result;
	}

	void deallocate(Blk b)
	{
		auto ptr = static_cast<char *>(b.ptr);
		if (ptr + roundToAlign(b.size) == m_ptr) {
			m_ptr = ptr;
		}
	}

	void deallocateAll()
	{
		m_ptr = m_d;
	}

	bool expand(Blk &b, size_t delta)
	{
		/* can only expand if the block was the last one allocated... */
		auto ptr = static_cast<char *>(b.ptr);
		if (ptr + roundToAlign(b.size) != m_ptr) {
			return false;
		}

		/* ... and there is room for delta. */
		auto n1 = roundToAlign(delta);
		if (n1 > (m_d + STACK_SIZE) - m_ptr) {
			return false;
		}

		m_ptr += n1;
		b.size += n1;

		return true;
	}

	bool owns(Blk b)
	{
		return b.ptr >= m_d && b.ptr < (m_d + STACK_SIZE);
	}

	// void reallocate(Blk&, size_t delta);
	// Blk alignedReallocate(size_t, unsigned);
};

}  /* namespace memori */
