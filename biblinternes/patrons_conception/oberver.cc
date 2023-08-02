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

#include "oberver.h"

#include <algorithm>
#include <iostream>

void Observer::update(const Observable *observable) const
{
	std::cout << observable->status() << '\n';
}

void Observer::add(Observable *observable)
{
	m_observables.ajoute(observable);
}

void Observer::remove(Observable *observable)
{
	auto iter = std::find(m_observables.debut(), m_observables.fin(), observable);

	if (iter != m_observables.fin()) {
		m_observables.erase(iter);
	}
}

Observable::~Observable()
{
	for (auto &observer : m_observers) {
		observer->remove(this);
	}
}

void Observable::add(Observer *observer)
{
	m_observers.ajoute(observer);
	observer->add(this);
}

void Observable::remove(Observer *observer)
{
	auto iter = std::find(m_observers.debut(), m_observers.fin(), observer);

	if (iter != m_observers.fin()) {
		m_observers.erase(iter);
	}
}

void Observable::notify()
{
	for (auto &observer : m_observers) {
		observer->update(this);
	}
}

void Barometre::change(int valeur)
{
	pression = valeur;
	notify();
}

int Barometre::status() const
{
	return pression;
}

void Thermometre::change(int valeur)
{
	temperature = valeur;
	notify();
}

int Thermometre::status() const
{
	return temperature;
}
