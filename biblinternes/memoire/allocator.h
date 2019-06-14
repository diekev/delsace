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

/* allocators */
#include "allocators/freelist.h"
#include "allocators/mallocator.h"
#include "allocators/stack_allocator.h"

/* composers */
#include "composers/fallback_allocator.h"
#include "composers/segregator.h"

#include "guard.h"

#include <iostream>

/* required API for the allocators:
 * struct Allocator {
 *     Blk allocate(size_t n);
 *     Blk allocateAll();
 *
 *     bool expand(Blk&, size_t delta);
 *     void reallocate(Blk&, size_t delta);
 *
 *     bool owns(Blk b);
 *
 *     void deallocate(Blk);
 *     void deallocateAll();
 *
 *     Blk alignedAllocate(size_t, unsigned);
 *     Blk alignedReallocate(size_t, unsigned);
 * };
 *
 * optional:
 * static constexpr unsigned alignment;
 * static constexpr size_t goodSize(size_t); // round to the alignment
 */

//using LocalAllocator = FreeList<Mallocator, 0, 100>;
using LocalAllocator = memori::FallBackAllocator<memori::StackAllocator<16384>,
                                                 memori::Mallocator>;
//using LocalAllocator = Segregator<16384, StackAllocator<16384>, Mallocator>;

//using Allocator = Segregator<4096, Segregator<128, FreeList<Mallocator, 0, 128>, MediumAllocator>, Mallocator>;

// example

//using FList = FreeList<Mallocator, 0, -1>;
//using A = Segregator<
//	8, FreeList<Mallocator, 0, 8>,
//	128, Bucketizer<FList, 1, 128, 16>,
//	256, Bucketizer<FList, 129, 256, 32>,
//	512, Bucketizer<FList, 257, 512, 64>,
//	1024, Bucketizer<FList, 513, 1024, 128>,
//	2048, Bucketizer<FList, 1025, 2048, 256>,
//	3584, Bucketizer<FList, 2049, 2584, 512>,
//	4072 * 1024, CascadingAllocator<decltype(newHeapBloxk)>, Mallocator>;

using AllocateurGarde = memori::Garde<memori::AllocateurBlender>;

AllocateurGarde GA;

#ifndef NDEBUG

struct InformationAllocation {
	InformationAllocation *suivant;
	const char *fichier;
	int line;
	int count;
};

static InformationAllocation g_info_alloc{ nullptr, 0, 0, 0 };

#define MEM_allocate_ex__(size, type, name)                                    \
	static_cast<type *>(GA.allocate(size * sizeof(type)));                     \
	static InformationAllocation alloc_info##name{ nullptr, __FILE__, __LINE__, 0 }; \
	if (++alloc_info##name.count == 1) {                                       \
		alloc_info##name.suivant = g_info_alloc.suivant;                             \
		g_info_alloc.suivant = &alloc_info##name;                                 \
	}                                                                          \
	else {}

#define MEM_allocate_ex(size, type, name) MEM_allocate_ex__(size, type, name);

#define MEM_allocate(size, type) MEM_allocate_ex(size, type, __LINE__);

void note_infos_allocs()
{
	InformationAllocation *info = g_info_alloc.suivant;
	std::ostream &os = std::cerr;

	while (info != nullptr) {
		os << info->count << " allocation(s) dans le fichier: ";
		os << info->fichier << ":" << info->line << "\n";
		info = info->suivant;
	}
}

#else

#define MEM_allocate(size, type) \
	static_cast<type *>(GA.allocate(size * sizeof(type)));

void note_infos_allocs() {}

#endif

#define MEM_free(ptr, size) GA.deallocate(ptr, size)

void *operator new(std::size_t sz, const char *id)
{
	std::cout << id << '\n';
	return GA.allocate(sz);
}

void *operator new[](std::size_t sz, const char *id)
{
	std::cout << id << '\n';
	return GA.allocate(sz);
}

#define new new(__func__)

void operator delete(void *ptr)
{
	return GA.deallocate(ptr, 0);
}

void operator delete(void *ptr, std::size_t sz)
{
	return GA.deallocate(ptr, sz);
}

void operator delete[](void *ptr)
{
	return GA.deallocate(ptr, 0);
}

void operator delete[](void *ptr, std::size_t sz)
{
	return GA.deallocate(ptr, sz);
}
