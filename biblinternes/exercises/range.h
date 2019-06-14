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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

template <typename Iter>
class normal_range {
	Iter m_begin;
	Iter m_end;

public:
	using value_type = typename Iter::value_type;

	constexpr normal_range(Iter begin, Iter end) noexcept
	    : m_begin(begin)
	    , m_end(end)
	{}

	bool empty() noexcept
	{
		return m_begin >= m_end;
	}

	void pop() noexcept
	{
		++m_begin;
	}

	value_type front() noexcept
	{
		return *m_begin;
	}
};

template <typename Iter>
class range {
	Iter m_begin;
	Iter m_end;

public:
	using value_type = typename Iter::value_type;

	constexpr range(Iter begin, Iter end) noexcept
	    : m_begin(begin)
	    , m_end(end)
	{}

	bool empty() noexcept
	{
		return m_begin > m_end;
	}

	void pop_front() noexcept
	{
		++m_begin;
	}

	void pop_back() noexcept
	{
		--m_end;
	}

	value_type front() noexcept
	{
		return *m_begin;
	}

	value_type back() noexcept
	{
		return *m_end;
	}
};

template <typename Range>
class reverse_range {
	Range m_range;

public:
	using value_type = typename Range::value_type;

	constexpr reverse_range(Range r) noexcept
	    : m_range(r)
	{}

	bool empty() noexcept
	{
		return m_range.empty();
	}

	void pop_front() noexcept
	{
		m_range.pop_back();
	}

	void pop_back() noexcept
	{
		m_range.pop_front();
	}

	value_type front() noexcept
	{
		return m_range.back();
	}

	value_type back() noexcept
	{
		return m_range.front();
	}
};
