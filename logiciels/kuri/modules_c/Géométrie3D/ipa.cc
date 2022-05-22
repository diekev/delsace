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

#include "ipa.h"

#include "acceleration.hh"
#include "booleen_maillage.hh"
#include "creation.h"
#include "fracture.hh"
#include "import_objet.h"
#include "outils.hh"
#include "triangulation.hh"

#define RETOURNE_SI_NUL(x)                                                                        \
    if (!x) {                                                                                     \
        return;                                                                                   \
    }

#define RETOURNE_FAUX_SI_NUL(x)                                                                   \
    if (!x) {                                                                                     \
        return false;                                                                             \
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
                          const int subdivision,
                          const float centre_x,
                          const float centre_y,
                          const float centre_z)
{
    geo::Maillage maillage = geo::Maillage::enveloppe(adaptrice);
    geo::cree_icosphere(maillage, rayon, subdivision, centre_x, centre_y, centre_z);
}

void GEO3D_importe_fichier_obj(AdaptriceMaillage *adaptrice,
                               const char *chemin,
                               long taille_chemin)
{
    geo::Maillage maillage = geo::Maillage::enveloppe(adaptrice);
    geo::charge_fichier_OBJ(maillage, vers_std_string(chemin, taille_chemin));
}

void GEO3D_exporte_fichier_obj(AdaptriceMaillage *adaptrice,
                               const char *chemin,
                               long taille_chemin)
{
    geo::Maillage maillage = geo::Maillage::enveloppe(adaptrice);
    geo::ecris_fichier_OBJ(maillage, vers_std_string(chemin, taille_chemin));
}

void GEO3D_importe_fichier_stl(AdaptriceMaillage *adaptrice,
                               const char *chemin,
                               long taille_chemin)
{
    geo::Maillage maillage = geo::Maillage::enveloppe(adaptrice);
    geo::charge_fichier_STL(maillage, vers_std_string(chemin, taille_chemin));
}

void GEO3D_fracture_maillage(struct ParametresFracture *params,
                             struct AdaptriceMaillage *maillage_a_fracturer,
                             struct AdaptriceMaillage *nuage_de_points,
                             struct AdaptriceMaillage *maillage_sortie)
{
    RETOURNE_SI_NUL(params)
    RETOURNE_SI_NUL(maillage_a_fracturer)
    RETOURNE_SI_NUL(nuage_de_points)
    RETOURNE_SI_NUL(maillage_sortie)

    geo::Maillage maillage_a_fracturer_ = geo::Maillage::enveloppe(maillage_a_fracturer);
    geo::Maillage nuage_de_points_ = geo::Maillage::enveloppe(nuage_de_points);
    geo::Maillage maillage_sortie_ = geo::Maillage::enveloppe(maillage_sortie);
    geo::fracture_maillage_voronoi(
        *params, maillage_a_fracturer_, nuage_de_points_, maillage_sortie_);
}

bool GEO3D_performe_operation_booleenne(struct AdaptriceMaillage *maillage_a,
                                        struct AdaptriceMaillage *maillage_b,
                                        struct AdaptriceMaillage *maillage_sortie,
                                        enum TypeOperationBooleenne operation)
{
    RETOURNE_FAUX_SI_NUL(maillage_a)
    RETOURNE_FAUX_SI_NUL(maillage_b)
    RETOURNE_FAUX_SI_NUL(maillage_sortie)

    std::string str_operation;
    if (operation == TypeOperationBooleenne::OP_BOOL_INTERSECTION) {
        str_operation = "INTER";
    }
    else if (operation == TypeOperationBooleenne::OP_BOOL_SOUSTRACTION) {
        str_operation = "MINUS";
    }
    else {
        str_operation = "UNION";
    }

    geo::Maillage maillage_a_ = geo::Maillage::enveloppe(maillage_a);
    geo::Maillage maillage_b_ = geo::Maillage::enveloppe(maillage_b);
    geo::Maillage maillage_sortie_ = geo::Maillage::enveloppe(maillage_sortie);

    return geo::booleen_maillages(maillage_a_, maillage_b_, str_operation, maillage_sortie_);
}

void GEO3D_test_conversion_polyedre(struct AdaptriceMaillage *maillage_entree,
                                    struct AdaptriceMaillage *maillage_sortie)
{
    RETOURNE_SI_NUL(maillage_entree)
    RETOURNE_SI_NUL(maillage_sortie)
    geo::Maillage maillage_entree_ = geo::Maillage::enveloppe(maillage_entree);
    geo::Maillage maillage_sortie_ = geo::Maillage::enveloppe(maillage_sortie);
    geo::test_conversion_polyedre(maillage_entree_, maillage_sortie_);
}

/* ************************************* */

struct HierarchieBoiteEnglobante *GEO3D_cree_hierarchie_boite_englobante(
    struct AdaptriceMaillage *pour_maillage)
{
    if (!pour_maillage) {
        return nullptr;
    }

    geo::Maillage maillage = geo::Maillage::enveloppe(pour_maillage);
    return geo::cree_hierarchie_boite_englobante(maillage);
}

void GEO3D_detruit_hierarchie_boite_englobante(HierarchieBoiteEnglobante *hbe)
{
    if (!hbe) {
        return;
    }

    memoire::deloge("HierarchieBoiteEnglobante", hbe);
}

void GEO3D_visualise_hierarchie_boite_englobante(struct HierarchieBoiteEnglobante *hbe,
                                                 int niveau,
                                                 struct AdaptriceMaillage *maillage_sortie)
{
    if (!hbe || !maillage_sortie) {
        return;
    }

    geo::Maillage maillage = geo::Maillage::enveloppe(maillage_sortie);
    geo::visualise_hierarchie_au_niveau(*hbe, niveau, maillage);
}

/* ************************************* */

void GEO3D_calcule_enveloppe_convexe(struct AdaptriceMaillage *maillage_entree,
                                     struct AdaptriceMaillage *maillage_sortie)
{
    RETOURNE_SI_NUL(maillage_entree)
    RETOURNE_SI_NUL(maillage_sortie)
    geo::Maillage maillage_entree_ = geo::Maillage::enveloppe(maillage_entree);
    geo::Maillage maillage_sortie_ = geo::Maillage::enveloppe(maillage_sortie);
    geo::calcule_enveloppe_convexe(maillage_entree_, maillage_sortie_);
}
