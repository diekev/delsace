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

namespace objets {

class AdaptriceCreationObjet;

/**
 * Crée une boîte avec les tailles spécifiées.
 */
void cree_boite(AdaptriceCreationObjet *adaptrice,
				const float taille_x,
				const float taille_y,
				const float taille_z,
				const float centre_x = 0.0f,
				const float centre_y = 0.0f,
				const float centre_z = 0.0f);

/**
 * Crée une sphere UV avec les paramètres spécifiés.
 */
void cree_sphere_uv(AdaptriceCreationObjet *adaptrice,
					const float rayon,
					const float centre_x = 0.0f,
					const float centre_y = 0.0f,
					const float centre_z = 0.0f);

/**
 * Crée un torus avec les paramètres spécifiés.
 */
void cree_torus(AdaptriceCreationObjet *adaptrice,
				const float rayon_mineur,
				const float rayon_majeur,
				const unsigned long segment_mineur,
				const unsigned long segment_majeur,
				const float centre_x = 0.0f,
				const float centre_y = 0.0f,
				const float centre_z = 0.0f);

/**
 * Crée une grille avec les paramètres spécifiés.
 */
void cree_grille(AdaptriceCreationObjet *adaptrice,
				 const float taille_x,
				 const float taille_y,
				 const unsigned long lignes,
				 const unsigned long colonnes,
				 const float centre_x = 0.0f,
				 const float centre_y = 0.0f,
				 const float centre_z = 0.0f);

/**
 * Crée un cercle avec les paramètres spécifiés.
 */
void cree_cercle(AdaptriceCreationObjet *adaptrice,
				 const unsigned long segments,
				 const float rayon,
				 const float centre_x = 0.0f,
				 const float centre_y = 0.0f,
				 const float centre_z = 0.0f);

/**
 * Crée un cylindre avec les paramètres spécifiés.
 */
void cree_cylindre(AdaptriceCreationObjet *adaptrice,
				   const unsigned long segments,
				   const float rayon_mineur,
				   const float rayon_majeur,
				   const float profondeur,
				   const float centre_x = 0.0f,
				   const float centre_y = 0.0f,
				   const float centre_z = 0.0f);

/**
 * Crée une sphère ico avec les paramètres spécifiés.
 */
void cree_icosphere(AdaptriceCreationObjet *adaptrice,
					const float rayon,
					const float centre_x = 0.0f,
					const float centre_y = 0.0f,
					const float centre_z = 0.0f);


}  /* namespace objets */
