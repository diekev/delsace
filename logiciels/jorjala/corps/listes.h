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

#include "biblinternes/math/vecteur.hh"

#include "biblinternes/outils/iterateurs.h"

#include "biblinternes/structures/tableau.hh"

#include "primitive.hh"

struct Corps;

/* ************************************************************************** */

enum class type_polygone : char {
	FERME,
	OUVERT,
};

class Polygone final : public Primitive {
	dls::tableau<long> m_idx_points{};
	dls::tableau<long> m_idx_sommets{};

public:
	/* nouvelle entreface */
	Polygone() = default;

	Polygone(Polygone const &) = default;
	Polygone &operator=(Polygone const &) = default;

	/* Le type de ce polygone :
	 * type_polygone::FERME : un polygone avec une face
	 * type_polygone::OUVERT : un polygone sans face, une courbe
	 */
	type_polygone type = type_polygone::FERME;

	void ajoute_point(long idx_point, long idx_sommet);

	void reserve_sommets(long nombre);

	long nombre_sommets() const;

	long nombre_segments() const;

	long index_point(long i) const;

	long index_sommet(long i) const;

	type_primitive type_prim() const override
	{
		return type_primitive::POLYGONE;
	}

	void ajourne_index(long i, long j);
};

/* ************************************************************************** */

class ListePoints3D {
public:
	using type_liste = dls::tableau<dls::math::vec3f>;

private:
	typedef std::shared_ptr<type_liste> RefPtr;

	RefPtr m_sommets{};

public:
	~ListePoints3D();

	void reinitialise();

	void redimensionne(long const nombre);

	void reserve(long const nombre);

	long taille() const;

	void pousse(dls::math::vec3f const &p);

	dls::math::vec3f point(long i) const;

	void point(long i, dls::math::vec3f const &p);

	/* public pour pouvoir détacher avant de modifier dans des threads */
	void detache();
};

/* ************************************************************************** */

class ListePrimitives {
public:
	using type_liste = dls::tableau<Primitive *>;

private:
	typedef std::shared_ptr<type_liste> RefPtr;
	RefPtr m_primitives{};

public:
	~ListePrimitives();

	void reinitialise();

	void redimensionne(long const nombre);

	void reserve(long const nombre);

	long taille() const;

	void pousse(Primitive *s);

	Primitive *prim(long index) const;

	void prim(long i, Primitive *p);

	/* public pour pouvoir détacher avant de modifier dans des threads */
	void detache();
};
