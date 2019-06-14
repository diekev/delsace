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

#include <vector>

#include "function.h"

namespace jit {

class node;

enum class operation {
	mov,
	mov_reg,
	add,
	sub,
	div,
	mul,
	ret,
	push,
	pop,
	none,
};

enum class reg {
	r0 = 0,
	r1 = 1,
	r2 = 2,
	r3 = 3,

	v0 = 4,
	v1 = 5,
	v2 = 6,
	v3 = 7,

	f0 = 8,
	f1 = 9,
	f2 = 10,
	f3 = 11,
	f4 = 12,
	f5 = 13,
	f6 = 14,
	f7 = 15,

	fp = 16,

	none,
};

class compiler {
	std::vector<node *> m_nodes{};

public:
	compiler() = default;
	~compiler();

	void add_node(node *n);

	void compile(code_vector &code) const;
};

class state {
	compiler *m_compiler = nullptr;
	code_vector m_code{};

public:
	state();
	state(const state &other) = default;
	~state();

	state &operator=(const state &rhs) = default;

	void emit(operation op);
	void emit(operation op, int val, reg reg0);
	void emit(operation op, int val, reg reg1, reg reg0);
	void emit(operation op, reg reg0, reg reg1);

	void finalize();

	const code_vector &code() const;
	code_vector &code();
};

}  /* namespace jit */
