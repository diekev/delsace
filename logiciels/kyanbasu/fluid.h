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

#include <ego/bufferobject.h>
#include <ego/program.h>
#include <glm/glm.hpp>

#include "slab.h"

enum {
	FLUID_FIELD_DENSITY     = 0,
	FLUID_FIELD_OBSTACLE    = 1,
	FLUID_FIELD_DIVERGENCE  = 2,
	FLUID_FIELD_VELOCITY    = 3,
	FLUID_FIELD_PRESSURE    = 4,
	FLUID_FIELD_TEMPERATURE = 5,
};

class Fluid {
	numero7::ego::BufferObject::Ptr m_buffer;

	const float m_vertices[8] = {
	    -1.0f, -1.0f,
	     1.0f, -1.0f,
	     1.0f,  1.0f,
	    -1.0f,  1.0f
	};

	const GLushort m_indices[6] = { 0, 1, 2, 0, 2, 3 };
public:
	Slab m_velocity;
	Slab m_density;
	Slab m_pressure;
	Slab m_temperature;

	Surface m_obstacles;
	Surface m_obstacles_high_res;
	Surface m_divergence;

	numero7::ego::Program m_program;
	numero7::ego::Program m_advect_program;
	numero7::ego::Program m_jacobi_program;
	numero7::ego::Program m_gradient_program;
	numero7::ego::Program m_divergence_program;
	numero7::ego::Program m_impulse_program;
	numero7::ego::Program m_buoyancy_program;

	float m_impulse_temperature, m_impulse_density;
	float m_dissipation_rate;
	int m_jacobi_iterations;
	int m_grid_width, m_grid_height;

	float m_temperature_ambient;
	float m_cell_size;
	int m_width, m_height;
	float m_dt;
	float m_gradient_scale;
	float m_splat_radius;
	float m_smoke_density;
	float m_smoke_weight;
	float m_smoke_buoyancy;

	glm::vec2 m_impulse_pos;

public:
	Fluid();
	~Fluid() = default;

	void init(int width, int height);

	void step(unsigned int /*elapsedMiliseconds*/);

	void initPrograms();

	void jacobi(const Surface &pressure,
	            const Surface &divergence,
	            const Surface &obstacles,
	            const Surface &dest);

	void advect(const Surface &velocity,
	            const Surface &source,
	            const Surface &obstacles,
	            const Surface &dest);

	void applyImpulse(const Surface &dest, const glm::vec2 &position, float value);

	void subtractGradient(const Surface &velocity,
	                      const Surface &pressure,
	                      const Surface &obstacles,
	                      const Surface &dest);

	void computedivergence(const Surface &velocity,
	                       const Surface &obstacles,
	                       const Surface &dest);

	void applyBuoyancy(const Surface &velocity,
	                   const Surface &temperature,
	                   const Surface &density,
	                   const Surface &dest);

	void bindTexture(int field);
	void unbindTexture(int field);
};
