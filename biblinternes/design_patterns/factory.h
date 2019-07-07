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

#include "biblinternes/structures/dico_desordonne.hh"

template <typename T>
T *clone(const T * const instance)
{
	return new T(*instance);
}

template <typename Base, typename Key = std::string>
class FamilyFactory {
	typedef Base *(*CreateFunc)();

	dls::dico_desordonne<Key, CreateFunc> m_map{};

public:
	template <typename Derived>
	void registerType(const Key &key)
	{
		static_assert(std::is_base_of<Base, Derived>::value, "");

		const auto &iter = m_map.trouve(key);

		if (iter == m_map.fin()) {
			m_map[key] = &createImpl<Derived>;
		}
	}

	Base *operator()(const Key &key) const
	{
		const auto &iter = m_map.trouve(key);

		if (iter != m_map.fin()) {
			return (*iter).second();
		}

		return nullptr;
	}

private:
	template <typename Derived>
	static Base *createImpl()
	{
		return new Derived();
	}
};

/* Base must provide a clone() method */
template <typename Base, typename Key = std::string>
class CloneFactory {
	dls::dico_desordonne<Key, Base *> m_map{};

public:
	~CloneFactory()
	{
		for (auto &entry : m_map) {
			delete entry.second;
		}
	}

	template <typename Derived>
	void registerType(const Key &key, const Derived &)
	{
		const auto &iter = m_map.trouve(key);

		if (iter == m_map.fin()) {
			m_map[key] = new Derived();
		}
	}

	void registerInstance(const Key &key, Base *instance)
	{
		const auto &iter = m_map.trouve(key);

		if (iter == m_map.fin()) {
			m_map[key] = instance;
		}
	}

	void unregister(const Key &key)
	{
		const auto &iter = m_map.trouve(key);

		if (iter != m_map.fin()) {
			delete m_map[key];
			m_map.efface(iter);
		}
	}

	Base *operator()(const Key &key) const
	{
		const auto &iter = m_map.trouve(key);

		if (iter != m_map.fin()) {
			return (*iter).second->clone();
		}

		return nullptr;
	}
};
