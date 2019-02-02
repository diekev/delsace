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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "attribut.h"

template <typename Conteneur>
void copie(Conteneur const &de, Conteneur &vers)
{
	std::copy(de->begin(), de->end(), vers->begin());
}

Attribut::Attribut(std::string const &name, type_attribut type, portee_attr portee_, long taille)
	: m_nom(name)
	, m_type(type)
	, portee(portee_)
{
	assert(taille >= 0);
	auto taille_ns = static_cast<size_t>(taille);

	switch (m_type) {
		case type_attribut::ENT8:
			m_donnees.liste_ent8 = new std::vector<char>(taille_ns);
			break;
		case type_attribut::ENT32:
			m_donnees.liste_ent32 = new std::vector<int>(taille_ns);
			break;
		case type_attribut::DECIMAL:
			m_donnees.liste_decimal = new std::vector<float>(taille_ns);
			break;
		case type_attribut::CHAINE:
			m_donnees.liste_chaine = new std::vector<std::string>(taille_ns);
			break;
		case type_attribut::VEC2:
			m_donnees.liste_vec2 = new std::vector<dls::math::vec2f>(taille_ns);
			break;
		case type_attribut::VEC3:
			m_donnees.liste_vec3 = new std::vector<dls::math::vec3f>(taille_ns);
			break;
		case type_attribut::VEC4:
			m_donnees.liste_vec4 = new std::vector<dls::math::vec4f>(taille_ns);
			break;
		case type_attribut::MAT3:
			m_donnees.liste_mat3 = new std::vector<dls::math::mat3x3f>(taille_ns);
			break;
		case type_attribut::MAT4:
			m_donnees.liste_mat4 = new std::vector<dls::math::mat4x4f>(taille_ns);
			break;
		default:
			break;
	}
}

Attribut::Attribut(Attribut const &rhs)
	: Attribut(rhs.nom(), rhs.type(), rhs.portee, rhs.taille())
{
	switch (m_type) {
		case type_attribut::ENT8:
			copie(rhs.m_donnees.liste_ent8, m_donnees.liste_ent8);
			break;
		case type_attribut::ENT32:
			copie(rhs.m_donnees.liste_ent32, m_donnees.liste_ent32);
			break;
		case type_attribut::DECIMAL:
			copie(rhs.m_donnees.liste_decimal, m_donnees.liste_decimal);
			break;
		case type_attribut::CHAINE:
			copie(rhs.m_donnees.liste_chaine, m_donnees.liste_chaine);
			break;
		case type_attribut::VEC2:
			copie(rhs.m_donnees.liste_vec2, m_donnees.liste_vec2);
			break;
		case type_attribut::VEC3:
			copie(rhs.m_donnees.liste_vec3, m_donnees.liste_vec3);
			break;
		case type_attribut::VEC4:
			copie(rhs.m_donnees.liste_vec4, m_donnees.liste_vec4);
			break;
		case type_attribut::MAT3:
			copie(rhs.m_donnees.liste_mat3, m_donnees.liste_mat3);
			break;
		case type_attribut::MAT4:
			copie(rhs.m_donnees.liste_mat4, m_donnees.liste_mat4);
			break;
		default:
			break;
	}
}

Attribut::~Attribut()
{
	switch (m_type) {
		case type_attribut::ENT8:
			delete m_donnees.liste_ent8;
			break;
		case type_attribut::ENT32:
			delete m_donnees.liste_ent32;
			break;
		case type_attribut::DECIMAL:
			delete m_donnees.liste_decimal;
			break;
		case type_attribut::CHAINE:
			delete m_donnees.liste_chaine;
			break;
		case type_attribut::VEC2:
			delete m_donnees.liste_vec2;
			break;
		case type_attribut::VEC3:
			delete m_donnees.liste_vec3;
			break;
		case type_attribut::VEC4:
			delete m_donnees.liste_vec4;
			break;
		case type_attribut::MAT3:
			delete m_donnees.liste_mat3;
			break;
		case type_attribut::MAT4:
			delete m_donnees.liste_mat4;
			break;
		default:
			break;
	}
}

type_attribut Attribut::type() const
{
	return m_type;
}

std::string Attribut::nom() const
{
	return m_nom;
}

void Attribut::reserve(long n)
{
	assert(n >= 0);
	auto nt = static_cast<size_t>(n);

	switch (m_type) {
		case type_attribut::ENT8:
			m_donnees.liste_ent8->reserve(nt);
			break;
		case type_attribut::ENT32:
			m_donnees.liste_ent32->reserve(nt);
			break;
		case type_attribut::DECIMAL:
			m_donnees.liste_decimal->reserve(nt);
			break;
		case type_attribut::CHAINE:
			m_donnees.liste_chaine->reserve(nt);
			break;
		case type_attribut::VEC2:
			m_donnees.liste_vec2->reserve(nt);
			break;
		case type_attribut::VEC3:
			m_donnees.liste_vec3->reserve(nt);
			break;
		case type_attribut::VEC4:
			m_donnees.liste_vec4->reserve(nt);
			break;
		case type_attribut::MAT3:
			m_donnees.liste_mat3->reserve(nt);
			break;
		case type_attribut::MAT4:
			m_donnees.liste_mat4->reserve(nt);
			break;
		default:
			break;
	}
}

