/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

#include <stdint.h>

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
    void (*rapporte_erreur)(void *, const char *, int64_t);
    void (*rapporte_avertissement)(void *, const char *, int64_t);

    void *donnees_utilisateur;
};

/* Structure servant à rafiner les polygones n'étant ni des triangles, ni des quadrilatères. */
struct RafineusePolygone {
    void (*ajoute_triangle)(struct RafineusePolygone *, int64_t v1, int64_t v2, int64_t v3);
    void (*ajoute_quadrilatere)(
        struct RafineusePolygone *, int64_t v1, int64_t v2, int64_t v3, int64_t v4);
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
    void (*lis_bool_pour_index)(void *, int64_t, bool *);
    void (*lis_entier_pour_index)(void *, int64_t, int *);
    void (*lis_reel_pour_index)(void *, int64_t, float *);
    void (*lis_vec2_pour_index)(void *, int64_t, float *);
    void (*lis_vec3_pour_index)(void *, int64_t, float *);
    void (*lis_vec4_pour_index)(void *, int64_t, float *);
    void (*lis_couleur_pour_index)(void *, int64_t, float *);

    void (*ecris_bool_a_l_index)(void *, int64_t, bool);
    void (*ecris_entier_a_l_index)(void *, int64_t, int);
    void (*ecris_reel_a_l_index)(void *, int64_t, float);
    void (*ecris_vec2_a_l_index)(void *, int64_t, float *);
    void (*ecris_vec3_a_l_index)(void *, int64_t, float *);
    void (*ecris_vec4_a_l_index)(void *, int64_t, float *);
    void (*ecris_couleur_a_l_index)(void *, int64_t, float *);

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

    int64_t (*nombre_de_points)(void *);
    void (*point_pour_index)(void *, int64_t n, float *x, float *y, float *z);
    void (*remplace_point_a_l_index)(void *, int64_t n, float x, float y, float z);

    void (*reserve_nombre_de_points)(void *, int64_t nombre);
    void (*ajoute_un_point)(void *, float x, float y, float z);
    void (*ajoute_plusieurs_points)(void *, float *points, int64_t nombre);

    void (*ajoute_attribut_sur_points)(
        void *, enum TypeAttributGeo3D type, const char *, int64_t, struct AdaptriceAttribut *);

    void (*accede_attribut_sur_points)(
        void *, enum TypeAttributGeo3D type, const char *, int64_t, struct AdaptriceAttribut *);

    /* Interface pour les polygones. */

    int64_t (*nombre_de_polygones)(void *);
    int64_t (*nombre_de_sommets_polygone)(void *, int64_t n);

    void (*point_pour_sommet_polygone)(void *, int64_t p, int64_t s, float *x, float *y, float *z);
    void (*index_points_sommets_polygone)(void *, int64_t n, int *index);

    void (*calcule_normal_polygone)(void *, int64_t p, float *nx, float *ny, float *nz);
    void (*reserve_nombre_de_polygones)(void *, int64_t nombre);
    void (*ajoute_un_polygone)(void *, const int *sommets, int taille);
    void (*ajoute_liste_polygones)(void *,
                                   const int *sommets,
                                   const int *sommets_par_polygone,
                                   int64_t nombre_polygones);

    void (*ajoute_attribut_sur_polygones)(
        void *, enum TypeAttributGeo3D type, const char *, int64_t, struct AdaptriceAttribut *);

    void (*accede_attribut_sur_polygones)(
        void *, enum TypeAttributGeo3D type, const char *, int64_t, struct AdaptriceAttribut *);

    void (*ajoute_attribut_sur_sommets_polygones)(
        void *, enum TypeAttributGeo3D type, const char *, int64_t, struct AdaptriceAttribut *);

    void (*accede_attribut_sur_sommets_polygones)(
        void *, enum TypeAttributGeo3D type, const char *, int64_t, struct AdaptriceAttribut *);

    /* Appelée si un polygone possède plus que 4 sommet afin que l'application cliente définissent
     * comment rafiner ces polygones. */
    void (*rafine_polygone)(void *, int64_t n, const struct RafineusePolygone *);

    /* Interface pour les groupes. */

