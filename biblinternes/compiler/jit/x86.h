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

#include "function.h"

namespace jit {

int register_index(int value);

enum class x86_reg_index {
	/* Index of al/ah/ax/eax/rax registers. */
	ax = 0,
	/* Index of cl/ch/cx/ecx/rcx registers. */
	cx = 1,
	/* Index of dl/dh/dx/edx/rdx registers. */
	dx = 2,
	/* Index of bl/bh/bx/ebx/rbx registers. */
	bx = 3,
	/* Index of spl/sp/esp/rsp registers. */
	sp = 4,
	/* Index of bpl/bp/ebp/rbp registers. */
	bp = 5,
	/* Index of sil/si/esi/rsi registers. */
	si = 6,
	/* Index of dil/di/edi/rdi registers. */
	di = 7,
	/* Index of r8b/r8w/r8d/r8 registers (64-bit only). */
	r8 = 8,
	/* Index of r9b/r9w/r9d/r9 registers (64-bit only). */
	r9 = 9,
	/* Index of r10b/r10w/r10d/r10 registers (64-bit only). */
	r10 = 10,
	/* Index of r11b/r11w/r11d/r11 registers (64-bit only). */
	r11 = 11,
	/* Index of r12b/r12w/r12d/r12 registers (64-bit only). */
	r12 = 12,
	/* Index of r13b/r13w/r13d/r13 registers (64-bit only). */
	r13 = 13,
	/* Index of r14b/r14w/r14d/r14 registers (64-bit only). */
	r14 = 14,
	/* Index of r15b/r15w/r15d/r15 registers (64-bit only). */
	r15 = 15
};

namespace x86 {

void rex(code_vector &vec, int l, int w, int r, int x, int b);

void rx(code_vector &vec, int rd, int md, int rb, int ri, int ms);

void mov(code_vector &vec, int r0, int i0);

void mov_reg(code_vector &vec, int r0, int r1);

/* load effective address */
void lea(code_vector &vec, int md, int rb, int ri, int ms, int rd);

/* ldx */
void pop_value(code_vector &vec, int r0, int r1, int i0);

/* stx */
void push_value(code_vector &vec, int r0, int r1, int i0);

/* pop stack */
void pop(code_vector &vec, int r0);

/* push stack */
void push(code_vector &vec, int r0);

/* pop stack */
void ret(code_vector &vec);

void add(code_vector &vec, int r0, int r1, int r2);

void unr(code_vector &vec, int code, int r0);

void neg_reg(code_vector &vec, int r0);

void alu_reg(code_vector &vec, int code, int r0, int r1);

void sub_reg(code_vector &vec, int r0, int r1);

void xor_reg(code_vector &vec, int r0, int r1);

void neg_reg(code_vector &vec, int r0, int r1);

void sub(code_vector &vec, int r0, int r1, int r2);

void mul_reg(code_vector &vec, int r0, int r1);

void mul(code_vector &vec, int r0, int r1, int r2);

void div_reg(code_vector &vec, int r0);

void div(code_vector &vec, int r0, int r1, int r2);

}  /* namespace x86 */
}  /* namespace jit */
