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

#include "biblinternes/ego/outils.h"
#include "biblinternes/outils/fichier.hh"

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
	m_splat_radius = static_cast<float>(m_width) / 8.0f;

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

	m_impulse_pos = dls::math::vec2f{ static_cast<float>(width) / 2.0f, -m_splat_radius / 2.0f};

	m_buffer = dls::ego::TamponObjet::cree_unique();
	m_buffer->attache();
	m_buffer->genere_tampon_sommet(m_vertices, sizeof(float) * 8);
	m_buffer->genere_tampon_index(&m_indices[0], sizeof(GLushort) * 6);
	m_buffer->detache();
}

void Fluid::step(unsigned int)
{
	glViewport(0, 0, m_grid_width, m_grid_height);

	dls::ego::util::GPU_check_errors("");

	m_buffer->attache();

	dls::ego::util::GPU_check_errors("");

	advect(m_velocity.ping, m_velocity.ping, m_obstacles, m_velocity.pong);
	swap_surfaces(&m_velocity);

	dls::ego::util::GPU_check_errors("");

	advect(m_velocity.ping, m_temperature.ping, m_obstacles, m_temperature.pong);
	swap_surfaces(&m_temperature);

	dls::ego::util::GPU_check_errors("");

	advect(m_velocity.ping, m_density.ping, m_obstacles, m_density.pong);
	swap_surfaces(&m_density);

	dls::ego::util::GPU_check_errors("");

	applyBuoyancy(m_velocity.ping, m_temperature.ping, m_density.ping, m_velocity.pong);
	swap_surfaces(&m_velocity);

	dls::ego::util::GPU_check_errors("");

	applyImpulse(m_temperature.ping, m_impulse_pos, m_impulse_temperature);
	applyImpulse(m_density.ping, m_impulse_pos, m_impulse_density);

	dls::ego::util::GPU_check_errors("");

	computedivergence(m_velocity.ping, m_obstacles, m_divergence);
	clear_surface(m_pressure.ping, 0.0f);

	dls::ego::util::GPU_check_errors("");

	for (int i = 0; i < m_jacobi_iterations; ++i) {
		jacobi(m_pressure.ping, m_divergence, m_obstacles, m_pressure.pong);
		swap_surfaces(&m_pressure);
	}

	dls::ego::util::GPU_check_errors("");

	subtractGradient(m_velocity.ping, m_pressure.ping, m_obstacles, m_velocity.pong);
	swap_surfaces(&m_velocity);

	dls::ego::util::GPU_check_errors("");

	m_buffer->detache();
}

void Fluid::initPrograms()
{
	auto p = &m_advect_program;
	p->charge(dls::ego::Nuanceur::VERTEX, dls::contenu_fichier("shaders/simple.vert"));
	p->charge(dls::ego::Nuanceur::FRAGMENT, dls::contenu_fichier("shaders/advect.frag"));
	p->cree_et_lie_programme();
	p->active();
	p->ajoute_attribut("vertex");
	p->ajoute_uniforme("inverse_size");
	p->ajoute_uniforme("dt");
	p->ajoute_uniforme("dissipation");
	p->ajoute_uniforme("source");
	p->ajoute_uniforme("obstacles");
	p->desactive();

	p = &m_jacobi_program;
	p->charge(dls::ego::Nuanceur::VERTEX, dls::contenu_fichier("shaders/simple.vert"));
	p->charge(dls::ego::Nuanceur::FRAGMENT, dls::contenu_fichier("shaders/jacobi.frag"));
	p->cree_et_lie_programme();
	p->active();
	p->ajoute_attribut("vertex");
	p->ajoute_uniforme("alpha");
	p->ajoute_uniforme("inverse_beta");
	p->ajoute_uniforme("divergence");
	p->ajoute_uniforme("obstacles");
	p->ajoute_uniforme("pressure");
	p->desactive();

	p = &m_gradient_program;
	p->charge(dls::ego::Nuanceur::VERTEX, dls::contenu_fichier("shaders/simple.vert"));
	p->charge(dls::ego::Nuanceur::FRAGMENT, dls::contenu_fichier("shaders/subtract_gradient.frag"));
	p->cree_et_lie_programme();
	p->active();
	p->ajoute_attribut("vertex");
	p->ajoute_uniforme("gradient_scale");
	p->ajoute_uniforme("half_inverse_dh");
	p->ajoute_uniforme("pressure");
	p->ajoute_uniforme("obstacles");
	p->desactive();

	p = &m_divergence_program;
	p->charge(dls::ego::Nuanceur::VERTEX, dls::contenu_fichier("shaders/simple.vert"));
	p->charge(dls::ego::Nuanceur::FRAGMENT, dls::contenu_fichier("shaders/divergence.frag"));
	p->cree_et_lie_programme();
	p->active();
	p->ajoute_attribut("vertex");
	p->ajoute_uniforme("half_inverse_dh");
	p->ajoute_uniforme("obstacles");
	p->desactive();

	p = &m_impulse_program;
	p->charge(dls::ego::Nuanceur::VERTEX, dls::contenu_fichier("shaders/simple.vert"));
	p->charge(dls::ego::Nuanceur::FRAGMENT, dls::contenu_fichier("shaders/splat.frag"));
	p->cree_et_lie_programme();
	p->active();
	p->ajoute_attribut("vertex");
	p->ajoute_uniforme("point");
	p->ajoute_uniforme("radius");
	p->ajoute_uniforme("fill_color");
	p->desactive();

	p = &m_buoyancy_program;
	p->charge(dls::ego::Nuanceur::VERTEX, dls::contenu_fichier("shaders/simple.vert"));
	p->charge(dls::ego::Nuanceur::FRAGMENT, dls::contenu_fichier("shaders/buoyancy.frag"));
	p->cree_et_lie_programme();
	p->active();
	p->ajoute_attribut("vertex");
	p->ajoute_uniforme("temperature");
	p->ajoute_uniforme("density");
	p->ajoute_uniforme("temperature_ambient");
	p->ajoute_uniforme("dt");
	p->ajoute_uniforme("sigma");
	p->ajoute_uniforme("kappa");
	p->desactive();
}

