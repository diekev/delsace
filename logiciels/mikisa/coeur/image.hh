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

#include "biblinternes/math/matrice/matrice.hh"
#include "biblinternes/math/rectangle.hh"
#include "biblinternes/image/pixel.h"
#include "biblinternes/outils/iterateurs.h"
#include "biblinternes/structures/liste.hh"

using type_image = dls::math::matrice_dyn<dls::image::Pixel<float>>;

/* ************************************************************************** */

struct Calque {
	dls::chaine nom{};
	type_image tampon{};

	/**
	 * Retourne la valeur du tampon de ce calque à la position <x, y>.
	 */
	dls::image::Pixel<float> valeur(size_t x, size_t y) const;

	/**
	 * Ajourne la valeur du tampon de ce calque à la position <x, y>.
	 */
	void valeur(size_t x, size_t y, dls::image::Pixel<float> const &pixel);

	/**
	 * Échantillonne le tampon de ce calque à la position <x, y> en utilisant
	 * une entrepolation linéaire entre les pixels se trouvant entre les quatre
	 * coins de la position spécifiée.
	 */
	dls::image::Pixel<float> echantillone(float x, float y) const;
};

/* ************************************************************************** */

struct Image {
private:
	dls::liste<Calque *> m_calques{};
	dls::chaine m_nom_calque{};

public:
	using plage_calques = dls::outils::plage_iterable<dls::liste<Calque *>::iteratrice>;
	using plage_calques_const = dls::outils::plage_iterable<dls::liste<Calque *>::const_iteratrice>;

	~Image();

	/**
	 * Ajoute un calque à cette image avec le nom spécifié. La taille du calque
	 * est définie par le rectangle passé en paramètre. Retourne un pointeur
	 * vers le calque ajouté.
	 */
	Calque *ajoute_calque(dls::chaine const &nom, Rectangle const &rectangle);

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
	void reinitialise(bool garde_memoires = false);

	/**
	 * Renseigne le nom du calque actif.
	 */
	void nom_calque_actif(dls::chaine const &nom);

	/**
	 * Retourne le nom du calque actif.
	 */
	dls::chaine const &nom_calque_actif() const;
};
