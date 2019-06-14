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

#include <atomic>
#include <map>
#include <mutex>

void rcu_read_lock() {}
void rcu_read_unlock() {}
void synchronize_rcu() {}

template <typename Key, typename Value>
class COWLockMap{
	std::atomic<std::map<Key, Value> *> m_map_ref;
	std::mutex m_mutex;

	auto find(const Key &key) -> Value
	{
		rcu_read_lock();
		auto ret = (m_map_ref.load())->find(key);
		rcu_read_unlock();
		return ret;
	}

	auto insert(std::pair<Key, Value> val) -> std::pair<std::pair<Key, Value>, bool>
	{
		m_mutex.lock();

		std::map<Key, Value> *cur_map = m_map_ref.load(std::memory_order_relaxed);
		std::map<Key, Value> *new_map = new std::map<Key, Value>(*cur_map);

		auto ret = new_map->insert(val);
		m_map_ref.store(new_map);

		/* wait for older readers before deleting the old map */
		synchronize_rcu();

		m_mutex.unlock();

		cur_map->clear();
		delete cur_map;

		return ret;
	}
};
