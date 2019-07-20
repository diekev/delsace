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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "vecteur.hh"

#include "biblinternes/structures/tableau.hh"

namespace dls::math {

class BruitPerlin2D {
protected:
	static const unsigned int N=128;
	vec2f m_basis[N];
	unsigned int m_perm[N];

	unsigned int hash_index(unsigned int i, unsigned int j) const;

public:
	explicit BruitPerlin2D(unsigned int seed=171717);
	void reinitialize(unsigned int seed);
	float operator()(float x, float y) const;
	float operator()(const vec2f &x) const;
};

/**
 * Long-Period Hash Functions for Procedural Texturing
 * http://graphics.cs.kuleuven.be/publications/LD06LPHFPT/LD06LPHFPT_paper.pdf
 */
class BruitPerlinLong2D {
	dls::tableau<unsigned int> m_table;

public:
	BruitPerlinLong2D();
	~BruitPerlinLong2D() = default;

	float operator()(float x, float y, float z) const;
	unsigned int hash(unsigned int x, unsigned int y) const;
};

class BruitPerlin3D {
protected:
	static const unsigned int N = 128;
	vec3f m_basis[N];
	unsigned int m_perm[N];

public:
	explicit BruitPerlin3D(unsigned int seed = 171717);

	void reinitialise(unsigned int seed);

	float operator()(float x, float y, float z) const;

	float operator()(const vec3f &x) const;

	unsigned int index_hash(unsigned int i, unsigned int j, unsigned int k) const;
};

class BruitFlux2D: public BruitPerlin2D {
protected:
	vec2f m_base_originelle[N];
	float m_taux_tournoiement[N];

public:
	BruitFlux2D(unsigned int graine = 171717, float variation_tournoiement = 0.2f);

	/* La période de répétition est approximativement de 1. */
	void change_temps(float temps);
};

class BruitFlux3D : public BruitPerlin3D {
protected:
	vec3f original_basis[N];
	float taux_tournoiement[N];
	vec3f axe_tournoiement[N];

public:
	BruitFlux3D(unsigned int graine = 171717, float variation_tournoiement = 0.2f);

	/* La période de répétition est approximativement de 1. */
	void change_temps(float temps);
};

float bruit_simplex_3d(float x, float y, float z);

class BruitCourbe2D {
	float m_temps = 0.0f;

	/* Utilisée pour estimer la différence fini de la courbe. */
	float m_delta_x = 1e-4f;

	vec2f m_centre_disque{ 0.7f, 0.5f };
	float m_rayon_disque = 1.0f;
	float m_influence_bruit = 0.25f;
	float ymin = 0.0f;
	float ymax = 0.75f;
	float m_initial_xmin = 1.5f;
	float m_initial_xmax = 3.5f;
	float m_vitesse_flux_arriere_plan = -0.25f;
	float m_expansion_sillage = 0.3f;

	/* Le premier élément est la longueur du bruit, le second son accroissement. */
	dls::tableau<std::pair<float, float>> m_echelle_longueur_accroissement_bruit = {};

	BruitFlux2D m_noise{};

public:
	BruitCourbe2D();

	virtual ~BruitCourbe2D() = default;

	float distance_solide(float x, float y) const;

	float potentiel(float x, float y) const;

	void avance_temps(float dt);

	void velocite(const vec2f &x, vec2f &v) const;

	float operator()(float x, float y) const;
};

}  /* namespace dls::math */
