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

#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

template<typename T>
void draw(const T &x, std::ostream &out, size_t position)
{
	out << std::string(position, ' ') << "\033[0m" << x << "\n";
}

class object_t {
	struct concept_t {
		virtual ~concept_t() = default;
		virtual void draw_impl(std::ostream &, size_t) const = 0;
	};

	std::shared_ptr<const concept_t> m_self;

	template<typename T>
	struct model : concept_t {
		model(T x)
		    : m_data(std::move(x))
		{}

		void draw_impl(std::ostream &out, size_t position) const
		{
			draw(m_data, out, position);
		}

		T m_data;
	};

public:
	template<typename T>
	explicit object_t(T x)
	    : m_self(std::make_shared<model<T>>(std::move(x)))
	{
		/* std::cout << "ctor\n"; */
	}

	friend void draw(const object_t &x, std::ostream &out, size_t position)
	{
		x.m_self->draw_impl(out, position);
	}
};

using document_t = std::vector<object_t>;

void draw(const document_t &x, std::ostream &out, size_t position);

using history_t = std::vector<document_t>;

void commit(history_t &x);
void undo(history_t &x);
document_t &current(history_t &x);

void test_history();

class my_class_t {
	/* ... */
};

void draw(const my_class_t &, std::ostream &out, size_t position);

