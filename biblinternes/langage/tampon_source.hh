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

#include "biblinternes/structures/tableau.hh"
#include "biblinternes/structures/chaine.hh"

namespace lng {

class tampon_source {
	dls::chaine m_tampon{};
	dls::tableau<dls::vue_chaine> m_lignes{};

public:
	/**
	 * Construit une instance de tampon_source à partir d'une chaîne C
	 * terminée par zéro.
	 */
	explicit tampon_source(const char *chaine);

	/**
	 * Construit une instance de tampon_source avec une dls::chaine qui
	 * est 'bougée' dans la tampon. Après cette opération la dls::chaine
	 * passée en paramètre sera vide.
	 */
	explicit tampon_source(dls::chaine chaine) noexcept;

	/**
	 * Retourne un pointeur vers le début du tampon.
	 */
	const char *debut() const noexcept;

	/**
	 * Retourne un pointeur vers la fin du tampon.
	 */
	const char *fin() const noexcept;

	/**
	 * Retourne un dls::vue_chaine vers la ligne indiquée par l'index i
	 * spécifié. Aucune vérification pour savoir si i est dans la portée
	 * du tampon n'est effectuée, de sorte que si i n'est pas dans la
	 * portée, le programme crashera.
	 */
	dls::vue_chaine operator[](long i) const noexcept;

	/**
	 * Retourne le nombre de ligne dans le tampon.
	 */
	size_t nombre_lignes() const noexcept;

	/**
	 * Retourne la taille des données en octets du tampon.
	 */
	size_t taille_donnees() const noexcept;

private:
	/**
	 * Construit le vecteur contenant les données de chaque ligne du tampon.
	 */
	void construit_lignes();
};

}  /* namespace lng */
