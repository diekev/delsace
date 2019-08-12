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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <memory>

#include "biblinternes/math/matrice/matrice.hh"
#include "biblinternes/math/rectangle.hh"
#include "biblinternes/image/pixel.h"
#include "biblinternes/outils/iterateurs.h"
#include "biblinternes/structures/liste.hh"

#include "wolika/grille_dense.hh"

using type_image = dls::math::matrice_dyn<dls::image::Pixel<float>>;

/* ************************************************************************** */

namespace wlk {
enum class type_grille : int;
}

/* À FAIRE : déduplique la structure calque */
struct calque_image {
	dls::chaine nom{};
	wlk::base_grille_2d *tampon = nullptr;

	calque_image() = default;

	calque_image(calque_image const &autre);

	calque_image(calque_image &&autre);

	calque_image &operator=(calque_image const &autre);

	calque_image &operator=(calque_image &&autre);

	~calque_image();

	static calque_image construit_calque(wlk::base_grille_2d::type_desc const &desc, wlk::type_grille type_donnees);
};

/* ************************************************************************** */

struct Calque {
	dls::chaine nom{};
	type_image tampon{};

	/**
	 * Retourne la valeur du tampon de ce calque à la position <x, y>.
	 */
	dls::image::Pixel<float> valeur(long x, long y) const;

	/**
	 * Ajourne la valeur du tampon de ce calque à la position <x, y>.
	 */
	void valeur(long x, long y, dls::image::Pixel<float> const &pixel);

	/**
	 * Échantillonne le tampon de ce calque à la position <x, y> en utilisant
	 * une entrepolation linéaire entre les pixels se trouvant entre les quatre
	 * coins de la position spécifiée.
	 */
	dls::image::Pixel<float> echantillone(float x, float y) const;
};

void copie_donnees_calque(
		type_image const &tampon_de,
		type_image &tampon_vers);

/* ************************************************************************** */

struct Image {
private:
	using ptr_calque = std::shared_ptr<Calque>;
	using ptr_calque_profond = std::shared_ptr<calque_image>;

	dls::liste<ptr_calque> m_calques{};
	dls::chaine m_nom_calque{};


public:
	using plage_calques = dls::outils::plage_iterable<dls::liste<ptr_calque>::iteratrice>;
	using plage_calques_const = dls::outils::plage_iterable<dls::liste<ptr_calque>::const_iteratrice>;

	dls::liste<ptr_calque_profond> m_calques_profond{};
	bool est_profonde = false;

	~Image();

	/**
	 * Ajoute un calque à cette image avec le nom spécifié. La taille du calque
	 * est définie par le rectangle passé en paramètre. Retourne un pointeur
	 * vers le calque ajouté.
	 */
	Calque *ajoute_calque(dls::chaine const &nom, Rectangle const &rectangle);

	calque_image *ajoute_calque_profond(dls::chaine const &nom, wlk::desc_grille_2d const &desc, wlk::type_grille type);

	/**
	 * Retourne un pointeur vers le calque portant le nom passé en paramètre. Si
	 * aucun calque ne portant ce nom est trouvé, retourne nullptr.
	 */
	Calque *calque(dls::chaine const &nom) const;

	/**
	 * Retourne une plage itérable sur la liste de calques de cette Image.
	 */
	plage_calques calques();

	/**
	 * Retourne une plage itérable constante sur la liste de calques de cette Image.
	 */
	plage_calques_const calques() const;

	/**
	 * Vide la liste de calques de cette image. Si garde_memoires est faux,
	 * les calques seront supprimés. Cette méthode est à utiliser pour
	 * transférer la propriété des calques d'une image à une autre.
	 */
	void reinitialise();

	/**
	 * Renseigne le nom du calque actif.
	 */
	void nom_calque_actif(dls::chaine const &nom);

	/**
	 * Retourne le nom du calque actif.
	 */
	dls::chaine const &nom_calque_actif() const;

	calque_image *calque_profond(const dls::chaine &nom) const;
};
