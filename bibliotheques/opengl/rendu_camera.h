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

class ContexteRendu;
class TamponRendu;

namespace vision {
class Camera3D;
}  /* namespace vision */

/**
 * La classe RenduCamera contient la logique de rendu d'un maillage dans la
 * scène 3D.
 */
class RenduCamera {
	TamponRendu *m_tampon_aretes = nullptr;
	TamponRendu *m_tampon_polys = nullptr;
	vision::Camera3D *m_camera = nullptr;

public:
	/**
	 * Construit une instance de RenduCamera pour le maillage spécifié.
	 */
	explicit RenduCamera(vision::Camera3D *camera);

	RenduCamera(RenduCamera const &) = default;
	RenduCamera &operator=(RenduCamera const &) = default;

	/**
	 * Détruit les données de l'instance. Les tampons de rendu sont détruits et
	 * utiliser l'instance crashera le programme.
	 */
	~RenduCamera();

	void initialise();

	/**
	 * Dessine le maillage dans le contexte spécifié.
	 */
	void dessine(const ContexteRendu &contexte);
};