    void *(*cree_un_groupe_de_points)(void *donnees, const char *nom, int64_t taille_nom);
    void *(*cree_un_groupe_de_polygones)(void *donnees, const char *nom, int64_t taille_nom);
    void (*ajoute_au_groupe)(void *poignee_groupe, int64_t index);
    void (*ajoute_plage_au_groupe)(void *poignee_groupe, int64_t index_debut, int64_t index_fin);
    bool (*groupe_polygone_possede_point)(const void *poignee_groupe, int64_t index);

    /* Données générale sur le maillage. */

    void (*ajoute_attribut_sur_maillage)(
        void *, enum TypeAttributGeo3D type, const char *, int64_t, struct AdaptriceAttribut *);

    void (*accede_attribut_sur_maillage)(
        void *, enum TypeAttributGeo3D type, const char *, int64_t, struct AdaptriceAttribut *);

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
                      const int64_t segment_mineur,
                      const int64_t segment_majeur,
                      const float centre_x,
                      const float centre_y,
                      const float centre_z);

/**
 * Crée une grille avec les paramètres spécifiés.
 */
void GEO3D_cree_grille(struct AdaptriceMaillage *adaptrice,
                       const float taille_x,
                       const float taille_y,
                       const int64_t lignes,
                       const int64_t colonnes,
                       const float centre_x,
                       const float centre_y,
                       const float centre_z);

/**
 * Crée un cercle avec les paramètres spécifiés.
 */
void GEO3D_cree_cercle(struct AdaptriceMaillage *adaptrice,
                       const int64_t segments,
                       const float rayon,
                       const float centre_x,
                       const float centre_y,
                       const float centre_z);

/**
 * Crée un cylindre avec les paramètres spécifiés.
 */
void GEO3D_cree_cylindre(struct AdaptriceMaillage *adaptrice,
                         const int64_t segments,
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
                               int64_t taille_chemin);

void GEO3D_exporte_fichier_obj(struct AdaptriceMaillage *adaptrice,
                               const char *chemin,
                               int64_t taille_chemin);

void GEO3D_importe_fichier_stl(struct AdaptriceMaillage *adaptrice,
                               const char *chemin,
                               int64_t taille_chemin);

struct ParametresFracture {
    bool periodique_x;
    bool periodique_y;
    bool periodique_z;

    bool utilise_rayon;
    const char *ptr_nom_attribut_rayon;
    int64_t taille_nom_attribut_rayon;

    bool genere_groupes;
    const char *ptr_nom_base_groupe;
    int64_t taille_nom_base_groupe;

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

/* ************************************* */

struct HierarchieBoiteEnglobante;

struct HierarchieBoiteEnglobante *GEO3D_cree_hierarchie_boite_englobante(
    struct AdaptriceMaillage *pour_maillage);

void GEO3D_detruit_hierarchie_boite_englobante(struct HierarchieBoiteEnglobante *hbe);

void GEO3D_visualise_hierarchie_boite_englobante(struct HierarchieBoiteEnglobante *hbe,
                                                 int niveau,
                                                 struct AdaptriceMaillage *maillage_sortie);

/* ************************************* */

void GEO3D_calcule_enveloppe_convexe(struct AdaptriceMaillage *maillage_entree,
                                     struct AdaptriceMaillage *maillage_sortie);

/* ************************************* */

enum DeterminationQuantitePoints {
    DET_QT_PNT_PAR_NOMBRE_ABSOLU,
    DET_QT_PNT_PAR_DISTANCE,
};

enum TypeRayonnementPoint {
    RAYONNEMENT_UNIFORME,
    RAYONNEMENT_ALEATOIRE,
};

struct ParametreDistributionParticules {
    int graine;

    enum DeterminationQuantitePoints determination_quantite_points;

    int nombre_absolu;

    enum TypeRayonnementPoint type_rayonnement;

    /* Distance minimale entre deux points. */
    float distance_minimale;
    float distance_maximale;

    /* Paramètres pour contenir la génération de particules aux primitives d'un groupe. */
    bool utilise_groupe;
    const char *ptr_nom_groupe_primitive;
    int64_t taille_nom_groupe_primitive;

    /* Paramètres pour exporter un attribut pour les rayons. */
    bool exporte_rayon;
    const char *ptr_nom_rayon;
    int64_t taille_nom_rayon;
};

void GEO3D_distribue_particules_sur_surface(struct ParametreDistributionParticules *params,
                                            struct AdaptriceMaillage *surface,
                                            struct AdaptriceMaillage *points_resultants);

struct ParametresDistributionPoisson2D {
    /* Graine pour ensemmencer le générateur de nombre aléatoire. */
    unsigned graine;

