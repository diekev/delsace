﻿/*
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
#include "biblinternes/structures/tableau.hh"

class ContexteRendu;
class TamponRendu;

namespace JJL {
class Corps;
}

struct StatistiquesRendu {
	long nombre_objets = 0;
	long nombre_polygones = 0;
	long nombre_polylignes = 0;
	long nombre_volumes = 0;
	long nombre_points = 0;
	double temps = 0.0;
};

/**
 * La classe RenduCorps contient la logique de rendu d'un corps dans la
 * scène 3D.
 */
class RenduCorps {
	TamponRendu *m_tampon_points = nullptr;
	TamponRendu *m_tampon_polygones = nullptr;
	TamponRendu *m_tampon_segments = nullptr;
	TamponRendu *m_tampon_volume = nullptr;

    JJL::Corps &m_corps;

public:
	/**
	 * RenduCorps une instance de RenduMaillage pour le maillage spécifié.
	 */
    explicit RenduCorps(JJL::Corps &corps);

	RenduCorps(RenduCorps const &) = default;
	RenduCorps &operator=(RenduCorps const &) = default;

	/**
	 * Détruit les données de l'instance. Les tampons de rendu sont détruits et
	 * utiliser l'instance crashera le programme.
	 */
	~RenduCorps();

	void initialise(ContexteRendu const &contexte,
					StatistiquesRendu &stats,
					dls::tableau<dls::math::mat4x4f> &matrices);

	/**
	 * Dessine le maillage dans le contexte spécifié.
	 */
	void dessine(ContexteRendu const &contexte);
};