void Fluid::jacobi(const Surface &pressure, const Surface &divergence, const Surface &obstacles, const Surface &dest)
{
	auto p = &m_jacobi_program;

	m_buffer->pointeur_attribut(static_cast<unsigned>((*p)["vertex"]), 2);

	glEnable(GL_BLEND);

	if (p->est_valide()) {
		p->active();
		glUniform1f((*p)("alpha"), -m_cell_size * m_cell_size);
		glUniform1f((*p)("inverse_beta"), 0.25f);
		glUniform1i((*p)("divergence"), static_cast<int>(divergence.texture->code_attache()));
		glUniform1i((*p)("obstacles"), static_cast<int>(obstacles.texture->code_attache()));
		glUniform1i((*p)("pressure"), static_cast<int>(pressure.texture->code_attache()));

		dest.framebuffer->attache();
		pressure.texture->attache();
		divergence.texture->attache();
		obstacles.texture->attache();

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		pressure.texture->detache();
		divergence.texture->detache();
		obstacles.texture->detache();
		dest.framebuffer->detache();
		p->desactive();
	}

	glDisable(GL_BLEND);
}

void Fluid::advect(const Surface &velocity, const Surface &source, const Surface &obstacles, const Surface &dest)
{
	auto p = &m_advect_program;

	m_buffer->pointeur_attribut(static_cast<unsigned>((*p)["vertex"]), 2);

	glEnable(GL_BLEND);

	if (p->est_valide()) {
		p->active();
		glUniform2f((*p)("inverse_size"), 1.0f / static_cast<float>(m_width), 1.0f / static_cast<float>(m_height));
		glUniform1f((*p)("dt"), m_dt);
		glUniform1f((*p)("dissipation"), m_dissipation_rate);
		glUniform1i((*p)("source"), static_cast<int>(source.texture->code_attache()));
		glUniform1i((*p)("obstacles"), static_cast<int>(obstacles.texture->code_attache()));

		dest.framebuffer->attache();
		velocity.texture->attache();
		source.texture->attache();
		obstacles.texture->attache();

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		velocity.texture->detache();
		source.texture->detache();
		obstacles.texture->detache();
		dest.framebuffer->detache();
		p->desactive();
	}

	glDisable(GL_BLEND);
}

void Fluid::applyImpulse(const Surface &dest, const dls::math::vec2f &position, float value)
{
	auto p = &m_impulse_program;

	m_buffer->pointeur_attribut(static_cast<unsigned>((*p)["vertex"]), 2);

	if (p->est_valide()) {

		glEnable(GL_BLEND);
		p->active();
		glUniform2f((*p)("point"), position.x, position.y);
		glUniform1f((*p)("radius"), m_splat_radius);
		glUniform3f((*p)("fill_color"), value, value, value);

		dest.framebuffer->attache();
		glEnable(GL_BLEND);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		dest.framebuffer->detache();
		p->desactive();
		glDisable(GL_BLEND);
	}
}

