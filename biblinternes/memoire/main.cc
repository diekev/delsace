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

#include "allocator.h"

int main()
{
	std::atexit(note_infos_allocs);

#if 0
	LocalAllocator a;
	size_t n = 250;
	auto b1 = MEM_allocate(n * sizeof(int), b1);
	auto b2 = MEM_allocate(n * sizeof(int), b2);

	int *p = static_cast<int *>(b1.ptr);

	for (auto i = 0ul, ie = n; i < ie; ++i) {
		p[i] = i;
	}

	for (auto i = 0ul, ie = n; i < ie; ++i) {
		std::cout << p[i] << "\n";
	}

	a.deallocate(b1);
	a.deallocate(b2);
#endif

	size_t n = 250;
//	int *p1 = MEM_allocate(n, int);
//	int *p2 = MEM_allocate(n, int);
	int *p1 = new int[n];
	int *p2 = new int[n];

	for (auto i = 0ul, ie = n; i < ie; ++i) {
		p1[i] = static_cast<int>(i);
	}

	for (auto i = 0ul, ie = n; i < ie; ++i) {
		std::cout << p1[i] << '\n';
	}

//	MEM_free(p1, n * sizeof(int));
//	MEM_free(p2, n * sizeof(int));

	delete [] (p1);
	delete [] (p2);
}
