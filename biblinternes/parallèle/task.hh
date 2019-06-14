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
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

using lock_t = std::unique_lock<std::mutex>;

class notification_queue {
	std::deque<std::function<void()>> m_queue{};
	bool m_done{false};
	std::mutex m_mutex{};
	std::condition_variable m_ready{};

public:
	notification_queue() = default;
	notification_queue(notification_queue &&other);
	notification_queue(const notification_queue &other);

	notification_queue &operator=(notification_queue &&other);
	notification_queue &operator=(const notification_queue &other);

	bool try_pop(std::function<void()> &x);

	template <typename F>
	bool try_push(F &&f)
	{
		{
			lock_t lock(m_mutex, std::try_to_lock);

			if (!lock) {
				return false;
			}

			m_queue.emplace_back(std::forward<F>(f));
		}

		m_ready.notify_all();
		return true;
	}

	void done();

	bool pop(std::function<void()> &x);

	template <typename F>
	void push(F &&f)
	{
		{
			lock_t lock(m_mutex);
			m_queue.emplace_back(std::forward<F>(f));
		}

		m_ready.notify_one();
	}
};

class task_system {
	unsigned int m_count = std::thread::hardware_concurrency();
	std::vector<std::thread> m_threads{};
	std::vector<notification_queue> m_queue{};
	std::atomic<unsigned> m_index{};

	void run(unsigned i);

public:
	task_system();

	~task_system();

	template <typename F>
	void async_(F &&f)
	{
		auto i = m_index++;

		/* try to balance out the load on the threads */
		for (unsigned n = 0; n < m_count; ++n) {
			if (m_queue[(i + n) % m_count].try_push(std::forward<F>(f))) {
				return;
			}
		}

		m_queue[i % m_count].push(std::forward<F>(f));
	}
};
