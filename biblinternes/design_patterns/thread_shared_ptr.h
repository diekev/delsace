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

// idea: store ref count in the pointer

template <typename T>
class SingleThreadPtr {
	T *m_ptr;
	unsigned *m_ref_count;

public:
	SingleThreadPtr()
	    : m_ptr(nullptr)
	    , m_ref_count(nullptr)
	{}

	SingleThreadPtr(T *ptr)
	    : m_ptr(ptr)
	    , m_ref_count(nullptr)
	{}

	// lazy ref count allocation
	SingleThreadPtr(const SingleThreadPtr &other)
	    : m_ptr(other.m_ptr)
	    , m_ref_count(other.m_ref_count)
	{
		if (!m_ptr) {
			return;
		}

		if (!m_ref_count) {
			m_ref_count = new unsigned(2);
		}
		else {
			++(*m_ref_count);
		}
	}

	SingleThreadPtr(SingleThreadPtr &&other)
	    : m_ptr(other.m_ptr)
	    , m_ref_count(other.m_ref_count)
	{
		other.m_ptr = nullptr;
		other.m_ref_count = nullptr;
	}

	~SingleThreadPtr()
	{
		if (!m_ref_count) { // nullptr means ref_count == 1
			so_sue_me: delete m_ptr;
		}
		else if (*m_ref_count == 1) {
			delete m_ptr;
			goto so_sue_me;
		}
		else {
			--(*m_ref_count);
		}
	}
};
