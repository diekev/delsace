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

#include "biblinternes/structures/chaine.hh"

class ContexteRendu;
class TamponRendu;
class TextureImage;

namespace ftgl {
struct texture_atlas_t;
struct texture_font_t;
}

/**
 * La classe RenduTexte contient la logique pour rendre du texte à travers
 * OpenGL. Le texte est rendu en utilisant une texture dérivée d'une police qui
 * est projetée sur des polygones, à raison d'un quadrilatère (ou deux
 * triangles) par lettre.
 *
 * La texture est générée à partir du script python se trouvant dans le dossier
 * 'texte'.
 */
class RenduTexte {
	TamponRendu *m_tampon = nullptr;
	TextureImage *m_texture = nullptr;

	/* Initialisation à 1 pour éviter les divisions par 0. */
	int m_largeur = 1;
	int m_hauteur = 1;

	double m_decalage = 0.0;

	ftgl::texture_atlas_t *m_atlas = nullptr;
	ftgl::texture_font_t *m_font = nullptr;
	double m_taille_fonte = 16.0;

public:
	/**
	 * Construit une instance de RenduTexte avec des valeurs par défaut. Le
	 * tampon n'est pas initialisé.
	 */
	RenduTexte() = default;

	RenduTexte(RenduTexte const &) = default;
	RenduTexte &operator=(RenduTexte const &) = default;

	/**
	 * Détruit les données de l'instance. Les tampons de rendu sont détruits et
	 * utiliser l'instance crashera le programme.
	 */
	~RenduTexte();

	/**
	 * Ajourne les données du tampon avec le texte passé en paramètre. Cette
	 * fonction créé un nombre de polygones égal à la taille du texte et
	 * génère les coordonnées de projection UV de chaque lettre.
	 */
	void ajourne(dls::chaine const &texte);

	/**
	 * Dessine le texte passé en paramètre dans le contexte spécifié.
	 */
	void dessine(ContexteRendu const &contexte, dls::chaine const &texte);

	/**
	 * Établie les dimensions de la fenêtre où est dessinée le texte. Les
	 * dimensions sont nécessaires pour pouvoir définir la taille du texte
	 * relativiment aux dimensions de la fenêtre (c'est-à-dire qu'un texte de
	 * 13px de haut soit toujours déssiné sur 13px).
	 */
	void etablie_dimension_fenetre(int largeur, int hauteur);
	void reinitialise();
};
