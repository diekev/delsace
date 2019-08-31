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

#include <memory>

#include "biblinternes/math/matrice.hh"
#include "biblinternes/outils/iterateurs.h"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

struct Corps;

/* ************************************************************************** */

enum class type_attribut : char {
	INVALIDE = -1,
	Z8,
	Z16,
	Z32,
	Z64,
	N8,
	N16,
	N32,
	N64,
	R16,
	R32,
	R64,
	CHAINE,
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
	static constexpr auto type = type_attribut::Z8;
};

template <>
struct type_attribut_depuis_type<short> {
	static constexpr auto type = type_attribut::Z16;
};

template <>
struct type_attribut_depuis_type<int> {
	static constexpr auto type = type_attribut::Z32;
};

template <>
struct type_attribut_depuis_type<long> {
	static constexpr auto type = type_attribut::Z64;
};

template <>
struct type_attribut_depuis_type<unsigned char> {
	static constexpr auto type = type_attribut::N8;
};

template <>
struct type_attribut_depuis_type<unsigned short> {
	static constexpr auto type = type_attribut::N16;
};

template <>
struct type_attribut_depuis_type<unsigned int> {
	static constexpr auto type = type_attribut::N32;
};

template <>
struct type_attribut_depuis_type<unsigned long> {
	static constexpr auto type = type_attribut::N64;
};

// À FAIRE : type R16

template <>
struct type_attribut_depuis_type<float> {
	static constexpr auto type = type_attribut::R32;
};

template <>
struct type_attribut_depuis_type<double> {
	static constexpr auto type = type_attribut::R64;
};

template <>
struct type_attribut_depuis_type<dls::chaine> {
	static constexpr auto type = type_attribut::CHAINE;
};

/* ************************************************************************** */

#define ACCEDE_VALEUR_TYPE(__nom, __type) \
	__type *__nom(long idx) \
	{ \
		return this->valeur<__type>(idx); \
	} \
	__type const *__nom(long idx) const \
	{ \
		return this->valeur<__type>(idx); \
	}

class Attribut {
	dls::chaine m_nom;
	type_attribut m_type;

	using type_liste = dls::tableau<char>;
	using ptr_liste = std::shared_ptr<type_liste>;
	ptr_liste m_tampon{};

public:
	portee_attr portee;
	int dimensions = 0;

	Attribut(Attribut const &rhs);

	Attribut(dls::chaine const &nom, type_attribut type, int dims = 1, portee_attr portee = portee_attr::POINT, long taille = 0);

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
	auto *valeur(long idx)
	{
		assert(idx >= 0);
		assert(idx < taille());
		assert(this->type() == type_attribut_depuis_type<T>::type);
		detache();
		return reinterpret_cast<T *>(&(*m_tampon)[idx * dimensions * static_cast<long>(sizeof(T))]);
	}

	template <typename T>
	auto const *valeur(long idx) const
	{
		assert(idx >= 0);
		assert(idx < taille());
		assert(this->type() == type_attribut_depuis_type<T>::type);
		return reinterpret_cast<T const *>(&(*m_tampon)[idx * dimensions * static_cast<long>(sizeof(T))]);
	}

	ACCEDE_VALEUR_TYPE(z8, char)
	ACCEDE_VALEUR_TYPE(z16, short)
	ACCEDE_VALEUR_TYPE(z32, int)
	ACCEDE_VALEUR_TYPE(z64, long)
	ACCEDE_VALEUR_TYPE(n8, unsigned char)
	ACCEDE_VALEUR_TYPE(n16, unsigned short)
	ACCEDE_VALEUR_TYPE(n32, unsigned int)
	ACCEDE_VALEUR_TYPE(n64, unsigned long)
	ACCEDE_VALEUR_TYPE(r32, float)
	ACCEDE_VALEUR_TYPE(r64, double)
	ACCEDE_VALEUR_TYPE(chaine, dls::chaine)

	void detache();
};

template <typename T>
inline auto assigne(T *ptr, T valeur)
{
	*ptr = valeur;
}

template <typename T, int O, int... Ns>
inline auto assigne(T *ptr, dls::math::vecteur<O, T, Ns...> const &valeur)
{
	for (auto i = 0ul; i < sizeof...(Ns); ++i) {
		ptr[i] = valeur[i];
	}
}

template <typename T, int O, int... Ns>
inline auto extrait(T const *ptr, dls::math::vecteur<O, T, Ns...> &valeur)
{
	for (auto i = 0ul; i < sizeof...(Ns); ++i) {
		valeur[i] = ptr[i];
	}
}

/* ************************************************************************** */

void copie_attribut(
		Attribut const *attr_orig,
		long idx_orig,
		Attribut *attr_dest,
		long idx_dest);

template <typename T>
auto transforme_attr(Attribut &attr, std::function<void(T *)> fonction_rappel)
{
	for (auto i = 0; i < attr.taille(); ++i) {
		fonction_rappel(attr.valeur<T>(i));
	}
}

template <typename T>
auto accumule_attr(Attribut const &attr, T *r_ptr)
{
	for (auto i = 0; i < attr.taille(); ++i) {
		for (auto j = 0; j < attr.dimensions; ++j) {
			r_ptr[j] += attr.valeur<T>(i)[j];
		}
	}
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

/* ************************************************************************** */

enum {
	TRANSFERE_ATTR_POINTS  = (1 << 0),
	TRANSFERE_ATTR_PRIMS   = (1 << 1),
	TRANSFERE_ATTR_CORPS   = (1 << 2),
	TRANSFERE_ATTR_GROUPES = (1 << 3),
	TRANSFERE_ATTR_SOMMETS = (1 << 4),

	TRANSFERE_TOUT = (TRANSFERE_ATTR_POINTS | TRANSFERE_ATTR_PRIMS | TRANSFERE_ATTR_CORPS | TRANSFERE_ATTR_GROUPES | TRANSFERE_ATTR_SOMMETS)
};

struct TransferanteAttribut {
private:
	using type_paire = dls::tableau<std::pair<Attribut const *, Attribut *>>;
	type_paire m_attr_points{};
	type_paire m_attr_prims{};
	type_paire m_attr_sommets{};
	type_paire m_attr_corps{};
	type_paire m_attr_groupes{};

public:
	TransferanteAttribut(Corps const &corps_orig, Corps &corps_dest, int drapeaux = TRANSFERE_TOUT);

	void transfere_attributs_points(long idx_orig, long idx_dest);

	void transfere_attributs_prims(long idx_orig, long idx_dest);

	void transfere_attributs_sommets(long idx_orig, long idx_dest);

	void transfere_attributs_corps(long idx_orig, long idx_dest);

	void transfere_attributs_groupes(long idx_orig, long idx_dest);
};
