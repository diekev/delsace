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

#pragma once

#include <functional>
#include <mutex>

namespace dls {

/**
 * Cette structure sert à cacher les mutex autour d'un objet via des fonctions
 * encapsulantes.
 */
template <typename T>
struct synchronise {
	std::mutex m_mutex{};
	T m_ptr{};

public:
	synchronise() = default;

	synchronise &operator=(T ptr)
	{
		m_mutex.lock();
		m_ptr = ptr;
		m_mutex.unlock();

		return *this;
	}

	void accede_ecriture(std::function<void(T&)> &&op)
	{
		m_mutex.lock();
		op(m_ptr);
		m_mutex.unlock();
	}

	void accede_lecture(std::function<void(T const&)> &&op)
	{
		m_mutex.lock();
		op(m_ptr);
		m_mutex.unlock();
	}
};

}  /* namespace dls */

namespace parallele {

/**
 * Cette classe sert à vérouiller avec un mutex les méthodes d'un objet à chaque
 * fois que celles-ci sont invoquées.
 *
 * La classe englobe un méchanisme de minuterie de portée qui est construit
 * avant d'appeler la méthode désirée et détruit juste après son exécution,
 * permettant de chronométrer son temps d'exécution.
 */
template <typename __type>
class synchronise {
	__type *m_pointeur = nullptr;
	std::mutex m_mutex;

public:
	/**
	 * Cette classe sert à vérouiller le mutex pendant sa durée de vie.
	 *
	 * Nous nous servons de l'opérateur '->' pour accéder au pointeur contenu
	 * dans la classe parente et la méthode ainsi appelée sera vérouillée.
	 */
	class portee_verouillee {
		synchronise *m_parent = nullptr;

	public:
		explicit portee_verouillee(synchronise *parent)
			: m_parent(parent)
		{
			if (m_parent != nullptr) {
				m_parent->m_mutex.lock();
			}
		}

		~portee_verouillee()
		{
			if (m_parent != nullptr) {
				m_parent->m_mutex.unlock();
			}
		}

		portee_verouillee(const portee_verouillee &rhs) = delete;
		portee_verouillee &operator=(const portee_verouillee &rhs) = delete;

		portee_verouillee(portee_verouillee &&rhs)
			: m_parent(rhs.m_parent)
		{
			rhs.m_parent = nullptr;
		}

		portee_verouillee &operator=(portee_verouillee &&rhs)
		{
			m_parent = rhs.m_parent;

			return *this;
		}

		__type *operator->()
		{
			return (m_parent != nullptr) ? m_parent->m_pointeur : nullptr;
		}
	};

	synchronise() = delete;

	explicit synchronise(__type *pointeur)
		: m_pointeur(pointeur)
	{}

	synchronise(const synchronise &rhs) = delete;
	synchronise &operator=(const synchronise &rhs) = delete;

	synchronise(synchronise &&rhs)
		: m_pointeur(rhs.m_pointeur)
	{
		rhs.m_pointeur = nullptr;
	}

	synchronise &operator=(synchronise &&rhs)
	{
		m_pointeur = rhs.m_pointeur;
		rhs.m_pointeur = nullptr;

		return *this;
	}

	/**
	 * Construit le chronomètre temporaire qui nous servira à chronométrer la
	 * méthode invoquée. Le pointeur contenu dans cette classe sera accédé dans
	 * l'objet temporaire créé.
	 */
	portee_verouillee operator->()
	{
		return portee_verouillee(this);
	}
};

}  /* parallele */
