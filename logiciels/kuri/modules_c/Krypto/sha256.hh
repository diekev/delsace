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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

namespace sha256 {

using uint8 = unsigned char;
using uint32 = unsigned int;
using uint64 = unsigned long long;

class SHA256 {
protected:
	const static uint32 sha256_k[];
	static const uint32 SHA224_256_BLOCK_SIZE = (512 / 8);

public:
	void init();
	void update(const uint8 *message, uint32 len);
	void final(uint8 *digest);
	static const uint32 DIGEST_SIZE = ( 256 / 8);

protected:
	void transform(const uint8 *message, uint32 block_nb);
	uint32 m_tot_len;
	uint32 m_len;
	uint8 m_block[2*SHA224_256_BLOCK_SIZE];
	uint32 m_h[8];
};

}
