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

#include "filebuffer.h"

#include <fstream>

FileBuffer::FileBuffer(const dls::chaine &filename) noexcept(false)
    : m_buffer{}
{
	std::ifstream file(filename.c_str());

	if (!file.is_open()) {
		std::printf("File not open!\n");
		return;
	}

	m_buffer = {
	    std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()
	};
}

auto FileBuffer::size() const noexcept
{
	return m_buffer.taille();
}

const char *FileBuffer::begin() const noexcept
{
	return &m_buffer[0];
}

const char *FileBuffer::end() const noexcept
{
	return &m_buffer[m_buffer.taille() - 1];
}

FileBuffer::operator bool() const noexcept
{
	return m_buffer.taille() != 0;
}
