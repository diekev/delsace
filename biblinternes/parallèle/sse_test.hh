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

#define ROUND_DOWN(x, s) ((x) & ~((s) - 1))

template <typename OutputPointer, typename InputPointer, typename OpType>
void transform_n(OutputPointer dst, InputPointer src, size_t len, OpType &&op)
{
	for (size_t i(0); i < len; ++i) {
		op(dst + i, src + i);
	}
}

template <typename OutputPointer, typename InputPointer, typename OpType>
void transform_n_sse(OutputPointer dst, InputPointer src, size_t len, OpType &&op)
{
	size_t n, ne = ROUND_DOWN(len, op.BLOCK_SIZE);

	for (n = 0; n < ne; n += op.BLOCK_SIZE) {
		op.block(dst + n, src + n);
	}

	for (; n < len; ++n) {
		op(dst + n, src + n);
	}
}
