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

#pragma once

#ifdef __cplusplus
extern "C" {
#else
typedef unsigned char bool;
#endif

/* Structure de rappels pour gérer les longs calculs. Ceci sert à interrompre au besoin lesdits
 * longs calculs. */
struct Interruptrice {
    void (*commence)(void *, const char *message);
    void (*termine)(void *);
    bool (*doit_interrompre)(void *, int pourcentage);
    void *donnees;
};

/* Structure servant principalement à passer des messages d'erreurs. */
struct ContexteEvaluation {
    void (*rapporte_erreur)(void *, const char *, long);
    void (*rapporte_avertissement)(void *, const char *, long);

    void *donnees_utilisateur;
};

/* Structure servant à rafiner les polygones n'étant ni des triangles, ni des quadrilatères. */
struct RafineusePolygone {
    void (*ajoute_triangle)(struct RafineusePolygone *, long v1, long v2, long v3);
    void (*ajoute_quadrilatere)(struct RafineusePolygone *, long v1, long v2, long v3, long v4);
};

enum TypeAttributGeo3D {
    BOOL,
    Z32,
    R32,
    VEC2,
    VEC3,
    VEC4,
    COULEUR,
};

struct AdaptriceAttribut {
#ifdef __cplusplus
    AdaptriceAttribut()
        : lis_bool_pour_index(nullptr), lis_entier_pour_index(nullptr),
          lis_reel_pour_index(nullptr), lis_vec2_pour_index(nullptr), lis_vec3_pour_index(nullptr),
          lis_vec4_pour_index(nullptr), lis_couleur_pour_index(nullptr),
          ecris_bool_a_l_index(nullptr), ecris_entier_a_l_index(nullptr),
          ecris_reel_a_l_index(nullptr), ecris_vec2_a_l_index(nullptr),
          ecris_vec3_a_l_index(nullptr), ecris_vec4_a_l_index(nullptr),
          ecris_couleur_a_l_index(nullptr), donnees_utilisateur(nullptr)
    {
    }

  protected:
#endif
    void (*lis_bool_pour_index)(void *, long, bool *);
    void (*lis_entier_pour_index)(void *, long, int *);
    void (*lis_reel_pour_index)(void *, long, float *);
    void (*lis_vec2_pour_index)(void *, long, float *);
    void (*lis_vec3_pour_index)(void *, long, float *);
    void (*lis_vec4_pour_index)(void *, long, float *);
    void (*lis_couleur_pour_index)(void *, long, float *);

    void (*ecris_bool_a_l_index)(void *, long, bool);
    void (*ecris_entier_a_l_index)(void *, long, int);
    void (*ecris_reel_a_l_index)(void *, long, float);
    void (*ecris_vec2_a_l_index)(void *, long, float *);
    void (*ecris_vec3_a_l_index)(void *, long, float *);
    void (*ecris_vec4_a_l_index)(void *, long, float *);
    void (*ecris_couleur_a_l_index)(void *, long, float *);

    void *donnees_utilisateur;

#ifdef __cplusplus
  public:
    operator bool() const
    {
        return this->donnees_utilisateur != nullptr;
    }
#endif
};

struct AdaptriceMaillage {
#ifdef __cplusplus
  protected:
#endif
    /* Interface pour les points. */

    long (*nombre_de_points)(void *);
    void (*point_pour_index)(void *, long n, float *x, float *y, float *z);
    void (*remplace_point_a_l_index)(void *, long n, float x, float y, float z);

    void (*reserve_nombre_de_points)(void *, long nombre);
    void (*ajoute_un_point)(void *, float x, float y, float z);
    void (*ajoute_plusieurs_points)(void *, float *points, long nombre);

    void (*ajoute_attribut_sur_points)(
        void *, enum TypeAttributGeo3D type, const char *, long, struct AdaptriceAttribut *);

    void (*accede_attribut_sur_points)(
        void *, enum TypeAttributGeo3D type, const char *, long, struct AdaptriceAttribut *);

    /* Interface pour les polygones. */

    long (*nombre_de_polygones)(void *);
    long (*nombre_de_sommets_polygone)(void *, long n);

    void (*point_pour_sommet_polygone)(void *, long p, long s, float *x, float *y, float *z);
    void (*index_points_sommets_polygone)(void *, long n, int *index);

    void (*calcule_normal_polygone)(void *, long p, float *nx, float *ny, float *nz);
    void (*reserve_nombre_de_polygones)(void *, long nombre);
    void (*ajoute_un_polygone)(void *, const int *sommets, int taille);
    void (*ajoute_liste_polygones)(void *,
                                   const int *sommets,
                                   const int *sommets_par_polygone,
                                   long nombre_polygones);

    void (*ajoute_attribut_sur_polygones)(
        void *, enum TypeAttributGeo3D type, const char *, long, struct AdaptriceAttribut *);

    void (*accede_attribut_sur_polygones)(
        void *, enum TypeAttributGeo3D type, const char *, long, struct AdaptriceAttribut *);

    void (*ajoute_attribut_sur_sommets_polygones)(
        void *, enum TypeAttributGeo3D type, const char *, long, struct AdaptriceAttribut *);

    void (*accede_attribut_sur_sommets_polygones)(
        void *, enum TypeAttributGeo3D type, const char *, long, struct AdaptriceAttribut *);

    /* Appelée si un polygone possède plus que 4 sommet afin que l'application cliente définissent
     * comment rafiner ces polygones. */
    void (*rafine_polygone)(void *, long n, const struct RafineusePolygone *);

    /* Interface pour les groupes. */

    void *(*cree_un_groupe_de_points)(void *donnees, const char *nom, long taille_nom);
    void *(*cree_un_groupe_de_polygones)(void *donnees, const char *nom, long taille_nom);
    void (*ajoute_au_groupe)(void *poignee_groupe, long index);
    void (*ajoute_plage_au_groupe)(void *poignee_groupe, long index_debut, long index_fin);
    bool (*groupe_polygone_possede_point)(const void *poignee_groupe, long index);

    /* Données générale sur le maillage. */

    void (*ajoute_attribut_sur_maillage)(
        void *, enum TypeAttributGeo3D type, const char *, long, struct AdaptriceAttribut *);

    void (*accede_attribut_sur_maillage)(
        void *, enum TypeAttributGeo3D type, const char *, long, struct AdaptriceAttribut *);

    /* Outils. */

    void (*calcule_boite_englobante)(void *,
                                     float *min_x,
                                     float *min_y,
                                     float *min_z,
                                     float *max_x,
                                     float *max_y,
                                     float *max_z);

    void *donnees;
};

/**
 * Crée une boîte avec les tailles spécifiées.
 */
void GEO3D_cree_boite(struct AdaptriceMaillage *adaptrice,
                      const float taille_x,
                      const float taille_y,
                      const float taille_z,
                      const float centre_x,
                      const float centre_y,
                      const float centre_z);

/**
 * Crée une sphere UV avec les paramètres spécifiés.
 */
void GEO3D_cree_sphere_uv(struct AdaptriceMaillage *adaptrice,
                          const float rayon,
                          int const resolution_u,
                          int const resolution_v,
                          const float centre_x,
                          const float centre_y,
                          const float centre_z);

/**
 * Crée un torus avec les paramètres spécifiés.
 */
void GEO3D_cree_torus(struct AdaptriceMaillage *adaptrice,
                      const float rayon_mineur,
                      const float rayon_majeur,
                      const long segment_mineur,
                      const long segment_majeur,
                      const float centre_x,
                      const float centre_y,
                      const float centre_z);

/**
 * Crée une grille avec les paramètres spécifiés.
 */
void GEO3D_cree_grille(struct AdaptriceMaillage *adaptrice,
                       const float taille_x,
                       const float taille_y,
                       const long lignes,
                       const long colonnes,
                       const float centre_x,
                       const float centre_y,
                       const float centre_z);

/**
 * Crée un cercle avec les paramètres spécifiés.
 */
void GEO3D_cree_cercle(struct AdaptriceMaillage *adaptrice,
                       const long segments,
                       const float rayon,
                       const float centre_x,
                       const float centre_y,
                       const float centre_z);

/**
 * Crée un cylindre avec les paramètres spécifiés.
 */
void GEO3D_cree_cylindre(struct AdaptriceMaillage *adaptrice,
                         const long segments,
                         const float rayon_mineur,
                         const float rayon_majeur,
                         const float profondeur,
                         const float centre_x,
                         const float centre_y,
                         const float centre_z);

/**
 * Crée une sphère ico avec les paramètres spécifiés.
 */
void GEO3D_cree_icosphere(struct AdaptriceMaillage *adaptrice,
                          const float rayon,
                          const int subdivision,
                          const float centre_x,
                          const float centre_y,
                          const float centre_z);

void GEO3D_importe_fichier_obj(struct AdaptriceMaillage *adaptrice,
                               const char *chemin,
                               long taille_chemin);

void GEO3D_exporte_fichier_obj(struct AdaptriceMaillage *adaptrice,
                               const char *chemin,
                               long taille_chemin);

void GEO3D_importe_fichier_stl(struct AdaptriceMaillage *adaptrice,
                               const char *chemin,
                               long taille_chemin);

struct ParametresFracture {
    bool periodique_x;
    bool periodique_y;
    bool periodique_z;

    bool utilise_rayon;
    const char *ptr_nom_attribut_rayon;
    long taille_nom_attribut_rayon;

    bool genere_groupes;
    const char *ptr_nom_base_groupe;
    long taille_nom_base_groupe;

    /** Crée un attribut sur chaque polygone contenant l'index de la cellule voisine.
     *  Cet index sera également celui du groupe, si les groupes sont générés.
     */
    bool cree_attribut_cellule_voisine;

    /**
     * Crée un attribut sur chaque polygone contenant le volume de la cellule dont
     * il fait partie.
     * À FAIRE : attributs sur les groupes plutôt.
     */
    bool cree_attribut_volume_cellule;

    /**
     * Crée un attribut sur chaque polygone contenant le centroide de la cellule dont
     * il fait partie.
     * À FAIRE : attributs sur les groupes plutôt.
     */
    bool cree_attribut_centroide;
};

void GEO3D_fracture_maillage(struct ParametresFracture *params,
                             struct AdaptriceMaillage *maillage_a_fracturer,
                             struct AdaptriceMaillage *nuage_de_points,
                             struct AdaptriceMaillage *maillage_sortie);

enum TypeOperationBooleenne {
    OP_BOOL_INTERSECTION,
    OP_BOOL_SOUSTRACTION,
    OP_BOOL_UNION,
};

bool GEO3D_performe_operation_booleenne(struct AdaptriceMaillage *maillage_a,
                                        struct AdaptriceMaillage *maillage_b,
                                        struct AdaptriceMaillage *maillage_sortie,
                                        enum TypeOperationBooleenne operation);

void GEO3D_test_conversion_polyedre(struct AdaptriceMaillage *maillage_entree,
                                    struct AdaptriceMaillage *maillage_sortie);

#ifdef __cplusplus
}
#endif
