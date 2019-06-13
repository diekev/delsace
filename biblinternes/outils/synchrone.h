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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <shared_mutex>

namespace dls {
namespace outils {

template <typename T, class Mutex = std::shared_mutex>
class Synchrone {
	T m_donnee;  /* CRUCIAL : non-mutable ! */
	mutable Mutex m_mutex;

public:
	class PointeurVerrouille {
		Synchrone *m_parent;

	public:
		PointeurVerrouille() = delete;

		explicit PointeurVerrouille(Synchrone *parent)
			: m_parent(parent)
		{
			if (m_parent) {
				m_parent->m_mutex.lock();
			}
		}

		PointeurVerrouille(const PointeurVerrouille &rhs)
			: m_parent(rhs.m_parent)
		{
			if (m_parent) {
				m_parent->m_mutex.lock();
			}
		}

		const PointeurVerrouille &operator=(const PointeurVerrouille &rhs)
		{
			if (this != &rhs) {
				if (m_parent) {
					m_parent->m_mutex.unlock();
				}

				m_parent = rhs.m_parent;

				if (m_parent) {
					m_parent->m_mutex.lock();
				}
			}

			return *this;
		}

		PointeurVerrouille(PointeurVerrouille &&rhs)
			: m_parent(rhs.m_parent)
		{
			rhs.m_parent = nullptr;
		}

		PointeurVerrouille &operator=(PointeurVerrouille &&rhs)
		{
			if (this != &rhs) {
				if (m_parent) {
					m_parent->m_mutex.unlock();
				}

				m_parent = rhs.m_parent;
				rhs.m_parent = nullptr;
			}

			return *this;
		}

		~PointeurVerrouille()
		{
			if (m_parent) {
				m_parent->m_mutex.unlock();
			}
		}

		T *operator->()
		{
			return m_parent ? &m_parent->m_donnee : nullptr;
		}
	};

	/* Cet opérateur va appelé PointeurVerrouille::operator->() de manière
	 * transitive. Quand cet opérateur sera créé, un objet de type
	 * PointeurVerrouille sera créé, puis son opérateur '->' sera appelé, enfin,
	 * quand tout sera fini, l'objet sera détruit. Ce méchanisme, ctor/dtor, est
	 * utilisé pour synchroniser le pointeur. */
	PointeurVerrouille operator->()
	{
		return PointeurVerrouille(this);
	}

	class ConstPointeurVerrouille {
		Synchrone *m_parent;

	public:
		ConstPointeurVerrouille() = delete;

		explicit ConstPointeurVerrouille(Synchrone *parent)
			: m_parent(parent)
		{
			if (m_parent) {
				m_parent->m_mutex.lock_shared();
			}
		}

		ConstPointeurVerrouille(const ConstPointeurVerrouille &rhs)
			: m_parent(rhs.m_parent)
		{
			if (m_parent) {
				m_parent->m_mutex.lock_shared();
			}
		}

		const ConstPointeurVerrouille &operator=(const ConstPointeurVerrouille &rhs)
		{
			if (this != &rhs) {
				if (m_parent) {
					m_parent->m_mutex.unlock_shared();
				}

				m_parent = rhs.m_parent;

				if (m_parent) {
					m_parent->m_mutex.lock_shared();
				}
			}

			return *this;
		}

		ConstPointeurVerrouille(ConstPointeurVerrouille &&rhs)
			: m_parent(rhs.m_parent)
		{
			rhs.m_parent = nullptr;
		}

		ConstPointeurVerrouille &operator=(ConstPointeurVerrouille &&rhs)
		{
			if (this != &rhs) {
				if (m_parent) {
					m_parent->m_mutex.unlock_shared();
				}

				m_parent = rhs.m_parent;
				rhs.m_parent = nullptr;
			}

			return *this;
		}

		~ConstPointeurVerrouille()
		{
			if (m_parent) {
				m_parent->m_mutex.unlock_shared();
			}
		}

		T *operator->()
		{
			return m_parent ? &m_parent->m_donnee : nullptr;
		}
	};

