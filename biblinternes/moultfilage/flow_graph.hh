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

#include "biblinternes/structures/chaine.hh"
#include <tuple>

#include <tbb/flow_graph.h>

struct body {
	dls::chaine m_name;

	explicit body(const dls::chaine &name)
	    : m_name(name)
	{}

	void operator()(tbb::flow::continue_msg) const
	{
		printf("%s\n", m_name.c_str());
	}
};

struct square {
	int operator()(int v)
	{
		return v * v;
	}
};

struct cube {
	int operator()(int v)
	{
		return v * v * v;
	}
};

class sum {
	int &m_sum;

public:
	explicit sum(int &s)
	    : m_sum(s)
	{}

	int operator()(std::tuple<int, int> v)
	{
		m_sum += std::get<0>(v) + std::get<1>(v);

		return m_sum;
	}
};

void test_flow_graph();

