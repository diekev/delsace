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

#include <functional>
#include "biblinternes/structures/tableau.hh"

#include "biblinternes/structures/dico.hh"

/* from https://juanchopanzacpp.wordpress.com/2013/02/24/simple-observer-pattern-implementation-c11/ */

enum class event : int {
	red,
	green,
	blue,
	orange,
	magenta,
	cyan,
};

template <typename EventType>
class Subject {
	dls::dico<event, dls::tableau<std::function<void()>>> m_observers{};

public:
	template <typename ObserverType>
	void add(const EventType &e, ObserverType &&observer)
	{
		(m_observers[e]).pousse(std::forward<ObserverType>(observer));
	}

	template <typename ObserverType>
	void add(EventType &&e, ObserverType &&observer)
	{
		(m_observers[std::move(e)]).pousse(std::forward<ObserverType>(observer));
	}

	void notify(const EventType &e) const
	{
		for (const auto &observer : m_observers.a(e)) {
			observer();
		}
	}
};

/* ****************************** */

class Observable;

class Observer {
protected:
	dls::tableau<Observable *> m_observables{};

	virtual ~Observer() = default;

public:
	virtual void update(const Observable *observable) const;

	void add(Observable *observable);
	void remove(Observable *observable);
};

class Observable {
	dls::tableau<Observer *> m_observers{};

public:
	virtual ~Observable();

	void add(Observer *observer);
	void remove(Observer *observer);

	virtual int status() const = 0;

protected:
	void notify();
};

class Barometre : public Observable {
    int pression = 0;

public:
    void change(int valeur);

    int status() const override;
};

class Thermometre : public Observable {
    int temperature = 0;

public:
	void change(int valeur);

    int status() const override;
};

class MeteoFrance : public Observer {

};
