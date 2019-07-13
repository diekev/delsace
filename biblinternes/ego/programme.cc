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

#include "programme.h"

#include <cassert>
#include <GL/glew.h>

#include "outils.h"

namespace dls {
namespace ego {
EGO_VERSION_NAMESPACE_BEGIN

Programme::~Programme()
{
	if (glIsProgram(m_programme)) {
		glDeleteProgram(m_programme);
	}

	m_attributs.efface();
	m_uniformes.efface();
}

Programme::Ptr Programme::cree_unique()
{
	return Ptr(new Programme());
}

Programme::SPtr Programme::cree_partage()
{
	return SPtr(new Programme());
}

void Programme::charge(Nuanceur type_nuanceur, dls::chaine const &source, std::ostream &os)
{
	unsigned int type;

	switch (type_nuanceur) {
		case Nuanceur::VERTEX:
			type = GL_VERTEX_SHADER;
			break;
		case Nuanceur::FRAGMENT:
			type = GL_FRAGMENT_SHADER;
			break;
		case Nuanceur::GEOMETRIE:
			type = GL_GEOMETRY_SHADER;
			break;
		case Nuanceur::CONTROLE_TESSELATION:
			type = GL_TESS_CONTROL_SHADER;
			break;
		case Nuanceur::EVALUATION_TESSELATION:
			type = GL_TESS_EVALUATION_SHADER;
			break;
		case Nuanceur::CALCUL:
			type = GL_COMPUTE_SHADER;
			break;
		default:
			assert(0);
			break;
	}

	unsigned int nuanceur = glCreateShader(type);
	const char *ptmp = source.c_str();

	glShaderSource(nuanceur, 1, &ptmp, nullptr);
	glCompileShader(nuanceur);

	const bool ok = util::check_status(nuanceur, GL_COMPILE_STATUS, "Compile log",
	                                   glGetShaderiv, glGetShaderInfoLog, os);

	if (!ok) {
		glDeleteShader(nuanceur);
		return;
	}

	m_nuanceurs[static_cast<int>(type_nuanceur)] = nuanceur;
}

void Programme::cree_et_lie_programme(std::ostream &os)
{
	m_programme = glCreateProgram();

	for (int i = 0; i < NOMBRE_NUANCEURS; ++i) {
		if (glIsShader(m_nuanceurs[i])) {
			glAttachShader(m_programme, m_nuanceurs[i]);
		}
	}

	glLinkProgram(m_programme);

	const bool ok = util::check_status(m_programme, GL_LINK_STATUS, "Linking log",
	                                   glGetProgramiv, glGetProgramInfoLog, os);

	/* Delete shaders now to free up some memory. */
	for (int i = 0; i < NOMBRE_NUANCEURS; ++i) {
		if (glIsShader(m_nuanceurs[i])) {
			glDetachShader(m_programme, m_nuanceurs[i]);
			glDeleteShader(m_nuanceurs[i]);
		}
	}

	if (!ok) {
		glDeleteProgram(m_programme);
	}
}

void Programme::active() const
{
	glUseProgram(m_programme);
}

void Programme::desactive() const
{
	glUseProgram(0);
}

bool Programme::est_valide(std::ostream &os) const
{
	if (glIsProgram(m_programme)) {
		glValidateProgram(m_programme);

		return util::check_status(m_programme, GL_VALIDATE_STATUS, "Validation log",
		                          glGetProgramiv, glGetProgramInfoLog, os);
	}

	return false;
}

void Programme::ajoute_attribut(dls::chaine const &attribut)
{
	m_attributs[attribut] = glGetAttribLocation(m_programme, attribut.c_str());
}

void Programme::ajoute_uniforme(dls::chaine const &uniform)
{
	m_uniformes[uniform] = glGetUniformLocation(m_programme, uniform.c_str());
}

int Programme::operator[](dls::chaine const &attribute)
{
	return m_attributs[attribute];
}

int Programme::operator()(dls::chaine const &uniform)
{
	return m_uniformes[uniform];
}

/* ************************** set up uniform values ************************* */

void Programme::uniforme(dls::chaine const &uniform, double const value)
{
	glUniform1d(m_uniformes[uniform], value);
}

void Programme::uniforme(dls::chaine const &uniform, double const v0, double const v1)
{
	glUniform2d(m_uniformes[uniform], v0, v1);
}

void Programme::uniforme(dls::chaine const &uniform, double const v0, double const v1, double const v2)
{
	glUniform3d(m_uniformes[uniform], v0, v1, v2);
}

void Programme::uniforme(dls::chaine const &uniform, double const v0, double const v1, double const v2, double const v4)
{
	glUniform4d(m_uniformes[uniform], v0, v1, v2, v4);
}

void Programme::uniforme(dls::chaine const &uniform, float const value)
{
	glUniform1f(m_uniformes[uniform], value);
}

void Programme::uniforme(dls::chaine const &uniform, float const v0, float const v1)
{
	glUniform2f(m_uniformes[uniform], v0, v1);
}

void Programme::uniforme(dls::chaine const &uniform, float const v0, float const v1, float const v2)
{
	glUniform3f(m_uniformes[uniform], v0, v1, v2);
}

void Programme::uniforme(dls::chaine const &uniform, float const v0, float const v1, float const v2, float const v4)
{
	glUniform4f(m_uniformes[uniform], v0, v1, v2, v4);
}

void Programme::uniforme(dls::chaine const &uniform, int const value)
{
	glUniform1i(m_uniformes[uniform], value);
}

void Programme::uniforme(dls::chaine const &uniform, int const v0, int const v1)
{
	glUniform2i(m_uniformes[uniform], v0, v1);
}

void Programme::uniforme(dls::chaine const &uniform, int const v0, int const v1, int const v2)
{
	glUniform3i(m_uniformes[uniform], v0, v1, v2);
}

void Programme::uniforme(dls::chaine const &uniform, int const v0, int const v1, int const v2, int const v4)
{
	glUniform4i(m_uniformes[uniform], v0, v1, v2, v4);
}

void Programme::uniforme(dls::chaine const &uniform, const unsigned int value)
{
	glUniform1ui(m_uniformes[uniform], value);
}

void Programme::uniforme(dls::chaine const &uniform, const unsigned int v0, const unsigned int v1)
{
	glUniform2ui(m_uniformes[uniform], v0, v1);
}

void Programme::uniforme(dls::chaine const &uniform, const unsigned int v0, const unsigned int v1, const unsigned int v2)
{
	glUniform3ui(m_uniformes[uniform], v0, v1, v2);
}

void Programme::uniforme(dls::chaine const &uniform, const unsigned int v0, const unsigned int v1, const unsigned int v2, const unsigned int v4)
{
	glUniform4ui(m_uniformes[uniform], v0, v1, v2, v4);
}

void Programme::uniforme(dls::chaine const &uniform, double const *value, int const n)
{
	glUniform1dv(m_uniformes[uniform], n, value);
}

void Programme::uniforme(dls::chaine const &uniform, float const *value, int const n)
{
	glUniform1fv(m_uniformes[uniform], n, value);
}

void Programme::uniforme(dls::chaine const &uniform, int const *value, int const n)
{
	glUniform1iv(m_uniformes[uniform], n, value);
}

void Programme::uniforme (dls::chaine const &uniform, const unsigned int *value, int const n)
{
	glUniform1uiv(m_uniformes[uniform], n, value);
}

EGO_VERSION_NAMESPACE_END
}  /* namespace ego */
}  /* namespace dls */
