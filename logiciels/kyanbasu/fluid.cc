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

#include "fluid.h"

#include <ego/utils.h>

Fluid::Fluid()
    : m_impulse_temperature(10.0f)
    , m_impulse_density(1.0f)
    , m_dissipation_rate(0.9999f)
    , m_jacobi_iterations(40)
    , m_grid_width(0)
    , m_grid_height(0)
    , m_temperature_ambient(0.0f)
    , m_cell_size(1.25f)
    , m_width(0)
    , m_height(0)
    , m_dt(0.125f)
    , m_gradient_scale(1.0f / m_cell_size)
    , m_splat_radius(0.0f)
    , m_smoke_density(1.0f)
    , m_smoke_weight(0.05f)
    , m_smoke_buoyancy(1.0f)
{}

void Fluid::init(int width, int height)
{
	m_width = width / 2;
	m_height = height / 2;
	m_splat_radius = m_width / 8.0f;

	int num_textures = 0;

	// Note: CreateSlab increments num_textures
	m_velocity = create_slab(m_width, m_height, 2, num_textures);
	m_density = create_slab(m_width, m_height, 1, num_textures);
	m_pressure = create_slab(m_width, m_height, 1, num_textures);
	m_temperature = create_slab(m_width, m_height, 1, num_textures);

	m_divergence = Surface(m_width, m_height, 3, num_textures++);
	initPrograms();

	m_obstacles = Surface(m_width, m_height, 3, num_textures++);
	create_obstacles(m_obstacles, m_width, m_height);

	int w = width * 2;
	int h = height * 2;

	m_obstacles_high_res = Surface(w, h, 1, num_textures++);
	create_obstacles(m_obstacles_high_res, w, h);

	//		QuadVao = CreateQuad();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	clear_surface(m_temperature.ping, m_temperature_ambient);

	m_impulse_pos = glm::vec2{ width / 2, -m_splat_radius / 2 };

	m_buffer = numero7::ego::BufferObject::create();
	m_buffer->bind();
	m_buffer->generateVertexBuffer(m_vertices, sizeof(float) * 8);
	m_buffer->generateIndexBuffer(&m_indices[0], sizeof(GLushort) * 6);
	m_buffer->unbind();
}

void Fluid::step(unsigned int)
{
	glViewport(0, 0, m_grid_width, m_grid_height);

	numero7::ego::util::GPU_check_errors("");

	m_buffer->bind();

	numero7::ego::util::GPU_check_errors("");

	advect(m_velocity.ping, m_velocity.ping, m_obstacles, m_velocity.pong);
	swap_surfaces(&m_velocity);

	numero7::ego::util::GPU_check_errors("");

	advect(m_velocity.ping, m_temperature.ping, m_obstacles, m_temperature.pong);
	swap_surfaces(&m_temperature);

	numero7::ego::util::GPU_check_errors("");

	advect(m_velocity.ping, m_density.ping, m_obstacles, m_density.pong);
	swap_surfaces(&m_density);

	numero7::ego::util::GPU_check_errors("");

	applyBuoyancy(m_velocity.ping, m_temperature.ping, m_density.ping, m_velocity.pong);
	swap_surfaces(&m_velocity);

	numero7::ego::util::GPU_check_errors("");

	applyImpulse(m_temperature.ping, m_impulse_pos, m_impulse_temperature);
	applyImpulse(m_density.ping, m_impulse_pos, m_impulse_density);

	numero7::ego::util::GPU_check_errors("");

	computedivergence(m_velocity.ping, m_obstacles, m_divergence);
	clear_surface(m_pressure.ping, 0.0f);

	numero7::ego::util::GPU_check_errors("");

	for (int i = 0; i < m_jacobi_iterations; ++i) {
		jacobi(m_pressure.ping, m_divergence, m_obstacles, m_pressure.pong);
		swap_surfaces(&m_pressure);
	}

	numero7::ego::util::GPU_check_errors("");

	subtractGradient(m_velocity.ping, m_pressure.ping, m_obstacles, m_velocity.pong);
	swap_surfaces(&m_velocity);

	numero7::ego::util::GPU_check_errors("");

	m_buffer->unbind();
}

