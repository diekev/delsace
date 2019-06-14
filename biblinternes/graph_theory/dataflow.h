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

#include <cmath>

void test_planification();

/* from http://www.laputan.org/pub/patterns/manolescu/dfa.pdf */

template <typename DataType>
class PushInput;

template <typename DataType>
class PushPort {
	PushInput<DataType> *m_module;

public:
	PushPort(PushInput<DataType> *module)
	    : m_module(module)
	{}

	~PushPort() = default;

	inline void output(DataType &data)
	{
		m_module->put(data);
	}
};

template <typename DataType>
class PushInput {
public:
	virtual ~PushInput() = default;
	virtual void put(DataType &data) = 0;
};

template <typename DataType>
class PushOutput {
	PushPort<DataType> *m_port;

public:
	virtual ~PushOutput() = default;

	virtual void attach(PushInput<DataType> *next)
	{
		m_port = new PushPort<DataType>(next);
	}

	virtual void detach()
	{
		delete m_port;
	}

protected:
	inline void output(DataType &data)
	{
		m_port->output(data);
	}
};

class Sqrt : public PushInput<int>, public PushOutput<float> {
public:
	void put(int &i)
	{
		auto d = std::sqrt(static_cast<float>(i));
		output(d);
	}
};

class FastInc : public PushInput<int>, public PushOutput<int> {
public:
	void put(int &i)
	{
		int j = i;
		transform(j);
		output(j);
	}

	void transform(int &j)
	{
		++j;
	}
};

template <typename T>
class FastFilter : public PushInput<int>, public PushOutput<int> {
	T m_component;

public:
	FastFilter(T component)
	    : m_component(component)
	{}

	void put(int &i)
	{
		int j = i;
		m_component.transform(j);
		output(j);
	}
};
