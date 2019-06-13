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

#include "file.h"

#include <cstdarg>

namespace dls {
namespace systeme_fichier {

File::File(const std::string &filename, const std::string &modes)
    : File()
{
	open(filename, modes);
}

void File::open(const std::string &filename, const std::string &modes)
{
	auto fptr = std::fopen(filename.c_str(), modes.c_str());

	if (fptr == nullptr) {
		std::perror("Cannot open file");
		return;
	}

	m_file = Ptr(fptr, FileDeleter());

	std::fseek(m_file.get(), 0l, SEEK_END);
	m_size = static_cast<size_t>(std::ftell(m_file.get()));
	std::fseek(m_file.get(), 0l, SEEK_SET);
}

bool File::isGood() const noexcept
{
	return m_file.get() != nullptr;
}

int File::getc()
{
	return std::getc(m_file.get());
}

int File::putc(int c)
{
	return std::putc(c, m_file.get());
}

int File::print(const char *fmt, ...)
{
	std::va_list args;
	va_start(args, fmt);
	auto ret = std::vfprintf(m_file.get(), fmt, args);
	va_end(args);

	return ret;
}

int File::scan(const char *fmt, ...)
{
	std::va_list args;
	va_start(args, fmt);
	auto ret = std::vfscanf(m_file.get(), fmt, args);
	va_end(args);

	return ret;
}

size_t File::read(void *ptr, size_t element_size, size_t number)
{
	return std::fread(ptr, element_size, number, m_file.get());
}

size_t File::write(const void *ptr, size_t element_size, size_t number)
{
	return std::fwrite(ptr, element_size, number, m_file.get());
}

void File::rewind()
{
	std::rewind(m_file.get());
}

size_t File::size() const noexcept
{
	return m_size;
}

File::operator bool() const noexcept
{
	return m_file != nullptr;
}

Pipe::Pipe()
    : m_file(nullptr)
{}

Pipe::Pipe(const std::string &filename, const std::string &modes)
    : Pipe()
{
	open(filename, modes);
}

void Pipe::open(const std::string &filename, const std::string &modes)
{
	m_file = Ptr(popen(filename.c_str(), modes.c_str()), FileDeleter());
}

std::string Pipe::output()
{
	std::string output;

	char buffer[128];
	while (!std::feof(m_file.get())) {
		if (std::fgets(buffer, 128, m_file.get()) != nullptr) {
			output += buffer;
		}
	}

	return output;
}

}  /* namespace systeme_fichier */
}  /* namespace dls */
