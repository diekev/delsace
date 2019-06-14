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

#include <cstdio>
#include <cstdint>
#include <new>
#include <string.h>
#include <sys/mman.h>
#include <utility>
#include <vector>

class code_vector : public std::vector<std::uint8_t> {
public:
	template <typename T>
	void push_value(iterator whither, T what)
	{
		auto position = whither - begin();
		insert(whither, sizeof(T), 0x00);
		*reinterpret_cast<T*>(&(*this)[static_cast<size_t>(position)]) = what;
	}
};

template <typename>
class function;

template <typename R, typename... Args>
class function<R(Args...)> {
	size_t m_size;
	void *m_data;
	bool m_executable;

public:
	function() noexcept
	    : function(0)
	{}

	explicit function(size_t size) noexcept(false)
	    : m_size(size)
	    , m_data(nullptr)
	    , m_executable(false)
	{
		if (size == 0) {
			return;
		}

		const auto protection = PROT_READ | PROT_WRITE;
		const auto flags = MAP_ANONYMOUS | MAP_PRIVATE;

		void *ptr = mmap(nullptr, m_size, protection, flags, -1, 0);

		if (ptr == MAP_FAILED) {
			throw std::bad_alloc();
		}

		m_data = ptr;
	}

	function(void *data, size_t size) noexcept(false)
	    : function(size)
	{
		memcpy(m_data, data, size);
		set_executable(true);
	}

	template<typename Iterator>
	function(Iterator first, Iterator last) noexcept(false)
		: function(&(*first), static_cast<size_t>(last - first))
	{}

	function(const function &other) noexcept
	    : function()
	{
		function copy(other.m_size);
		memcpy(copy.m_data, other.m_data, other.m_size);
		copy.set_executable(other.m_executable);

		swap(copy);
	}

	function(function &&other) noexcept
		: function()
	{
		swap(other);
	}

	~function() noexcept
	{
		if (m_data != nullptr) {
			const auto ret = munmap(m_data, m_size);

			if (ret != 0) {
				std::perror(nullptr);
			}
		}
	}

	function &operator=(function other) noexcept
	{
		swap(other);
		return *this;
	}

	template <typename... RArgsTypes>
	R operator()(RArgsTypes &&... args) const noexcept
	{
		return reinterpret_cast<R(*)(Args...)>(m_data)(std::forward<RArgsTypes>(args)...);
	}

	void set_executable(bool yesno) noexcept
	{
		if (m_executable != yesno) {
			const auto ret = mprotect(m_data, m_size, PROT_READ | PROT_EXEC);

			if (ret != 0) {
				std::perror("Cannot mark memory as executable");
			}

			m_executable = yesno;
		}
	}

	void swap(function &other) noexcept
	{
		using std::swap;
		swap(m_size, other.m_size);
		swap(m_data, other.m_data);
		swap(m_executable, other.m_executable);
	}
};
