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

#include "biblinternes/graphe/graphe.h"
#include "biblinternes/math/transformation.hh"
#include "biblinternes/moultfilage/synchronise.hh"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/vision/camera.h"

#include "danjo/manipulable.h"

#include "corps/corps.h"

enum class type_objet : char {
	NUL,
	CORPS,
	CAMERA,
};

struct DonneesObjet {};

struct DonneesCorps : public DonneesObjet {
	Corps corps{};
};

struct DonneesCamera : public DonneesObjet {
	vision::Camera3D camera;

	DonneesCamera(int largeur, int hauteur)
		: camera(largeur, hauteur)
	{
		camera.ajourne();
	}
};

struct Objet : public danjo::Manipulable {
	type_objet type = type_objet::NUL;

	bool rendu_scene = true;

	/* transformation */
	math::transformation transformation = math::transformation();
	dls::math::point3f pivot        = dls::math::point3f(0.0f);
	dls::math::point3f position     = dls::math::point3f(0.0f);
	dls::math::point3f echelle      = dls::math::point3f(1.0f);
	dls::math::point3f rotation     = dls::math::point3f(0.0f);
	float echelle_uniforme              = 1.0f;

	/* autres propriétés */
	dls::chaine nom = "objet";

	dls::synchronise<DonneesObjet *> donnees{};

	Graphe graphe;

	Objet();
	~Objet() override;

	Objet(Objet const &) = default;
	Objet &operator=(Objet const &) = default;

	void performe_versionnage() override;

	const char *chemin_entreface() const;

	void ajourne_parametres();
};

inline Objet *extrait_objet(std::any const &any)
{
	return std::any_cast<Objet *>(any);
}
