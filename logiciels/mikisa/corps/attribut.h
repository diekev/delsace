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

#include "biblinternes/math/matrice.hh"

#include "biblinternes/outils/iterateurs.h"

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

/* ************************************************************************** */

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

long taille_octet_type_attribut(type_attribut type);

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

/* ************************************************************************** */

/* structure pour trouver le type_attribut correspondant à un type de données */

template <typename T>
struct type_attribut_depuis_type;

template <>
struct type_attribut_depuis_type<char> {
	static constexpr auto type = type_attribut::ENT8;
};

template <>
struct type_attribut_depuis_type<int> {
	static constexpr auto type = type_attribut::ENT32;
};

template <>
struct type_attribut_depuis_type<float> {
	static constexpr auto type = type_attribut::DECIMAL;
};

template <>
struct type_attribut_depuis_type<dls::math::vec2f> {
	static constexpr auto type = type_attribut::VEC2;
};

template <>
struct type_attribut_depuis_type<dls::math::vec3f> {
	static constexpr auto type = type_attribut::VEC3;
};

template <>
struct type_attribut_depuis_type<dls::math::vec4f> {
	static constexpr auto type = type_attribut::VEC4;
};

template <>
struct type_attribut_depuis_type<dls::math::mat3x3f> {
	static constexpr auto type = type_attribut::MAT3;
};

template <>
struct type_attribut_depuis_type<dls::math::mat4x4f> {
	static constexpr auto type = type_attribut::MAT4;
};

template <>
struct type_attribut_depuis_type<dls::chaine> {
	static constexpr auto type = type_attribut::CHAINE;
};

/* ************************************************************************** */

#define ACCEDE_VALEUR_TYPE(__nom, __type) \
	__type &__nom(long idx) \
	{ \
		return this->valeur<__type>(idx); \
	} \
	__type const &__nom(long idx) const \
	{ \
		return this->valeur<__type>(idx); \
	}

class Attribut {
	dls::chaine m_nom;
	type_attribut m_type;

	dls::tableau<char> m_tampon{};

public:
	portee_attr portee;

	Attribut(Attribut const &rhs);

	Attribut(dls::chaine const &nom, type_attribut type, portee_attr portee = portee_attr::POINT, long taille = 0);

	Attribut &operator=(Attribut const &rhs) = default;

	type_attribut type() const;

	dls::chaine nom() const;
	void nom(const dls::chaine &n);

	void reserve(long n);
	void redimensionne(long n);

	void reinitialise();

	void *donnees();
	const void *donnees() const;

	long taille_octets() const;
	long taille() const;

	template <typename T>
	auto &valeur(long idx)
	{
		assert(idx >= 0);
		assert(idx < taille());
		assert(this->type() == type_attribut_depuis_type<T>::type);
		return *reinterpret_cast<T *>(&m_tampon[idx * static_cast<long>(sizeof(T))]);
	}

	template <typename T>
	auto const &valeur(long idx) const
	{
		assert(idx >= 0);
		assert(idx < taille());
		assert(this->type() == type_attribut_depuis_type<T>::type);
		return *reinterpret_cast<T const *>(&m_tampon[idx * static_cast<long>(sizeof(T))]);
	}

	template <typename T>
	auto valeur(long idx, T const &v)
	{
		this->valeur<T>(idx) = v;
	}

	ACCEDE_VALEUR_TYPE(ent8, char)
	ACCEDE_VALEUR_TYPE(ent32, int)
	ACCEDE_VALEUR_TYPE(decimal, float)
	ACCEDE_VALEUR_TYPE(vec2, dls::math::vec2f)
	ACCEDE_VALEUR_TYPE(vec3, dls::math::vec3f)
	ACCEDE_VALEUR_TYPE(vec4, dls::math::vec4f)
	ACCEDE_VALEUR_TYPE(mat3, dls::math::mat3x3f)
	ACCEDE_VALEUR_TYPE(mat4, dls::math::mat4x4f)
	ACCEDE_VALEUR_TYPE(chaine, dls::chaine)

	template <typename T>
	auto pousse(T const &v)
	{
		redimensionne(taille() + 1);
		this->valeur<T>(taille() - 1) = v;
	}
};

/* ************************************************************************** */

void copie_attribut(
		Attribut *attr_orig,
		long idx_orig,
		Attribut *attr_dest,
		long idx_dest);

template <typename T>
auto transforme_attr(Attribut &attr, std::function<T(T const&)> fonction_rappel)
{
	for (auto i = 0; i < attr.taille(); ++i) {
		attr.valeur<T>(i) = fonction_rappel(attr.valeur<T>(i));
	}
}

template <typename T>
auto accumule_attr(Attribut const &attr)
{
	auto depart = T{};

	for (auto i = 0; i < attr.taille(); ++i) {
		depart += attr.valeur<T>(i);
	}

	return depart;
}

#if 0
/* déclaration explicite car C++ converti les char vers des int pour les
 * additions, et ce genre de conversion silencieuse ne sont pas autorisées avec
 * nos drapeaux strictes de compilation */
template <>
auto accumule_attr<char>(Attribut const &attr)
{
	assert(attr.type() == type_attribut::ENT8);

	auto depart = char(0);

	for (auto i = 0; i < attr.taille(); ++i) {
		depart = static_cast<char>(depart + attr.ent8(i)); ;
	}

	return depart;
}
#endif
