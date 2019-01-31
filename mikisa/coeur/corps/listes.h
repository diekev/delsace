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

#include <vector>

#include <delsace/math/vecteur.hh>

#include "bibliotheques/outils/iterateurs.h"

#include "primitive.hh"

struct Corps;

struct Point3D {
	float x, y, z;
};

/* ************************************************************************** */

/**
 * Représentation d'un sommet dans l'espace tridimensionel.
 */
struct Sommet {
	dls::math::vec3f pos;
	int index;
};

class Polygone;

/* ************************************************************************** */

/**
 * Représentation d'une arrête d'un polygone.
 */
struct Arrete {
	/* Les sommets connectés à cette arrête. */
	Sommet *s[2] = { nullptr, nullptr };

	/* Le polygone propriétaire de cette arrête. */
	Polygone *p = nullptr;

	/* L'arrête du polygone adjacent allant dans la direction opposée de
	 * celle-ci. */
	Arrete *opposee = nullptr;

	/* L'index de l'arrête dans la boucle d'arrêtes du polygone. */
	char index = 0;

	Arrete() = default;
};

/* ************************************************************************** */

enum class type_polygone : char {
	FERME,
	OUVERT,
};

/**
 * Représentation d'un quadrilatère et de son vecteur normal dans l'espace
 * tridimensionel.
 */
class Polygone final : public Primitive {
	std::vector<size_t> m_sommets{};

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

	/* Le vecteur normal de ce polygone. */
	dls::math::vec3f nor{};

	static Polygone *construit(Corps *corps, type_polygone type_poly = type_polygone::FERME, size_t nombre_sommets = 0);

	void ajoute_sommet(size_t sommet);

	void reserve_sommets(size_t nombre);

	size_t nombre_sommets() const;

	size_t nombre_segments() const;

	size_t index_point(size_t i);

	type_primitive type_prim() const override
	{
		return type_primitive::POLYGONE;
	}
};

/* ************************************************************************** */

class ListePoints3D {
	std::vector<Point3D *> m_sommets{};

public:
	using plage_sommets = plage_iterable<std::vector<Point3D *>::iterator>;
	using plage_const_sommets = plage_iterable<std::vector<Point3D *>::const_iterator>;

	~ListePoints3D();

	void reinitialise();

	void redimensionne(const size_t nombre);

	void reserve(const size_t nombre);

	size_t taille() const;

	void pousse(Point3D *p);

	dls::math::vec3f point(size_t i) const;
	void point(size_t i, const dls::math::vec3f &p) const;

	plage_sommets points();

	plage_const_sommets points() const;
};

/* ************************************************************************** */

class ListePoints {
	std::vector<Sommet *> m_sommets{};

public:
	using plage_sommets = plage_iterable<std::vector<Sommet *>::iterator>;
	using plage_const_sommets = plage_iterable<std::vector<Sommet *>::const_iterator>;

	~ListePoints();

	void reinitialise();

	void redimensionne(const size_t nombre);

	void reserve(const size_t nombre);

	size_t taille() const;

	void pousse(Sommet *s);

	plage_sommets sommets();

	plage_const_sommets sommets() const;
};

/* ************************************************************************** */

class ListeSegments {
	std::vector<Arrete *> m_sommets{};

public:
	using plage_arretes = plage_iterable<std::vector<Arrete *>::iterator>;
	using plage_const_arretes = plage_iterable<std::vector<Arrete *>::const_iterator>;

	~ListeSegments();

	void reinitialise();

	void redimensionne(const size_t nombre);

	void reserve(const size_t nombre);

	size_t taille() const;

	void pousse(Arrete *s);

	plage_arretes arretes();

	plage_const_arretes arretes() const;
};

/* ************************************************************************** */

class ListePrimitives {
	std::vector<Primitive *> m_primitives{};

public:
	using plage_prims = plage_iterable<std::vector<Primitive *>::iterator>;
	using plage_const_prims = plage_iterable<std::vector<Primitive *>::const_iterator>;

	~ListePrimitives();

	void reinitialise();

	void redimensionne(const size_t nombre);

	void reserve(const size_t nombre);

	size_t taille() const;

	void pousse(Primitive *s);

	plage_prims prims();

	plage_const_prims prims() const;
};
