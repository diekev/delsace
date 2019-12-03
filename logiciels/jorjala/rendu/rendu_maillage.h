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

namespace kdo {
class Maillage;
class Scene;
}

class ContexteRendu;
class TamponRendu;

/**
 * La classe RenduMaillage contient la logique de rendu d'un maillage dans la
 * scène 3D.
 */
class RenduMaillage {
	TamponRendu *m_tampon_surface = nullptr;
	TamponRendu *m_tampon_normal = nullptr;
	kdo::Maillage *m_maillage = nullptr;

public:
	/**
	 * Construit une instance de RenduMaillage pour le maillage spécifié.
	 */
	explicit RenduMaillage(kdo::Maillage *maillage);

	RenduMaillage(RenduMaillage const &) = default;
	RenduMaillage &operator=(RenduMaillage const &) = default;

	/**
	 * Détruit les données de l'instance. Les tampons de rendu sont détruits et
	 * utiliser l'instance crashera le programme.
	 */
	~RenduMaillage();

	void initialise();

	/**
	 * Dessine le maillage dans le contexte spécifié.
	 */
	void dessine(ContexteRendu const &contexte, kdo::Scene const &scene);

	/**
	 * Retourne la matrice du maillage.
	 */
	dls::math::mat4x4d matrice() const;

private:
	void genere_tampon_surface();
	void genere_tampon_normal();
};
