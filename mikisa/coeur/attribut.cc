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
void copie(const Conteneur &de, Conteneur &vers)
{
	std::copy(de->begin(), de->end(), vers->begin());
}

Attribut::Attribut(const std::string &name, type_attribut type, int portee, size_t size)
	: m_nom(name)
	, m_type(type)
	, portee(portee)
{
	switch (m_type) {
		case ATTRIBUT_ENT8:
			m_donnees.liste_ent8 = new std::vector<char>(size);
			break;
		case ATTRIBUT_ENT32:
			m_donnees.liste_ent32 = new std::vector<int>(size);
			break;
		case ATTRIBUT_DECIMAL:
			m_donnees.liste_decimal = new std::vector<float>(size);
			break;
		case ATTRIBUT_CHAINE:
			m_donnees.liste_chaine = new std::vector<std::string>(size);
			break;
		case ATTRIBUT_VEC2:
			m_donnees.liste_vec2 = new std::vector<numero7::math::vec2f>(size);
			break;
		case ATTRIBUT_VEC3:
			m_donnees.liste_vec3 = new std::vector<numero7::math::vec3f>(size);
			break;
		case ATTRIBUT_VEC4:
			m_donnees.liste_vec4 = new std::vector<numero7::math::vec4f>(size);
			break;
		case ATTRIBUT_MAT3:
			m_donnees.liste_mat3 = new std::vector<numero7::math::mat3>(size);
			break;
		case ATTRIBUT_MAT4:
			m_donnees.liste_mat4 = new std::vector<numero7::math::mat4>(size);
			break;
		default:
			break;
	}
}

Attribut::Attribut(const Attribut &rhs)
	: Attribut(rhs.nom(), rhs.type(), rhs.portee, rhs.taille())
{
	switch (m_type) {
		case ATTRIBUT_ENT8:
			copie(rhs.m_donnees.liste_ent8, m_donnees.liste_ent8);
			break;
		case ATTRIBUT_ENT32:
			copie(rhs.m_donnees.liste_ent32, m_donnees.liste_ent32);
			break;
		case ATTRIBUT_DECIMAL:
			copie(rhs.m_donnees.liste_decimal, m_donnees.liste_decimal);
			break;
		case ATTRIBUT_CHAINE:
			copie(rhs.m_donnees.liste_chaine, m_donnees.liste_chaine);
			break;
		case ATTRIBUT_VEC2:
			copie(rhs.m_donnees.liste_vec2, m_donnees.liste_vec2);
			break;
		case ATTRIBUT_VEC3:
			copie(rhs.m_donnees.liste_vec3, m_donnees.liste_vec3);
			break;
		case ATTRIBUT_VEC4:
			copie(rhs.m_donnees.liste_vec4, m_donnees.liste_vec4);
			break;
		case ATTRIBUT_MAT3:
			copie(rhs.m_donnees.liste_mat3, m_donnees.liste_mat3);
			break;
		case ATTRIBUT_MAT4:
			copie(rhs.m_donnees.liste_mat4, m_donnees.liste_mat4);
			break;
		default:
			break;
	}
}