void Fluid::initPrograms()
{
	numero7::ego::Program *p = &m_advect_program;
	p->load(numero7::ego::VERTEX_SHADER, numero7::ego::util::str_from_file("shaders/simple.vert"));
	p->load(numero7::ego::FRAGMENT_SHADER, numero7::ego::util::str_from_file("shaders/advect.frag"));
	p->createAndLinkProgram();
	p->enable();
	p->addAttribute("vertex");
	p->addUniform("inverse_size");
	p->addUniform("dt");
	p->addUniform("dissipation");
	p->addUniform("source");
	p->addUniform("obstacles");
	p->disable();

	p = &m_jacobi_program;
	p->load(numero7::ego::VERTEX_SHADER, numero7::ego::util::str_from_file("shaders/simple.vert"));
	p->load(numero7::ego::FRAGMENT_SHADER, numero7::ego::util::str_from_file("shaders/jacobi.frag"));
	p->createAndLinkProgram();
	p->enable();
	p->addAttribute("vertex");
	p->addUniform("alpha");
	p->addUniform("inverse_beta");
	p->addUniform("divergence");
	p->addUniform("obstacles");
	p->addUniform("pressure");
	p->disable();

	p = &m_gradient_program;
	p->load(numero7::ego::VERTEX_SHADER, numero7::ego::util::str_from_file("shaders/simple.vert"));
	p->load(numero7::ego::FRAGMENT_SHADER, numero7::ego::util::str_from_file("shaders/subtract_gradient.frag"));
	p->createAndLinkProgram();
	p->enable();
	p->addAttribute("vertex");
	p->addUniform("gradient_scale");
	p->addUniform("half_inverse_dh");
	p->addUniform("pressure");
	p->addUniform("obstacles");
	p->disable();

	p = &m_divergence_program;
	p->load(numero7::ego::VERTEX_SHADER, numero7::ego::util::str_from_file("shaders/simple.vert"));
	p->load(numero7::ego::FRAGMENT_SHADER, numero7::ego::util::str_from_file("shaders/divergence.frag"));
	p->createAndLinkProgram();
	p->enable();
	p->addAttribute("vertex");
	p->addUniform("half_inverse_dh");
	p->addUniform("obstacles");
	p->disable();

	p = &m_impulse_program;
	p->load(numero7::ego::VERTEX_SHADER, numero7::ego::util::str_from_file("shaders/simple.vert"));
	p->load(numero7::ego::FRAGMENT_SHADER, numero7::ego::util::str_from_file("shaders/splat.frag"));
	p->createAndLinkProgram();
	p->enable();
	p->addAttribute("vertex");
	p->addUniform("point");
	p->addUniform("radius");
	p->addUniform("fill_color");
	p->disable();

	p = &m_buoyancy_program;
	p->load(numero7::ego::VERTEX_SHADER, numero7::ego::util::str_from_file("shaders/simple.vert"));
	p->load(numero7::ego::FRAGMENT_SHADER, numero7::ego::util::str_from_file("shaders/buoyancy.frag"));
	p->createAndLinkProgram();
	p->enable();
	p->addAttribute("vertex");
	p->addUniform("temperature");
	p->addUniform("density");
	p->addUniform("temperature_ambient");
	p->addUniform("dt");
	p->addUniform("sigma");
	p->addUniform("kappa");
	p->disable();
}

void Fluid::jacobi(const Surface &pressure, const Surface &divergence, const Surface &obstacles, const Surface &dest)
{
	numero7::ego::Program *p = &m_jacobi_program;

	m_buffer->attribPointer((*p)["vertex"], 2);

	glEnable(GL_BLEND);

	if (p->isValid()) {
		p->enable();
		glUniform1f((*p)("alpha"), -m_cell_size * m_cell_size);
		glUniform1f((*p)("inverse_beta"), 0.25f);
		glUniform1i((*p)("divergence"), divergence.texture->number());
		glUniform1i((*p)("obstacles"), obstacles.texture->number());
		glUniform1i((*p)("pressure"), pressure.texture->number());

		dest.framebuffer->bind();
		pressure.texture->bind();
		divergence.texture->bind();
		obstacles.texture->bind();

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		pressure.texture->unbind();
		divergence.texture->unbind();
		obstacles.texture->unbind();
		dest.framebuffer->unbind();
		p->disable();
	}

	glDisable(GL_BLEND);
}

void Fluid::advect(const Surface &velocity, const Surface &source, const Surface &obstacles, const Surface &dest)
{
	numero7::ego::Program *p = &m_advect_program;

	m_buffer->attribPointer((*p)["vertex"], 2);

	glEnable(GL_BLEND);

	if (p->isValid()) {
		p->enable();
		glUniform2f((*p)("inverse_size"), 1.0f / m_width, 1.0f / m_height);
		glUniform1f((*p)("dt"), m_dt);
		glUniform1f((*p)("dissipation"), m_dissipation_rate);
		glUniform1i((*p)("source"), source.texture->number());
		glUniform1i((*p)("obstacles"), obstacles.texture->number());

		dest.framebuffer->bind();
		velocity.texture->bind();
		source.texture->bind();
		obstacles.texture->bind();

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		velocity.texture->unbind();
		source.texture->unbind();
		obstacles.texture->unbind();
		dest.framebuffer->unbind();
		p->disable();
	}

	glDisable(GL_BLEND);
}

