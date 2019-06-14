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

#include "compiler.h"

#include <cassert>
#include <iostream>

#include "node.h"
#include "x86.h"

namespace jit {

using namespace x86;

compiler::~compiler()
{
	for (auto &n : m_nodes) {
		delete n;
	}
}

void compiler::add_node(node *n)
{
	if (!m_nodes.empty()) {
		n->link(m_nodes.back());
	}

	m_nodes.push_back(n);
}

void compiler::compile(code_vector &code) const
{
	std::ostream &os = std::cout;
	os << std::hex << std::showbase;

	auto get_register = [&](int value)
	{
		auto r = static_cast<reg>(value);

		switch (r) {
			default:
			case reg::r0: return "%rax";
			case reg::r1: return "%rcx";
			case reg::r2: return "%rdx";
			case reg::fp: return "%rbp";
		}
	};

	for (node *n : m_nodes) {
		switch (n->code) {
			case operation::add:
				os << "add " << get_register(n->u.w) << ' ' << get_register(n->v.w) << '\n';
				add(code, n->u.w, n->v.w, n->u.w);
				break;
			case operation::sub:
				os << "sub " << get_register(n->u.w) << ' ' << get_register(n->v.w) << '\n';
				sub(code, n->u.w, n->v.w, n->u.w);
				break;
			case operation::mul:
				os << "mul " << get_register(n->u.w) << ' ' << get_register(n->v.w) << '\n';
				mul(code, n->u.w, n->v.w, n->u.w);
				break;
			case operation::div:
				os << "div " << get_register(n->u.w) << ' ' << get_register(n->v.w) << '\n';
				div(code, n->u.w, n->v.w, n->u.w);
				break;
			case operation::mov:
				os << "mov " << n->u.w << ' ' << get_register(n->v.w) << '\n';
				mov(code, n->v.w, n->u.w);
				break;
			case operation::mov_reg:
				os << "mov " << get_register(n->u.w) << ' ' << get_register(n->v.w) << '\n';
				mov_reg(code, n->u.w, n->v.w);
				break;
			case operation::push:
				os << "push " << n->u.w << ' ' << get_register(n->v.w) << ' ' << get_register(n->w.w) << '\n';
				push_value(code, n->v.w, n->w.w, n->u.w);
				break;
			case operation::pop:
				os << "pop " << n->u.w << ' ' << get_register(n->v.w) << ' ' << get_register(n->w.w) << '\n';
				pop_value(code, n->v.w, n->w.w, n->u.w);
				break;
			case operation::ret:
				os << "ret" << '\n';
				ret(code);
			default:
				break;
		}
	}
}

state::state()
    : m_compiler(new compiler)
{}

state::~state()
{
	delete m_compiler;
}

void state::emit(operation op)
{
	node *n = new node(op, 0, 0);
	m_compiler->add_node(n);
}

void state::emit(operation op, int val, reg r)
{
	node *n = new node(op, val, static_cast<int>(r));
	m_compiler->add_node(n);
}

void state::emit(operation op, reg reg0, reg reg1)
{
	node *n = new node(op, static_cast<int>(reg0), static_cast<int>(reg1));
	m_compiler->add_node(n);
}

void state::emit(operation op, int val, reg reg0, reg reg1)
{
	node *n = new node(op, val, static_cast<int>(reg0), static_cast<int>(reg1));
	m_compiler->add_node(n);
}

void state::finalize()
{
	m_compiler->compile(m_code);
}

const code_vector &state::code() const
{
	return m_code;
}

code_vector &state::code()
{
	return m_code;
}

}  /* namespace jit */