	/* Cet opérateur va appelé ConstPointeurVerrouille::operator->() de manière
	 * transitive. Quand cet opérateur sera créé, un objet de type
	 * ConstPointeurVerrouille sera créé, puis son opérateur '->' sera appelé,
	 * enfin, quand tout sera fini, l'objet sera détruit. Ce méchanisme,
	 * ctor/dtor, est utilisé pour synchroniser le pointeur. */
	ConstPointeurVerrouille operator->() const
	{
		return ConstPointeurVerrouille(this);
	}

	Synchrone() = default;

private:
	Synchrone(const Synchrone &rhs, const ConstPointeurVerrouille &)
		: m_donnee(rhs.m_donnee)
	{}

public:
	Synchrone(const Synchrone &rhs) noexcept(std::is_nothrow_copy_constructible<T>::value)
		: Synchrone(rhs, rhs.operator->())
	{}

	Synchrone(Synchrone &&rhs) noexcept(std::is_nothrow_move_constructible<T>::value)
		: m_donnee(std::move(rhs.m_donnee))
	{}

	explicit Synchrone(const T &rhs) noexcept(std::is_nothrow_copy_constructible<T>::value)
		: m_donnee(rhs)
	{}

	explicit Synchrone(T &&rhs) noexcept(std::is_nothrow_move_constructible<T>::value)
		: m_donnee(std::move(rhs))
	{}

	const Synchrone &operator=(const Synchrone &rhs) noexcept(std::is_nothrow_assignable<T>::value)
	{
		/* Fais en sorte que les verroux sont acquis dans le bon ordre. */
		if (std::less<void *>()(this, &rhs)) {
			auto g1 = operator->(), g2 = rhs.operator->();
			m_donnee = rhs.m_donnee;
		}
		else if (std::less<void *>()(&rhs, this)) {
			auto g1 = rhs.operator->(), g2 = operator->();
			m_donnee = rhs.m_donnee;
		}

		return *this;
	}

	const Synchrone &operator=(Synchrone &&rhs) noexcept(std::is_nothrow_move_assignable<T>::value)
	{
		/* Fais en sorte que les verroux sont acquis dans le bon ordre. */
		if (std::less<void *>()(this, &rhs)) {
			auto g1 = operator->(), g2 = rhs.operator->();
			m_donnee = std::move(rhs.m_donnee);
		}
		else if (std::less<void *>()(&rhs, this)) {
			auto g1 = rhs.operator->(), g2 = operator->();
			m_donnee = std::move(rhs.m_donnee);
		}

		return *this;
	}

	const Synchrone &operator=(const T &rhs) noexcept(std::is_nothrow_assignable<T>::value)
	{
		auto g1 = operator->();
		m_donnee = rhs;

		return *this;
	}

	const Synchrone &operator=(T &&rhs) noexcept(std::is_nothrow_move_assignable<T>::value)
	{
		auto g1 = operator->();
		m_donnee = std::move(rhs);

		return *this;
	}

	void swap(Synchrone &&rhs) noexcept
	{
		/* Fais en sorte que les verroux sont acquis dans le bon ordre. */
		if (std::less<void *>()(this, &rhs)) {
			auto g1 = operator->(), g2 = rhs.operator->();
			std::swap(m_donnee, rhs.m_donnee);
		}
		else if (std::less<void *>()(&rhs, this)) {
			auto g1 = rhs.operator->(), g2 = operator->();
			std::swap(m_donnee, rhs.m_donnee);
		}
	}

	const Synchrone &en_const() const
	{
		return *this;
	}
};

#define ARG_1(a, ...) a

#define ARG_2_OU_1_IMPL(a, b, ...) b

#define ARG_2_OU_1(...) \
	ARG_2_OU_1_IMPL(__VA_ARGS__, __VA_ARGS__)

#define SYNCHRONISE(...) \
	if (bool _1 = false) {} else \
		for (auto _2 = ARG_2_OU_1(__VA_ARGS__).operator->(); !_1; _1 = true) \
			for (auto &ARG_1(__VA_ARGS__) = *_2.operator->(); !_1; _1 = true)

#define SYNCHRONISE_CONST(...) \
	SYNCHRONISE(ARG_1(__VA_ARGS__), (ARG_2_OU_1(__VA_ARGS__)).en_const())

}  /* namespace outils */
}  /* namespace dls */
