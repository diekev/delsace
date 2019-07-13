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

#if 0

#include <openvdb/openvdb.h>

using ScalarGrid = openvdb::FloatGrid;
using VectorGrid = openvdb::Vec3SGrid;

#define MAX_STEPS 10

template<typename T>
T lerp(T a, T b, T dt)
{
	return a * (1.0f - dt) + b * dt;
}

class Retimer {
	int m_num_steps;
	float m_time_scale;
	float m_shutter_speed;
	bool m_threaded;
	openvdb::GridPtrVec m_grids;
	dls::tableau<dls::chaine> m_grid_names;

	void readvectSL(ScalarGrid::Ptr work,
					ScalarGrid::Ptr gridA,
					ScalarGrid::Ptr gridB,
					VectorGrid::Ptr velA,
					VectorGrid::Ptr velB);

public:
	Retimer();
	Retimer(int steps, float time_scale, float shutter_speed);
	~Retimer();

	void setTimeScale(const float scale);
	void setGridNames(std::initializer_list<dls::chaine> list);
	void retime(const dls::chaine &previous, const dls::chaine &cur, const dls::chaine &to);
	void setThreaded(const bool threaded);
};

#endif
