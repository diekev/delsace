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

#include "xml.h" // pour TIXMLASSERT

namespace dls {
namespace xml {

/**
 * A dynamic array of Plain Old Data. Doesn't support constructors, etc.
 * Has a small initial memory pool, so that low or no usage will not cause a
 * call to new/delete.
 */
template <class T, unsigned int INITIAL_SIZE>
class DynArray {
	T*  _mem{};
	T   _pool[INITIAL_SIZE];

	int _allocated{};		// objects allocated
	int _size{};			// number objects in use

public:
	DynArray();

	~DynArray();

	void Clear();

	void Push(T t);

	T* PushArr(int count);

	T Pop();

	void PopArr(int count);

	bool Empty() const;

	T& operator[](int i);

	const T& operator[](int i) const;

	const T& PeekTop() const;

	int Size() const;

	int Capacity() const;

	const T* Mem() const;

	T* Mem();

private:
	DynArray(const DynArray&); // not supported
	void operator=(const DynArray&); // not supported

	void EnsureCapacity(int cap);
};

template<class T, unsigned int INITIAL_SIZE>
DynArray<T, INITIAL_SIZE>::DynArray()
{
	_mem = _pool;
	_allocated = INITIAL_SIZE;
	_size = 0;
}

template<class T, unsigned int INITIAL_SIZE>
DynArray<T, INITIAL_SIZE>::~DynArray()
{
	if (_mem != _pool) {
		delete [] _mem;
	}
}

template<class T, unsigned int INITIAL_SIZE>
void DynArray<T, INITIAL_SIZE>::Clear()
{
	_size = 0;
}

template<class T, unsigned int INITIAL_SIZE>
void DynArray<T, INITIAL_SIZE>::Push(T t)
{
	TIXMLASSERT(_size < INT_MAX);
	EnsureCapacity(_size+1);
	_mem[_size++] = t;
}

template<class T, unsigned int INITIAL_SIZE>
T *DynArray<T, INITIAL_SIZE>::PushArr(int count)
{
	TIXMLASSERT(count >= 0);
	TIXMLASSERT(_size <= INT_MAX - count);
	EnsureCapacity(_size+count);
	T* ret = &_mem[_size];
	_size += count;
	return ret;
}

template<class T, unsigned int INITIAL_SIZE>
T DynArray<T, INITIAL_SIZE>::Pop()
{
	TIXMLASSERT(_size > 0);
	return _mem[--_size];
}

template<class T, unsigned int INITIAL_SIZE>
void DynArray<T, INITIAL_SIZE>::PopArr(int count)
{
	TIXMLASSERT(_size >= count);
	_size -= count;
}

template<class T, unsigned int INITIAL_SIZE>
bool DynArray<T, INITIAL_SIZE>::Empty() const
{
	return _size == 0;
}

template<class T, unsigned int INITIAL_SIZE>
T &DynArray<T, INITIAL_SIZE>::operator[](int i)
{
	TIXMLASSERT(i>= 0 && i < _size);
	return _mem[i];
}

template<class T, unsigned int INITIAL_SIZE>
const T &DynArray<T, INITIAL_SIZE>::operator[](int i) const
{
	TIXMLASSERT(i>= 0 && i < _size);
	return _mem[i];
}

template<class T, unsigned int INITIAL_SIZE>
const T &DynArray<T, INITIAL_SIZE>::PeekTop() const
{
	TIXMLASSERT(_size > 0);
	return _mem[ _size - 1];
}

template<class T, unsigned int INITIAL_SIZE>
int DynArray<T, INITIAL_SIZE>::Size() const
{
	TIXMLASSERT(_size >= 0);
	return _size;
}

template<class T, unsigned int INITIAL_SIZE>
int DynArray<T, INITIAL_SIZE>::Capacity() const
{
	TIXMLASSERT(_allocated >= INITIAL_SIZE);
	return _allocated;
}

template<class T, unsigned int INITIAL_SIZE>
const T *DynArray<T, INITIAL_SIZE>::Mem() const
{
	TIXMLASSERT(_mem);
	return _mem;
}

template<class T, unsigned int INITIAL_SIZE>
T *DynArray<T, INITIAL_SIZE>::Mem()
{
	TIXMLASSERT(_mem);
	return _mem;
}

template<class T, unsigned int INITIAL_SIZE>
void DynArray<T, INITIAL_SIZE>::EnsureCapacity(int cap)
{
	TIXMLASSERT(cap > 0);

	if (cap > _allocated) {
		TIXMLASSERT(cap <= INT_MAX / 2);
		auto newAllocated = cap * 2;
		auto newMem = new T[newAllocated];
		memcpy(newMem, _mem, sizeof(T) * static_cast<unsigned>(_size));	// warning: not using constructors, only works for PODs

		if (_mem != _pool) {
			delete [] _mem;
		}

		_mem = newMem;
		_allocated = newAllocated;
	}
}

}  /* namespace xml */
}  /* namespace dls */
