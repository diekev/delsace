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

#include <experimental/filesystem>

class ConteneurControles;
class Manipulable;
class RepondantBouton;
class QBoxLayout;
class QMenu;

struct DonneesInterface {
	Manipulable *manipulable;
	RepondantBouton *repondant_bouton;
	ConteneurControles *conteneur;
};

QMenu *compile_menu(DonneesInterface &donnnes, const char *texte_entree);

/**
 * Compile le script d'interface contenu dans texte_entree, et retourne un
 * pointeur vers le QBoxLayout ainsi créé.
 */
QBoxLayout *compile_interface(
		DonneesInterface &donnnes,
		const char *texte_entree);

/**
 * Compile le script d'interface contenu dans le fichier dont le chemin est
 * spécifié, et retourne un pointeur vers le QBoxLayout ainsi créé.
 */
QBoxLayout *compile_interface(
		DonneesInterface &donnnes,
		const std::experimental::filesystem::path &chemin_texte);
