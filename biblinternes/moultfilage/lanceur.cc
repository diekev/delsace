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

#include <iostream>

#include "flow_graph.hh"
#include "image_graph.hh"
#include "task.hh"

#include <cmath>
#include <functional>

#include "../chrono/chronometre_de_portee.hh"

class range {
	int m_begin = 0;
	int m_end = 0;

public:
	range() = default;

	range(int begin, int end)
		: m_begin(begin)
		, m_end(end)
	{}

	int debut() const noexcept
	{
		return m_begin;
	}

	int fin() const noexcept
	{
		return m_end;
	}
};

template <typename OpType>
void parallel_for(const range &r, const OpType &op, int grain_size = 1)
{
	const auto task_size = r.fin() - r.debut();
	const auto num_subtasks = task_size / grain_size;
	const auto stride = task_size / num_subtasks;

	auto start = r.debut();

	task_system task;

	for (auto c = 0; c != num_subtasks; ++c) {
		if ((start + stride) >= r.fin()) {
			auto func = std::bind(op, range(start, r.fin()));
//			std::cerr << "Range: " << start << ", " << r.fin() << '\n';
			task.async_(func);
			break;
		}

		range sub(start, (start + stride));

//		std::cerr << "Range: " << start << ", " << (start + stride) - 1 << '\n';
		start += stride;

		auto func = std::bind(op, sub);
		task.async_(func);
	}
}

template <typename OpType>
void serial_for(const range &r, OpType op, int /* grain_size */)
{
	op(r);
}

int main()
{
	std::atomic<size_t> total(0);

	auto task = [&](const range &r)
	{
		size_t sub_total = 0;
		const auto sqrt_5 = std::sqrt(5.0f);

		for (auto i = r.debut(); i != r.fin(); ++i) {
			sub_total += static_cast<size_t>(static_cast<float>(i) * sqrt_5);
		}

		total += sub_total;
	};

	const auto grain_size = 100000;
	range r(0, 1600000000);

	{
		CHRONOMETRE_PORTEE("Serial loop", std::cout);
		total = 0;

		serial_for(r, task, grain_size);

		std::cout << "Total: " << total << '\n';
	}

	{
		CHRONOMETRE_PORTEE("Parallel loop", std::cout);
		total = 0;

		parallel_for(r, task, grain_size);

		std::cout << "Total: " << total << '\n';
	}
}
