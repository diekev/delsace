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

/* ************************************************************************** */

static auto taille_octet_type_attribut(type_attribut type)
{
	switch (type) {
		case type_attribut::ENT8:
			return sizeof(char);
		case type_attribut::ENT32:
			return sizeof(int);
		case type_attribut::DECIMAL:
			return sizeof(float);
		case type_attribut::CHAINE:
			return sizeof(std::string);
		case type_attribut::VEC2:
			return sizeof(dls::math::vec2f);
		case type_attribut::VEC3:
			return sizeof(dls::math::vec3f);
		case type_attribut::VEC4:
			return sizeof(dls::math::vec4f);
		case type_attribut::MAT3:
			return sizeof(dls::math::mat3x3f);
		case type_attribut::MAT4:
			return sizeof(dls::math::mat4x4f);
		default:
			return 0ul;
	}
}

/* ************************************************************************** */

Attribut::Attribut(std::string const &name, type_attribut type, portee_attr portee_, long taille)
	: m_nom(name)
	, m_type(type)
	, portee(portee_)
{
	assert(taille >= 0);
	auto taille_ns = static_cast<size_t>(taille);

	m_tampon.resize(taille_ns * taille_octet_type_attribut(m_type));
}

Attribut::Attribut(Attribut const &rhs)
	: Attribut(rhs.nom(), rhs.type(), rhs.portee, rhs.taille())
{
	std::copy(rhs.m_tampon.begin(), rhs.m_tampon.end(), m_tampon.begin());
}

type_attribut Attribut::type() const
{
	return m_type;
}

std::string Attribut::nom() const
{
	return m_nom;
}

void Attribut::nom(std::string const &n)
{
	m_nom = n;
}

void Attribut::reserve(long n)
{
	assert(n >= 0);
	auto nt = static_cast<size_t>(n);
	m_tampon.reserve(nt * taille_octet_type_attribut(m_type));
}

void Attribut::redimensionne(long n)
{
	assert(n >= 0);
	auto nt = static_cast<size_t>(n);
	m_tampon.resize(nt * taille_octet_type_attribut(m_type));
}

long Attribut::taille() const
{
	return static_cast<long>(m_tampon.size() / taille_octet_type_attribut(m_type));
}

void Attribut::reinitialise()
{
	m_tampon.clear();
}

const void *Attribut::donnees() const
{
	return m_tampon.data();
}

void *Attribut::donnees()
{
	return m_tampon.data();
}

long Attribut::taille_octets() const
{
	return static_cast<long>(m_tampon.size());
}

/* ************************************************************************** */

void copie_attribut(Attribut *attr_orig, long idx_orig, Attribut *attr_dest, long idx_dest)
{
	switch (attr_orig->type()) {
		case type_attribut::ENT8:
			attr_dest->valeur(idx_dest, attr_orig->ent8(idx_orig));
			break;
		case type_attribut::ENT32:
			attr_dest->valeur(idx_dest, attr_orig->ent32(idx_orig));
			break;
		case type_attribut::DECIMAL:
			attr_dest->valeur(idx_dest, attr_orig->decimal(idx_orig));
			break;
		case type_attribut::VEC2:
			attr_dest->valeur(idx_dest, attr_orig->vec2(idx_orig));
			break;
		case type_attribut::VEC3:
			attr_dest->valeur(idx_dest, attr_orig->vec3(idx_orig));
			break;
		case type_attribut::VEC4:
			attr_dest->valeur(idx_dest, attr_orig->vec4(idx_orig));
			break;
		case type_attribut::MAT3:
			attr_dest->valeur(idx_dest, attr_orig->mat3(idx_orig));
			break;
		case type_attribut::MAT4:
			attr_dest->valeur(idx_dest, attr_orig->mat4(idx_orig));
			break;
		case type_attribut::CHAINE:
			attr_dest->valeur(idx_dest, attr_orig->chaine(idx_orig));
			break;
		case type_attribut::INVALIDE:
			break;
	}
}
