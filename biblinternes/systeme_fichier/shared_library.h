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

#include <experimental/filesystem>
#include <functional>
#include <string>

namespace dls {
namespace systeme_fichier {

/* same values as in dlfcn.h, avoids including the header here, type safety... */
enum class dso_loading {
	local     = 0,       /* RTLD_LOCAL */
	lazy      = 0x00001, /* RTLD_LAZY */
	now       = 0x00002, /* RTLD_NOW */
	no_load   = 0x00004, /* RTLD_NOLOAD */
	deep_bind = 0x00008, /* RTLD_DEEPBIND */
	global    = 0x00100, /* RTLD_GLOBAL */
	no_delete = 0x01000, /* RTLD_NODELETE */
};

constexpr dso_loading operator&(dso_loading lhs, dso_loading rhs)
{
	return static_cast<dso_loading>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

constexpr dso_loading operator|(dso_loading lhs, dso_loading rhs)
{
	return static_cast<dso_loading>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

constexpr dso_loading operator^(dso_loading lhs, dso_loading rhs)
{
	return static_cast<dso_loading>(static_cast<int>(lhs) ^ static_cast<int>(rhs));
}

constexpr dso_loading operator~(dso_loading lhs)
{
	return static_cast<dso_loading>(~static_cast<int>(lhs));
}

dso_loading &operator|=(dso_loading &lhs, dso_loading rhs);
dso_loading &operator&=(dso_loading &lhs, dso_loading rhs);
dso_loading &operator^=(dso_loading &lhs, dso_loading rhs);

/* wrapper around a void* for type safety */
class dso_symbol {
	void *m_ptr;

	explicit dso_symbol(void *ptr)
	    : m_ptr(ptr)
	{}

	friend class shared_library;

	template <typename>
	friend class dso_function;

	friend dso_symbol symbol_default(const std::string &name);
	friend dso_symbol symbol_default(const std::string &name, std::error_code &ec);
	friend dso_symbol symbol_next(const std::string &name);
	friend dso_symbol symbol_next(const std::string &name, std::error_code &ec);
};

template <typename>
class dso_function;

template <typename R, typename... Args>
class dso_function<R(Args...)> {
	void *m_ptr = nullptr;

public:
	explicit dso_function(dso_symbol symbol)
	    : m_ptr(symbol.m_ptr)
	{}

	explicit operator bool() const noexcept
	{
		return m_ptr != nullptr;
	}

	template <typename... A>
	R operator()(A &&... args)
	{
		return reinterpret_cast<R(*)(Args...)>(m_ptr)(std::forward<A>(args)...);
	}
};

template <typename R, typename... Args>
bool operator==(const dso_function<R(Args...)> &func, std::nullptr_t)
{
	return func.operator bool();
}

template <typename R, typename... Args>
bool operator!=(const dso_function<R(Args...)> &func, std::nullptr_t)
{
	return !(func == nullptr);
}

class shared_library {
	void *m_handle = nullptr;
	std::string m_chemin{};

public:
	shared_library() = default;

	explicit shared_library(const std::experimental::filesystem::path &filename,
	                        dso_loading flag = dso_loading::lazy);

	shared_library(const std::experimental::filesystem::path &filename,
	               std::error_code &ec,
	               dso_loading flag = dso_loading::lazy) noexcept;

	~shared_library() noexcept;

	shared_library(const shared_library &) = delete;
	shared_library &operator=(const shared_library &) = delete;

	shared_library(shared_library &&other);
	shared_library &operator=(shared_library &&other);

	void swap(shared_library &other);

	void open(const std::experimental::filesystem::path &filename, dso_loading flag = dso_loading::lazy);
	void open(const std::experimental::filesystem::path &filename, std::error_code &ec, dso_loading flag = dso_loading::lazy) noexcept;

	explicit operator bool() const noexcept;

	dso_symbol operator()(const std::string &symbol_name);
	dso_symbol operator()(const std::string &symbol_name, std::error_code &ec) noexcept;

	std::experimental::filesystem::path chemin() const;
};

/**
 * Find the first occurrence of the desired symbol using the default library
 * search order.
 */
dso_symbol symbol_default(const std::string &name);

/**
 * Find the first occurrence of the desired symbol using the default library
 * search order.
 */
dso_symbol symbol_default(const std::string &name, std::error_code &ec);

/**
 * Find the next occurrence of a function in the search order after the
 * current library.
 */
dso_symbol symbol_next(const std::string &name);

/**
 * Find the next occurrence of a function in the search order after the
 * current library.
 */
dso_symbol symbol_next(const std::string &name, std::error_code &ec);

}  /* namespace systeme_fichier */
}  /* namespace dls */
