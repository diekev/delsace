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

// Extend FreeList
//FreeList<
//	A,      // parent allocator
//	17,     // Use list for object sizes 17 ...
//	32,     // ... through to 32
//	8,      // allocate 8 at a time
//	1024>;  // no more than 1024 remembered

template <class AllocatorType, size_t MIN, size_t MAX>
class FreeList {
	AllocatorType m_parent;

	struct Node {
		Node *next;
	};

	Node *m_root;

public:
	FreeList()
	    : m_root(nullptr)
	{}

	~FreeList()
	{
		auto ptr = m_root;

		while (ptr != nullptr) {
			Blk b = { ptr, 0 };
			ptr = ptr->next;
			m_parent.deallocate(b);
		}
	}

	Blk allocate(size_t n)
	{
		if ((n >= MIN && n <= MAX) && m_root) {
			Blk r = { m_root, MAX };
			m_root = m_root->next;
			return r;
		}

		return m_parent.allocate(MAX);
	}

	void deallocate(Blk b)
	{
		if ((b.size >= MIN && b.size <= MAX)) {
			auto ptr = static_cast<Node *>(b.ptr);
			ptr->next = m_root;
			m_root = ptr;
			return;
		}

		m_parent.deallocate(b);
	}

	void reallocate(Blk &b, size_t new_size)
	{
		if ((new_size >= MIN && new_size <= MAX) && m_root) {
			b.ptr = m_root;
			b.size = new_size;
			m_root = m_root->next;
			return;
		}

		m_parent.reallocate(b, new_size);
	}

	bool owns(Blk b)
	{
		return (b.size >= MIN && b.size <= MAX) || m_parent.owns(b);
	}
};

}  /* namespace memori */