void Attribut::redimensionne(long n)
{
	assert(n >= 0);
	auto nt = static_cast<size_t>(n);

	switch (m_type) {
		case type_attribut::ENT8:
			m_donnees.liste_ent8->resize(nt);
			break;
		case type_attribut::ENT32:
			m_donnees.liste_ent32->resize(nt);
			break;
		case type_attribut::DECIMAL:
			m_donnees.liste_decimal->resize(nt);
			break;
		case type_attribut::CHAINE:
			m_donnees.liste_chaine->resize(nt);
			break;
		case type_attribut::VEC2:
			m_donnees.liste_vec2->resize(nt);
			break;
		case type_attribut::VEC3:
			m_donnees.liste_vec3->resize(nt);
			break;
		case type_attribut::VEC4:
			m_donnees.liste_vec4->resize(nt);
			break;
		case type_attribut::MAT3:
			m_donnees.liste_mat3->resize(nt);
			break;
		case type_attribut::MAT4:
			m_donnees.liste_mat4->resize(nt);
			break;
		default:
			break;
	}
}

long Attribut::taille() const
{
	auto octets = 0ul;

	switch (m_type) {
		case type_attribut::ENT8:
			octets = m_donnees.liste_ent8->size();
			break;
		case type_attribut::ENT32:
			octets = m_donnees.liste_ent32->size();
			break;
		case type_attribut::DECIMAL:
			octets = m_donnees.liste_decimal->size();
			break;
		case type_attribut::CHAINE:
			octets = m_donnees.liste_chaine->size();
			break;
		case type_attribut::VEC2:
			octets = m_donnees.liste_vec2->size();
			break;
		case type_attribut::VEC3:
			octets = m_donnees.liste_vec3->size();
			break;
		case type_attribut::VEC4:
			octets = m_donnees.liste_vec4->size();
			break;
		case type_attribut::MAT3:
			octets = m_donnees.liste_mat3->size();
			break;
		case type_attribut::MAT4:
			octets = m_donnees.liste_mat4->size();
			break;
		default:
			break;
	}

	return static_cast<long>(octets);
}

void Attribut::reinitialise()
{
	switch (m_type) {
		case type_attribut::ENT8:
			m_donnees.liste_ent8->clear();
			break;
		case type_attribut::ENT32:
			m_donnees.liste_ent32->clear();
			break;
		case type_attribut::DECIMAL:
			m_donnees.liste_decimal->clear();
			break;
		case type_attribut::CHAINE:
			m_donnees.liste_chaine->clear();
			break;
		case type_attribut::VEC2:
			m_donnees.liste_vec2->clear();
			break;
		case type_attribut::VEC3:
			m_donnees.liste_vec3->clear();
			break;
		case type_attribut::VEC4:
			m_donnees.liste_vec4->clear();
			break;
		case type_attribut::MAT3:
			m_donnees.liste_mat3->clear();
			break;
		case type_attribut::MAT4:
			m_donnees.liste_mat4->clear();
			break;
		default:
			break;
	}
}

const void *Attribut::donnees() const
{
	switch (m_type) {
		case type_attribut::ENT8:
			return m_donnees.liste_ent8->data();
		case type_attribut::ENT32:
			return m_donnees.liste_ent32->data();
		case type_attribut::DECIMAL:
			return m_donnees.liste_decimal->data();
		case type_attribut::CHAINE:
			return m_donnees.liste_chaine->data();
		case type_attribut::VEC2:
			return m_donnees.liste_vec2->data();
		case type_attribut::VEC3:
			return m_donnees.liste_vec3->data();
		case type_attribut::VEC4:
			return m_donnees.liste_vec4->data();
		case type_attribut::MAT3:
			return m_donnees.liste_mat3->data();
		case type_attribut::MAT4:
			return m_donnees.liste_mat4->data();
		default:
			return nullptr;
	}
}

void *Attribut::donnees()
{
	switch (m_type) {
		case type_attribut::ENT8:
			return m_donnees.liste_ent8->data();
		case type_attribut::ENT32:
			return m_donnees.liste_ent32->data();
		case type_attribut::DECIMAL:
			return m_donnees.liste_decimal->data();
		case type_attribut::CHAINE:
			return m_donnees.liste_chaine->data();
		case type_attribut::VEC2:
			return m_donnees.liste_vec2->data();
		case type_attribut::VEC3:
			return m_donnees.liste_vec3->data();
		case type_attribut::VEC4:
			return m_donnees.liste_vec4->data();
		case type_attribut::MAT3:
			return m_donnees.liste_mat3->data();
		case type_attribut::MAT4:
			return m_donnees.liste_mat4->data();
		default:
			return nullptr;
	}
}

long Attribut::taille_octets() const
{
	auto octets = 0ul;

	switch (m_type) {
		case type_attribut::ENT8:
			octets = m_donnees.liste_ent8->size() * sizeof(char);
			break;
		case type_attribut::ENT32:
			octets = m_donnees.liste_ent32->size() * sizeof(int);
			break;
		case type_attribut::DECIMAL:
			octets = m_donnees.liste_decimal->size() * sizeof(float);
			break;
		case type_attribut::CHAINE:
			octets = m_donnees.liste_chaine->size() * sizeof(std::string);
			break;
		case type_attribut::VEC2:
			octets = m_donnees.liste_vec2->size() * sizeof(dls::math::vec2f);
			break;
		case type_attribut::VEC3:
			octets = m_donnees.liste_vec3->size() * sizeof(dls::math::vec3f);
			break;
		case type_attribut::VEC4:
			octets = m_donnees.liste_vec4->size() * sizeof(dls::math::vec4f);
			break;
		case type_attribut::MAT3:
			octets = m_donnees.liste_mat3->size() * sizeof(dls::math::mat3x3f);
			break;
		case type_attribut::MAT4:
			octets = m_donnees.liste_mat4->size() * sizeof(dls::math::mat4x4f);
			break;
		default:
			break;
	}

	return static_cast<long>(octets);
}
