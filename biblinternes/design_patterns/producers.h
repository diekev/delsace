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

#include <atomic>
#include <cstddef>

/* global static variable */
static constexpr auto maxN = 1000000000;
static std::atomic<size_t> N = 0;
static std::atomic<unsigned long> records[maxN];

class Record {
	std::atomic<size_t> count_;
	int data_[maxN];

public:
	Record()
	{
		// initialize data
		int n = 0; // or whatever is appropriate

		// must be done last
		count_.store(n, std::memory_order_release);
	}

	bool ready() const
	{
		return count_.load(std::memory_order_acquire);
	}
};

class Producer {
public:
	/* produce some task to be handled by consumers */
	void produce()
	{
		size_t oldN; // = N.release_increment(1);

		do {
			oldN = N;
			/* ... do something with oldN ...
			 * e.g. build_record(R, records[0], ..., records[oldN - 1]) */
		} while (N.compare_exchange_weak(oldN, oldN + 1) != oldN + 1);

		/* slot oldN + 1 is ours, we can push it to the queue */

		// unsigned long x = compute_new_x();
		// records[oldN].store(x);
	}
};

class Consumer {
public:
	void consume()
	{
		size_t currentN = N.load(std::memory_order_acquire);

		for (auto i = 0; i < currentN && records[i].ready(); ++i) {
			// process_data(records[i]);
		}
	}
};
