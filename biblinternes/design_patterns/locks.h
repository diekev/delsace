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

#include <thread>

class spin_lock {
	std::atomic<int> m_lock;

	enum state : int {
		unlock = 0,
		lock = 1,
	};

public:
	spin_lock()
	    : m_lock(unlock)
	{}

	void lock()
	{
		while (m_lock.exchange(state::lock) == state::lock) {}
	}

	void unlock()
	{
		m_lock.exchange(state::unlock);
	}
};

namespace starvation {

class spin_lock {
	std::atomic<int> m_lock;

	enum state : int {
		unlock = 0,
		lock = 1,
	};

public:
	spin_lock()
	    : m_lock(state::unlock)
	{}

	void lock()
	{
		int tmp = state::unlock;

		while (!m_lock.compare_exchange_strong(tmp, state::lock)) {
			std::this_thread::yield();
			tmp = state::unlock;
		}
	}

	void unlock()
	{
		m_lock.store(state::unlock, std::memory_order_release);
	}
};

}  /* namespace starvation */

// starvation-free (at least on x86)
class ticket_lock {
	std::atomic<long> m_ticket;
	std::atomic<long> m_grant;

public:
	ticket_lock()
	    : m_ticket(0)
	    , m_grant(0)
	{}

	void lock()
	{
		long lticket = m_ticket.fetch_add(1);

		while (lticket != m_grant.load()) {
			std::this_thread::yield();
		}
	}

	void unlock()
	{
		long lgrant = m_grant.load(std::memory_order_relaxed);
		m_grant.store(lgrant + 1, std::memory_order_release);
	}
};
