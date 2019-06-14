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

#include "x86.h"

#include <cassert>
#include <iostream>

#include "compiler.h"

namespace jit {

#define _NOREG static_cast<int>(reg::none)

int register_index(int value)
{
	auto r = static_cast<reg>(value);

	switch (r) {
		/* local vars */
		case reg::r0:
		{
			return static_cast<int>(x86_reg_index::ax);
		}
		case reg::r1:
		{
			return static_cast<int>(x86_reg_index::r10);
		}
		case reg::r2:
		{
			return static_cast<int>(x86_reg_index::r11);
		}
		case reg::r3:
		{
			return static_cast<int>(x86_reg_index::r12);
		}
		/* arguments */
		case reg::v0:
		{
			return static_cast<int>(x86_reg_index::bx);
		}
		case reg::v1:
		{
			return static_cast<int>(x86_reg_index::r13);
		}
		case reg::v2:
		{
			return static_cast<int>(x86_reg_index::r14);
		}
		case reg::v3:
		{
			return static_cast<int>(x86_reg_index::r15);
		}
		/* frame pointer */
		case reg::fp:
		{
			return static_cast<int>(x86_reg_index::bp);
		}
		default:
		{
			assert(false);
			break;
		}
	}

	return 0;
}

namespace x86 {

#define WIDE 1

#define X86_DIV 6
#define X86_SUB (5 << 3)
#define X86_XOR (6 << 3)

#define _RDX_REGNO 2
#define _RSP_REGNO 4
#define _RBP_REGNO 5

#define mrm(md, r, m) (static_cast<unsigned char>((md << 6) | (r << 3) | m))
#define sib(sc, i, b) (static_cast<unsigned char>((sc << 6) | (i << 3) | b))

void rex(code_vector &vec, int l, int w, int r, int x, int b)
{
	int v = 0x40 | (w << 3);

	if (r != static_cast<int>(reg::none)) {
		v |= (r & 8);
	}

	if (x != static_cast<int>(reg::none)) {
		v |= (x & 8);
	}

	if (b != static_cast<int>(reg::none)) {
		v |= (b & 8);
	}

	if (l || v != 0x40) {
		vec.insert(vec.end(), static_cast<unsigned char>(v));
	}
}

void rx(code_vector &vec, int rd, int md, int rb, int ri, int ms)
{
	if (ri == _NOREG) {
		if (rb == _NOREG) {
			vec.insert(vec.end(), mrm(0x00, rd, 0x04));
			vec.insert(vec.end(), sib(0x00, 0x04, 0x05));
			vec.push_value(vec.end(), md); // push_value
		}
		else if (rb == _RSP_REGNO) {
			if (md == 0) {
				vec.insert(vec.end(), mrm(0x00, rd, 0x04));
				vec.insert(vec.end(), sib(ms, 0x04, 0x04));
			}
			else if (true /*md == md*/) {
				vec.insert(vec.end(), mrm(0x01, rd, 0x04));
				vec.insert(vec.end(), sib(ms, 0x04, 0x04));
				vec.insert(vec.end(), static_cast<unsigned char>(md));
			}
			else {
				vec.insert(vec.end(), mrm(0x02, rd, 0x04));
				vec.insert(vec.end(), sib(ms, 0x04, 0x04));
				vec.push_value(vec.end(), md); // push_value
			}
		}
		else {
			if (md == 0 && rb != _RBP_REGNO)
				vec.insert(vec.end(), mrm(0x00, rd, rb));
			else if (true /*md == md*/) {
				vec.insert(vec.end(), mrm(0x01, rd, rb));
				vec.insert(vec.end(), static_cast<unsigned char>(md));
			}
			else {
				vec.insert(vec.end(), mrm(0x02, rd, rb));
				vec.push_value(vec.end(), md); // push_value
			}
		}
	}
	else if (rb == _NOREG) {
		vec.insert(vec.end(), mrm(0x00, rd, 0x04));
		vec.insert(vec.end(), sib(ms, ri, 0x05));
		vec.push_value(vec.end(), md); // push_value
	}
	else if (ri != _RSP_REGNO) {
		if (md == 0 && rb != _RBP_REGNO) {
			vec.insert(vec.end(), mrm(0x00, rd, 0x04));
			vec.insert(vec.end(), sib(ms, ri, rb));
		}
		else if (true /*md == md*/) {
			vec.insert(vec.end(), mrm(0x01, rd, 0x04));
			vec.insert(vec.end(), sib(ms, ri, rb));
			vec.insert(vec.end(), static_cast<unsigned char>(md));
		}
		else {
			vec.insert(vec.end(), mrm(0x02, rd, 0x04));
			vec.insert(vec.end(), sib(ms, ri, rb));
			vec.insert(vec.end(), static_cast<unsigned char>(md));
		}
	}
	else {
		std::cerr << "illegal index register\n";
		abort();
	}
}

void mov(code_vector &vec, int r0, int i0)
{
	rex(vec, 0, 1, static_cast<int>(reg::none), static_cast<int>(reg::none), r0);
	vec.insert(vec.end(), static_cast<unsigned char>(0xb8 | register_index(r0)));
	vec.push_value(vec.end(), i0);
}

void mov_reg(code_vector &vec, int r0, int r1)
{
	rex(vec, 0, 1, r1, static_cast<int>(reg::none), r0);
	vec.insert(vec.end(), 0x89);
	vec.insert(vec.end(), static_cast<unsigned char>(0xc0 | (register_index(r0) << 3) | register_index(r1)));
}

/* load effective address */
void lea(code_vector &vec, int md, int rb, int ri, int ms, int rd)
{
	rex(vec, 0, WIDE, rd, ri, rb);
	vec.insert(vec.end(), 0x8d);
	rx(vec, rd, md, rb, ri, ms);
}

/* push stack */
void push(code_vector &vec, int r0)
{
	rex(vec, 0, WIDE, 0, 0, r0);
	vec.insert(vec.end(), static_cast<unsigned char>(0x50 | register_index(r0)));
}

/* stx */
void push_value(code_vector &vec, int r0, int r1, int i0)
{
	rex(vec, 0, 0, r1, static_cast<int>(reg::none), r0);
	vec.insert(vec.end(), 0x89);
	rx(vec, r1, i0, r0, _NOREG, 0x00);
}

/* ldx */
void pop_value(code_vector &vec, int r0, int r1, int i0)
{
	rex(vec, 0, WIDE, r0, _NOREG, r1);
	vec.insert(vec.end(), 0x63);
	rx(vec, r0, i0, r1, _NOREG, 0x00);
}

/* pop stack */
void pop(code_vector &vec, int r0)
{
	rex(vec, 0, WIDE, 0, 0, r0);
	vec.insert(vec.end(), static_cast<unsigned char>(0x58 | register_index(r0)));
}

/* pop stack */
void ret(code_vector &vec)
{
	vec.insert(vec.end(), 0xc3);
}

void add(code_vector &vec, int r0, int r1, int r2)
{
	lea(vec, 0, r1, r2, 0x00, r0);
}

void unr(code_vector &vec, int code, int r0)
{
	rex(vec, 0, WIDE, _NOREG, _NOREG, r0);
	vec.insert(vec.end(), 0xf7);
	vec.insert(vec.end(), mrm(0x03, code, r0));
}

void neg_reg(code_vector &vec, int r0)
{
	unr(vec, 3, r0);
}

void alu_reg(code_vector &vec, int code, int r0, int r1)
{
	rex(vec, 0, WIDE, r1, _NOREG, r0);
	vec.insert(vec.end(), static_cast<unsigned char>(code | 0x01));
	vec.insert(vec.end(), mrm(0x03, r1, r0));
}

void sub_reg(code_vector &vec, int r0, int r1)
{
	alu_reg(vec, X86_SUB, r0, r1);
}

void xor_reg(code_vector &vec, int r0, int r1)
{
	alu_reg(vec, X86_XOR, r0, r1);
}

void neg_reg(code_vector &vec, int r0, int r1)
{
	if (r0 == r1) {
		neg_reg(vec, r0);
	}
	else {
		xor_reg(vec, r0, r0);
		sub_reg(vec, r0, r1);
	}
}

void sub(code_vector &vec, int r0, int r1, int r2)
{
	if (r1 == r2) {
		xor_reg(vec, r0, r0);
	}
	else if (r0 == r2) {
		sub_reg(vec, r0, r1);
		neg_reg(vec, r0);
	}
	else {
		mov_reg(vec, r0, r1);
	}
}

void mul_reg(code_vector &vec, int r0, int r1)
{
	rex(vec, 0, WIDE, r0, _NOREG, r1);
	vec.insert(vec.end(), 0x0f);
	vec.insert(vec.end(), 0xaf);
	vec.insert(vec.end(), mrm(0x03, r0, r1));
}

void mul(code_vector &vec, int r0, int r1, int r2)
{
	if (r0 == r1) {
		mul_reg(vec, r0, r2);
	}
	else if (r0 == r2) {
		mul_reg(vec, r0, r1);
	}
	else {
		mov_reg(vec, r0, r1);
		mul_reg(vec, r0, r2);
	}
}

void div_reg(code_vector &vec, int r0)
{
	unr(vec, X86_DIV, r0);
}

void div(code_vector &vec, int /*r0*/, int r1, int /*r2*/)
{
	xor_reg(vec, _RDX_REGNO, _RDX_REGNO);
	div_reg(vec, r1);
}

}  /* namespace x86 */
}  /* namespace jit */
