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

#include "tampon_objet.h"

#include <GL/glew.h>

#include "outils.h"

namespace dls {
namespace ego {
EGO_VERSION_NAMESPACE_BEGIN

static void genere_tampon(
		unsigned int &id,
		void const *data,
		long const size,
		unsigned int const target) noexcept(false)
{
	if (glIsBuffer(id)) {
		glDeleteBuffers(1, &id);
	}

	auto draw_type = static_cast<GLenum>(GL_STATIC_DRAW);

	// À FAIRE: feels a bit hackish
	if (data == nullptr) {
		draw_type = GL_DYNAMIC_DRAW;
	}

	glGenBuffers(1, &id);

	glBindBuffer(target, id);
	glBufferData(target, size, data, draw_type);

	if (util::check_bad_alloc()) {
		throw "Unable to allocate memory for buffer!";
	}
}

TamponObjet::TamponObjet() noexcept
{
	glGenVertexArrays(1, &m_objet_tableau_sommet);
}

TamponObjet::TamponObjet(TamponObjet &&buffer) noexcept
	: TamponObjet()
{
	*this = std::move(buffer);
}

TamponObjet::~TamponObjet() noexcept
{
	if (glIsBuffer(m_tampon_sommet)) {
		glDeleteBuffers(1, &m_tampon_sommet);
	}

	if (glIsBuffer(m_tampon_index)) {
		glDeleteBuffers(1, &m_tampon_index);
	}

	if (glIsBuffer(m_tampon_normal)) {
		glDeleteBuffers(1, &m_tampon_normal);
	}

	for (auto buffer : m_tampons_extra) {
		if (glIsBuffer(buffer)) {
			glDeleteBuffers(1, &buffer);
		}
	}

	if (glIsVertexArray(m_objet_tableau_sommet)) {
		glDeleteVertexArrays(1, &m_objet_tableau_sommet);
	}
}

TamponObjet &TamponObjet::operator=(TamponObjet &&buffer) noexcept
{
	using std::swap;
	swap(m_objet_tableau_sommet, buffer.m_objet_tableau_sommet);
	swap(m_tampon_sommet, buffer.m_tampon_sommet);
	swap(m_tampon_index, buffer.m_tampon_index);
	swap(m_tampon_normal, buffer.m_tampon_normal);
	return *this;
}

TamponObjet::Ptr TamponObjet::cree_unique() noexcept
{
	return Ptr(new TamponObjet());
}

TamponObjet::SPtr TamponObjet::cree_partage() noexcept
{
	return SPtr(new TamponObjet());
}

void TamponObjet::attache() const noexcept
{
	glBindVertexArray(m_objet_tableau_sommet);
}

void TamponObjet::detache() const noexcept
{
	glBindVertexArray(0);
}

void TamponObjet::pointeur_attribut(unsigned int index, int size, int stride, void *pointeur) const noexcept
{
	glEnableVertexAttribArray(index);
	glVertexAttribPointer(index, size, GL_FLOAT, GL_FALSE, stride, pointeur);
}

void TamponObjet::genere_tampon_sommet(void const *vertices, long const size) noexcept(false)
{
	genere_tampon(m_tampon_sommet, vertices, size, GL_ARRAY_BUFFER);
}

void TamponObjet::genere_tampon_index(void const *indices, long const size) noexcept(false)
{
	genere_tampon(m_tampon_index, indices, size, GL_ELEMENT_ARRAY_BUFFER);
}

void TamponObjet::genere_tampon_normal(void const *colors, long const size) noexcept(false)
{
	genere_tampon(m_tampon_normal, colors, size, GL_ARRAY_BUFFER);
}

void TamponObjet::genere_tampon_extra(void const *data, long const size) noexcept(false)
{
	auto extra_buffer = 0u;
	genere_tampon(extra_buffer, data, size, GL_ARRAY_BUFFER);
	m_tampons_extra.pousse(extra_buffer);
}

void TamponObjet::ajourne_tampon_sommet(void const *vertices, long const size) const noexcept
{
	if (glIsBuffer(m_tampon_sommet)) {
		glBindBuffer(GL_ARRAY_BUFFER, m_tampon_sommet);
		glBufferSubData(GL_ARRAY_BUFFER, 0, size, vertices);
	}
}

unsigned int TamponObjet::objet_tableau_sommet() const
{
	return m_objet_tableau_sommet;
}

void TamponObjet::ajourne_tampon_index(void const *indices, long const size) const noexcept
{
	if (glIsBuffer(m_tampon_index)) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_tampon_index);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, size, indices);
	}
}

bool operator==(TamponObjet const &b1, TamponObjet const &b2) noexcept
{
	return b1.objet_tableau_sommet() == b2.objet_tableau_sommet();
}

EGO_VERSION_NAMESPACE_END
}  /* namespace ego */
}  /* namespace dls */
