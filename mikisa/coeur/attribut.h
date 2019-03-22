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

#pragma once

#include <delsace/math/matrice.hh>
#include <string>
#include <vector>

#include "bibliotheques/outils/iterateurs.h"

enum class type_attribut : char {
	INVALIDE = -1,
	ENT8 = 0,
	ENT32,
	DECIMAL,
	CHAINE,
	VEC2,
	VEC3,
	VEC4,
	MAT3,
	MAT4,
};

enum class portee_attr : char {
	/* l'attribut varie pour chaque point */
	POINT,
	/* l'attribut varie pour chaque primitive */
	PRIMITIVE,
	/* l'attribut varie pour chaque vertex de chaque primitive */
	VERTEX,
	/* l'attribut est unique pour le corps */
	CORPS,
	/* l'attribut varie pour chaque groupe */
	GROUPE,
};

#define DEFINI_ITERATEURS(__nom, __type) \
	using plage_##__nom = plage_iterable<std::vector<__type>::iterator>; \
	using plage_const_##__nom = plage_iterable<std::vector<__type>::const_iterator>

#define DEFINI_ACCESSEURS_PLAGE(__nom) \
	plage_##__nom __nom() \
	{ \
		return plage_##__nom(m_donnees.liste_##__nom->begin(), m_donnees.liste_##__nom->end()); \
	} \
	plage_const_##__nom __nom() const \
	{ \
		return plage_const_##__nom(m_donnees.liste_##__nom->cbegin(), m_donnees.liste_##__nom->cend()); \
	}