Attribut::~Attribut()
{
	switch (m_type) {
		case ATTRIBUT_ENT8:
			delete m_donnees.liste_ent8;
			break;
		case ATTRIBUT_ENT32:
			delete m_donnees.liste_ent32;
			break;
		case ATTRIBUT_DECIMAL:
			delete m_donnees.liste_decimal;
			break;
		case ATTRIBUT_CHAINE:
			delete m_donnees.liste_chaine;
			break;
		case ATTRIBUT_VEC2:
			delete m_donnees.liste_vec2;
			break;
		case ATTRIBUT_VEC3:
			delete m_donnees.liste_vec3;
			break;
		case ATTRIBUT_VEC4:
			delete m_donnees.liste_vec4;
			break;
		case ATTRIBUT_MAT3:
			delete m_donnees.liste_mat3;
			break;
		case ATTRIBUT_MAT4:
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

void Attribut::reserve(size_t n)
{
	switch (m_type) {
		case ATTRIBUT_ENT8:
			m_donnees.liste_ent8->reserve(n);
			break;
		case ATTRIBUT_ENT32:
			m_donnees.liste_ent32->reserve(n);
			break;
		case ATTRIBUT_DECIMAL:
			m_donnees.liste_decimal->reserve(n);
			break;
		case ATTRIBUT_CHAINE:
			m_donnees.liste_chaine->reserve(n);
			break;
		case ATTRIBUT_VEC2:
			m_donnees.liste_vec2->reserve(n);
			break;
		case ATTRIBUT_VEC3:
			m_donnees.liste_vec3->reserve(n);
			break;
		case ATTRIBUT_VEC4:
			m_donnees.liste_vec4->reserve(n);
			break;
		case ATTRIBUT_MAT3:
			m_donnees.liste_mat3->reserve(n);
			break;
		case ATTRIBUT_MAT4:
			m_donnees.liste_mat4->reserve(n);
			break;
		default:
			break;
	}
}

void Attribut::redimensionne(size_t n)
{
	switch (m_type) {
		case ATTRIBUT_ENT8:
			m_donnees.liste_ent8->resize(n);
			break;
		case ATTRIBUT_ENT32:
			m_donnees.liste_ent32->resize(n);
			break;
		case ATTRIBUT_DECIMAL:
			m_donnees.liste_decimal->resize(n);
			break;
		case ATTRIBUT_CHAINE:
			m_donnees.liste_chaine->resize(n);
			break;
		case ATTRIBUT_VEC2:
			m_donnees.liste_vec2->resize(n);
			break;
		case ATTRIBUT_VEC3:
			m_donnees.liste_vec3->resize(n);
			break;
		case ATTRIBUT_VEC4:
			m_donnees.liste_vec4->resize(n);
			break;
		case ATTRIBUT_MAT3:
			m_donnees.liste_mat3->resize(n);
			break;
		case ATTRIBUT_MAT4:
			m_donnees.liste_mat4->resize(n);
			break;
		default:
			break;
	}
}

size_t Attribut::taille() const
{
	switch (m_type) {
		case ATTRIBUT_ENT8:
			return m_donnees.liste_ent8->size();
		case ATTRIBUT_ENT32:
			return m_donnees.liste_ent32->size();
		case ATTRIBUT_DECIMAL:
			return m_donnees.liste_decimal->size();
		case ATTRIBUT_CHAINE:
			return m_donnees.liste_chaine->size();
		case ATTRIBUT_VEC2:
			return m_donnees.liste_vec2->size();
		case ATTRIBUT_VEC3:
			return m_donnees.liste_vec3->size();
		case ATTRIBUT_VEC4:
			return m_donnees.liste_vec4->size();
		case ATTRIBUT_MAT3:
			return m_donnees.liste_mat3->size();
		case ATTRIBUT_MAT4:
			return m_donnees.liste_mat4->size();
		default:
			return 0;
	}
}

void Attribut::reinitialise()
{
	switch (m_type) {
		case ATTRIBUT_ENT8:
			m_donnees.liste_ent8->clear();
			break;
		case ATTRIBUT_ENT32:
			m_donnees.liste_ent32->clear();
			break;
		case ATTRIBUT_DECIMAL:
			m_donnees.liste_decimal->clear();
			break;
		case ATTRIBUT_CHAINE:
			m_donnees.liste_chaine->clear();
			break;
		case ATTRIBUT_VEC2:
			m_donnees.liste_vec2->clear();
			break;
		case ATTRIBUT_VEC3:
			m_donnees.liste_vec3->clear();
			break;
		case ATTRIBUT_VEC4:
			m_donnees.liste_vec4->clear();
			break;
		case ATTRIBUT_MAT3:
			m_donnees.liste_mat3->clear();
			break;
		case ATTRIBUT_MAT4:
			m_donnees.liste_mat4->clear();
			break;
		default:
			break;
	}
}

const void *Attribut::donnees() const
{
	switch (m_type) {
		case ATTRIBUT_ENT8:
			return m_donnees.liste_ent8->data();
		case ATTRIBUT_ENT32:
			return m_donnees.liste_ent32->data();
		case ATTRIBUT_DECIMAL:
			return m_donnees.liste_decimal->data();
		case ATTRIBUT_CHAINE:
			return m_donnees.liste_chaine->data();
		case ATTRIBUT_VEC2:
			return m_donnees.liste_vec2->data();
		case ATTRIBUT_VEC3:
			return m_donnees.liste_vec3->data();
		case ATTRIBUT_VEC4:
			return m_donnees.liste_vec4->data();
		case ATTRIBUT_MAT3:
			return m_donnees.liste_mat3->data();
		case ATTRIBUT_MAT4:
			return m_donnees.liste_mat4->data();
		default:
			return nullptr;
	}
}

size_t Attribut::taille_octets() const
{
	switch (m_type) {
		case ATTRIBUT_ENT8:
			return m_donnees.liste_ent8->size() * sizeof(char);
		case ATTRIBUT_ENT32:
			return m_donnees.liste_ent32->size() * sizeof(int);
		case ATTRIBUT_DECIMAL:
			return m_donnees.liste_decimal->size() * sizeof(float);
		case ATTRIBUT_CHAINE:
			return m_donnees.liste_chaine->size() * sizeof(std::string);
		case ATTRIBUT_VEC2:
			return m_donnees.liste_vec2->size() * sizeof(numero7::math::vec2f);
		case ATTRIBUT_VEC3:
			return m_donnees.liste_vec3->size() * sizeof(numero7::math::vec3f);
		case ATTRIBUT_VEC4:
			return m_donnees.liste_vec4->size() * sizeof(numero7::math::vec4f);
		case ATTRIBUT_MAT3:
			return m_donnees.liste_mat3->size() * sizeof(numero7::math::mat3);
		case ATTRIBUT_MAT4:
			return m_donnees.liste_mat4->size() * sizeof(numero7::math::mat4);
		default:
			return 0;
	}
}
