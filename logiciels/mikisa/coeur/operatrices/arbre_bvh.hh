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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/structures/tableau.hh"

struct Triangle;

struct ArbreBVH {
	float epsilon = 0.0f;
	unsigned char tree_type = 0;
	unsigned char axis = 0;
	char start_axis = 0;
	char stop_axis = 0;
	int totbranch = 0;
	int totleaf = 0;

	struct Noeud {
		float *bv = nullptr;
		Noeud **children = nullptr;
		Noeud *parent = nullptr;
		int index = 0;
		int main_axis = 0;
		int totnode = 0;
	};

	dls::tableau<ArbreBVH::Noeud *> nodes{};
	dls::tableau<float> nodebv{};
	dls::tableau<ArbreBVH::Noeud *> nodechild{};
	dls::tableau<ArbreBVH::Noeud> nodearray{};

	void insert_triangle(int index, Triangle const &tri);

	void balance();
};

ArbreBVH *nouvelle_arbre_bvh(int maxsize, float epsilon, unsigned char tree_type, unsigned char axis);
