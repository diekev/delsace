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

#include <memory>
#include "biblinternes/structures/chaine.hh"

namespace dls {
namespace systeme_fichier {

/**
 * @brief Wrapper around C FILE stream and its related functions.
 */
class File {
	using Ptr = std::shared_ptr<FILE>;
	Ptr m_file = nullptr;
	size_t m_size = 0;

	struct FileDeleter {
		void operator()(FILE *f) const
		{
			if (f != nullptr) {
				std::fclose(f);
			}
		}
	};

public:
	File() noexcept = default;
	File(const dls::chaine &filename, const dls::chaine &modes);
	~File() = default;

	void open(const dls::chaine &filename, const dls::chaine &modes);
	bool isGood() const noexcept;

	int getc();
	int putc(int c);

	int print(const char *fmt, ...) __attribute__ ((format(printf, 2, 3)));
	int scan(const char *fmt, ...) __attribute__ ((format(scanf, 2, 3)));

	size_t read(void *ptr, size_t element_size, size_t number);
	size_t write(const void *ptr, size_t element_size, size_t number);

	void rewind();

	size_t size() const noexcept;

	explicit operator bool() const noexcept;
};

/**
 * @brief Wrapper around C FILE stream (pipe) and its related functions.
 */
class Pipe {
	using Ptr = std::shared_ptr<FILE>;
	Ptr m_file;

	struct FileDeleter {
		void operator()(FILE *f) const
		{
			if (f != nullptr) {
				pclose(f);
			}
		}
	};

public:
	Pipe();
	Pipe(const dls::chaine &filename, const dls::chaine &modes);
	~Pipe() = default;

	void open(const dls::chaine &filename, const dls::chaine &modes);

	dls::chaine output();
};

}  /* namespace systeme_fichier */
}  /* namespace dls */