void Fluid::applyImpulse(const Surface &dest, const glm::vec2 &position, float value)
{
	numero7::ego::Program *p = &m_impulse_program;

	m_buffer->attribPointer((*p)["vertex"], 2);

	if (p->isValid()) {

		glEnable(GL_BLEND);
		p->enable();
		glUniform2f((*p)("point"), (float) position.x, (float) position.x);
		glUniform1f((*p)("radius"), m_splat_radius);
		glUniform3f((*p)("fill_color"), value, value, value);

		dest.framebuffer->bind();
		glEnable(GL_BLEND);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		dest.framebuffer->unbind();
		p->disable();
		glDisable(GL_BLEND);
	}
}

void Fluid::subtractGradient(const Surface &velocity, const Surface &pressure, const Surface &obstacles, const Surface &dest)
{
	numero7::ego::Program *p = &m_gradient_program;

	m_buffer->attribPointer((*p)["vertex"], 2);

	glEnable(GL_BLEND);

	if (p->isValid()) {
		p->enable();

		glUniform1f((*p)("gradient_scale"), m_gradient_scale);
		glUniform1f((*p)("half_inverse_dh"), 0.5f / m_cell_size);
		glUniform1i((*p)("pressure"), pressure.texture->number());
		glUniform1i((*p)("obstacles"), obstacles.texture->number());

		dest.framebuffer->bind();
		velocity.texture->bind();
		pressure.texture->bind();
		obstacles.texture->bind();

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		dest.framebuffer->unbind();
		velocity.texture->unbind();
		pressure.texture->unbind();
		obstacles.texture->unbind();
		p->disable();
	}

	glDisable(GL_BLEND);
}

void Fluid::computedivergence(const Surface &velocity, const Surface &obstacles, const Surface &dest)
{
	numero7::ego::Program *p = &m_divergence_program;

	m_buffer->attribPointer((*p)["vertex"], 2);

	glEnable(GL_BLEND);

	if (p->isValid()) {
		p->enable();

		glUniform1f((*p)("half_inverse_dh"), 0.5f / m_cell_size);
		glUniform1i((*p)("obstacles"), obstacles.texture->number());

		dest.framebuffer->bind();
		velocity.texture->bind();
		obstacles.texture->bind();

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		dest.framebuffer->unbind();
		velocity.texture->unbind();
		obstacles.texture->unbind();
		p->disable();
	}

	glDisable(GL_BLEND);
}

void Fluid::applyBuoyancy(const Surface &velocity, const Surface &temperature, const Surface &density, const Surface &dest)
{
	numero7::ego::Program *p = &m_buoyancy_program;

	m_buffer->attribPointer((*p)["vertex"], 2);

	glEnable(GL_BLEND);

	if (p->isValid()) {
		p->enable();

		glUniform1i((*p)("temperature"), temperature.texture->number());
		glUniform1i((*p)("density"), density.texture->number());
		glUniform1f((*p)("temperature_ambient"), m_temperature_ambient);
		glUniform1f((*p)("dt"), m_dt);
		glUniform1f((*p)("sigma"), m_smoke_density);
		glUniform1f((*p)("kappa"), m_smoke_weight);

		dest.framebuffer->bind();
		velocity.texture->bind();
		temperature.texture->bind();
		density.texture->bind();

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		dest.framebuffer->unbind();
		velocity.texture->unbind();
		temperature.texture->unbind();
		density.texture->unbind();
		p->disable();
	}

	glDisable(GL_BLEND);
}

void Fluid::bindTexture(int field)
{
	switch (field) {
		case FLUID_FIELD_DENSITY:
			m_density.ping.texture->bind();
			break;
		case FLUID_FIELD_DIVERGENCE:
			m_divergence.texture->bind();
			break;
		case FLUID_FIELD_PRESSURE:
			m_pressure.ping.texture->bind();
			break;
		case FLUID_FIELD_VELOCITY:
			m_velocity.ping.texture->bind();
			break;
		case FLUID_FIELD_TEMPERATURE:
			m_temperature.ping.texture->bind();
			break;
		case FLUID_FIELD_OBSTACLE:
			m_obstacles.texture->bind();
			break;
	}
}

void Fluid::unbindTexture(int field)
{
	switch (field) {
		case FLUID_FIELD_DENSITY:
			m_density.ping.texture->unbind();
			break;
		case FLUID_FIELD_DIVERGENCE:
			m_divergence.texture->unbind();
			break;
		case FLUID_FIELD_PRESSURE:
			m_pressure.ping.texture->unbind();
			break;
		case FLUID_FIELD_VELOCITY:
			m_velocity.ping.texture->unbind();
			break;
		case FLUID_FIELD_TEMPERATURE:
			m_temperature.ping.texture->unbind();
			break;
		case FLUID_FIELD_OBSTACLE:
			m_obstacles.texture->unbind();
			break;
	}
}
