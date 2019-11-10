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

#include <filesystem>

#include "biblinternes/image/pixel.h"
#include "biblinternes/math/matrice/matrice.hh"
#include "biblinternes/math/vecteur.hh"

#include "biblinternes/phys/spectre.hh"

namespace vision {
class Camera3D;
}  /* namespace vision */

enum class TypeTexture {
	COULEUR,
	IMAGE,
};

enum {
	PROJECTION_PLANAIRE    = 0,
	PROJECTION_TRIPLANAIRE = 1,
	PROJECTION_CAMERA      = 2,
	PROJECTION_CUBIQUE     = 3,
	PROJECTION_CYLINDRIQUE = 4,
	PROJECTION_SPHERIQUE   = 5,
	PROJECTION_UV          = 6,
};

enum {
	ENVELOPPAGE_REPETITION         = 0,
	ENVELOPPAGE_REPETITION_MIRROIR = 1,
	ENVELOPPAGE_RESTRICTION        = 2,
};

enum {
	ENTREPOLATION_LINEAIRE         = 0,
	ENTREPOLATION_VOISINAGE_PROCHE = 1,
};

/* ************************************************************************** */

class Texture {
public:
	virtual ~Texture() = default;

	virtual Spectre echantillone(dls::math::vec3d const &direction) const = 0;

	virtual TypeTexture type() const = 0;
};

void supprime_texture(Texture *&texture);

/* ************************************************************************** */

class TextureCouleur final : public Texture {
	Spectre m_spectre = Spectre(0.0);

public:
	TextureCouleur() = default;

	void etablie_spectre(Spectre const &spectre);

	Spectre spectre() const;

	Spectre echantillone(dls::math::vec3d const &direction) const override;

	TypeTexture type() const override;
};

/* ************************************************************************** */

class TextureImage final : public Texture {
	dls::math::matrice_dyn<Spectre> m_image{};
	std::filesystem::path m_chemin{};
	vision::Camera3D *m_camera = nullptr;

	int m_projection = PROJECTION_PLANAIRE;
	int m_entrepolation = ENTREPOLATION_LINEAIRE;
	int m_enveloppage = ENVELOPPAGE_REPETITION;

	dls::math::vec3f m_taille = dls::math::vec3f(1.0f);

public:
	TextureImage() = default;

	TextureImage(TextureImage const &) = default;
	TextureImage &operator=(TextureImage const &) = default;

	Spectre echantillone(dls::math::vec3d const &direction) const override;

	void etablie_image(dls::math::matrice_dyn<Spectre> const &image);

	SpectreRGB *donnees();
	int largeur();
	int hauteur();

	void projection(int p);

	int projection() const;

	void entrepolation(int i);

	int entrepolation() const;

	void enveloppage(int e);

	int enveloppage() const;

	void camera(vision::Camera3D *p);

	dls::math::vec3f taille() const;

	void taille(dls::math::vec3f const &taille);

	vision::Camera3D *camera() const;

	void etablie_chemin(std::filesystem::path const &chemin);

	std::filesystem::path const &chemin() const;

	TypeTexture type() const override;

	Spectre echantillone_uv(int x, int y);

	void charge_donnees(dls::math::matrice_dyn<dls::image::PixelFloat> const &donnees);
};

TextureImage *charge_texture(std::filesystem::path const &chemin);