#define DEFINI_ACESSEURS_POSITION(__nom, __type) \
	__type __nom(long const i) const \
	{ \
		assert(i >= 0 && static_cast<size_t>(i) < (m_donnees.liste_##__nom)->size()); \
		return (*m_donnees.liste_##__nom)[static_cast<size_t>(i)]; \
	} \
	void __nom(long const i, __type const &v) \
	{ \
		assert(i >= 0 && static_cast<size_t>(i) < (m_donnees.liste_##__nom)->size()); \
		(*m_donnees.liste_##__nom)[static_cast<size_t>(i)] = v; \
	}

#define DEFINI_POUSSE_VALEUR(__nom, __type) \
	void pousse_##__nom(__type const &v) \
	{ \
		m_donnees.liste_##__nom->push_back(v); \
	}

class Attribut {
	union {
		std::vector<char> *liste_ent8;
		std::vector<int> *liste_ent32;
		std::vector<float> *liste_decimal;
		std::vector<std::string> *liste_chaine;
		std::vector<dls::math::vec2f> *liste_vec2;
		std::vector<dls::math::vec3f> *liste_vec3;
		std::vector<dls::math::vec4f> *liste_vec4;
		std::vector<dls::math::mat3x3f> *liste_mat3;
		std::vector<dls::math::mat4x4f> *liste_mat4;
	} m_donnees{};

	std::string m_nom;
	type_attribut m_type;

public:
	DEFINI_ITERATEURS(ent8, char);
	DEFINI_ITERATEURS(ent32, int);
	DEFINI_ITERATEURS(decimal, float);
	DEFINI_ITERATEURS(chaine, std::string);
	DEFINI_ITERATEURS(vec2, dls::math::vec2f);
	DEFINI_ITERATEURS(vec3, dls::math::vec3f);
	DEFINI_ITERATEURS(vec4, dls::math::vec4f);
	DEFINI_ITERATEURS(mat3, dls::math::mat3x3f);
	DEFINI_ITERATEURS(mat4, dls::math::mat4x4f);

	portee_attr portee;

	Attribut(Attribut const &rhs);
	Attribut(std::string const &nom, type_attribut type, portee_attr portee = portee_attr::POINT, long taille = 0);
	~Attribut();

	Attribut &operator=(Attribut const &rhs) = default;

	type_attribut type() const;
	std::string nom() const;
	void nom(const std::string &n);

	void reserve(long n);
	void redimensionne(long n);

	void reinitialise();

	void *donnees();
	const void *donnees() const;

	long taille_octets() const;
	long taille() const;

	DEFINI_ACCESSEURS_PLAGE(ent8)
	DEFINI_ACCESSEURS_PLAGE(ent32)
	DEFINI_ACCESSEURS_PLAGE(decimal)
	DEFINI_ACCESSEURS_PLAGE(chaine)
	DEFINI_ACCESSEURS_PLAGE(vec2)
	DEFINI_ACCESSEURS_PLAGE(vec3)
	DEFINI_ACCESSEURS_PLAGE(vec4)
	DEFINI_ACCESSEURS_PLAGE(mat3)
	DEFINI_ACCESSEURS_PLAGE(mat4)

	DEFINI_ACESSEURS_POSITION(ent8, char)
	DEFINI_ACESSEURS_POSITION(ent32, int)
	DEFINI_ACESSEURS_POSITION(decimal, float)
	DEFINI_ACESSEURS_POSITION(chaine, std::string)
	DEFINI_ACESSEURS_POSITION(vec2, dls::math::vec2f)
	DEFINI_ACESSEURS_POSITION(vec3, dls::math::vec3f)
	DEFINI_ACESSEURS_POSITION(vec4, dls::math::vec4f)
	DEFINI_ACESSEURS_POSITION(mat3, dls::math::mat3x3f)
	DEFINI_ACESSEURS_POSITION(mat4, dls::math::mat4x4f)

	DEFINI_POUSSE_VALEUR(ent8, char)
	DEFINI_POUSSE_VALEUR(ent32, int)
	DEFINI_POUSSE_VALEUR(decimal, float)
	DEFINI_POUSSE_VALEUR(chaine, std::string)
	DEFINI_POUSSE_VALEUR(vec2, dls::math::vec2f)
	DEFINI_POUSSE_VALEUR(vec3, dls::math::vec3f)
	DEFINI_POUSSE_VALEUR(vec4, dls::math::vec4f)
	DEFINI_POUSSE_VALEUR(mat3, dls::math::mat3x3f)
	DEFINI_POUSSE_VALEUR(mat4, dls::math::mat4x4f)
};

#define TRANSFORME_ATTRIBUT(__nom, __type, __type_attr) \
	inline void transforme_attr_##__nom(Attribut &attr, std::function<__type(__type const&)> fonction_rappel) \
	{ \
		assert(attr.type() == __type_attr); \
		for (auto &v : attr.__nom()) { \
			v = fonction_rappel(v); \
		} \
	}

TRANSFORME_ATTRIBUT(ent8, char, type_attribut::ENT8)
TRANSFORME_ATTRIBUT(ent32, int, type_attribut::ENT32)
TRANSFORME_ATTRIBUT(decimal, float, type_attribut::DECIMAL)
TRANSFORME_ATTRIBUT(chaine, std::string, type_attribut::CHAINE)
TRANSFORME_ATTRIBUT(vec2, dls::math::vec2f, type_attribut::VEC2)
TRANSFORME_ATTRIBUT(vec3, dls::math::vec3f, type_attribut::VEC3)
TRANSFORME_ATTRIBUT(vec4, dls::math::vec4f, type_attribut::VEC4)
TRANSFORME_ATTRIBUT(mat3, dls::math::mat3x3f, type_attribut::MAT3)
TRANSFORME_ATTRIBUT(mat4, dls::math::mat4x4f, type_attribut::MAT4)

#define ACCUMULE_ATTRIBUT(__nom, __type, __type_attr) \
	inline __type accumule_attr_##__nom(Attribut &attr) \
	{ \
		assert(attr.type() == __type_attr); \
		auto depart = __type{}; \
		for (auto &v : attr.__nom()) { \
			depart += v; \
		} \
		return depart;\
	}

/* déclaration explicite car C++ converti les char vers des int pour les
 * additions, et ce genre de conversion silencieuse ne sont pas autorisées avec
 * nos drapeaux strictes de compilation */
inline auto accumule_attr_char(Attribut &attr)
{
	assert(attr.type() == type_attribut::ENT8);
	auto depart = char(0);
	for (auto &v : attr.ent8()) {
		depart = static_cast<char>(depart + v);
	}
	return depart;
}

ACCUMULE_ATTRIBUT(ent32, int, type_attribut::ENT32)
ACCUMULE_ATTRIBUT(decimal, float, type_attribut::DECIMAL)
ACCUMULE_ATTRIBUT(chaine, std::string, type_attribut::CHAINE)
ACCUMULE_ATTRIBUT(vec2, dls::math::vec2f, type_attribut::VEC2)
ACCUMULE_ATTRIBUT(vec3, dls::math::vec3f, type_attribut::VEC3)
ACCUMULE_ATTRIBUT(vec4, dls::math::vec4f, type_attribut::VEC4)
ACCUMULE_ATTRIBUT(mat3, dls::math::mat3x3f, type_attribut::MAT3)
ACCUMULE_ATTRIBUT(mat4, dls::math::mat4x4f, type_attribut::MAT4)
