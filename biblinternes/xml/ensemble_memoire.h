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

#include "dyn_array.h"

namespace dls {
namespace xml {

/**
 * Parent virtual class of a pool for fast allocation and deallocation of objects.
 */
class MemPool {
public:
	MemPool() {}
	virtual ~MemPool() {}

	virtual int ItemSize() const = 0;
	virtual void* Alloc() = 0;
	virtual void Free(void*) = 0;
	virtual void SetTracked() = 0;
	virtual void Clear() = 0;
};

/**
 * Template child class to create pools of the correct type.
 */
template <unsigned int SIZE>
class MemPoolT final : public MemPool {
	union Chunk {
		Chunk*  next{};
		char    mem[SIZE];
	};

	// This number is perf sensitive. 4k seems like a good tradeoff on my machine.
	// The test file is large, 170k.
	// Release:		VS2010 gcc(no opt)
	//		1k:		4000
	//		2k:		4000
	//		4k:		3900	21000
	//		16k:	5200
	//		32k:	4300
	//		64k:	4000	21000
	enum {
		COUNT = (4 * 1024) / SIZE
	}; // Some compilers do not accept to use COUNT in private part if COUNT is private

	struct Block {
		Chunk chunk[COUNT];
	};

	DynArray<Block *, 10> m_pointeurs_bloque{};
	Chunk* m_racine = nullptr;

	int m_allocations_courantes = 0;
	int m_nombre_allocations = 0;
	int m_allocations_max = 0;
	int m_nombre_non_suivis = 0;

public:
	MemPoolT() = default;

	MemPoolT(const MemPoolT&) = delete;

	~MemPoolT();

	MemPoolT &operator=(const MemPoolT&) = delete;

	void Clear();

	virtual int ItemSize() const;

	int CurrentAllocs() const;

	virtual void* Alloc();

	virtual void Free(void* mem);

	void Trace(const char* name);

	void SetTracked();

	int Untracked() const;
};

template <unsigned int SIZE>
MemPoolT<SIZE>::~MemPoolT()
{
	Clear();
}

template <unsigned int SIZE>
void MemPoolT<SIZE>::Clear() {
	// Delete the blocks.
	while(!m_pointeurs_bloque.Empty()) {
		Block* b  = m_pointeurs_bloque.Pop();
		delete b;
	}
	m_racine = 0;
	m_allocations_courantes = 0;
	m_nombre_allocations = 0;
	m_allocations_max = 0;
	m_nombre_non_suivis = 0;
}

template <unsigned int SIZE>
int MemPoolT<SIZE>::ItemSize() const
{
	return SIZE;
}

template <unsigned int SIZE>
int MemPoolT<SIZE>::CurrentAllocs() const
{
	return m_allocations_courantes;
}

template <unsigned int SIZE>
void *MemPoolT<SIZE>::Alloc()
{
	if (!m_racine) {
		// Need a new block.
		Block* block = new Block();
		m_pointeurs_bloque.Push(block);

		for(int i=0; i<COUNT-1; ++i) {
			block->chunk[i].next = &block->chunk[i+1];
		}
		block->chunk[COUNT-1].next = 0;
		m_racine = block->chunk;
	}

	void* result = m_racine;
	m_racine = m_racine->next;

	++m_allocations_courantes;

	if (m_allocations_courantes > m_allocations_max) {
		m_allocations_max = m_allocations_courantes;
	}

	m_nombre_allocations++;
	m_nombre_non_suivis++;

	return result;
}

template <unsigned int SIZE>
void MemPoolT<SIZE>::Free(void *mem)
{
	if (!mem) {
		return;
	}

	--m_allocations_courantes;
	Chunk* chunk = static_cast<Chunk*>(mem);
#ifdef DEBUG
	memset(chunk, 0xfe, sizeof(Chunk));
#endif
	chunk->next = m_racine;
	m_racine = chunk;
}

template <unsigned int SIZE>
void MemPoolT<SIZE>::Trace(const char *name)
{
	printf("Mempool %s watermark=%d [%dk] current=%d size=%d nAlloc=%d blocks=%d\n",
		   name, m_allocations_max, m_allocations_max*SIZE/1024, m_allocations_courantes, SIZE, m_nombre_allocations, m_pointeurs_bloque.Size());
}

template <unsigned int SIZE>
void MemPoolT<SIZE>::SetTracked()
{
	m_nombre_non_suivis--;
}

template <unsigned int SIZE>
int MemPoolT<SIZE>::Untracked() const
{
	return m_nombre_non_suivis;
}

}  /* namespace xml */
}  /* namespace dls */
