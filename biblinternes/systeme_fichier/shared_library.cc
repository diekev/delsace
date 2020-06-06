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

#include "shared_library.h"

#include <dlfcn.h>
#include <iostream>

namespace dls {
namespace systeme_fichier {

namespace __detail {

class dynamic_loading_category : public std::error_category {
public:
	constexpr dynamic_loading_category()
	    : error_category()
	{}

	const char *name() const noexcept override
	{
		return "dynamic_loading_category";
	}

	std::string message(int condition) const override
	{
		switch (condition) {
			case 0:
				return "Success";
			case 1:
				return "Failure";
		}

		return "Unknown";
	}
};

static dynamic_loading_category erreur_chargement;

static void *get_symbol(void *handle, const dls::chaine &name, std::error_code &ec)
{
	dlerror();  /* clear any existing error */

	const auto sym = dlsym(handle, name.c_str());

	const auto err = dlerror();
	if (err != nullptr) {
		ec.assign(1, dynamic_loading_category());
	}

	return sym;
}

static void *get_symbol(void *handle, const dls::chaine &name)
{
	std::error_code ec;
	const auto sym = get_symbol(handle, name, ec);

	if (ec != std::error_code()) {
		throw std::filesystem::filesystem_error("Cannot lookup symbol in library", ec);
	}

	return sym;
}

}  /* namespace __detail */

dso_loading &operator|=(dso_loading &lhs, dso_loading rhs)
{
	return (lhs = lhs | rhs);
}

dso_loading &operator&=(dso_loading &lhs, dso_loading rhs)
{
	return (lhs = lhs & rhs);
}

dso_loading &operator^=(dso_loading &lhs, dso_loading rhs)
{
	return (lhs = lhs ^ rhs);
}

shared_library::shared_library(const std::filesystem::path &filename, dso_loading flag)
    : shared_library()
{
	open(filename, flag);
}

shared_library::shared_library(const std::filesystem::path &filename, std::error_code &ec, dso_loading flag) noexcept
    : shared_library()
{
	open(filename, ec, flag);
}

shared_library::shared_library(shared_library &&other)
    : shared_library()
{
	swap(other);
}

shared_library &shared_library::operator=(shared_library &&other)
{
	swap(other);
	return *this;
}

shared_library::~shared_library() noexcept
{
	if (m_handle == nullptr) {
		return;
	}

	if (dlclose(m_handle) != 0) {
		std::cerr << dlerror() << '\n';
	}
}

void shared_library::swap(shared_library &other)
{
	using std::swap;
	swap(m_handle, other.m_handle);
	swap(m_chemin, other.m_chemin);
}

void shared_library::open(const std::filesystem::path &filename, dso_loading flag)
{
	if (m_chemin == filename.c_str()) {
		return;
	}

	std::error_code ec;
	open(filename, ec, flag);

	if (ec != std::error_code()) {
		throw std::filesystem::filesystem_error(dlerror(), m_chemin.c_str(), filename, ec);
	}
}

void shared_library::open(const std::filesystem::path &filename, std::error_code &ec, dso_loading flag) noexcept
{
	if (m_chemin == filename.c_str()) {
		return;
	}

	if (m_handle != nullptr) {
		if (dlclose(m_handle) != 0) {
			ec.assign(1, __detail::erreur_chargement);
			return;
		}
	}

	m_handle = dlopen(filename.c_str(), static_cast<int>(flag));

	if (m_handle == nullptr) {
		ec.assign(1, __detail::erreur_chargement);
		return;
	}

	m_chemin = filename.c_str();
}

dso_symbol shared_library::operator()(const dls::chaine &symbol_name)
{
	return dso_symbol{ __detail::get_symbol(m_handle, symbol_name) };
}

dso_symbol shared_library::operator()(const dls::chaine &symbol_name, std::error_code &ec) noexcept
{
	return dso_symbol{ __detail::get_symbol(m_handle, symbol_name, ec) };
}

std::filesystem::path shared_library::chemin() const
{
	return m_chemin.c_str();
}

shared_library::operator bool() const noexcept
{
	return (m_handle != nullptr);
}

dso_symbol symbol_next(const dls::chaine &name)
{
	return dso_symbol{ __detail::get_symbol(RTLD_NEXT, name) };
}

dso_symbol symbol_next(const dls::chaine &name, std::error_code &ec)
{
	return dso_symbol{ __detail::get_symbol(RTLD_NEXT, name, ec) };
}

dso_symbol symbol_default(const dls::chaine &name)
{
	return dso_symbol{ __detail::get_symbol(RTLD_DEFAULT, name) };
}

dso_symbol symbol_default(const dls::chaine &name, std::error_code &ec)
{
	return dso_symbol{ __detail::get_symbol(RTLD_DEFAULT, name, ec) };
}

}  /* namespace systeme_fichier */
}  /* namespace dls */
