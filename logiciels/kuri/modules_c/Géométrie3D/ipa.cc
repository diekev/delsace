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
 * The Original Code is Copyright (C) 2022 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "ipa.hh"

#include "creation.h"
#include "import_objet.h"
#include "outils.hh"

static std::string vers_std_string(const char *chemin, long taille_chemin)
{
    return std::string(chemin, static_cast<size_t>(taille_chemin));
}

/**
 * Crée une boîte avec les tailles spécifiées.
 */
void GEO3D_cree_boite(AdaptriceMaillage *adaptrice,
                      const float taille_x,
                      const float taille_y,
                      const float taille_z,
                      const float centre_x,
                      const float centre_y,
                      const float centre_z)
{
    geo::Maillage maillage = geo::Maillage::enveloppe(adaptrice);
    cree_boite(maillage, taille_x, taille_y, taille_z, centre_x, centre_y, centre_z);
}

/**
 * Crée une sphere UV avec les paramètres spécifiés.
 */
void GEO3D_cree_sphere_uv(AdaptriceMaillage *adaptrice,
                          const float rayon,
                          int const resolution_u,
                          int const resolution_v,
                          const float centre_x,
                          const float centre_y,
                          const float centre_z)
{
    geo::Maillage maillage = geo::Maillage::enveloppe(adaptrice);
    geo::cree_sphere_uv(maillage, rayon, resolution_u, resolution_v, centre_x, centre_y, centre_z);
}

/**
 * Crée un torus avec les paramètres spécifiés.
 */
void GEO3D_cree_torus(AdaptriceMaillage *adaptrice,
                      const float rayon_mineur,
                      const float rayon_majeur,
                      const long segment_mineur,
                      const long segment_majeur,
                      const float centre_x,
                      const float centre_y,
                      const float centre_z)
{
    geo::Maillage maillage = geo::Maillage::enveloppe(adaptrice);
    geo::cree_torus(maillage,
                    rayon_mineur,
                    rayon_majeur,
                    segment_mineur,
                    segment_majeur,
                    centre_x,
                    centre_y,
                    centre_z);
}

/**
 * Crée une grille avec les paramètres spécifiés.
 */
void GEO3D_cree_grille(AdaptriceMaillage *adaptrice,
                       const float taille_x,
                       const float taille_y,
                       const long lignes,
                       const long colonnes,
                       const float centre_x,
                       const float centre_y,
                       const float centre_z)
{
    geo::Maillage maillage = geo::Maillage::enveloppe(adaptrice);
    geo::cree_grille(maillage, taille_x, taille_y, lignes, colonnes, centre_x, centre_y, centre_z);
}

/**
 * Crée un cercle avec les paramètres spécifiés.
 */
void GEO3D_cree_cercle(AdaptriceMaillage *adaptrice,
                       const long segments,
                       const float rayon,
                       const float centre_x,
                       const float centre_y,
                       const float centre_z)
{
    geo::Maillage maillage = geo::Maillage::enveloppe(adaptrice);
    geo::cree_cercle(maillage, segments, rayon, centre_x, centre_y, centre_z);
}

/**
 * Crée un cylindre avec les paramètres spécifiés.
 */
void GEO3D_cree_cylindre(AdaptriceMaillage *adaptrice,
                         const long segments,
                         const float rayon_mineur,
                         const float rayon_majeur,
                         const float profondeur,
                         const float centre_x,
                         const float centre_y,
                         const float centre_z)
{
    geo::Maillage maillage = geo::Maillage::enveloppe(adaptrice);
    geo::cree_cylindre(
        maillage, segments, rayon_mineur, rayon_majeur, profondeur, centre_x, centre_y, centre_z);
}

/**
 * Crée une sphère ico avec les paramètres spécifiés.
 */
void GEO3D_cree_icosphere(AdaptriceMaillage *adaptrice,
                          const float rayon,
                          const float centre_x,
                          const float centre_y,
                          const float centre_z)
{
    geo::Maillage maillage = geo::Maillage::enveloppe(adaptrice);
    geo::cree_icosphere(maillage, rayon, centre_x, centre_y, centre_z);
}

void GEO3D_importe_fichier_obj(AdaptriceMaillage *adaptrice,
                               const char *chemin,
                               long taille_chemin)
{
    geo::Maillage maillage = geo::Maillage::enveloppe(adaptrice);
    geo::charge_fichier_OBJ(maillage, vers_std_string(chemin, taille_chemin));
}

void GEO3D_importe_fichier_stl(AdaptriceMaillage *adaptrice,
                               const char *chemin,
                               long taille_chemin)
{
    geo::Maillage maillage = geo::Maillage::enveloppe(adaptrice);
    geo::charge_fichier_STL(maillage, vers_std_string(chemin, taille_chemin));
}
