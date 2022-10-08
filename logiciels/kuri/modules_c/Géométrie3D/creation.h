/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#pragma once

namespace geo {

class Maillage;

/**
 * Crée une boîte avec les tailles spécifiées.
 */
void cree_boite(Maillage &maillage,
                const float taille_x,
                const float taille_y,
                const float taille_z,
                const float centre_x = 0.0f,
                const float centre_y = 0.0f,
                const float centre_z = 0.0f);

/**
 * Crée une sphere UV avec les paramètres spécifiés.
 */
void cree_sphere_uv(Maillage &maillage,
                    const float rayon,
                    int const resolution_u,
                    int const resolution_v,
                    const float centre_x = 0.0f,
                    const float centre_y = 0.0f,
                    const float centre_z = 0.0f);

/**
 * Crée un torus avec les paramètres spécifiés.
 */
void cree_torus(Maillage &maillage,
                const float rayon_mineur,
                const float rayon_majeur,
                const long segment_mineur,
                const long segment_majeur,
                const float centre_x = 0.0f,
                const float centre_y = 0.0f,
                const float centre_z = 0.0f);

/**
 * Crée une grille avec les paramètres spécifiés.
 */
void cree_grille(Maillage &maillage,
                 const float taille_x,
                 const float taille_y,
                 const long lignes,
                 const long colonnes,
                 const float centre_x = 0.0f,
                 const float centre_y = 0.0f,
                 const float centre_z = 0.0f);

/**
 * Crée un cercle avec les paramètres spécifiés.
 */
void cree_cercle(Maillage &maillage,
                 const long segments,
                 const float rayon,
                 const float centre_x = 0.0f,
                 const float centre_y = 0.0f,
                 const float centre_z = 0.0f);

/**
 * Crée un cylindre avec les paramètres spécifiés.
 */
void cree_cylindre(Maillage &maillage,
                   const long segments,
                   const float rayon_mineur,
                   const float rayon_majeur,
                   const float profondeur,
                   const float centre_x = 0.0f,
                   const float centre_y = 0.0f,
                   const float centre_z = 0.0f);

/**
 * Crée une sphère ico avec les paramètres spécifiés.
 */
void cree_icosphere(Maillage &maillage,
                    const float rayon,
                    const int subdivision,
                    const float centre_x = 0.0f,
                    const float centre_y = 0.0f,
                    const float centre_z = 0.0f);

}  // namespace geo
