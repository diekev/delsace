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

#include "task.hh"

notification_queue::notification_queue(notification_queue &&other)
    : m_done(other.m_done)
{
	other.m_done = false;
}

notification_queue::notification_queue(const notification_queue &other)
    : m_done(other.m_done)
{}

notification_queue &notification_queue::operator=(notification_queue &&other)
{
	m_done = std::move(other.m_done);
	other.m_done = false;
	return *this;
}

notification_queue &notification_queue::operator=(const notification_queue &other)
{
	m_done = other.m_done;
	return *this;
}

bool notification_queue::try_pop(std::function<void ()> &x)
{
	lock_t lock(m_mutex, std::try_to_lock);

	if (!lock || m_queue.empty()) {
		return false;
	}

	x = std::move(m_queue.front());
	m_queue.pop_front();

	return true;
}

void notification_queue::done()
{
	{
		lock_t lock(m_mutex);
		m_done = true;
	}

	m_ready.notify_all();
}

bool notification_queue::pop(std::function<void ()> &x)
{
	lock_t lock(m_mutex);

	while (m_queue.empty() && !m_done) {
		m_ready.wait(lock);
	}

	if (m_queue.empty()) {
		return false;
	}

	x = std::move(m_queue.front());
	m_queue.pop_front();

	return true;
}

/* ************************************************************************** */

task_system::task_system()
    : m_index(0)
{
	m_queue.resize(m_count);

	for (unsigned n = 0; n != m_count; ++n) {
		m_threads.emplace_back([&, n] { run(n); });
	}
}

task_system::~task_system()
{
	for (auto &e : m_queue) {
		e.done();
	}

	for (auto &thread : m_threads) {
		thread.join();
	}
}

void task_system::run(unsigned i)
{
	while (true) {
		std::function<void()> f;

		/* try to steal a task from another thread */
		for (unsigned n = 0; n < m_count; ++n) {
			if (m_queue[(i + n) % m_count].try_pop(f)) {
				break;
			}
		}

		if (!f && !m_queue[i].pop(f)) {
			break;
		}

		f();
	}
}