void Fluid::subtractGradient(const Surface &velocity, const Surface &pressure, const Surface &obstacles, const Surface &dest)
{
	auto p = &m_gradient_program;

	m_buffer->pointeur_attribut(static_cast<unsigned>((*p)["vertex"]), 2);

	glEnable(GL_BLEND);

	if (p->est_valide()) {
		p->active();

		glUniform1f((*p)("gradient_scale"), m_gradient_scale);
		glUniform1f((*p)("half_inverse_dh"), 0.5f / m_cell_size);
		glUniform1i((*p)("pressure"), static_cast<int>(pressure.texture->code_attache()));
		glUniform1i((*p)("obstacles"), static_cast<int>(obstacles.texture->code_attache()));

		dest.framebuffer->attache();
		velocity.texture->attache();
		pressure.texture->attache();
		obstacles.texture->attache();

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		dest.framebuffer->detache();
		velocity.texture->detache();
		pressure.texture->detache();
		obstacles.texture->detache();
		p->desactive();
	}

	glDisable(GL_BLEND);
}

void Fluid::computedivergence(const Surface &velocity, const Surface &obstacles, const Surface &dest)
{
	auto p = &m_divergence_program;

	m_buffer->pointeur_attribut(static_cast<unsigned>((*p)["vertex"]), 2);

	glEnable(GL_BLEND);

	if (p->est_valide()) {
		p->active();

		glUniform1f((*p)("half_inverse_dh"), 0.5f / m_cell_size);
		glUniform1i((*p)("obstacles"), static_cast<int>(obstacles.texture->code_attache()));

		dest.framebuffer->attache();
		velocity.texture->attache();
		obstacles.texture->attache();

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		dest.framebuffer->detache();
		velocity.texture->detache();
		obstacles.texture->detache();
		p->desactive();
	}

	glDisable(GL_BLEND);
}

void Fluid::applyBuoyancy(const Surface &velocity, const Surface &temperature, const Surface &density, const Surface &dest)
{
	auto p = &m_buoyancy_program;

	m_buffer->pointeur_attribut(static_cast<unsigned>((*p)["vertex"]), 2);

	glEnable(GL_BLEND);

	if (p->est_valide()) {
		p->active();

		glUniform1i((*p)("temperature"), static_cast<int>(temperature.texture->code_attache()));
		glUniform1i((*p)("density"), static_cast<int>(density.texture->code_attache()));
		glUniform1f((*p)("temperature_ambient"), m_temperature_ambient);
		glUniform1f((*p)("dt"), m_dt);
		glUniform1f((*p)("sigma"), m_smoke_density);
		glUniform1f((*p)("kappa"), m_smoke_weight);

		dest.framebuffer->attache();
		velocity.texture->attache();
		temperature.texture->attache();
		density.texture->attache();

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		dest.framebuffer->detache();
		velocity.texture->detache();
		temperature.texture->detache();
		density.texture->detache();
		p->desactive();
	}

	glDisable(GL_BLEND);
}

void Fluid::bindTexture(int field)
{
	switch (field) {
		case FLUID_FIELD_DENSITY:
			m_density.ping.texture->attache();
			break;
		case FLUID_FIELD_DIVERGENCE:
			m_divergence.texture->attache();
			break;
		case FLUID_FIELD_PRESSURE:
			m_pressure.ping.texture->attache();
			break;
		case FLUID_FIELD_VELOCITY:
			m_velocity.ping.texture->attache();
			break;
		case FLUID_FIELD_TEMPERATURE:
			m_temperature.ping.texture->attache();
			break;
		case FLUID_FIELD_OBSTACLE:
			m_obstacles.texture->attache();
			break;
	}
}

void Fluid::unbindTexture(int field)
{
	switch (field) {
		case FLUID_FIELD_DENSITY:
			m_density.ping.texture->detache();
			break;
		case FLUID_FIELD_DIVERGENCE:
			m_divergence.texture->detache();
			break;
		case FLUID_FIELD_PRESSURE:
			m_pressure.ping.texture->detache();
			break;
		case FLUID_FIELD_VELOCITY:
			m_velocity.ping.texture->detache();
			break;
		case FLUID_FIELD_TEMPERATURE:
			m_temperature.ping.texture->detache();
			break;
		case FLUID_FIELD_OBSTACLE:
			m_obstacles.texture->detache();
			break;
	}
}