    /* Distance minimale à respecter entre deux points. */
    float distance_minimale;

    /* Longueur de la zone à remplir. */
    float longueur;

    /* Largeur de la zone à remplir. */
    float largeur;

    /* Point de départ de la génération des points. */
    float origine_x;
    float origine_y;

    /* Rappel pour déterminer si un point peut être ajouté à la coordonnée définie les paramètres x
     * et y. Ceci peut par exemple être utile pour exclure des points se situant en dehors d'un
     * polygone non rectangulaire. */
    bool (*peut_ajouter_point)(void *, float x, float y);

    /* Données utilisateur pour le rappel. */
    void *donnees_utilisateur;
};

void GEO3D_distribue_points_poisson_2d(struct ParametresDistributionPoisson2D *params,
                                       struct AdaptriceMaillage *points_resultants);

void GEO3D_construit_maillage_alpha(struct AdaptriceMaillage *points,
                                    const float rayon,
                                    struct AdaptriceMaillage *maillage_résultat);

void GEO3D_triangulation_delaunay_2d_points_3d(struct AdaptriceMaillage *points,
                                               struct AdaptriceMaillage *résultat);

/* ************************************* */

struct ParametresErosionVent {
    float direction;
    int repetitions;
    float erosion_amont;
    float erosion_avale;
};

struct AdaptriceTerrain {
    void (*accede_resolution)(struct AdaptriceTerrain *donnees, int *res_x, int *res_y);

    void (*accede_taille)(struct AdaptriceTerrain *donnees, float *taille_x, float *taille_y);

    void (*accede_position)(struct AdaptriceTerrain *donnees, float *x, float *y, float *z);

    void (*accede_pointeur_donnees)(struct AdaptriceTerrain *terrain, float **pointeur_donnees);
};

void GEO3D_simule_erosion_vent(struct ParametresErosionVent *params,
                               struct AdaptriceTerrain *terrain,
                               struct AdaptriceTerrain *terrain_pour_facteur);

struct ParametresInclinaisonTerrain {
    float facteur;
    float decalage;
    bool inverse;
};

void GEO3D_incline_terrain(struct ParametresInclinaisonTerrain const *params,
                           struct AdaptriceTerrain *terrain);

enum TypeFiltreTerrain {
    BOITE,
    TRIANGULAIRE,
    QUADRATIC,
    CUBIQUE,
    GAUSSIEN,
    MITCHELL,
    CATROM,
};

struct ParametresFiltrageTerrain {
    enum TypeFiltreTerrain type;
    float rayon;
};

void GEO3D_filtrage_terrain(struct ParametresFiltrageTerrain const *params,
                            struct AdaptriceTerrain *terrain);

struct ParametresErosionSimple {
    int iterations;
    bool inverse;
    bool superficielle;
    bool rugueux;
    bool pente;
};

void GEO3D_erosion_simple(struct ParametresErosionSimple const *params,
                          struct AdaptriceTerrain *terrain,
                          struct AdaptriceTerrain *grille_poids);

struct ParametresErosionComplexe {
    int iterations;

    /* Paramètres pour l'érosion hydraulique (rivières). */
    int iterations_rivieres;
    float quantite_pluie;
    float variance_pluie;
    float cap_trans;
    float permea_sol;
    float taux_sedimentation;
    float dep_pente;
    float evaporation;
    float taux_fluvial;

    /* Paramètre pour l'érosion d'avalanche. */
    int iterations_avalanche;
    float angle_talus;
    float quantite_avale;

    /* Paramètre pour l'érosion générale. */
    int iterations_diffusion;
    float diffusion_thermale;
};

void GEO3D_erosion_complexe(struct ParametresErosionComplexe *params,
                            struct AdaptriceTerrain *terrain);

struct ParametresProjectionTerrain {
    float distance_max;

    bool utilise_touche_la_plus_eloignee;
};

void GEO3D_projette_geometrie_sur_terrain(struct ParametresProjectionTerrain const *params,
                                          struct AdaptriceTerrain *terrain,
                                          struct AdaptriceMaillage *geometrie);

#ifdef __cplusplus
}
#endif
