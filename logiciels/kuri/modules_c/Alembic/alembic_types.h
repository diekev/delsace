/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
typedef unsigned short r16;
#else
typedef unsigned char bool;
typedef unsigned short r16;
#endif

struct ArchiveCache;
struct AutriceArchive;
struct ContexteKuri;
struct EcrivainCache;
struct LectriceCache;

// --------------------------------------------------------------
// Ouverture de l'archive.

typedef enum eAbcPoliceErreur {
    SILENCIEUSE,
    BRUYANTE,
    LANCE_EXCEPTION,
} eAbcPoliceErreur;

typedef enum eAbcStrategieLectureOgawa {
    FLUX_DE_FICHIERS,
    FICHIERS_MAPPES_MEMOIRE,
} eAbcStrategieLectureOgawa;

/* Paramètres pour la bonne ouverture d'une archive Alembic. */
struct ContexteOuvertureArchive {
    /* Le nomrbe de chemins pour l'archive. Chaque chemin représente un calque qui remplacera les
     * données des calques précédents. */
    int (*nombre_de_chemins)(struct ContexteOuvertureArchive *ctx);
    /* Accède au chemin pour index données. */
    void (*chemin)(struct ContexteOuvertureArchive *ctx,
                   uint64_t i,
                   const char **pointeur,
                   uint64_t *taille);

    /* Pour les erreurs venant d'Alembic. */
    eAbcPoliceErreur (*police_erreur)(struct ContexteOuvertureArchive *ctx);

    /* Pour Ogawa. */
    int (*nombre_de_flux_ogawa_desires)(struct ContexteOuvertureArchive *ctx);
    eAbcStrategieLectureOgawa (*strategie_lecture_ogawa)(struct ContexteOuvertureArchive *ctx);

    /* Rappels pour les erreurs, afin de savoir ce qui s'est passé. */
    void (*erreur_aucun_chemin)(struct ContexteOuvertureArchive *ctx);
    void (*erreur_archive_invalide)(struct ContexteOuvertureArchive *ctx);

    /* Les données utilisateur du contexte. */
    void *donnees_utilisateur;
};

/* Paramètres pour la bonne création d'une archive Alembic. */
struct ContexteCreationArchive {
    /* Accède au chemin. */
    void (*donne_chemin)(struct ContexteCreationArchive *ctx,
                         const char **pointeur,
                         uint64_t *taille);

    /* Pour les erreurs venant d'Alembic. */
    eAbcPoliceErreur (*police_erreur)(struct ContexteCreationArchive *ctx);

    /* Rappels pour les erreurs, afin de savoir ce qui s'est passé. */
    void (*erreur_aucun_chemin)(struct ContexteCreationArchive *ctx);

    /* Pour les métadonnées. */

    /* Accède au nom de l'application qui a crée l'archive. */
    void (*donne_nom_application)(struct ContexteCreationArchive *ctx,
                                  const char **pointeur,
                                  uint64_t *taille);

    /* Accède à la description de l'archive. */
    void (*donne_description)(struct ContexteCreationArchive *ctx,
                              const char **pointeur,
                              uint64_t *taille);

    /* Doit retourner le nombre de frames par secondes. */
    float (*donne_frames_par_seconde)(struct ContexteCreationArchive *ctx);

    /* Les données utilisateur du contexte. */
    void *donnees_utilisateur;
};

// --------------------------------------------------------------
// Traversé de l'archive.

struct ContexteTraverseArchive {
    // Extraction du nom de l'objet courant.
    void (*extrait_nom_courant)(struct ContexteTraverseArchive *ctx,
                                const char *pointeur,
                                uint64_t taille);

    // Certaines tâches peuvent prendre du temps, ce rappel sers à annuler l'opération en cours.
    bool (*annule)(struct ContexteTraverseArchive *ctx);

    // Création d'objet pour tous les types.
    // cree_poly_mesh
    // cree_subd
    // cree_points
    // cree_courbe
    // cree_matrice
    // cree_instance

    // Concatène toutes les matrices de la hierarchie.
    // aplatis_hierarchie

    // Ignore les objets notés comme invisible.
    // ignore_invisible

    void *donnees_utilisateur;
};

// --------------------------------------------------------------
// Lecture des objets.

// À FAIRE : caméra, light, material, face set

typedef void (*TypeRappelReserveMemoire)(void *, uint64_t);
typedef void (*TypeRappelAjouteUnPoint)(void *, float, float, float);
typedef void (*TypeRappelAjouteTousLesPoints)(void *, const float *, uint64_t);
typedef void (*TypeRappelAjoutepolygone)(void *, uint64_t, const int *, int);
typedef void (*TypeRappelAjouteTousLesPolygones)(void *, const int *, uint64_t);
typedef void (*TypeRappelReserveCoinsPolygone)(void *, uint64_t, int);
typedef void (*TypeRappelAjouteCoinPolygone)(void *, uint64_t, int);
typedef void (*TypeRappelAjouteTousLesCoins)(void *, const int *, uint64_t);
typedef void (*TypeRappelMarquePolygoneTrou)(void *, int);
typedef void (*TypeRappelMarquePlisVertex)(void *, int, float);
typedef void (*TypeRappelMarquePlisAretes)(void *, int, int, float);
typedef void (*TypeRappelMarqueSchemaSubdivision)(void *, const char *, uint64_t);
typedef void (*TypeRappelMarquePropagationCoinsFaceVarying)(void *, int);
typedef void (*TypeRappelMarqueInterpolationFrontiereFaceVarying)(void *, int);
typedef void (*TypeRappelMarqueInterpolationFrontiere)(void *, int);
typedef void (*TypeRappelAjouteIndexPoint)(void *, uint64_t, uint64_t);

struct ConvertisseusePolyMesh {
    void *donnees;

    TypeRappelReserveMemoire reserve_points;
    TypeRappelAjouteUnPoint ajoute_un_point;
    TypeRappelAjouteTousLesPoints ajoute_tous_les_points;

    TypeRappelReserveMemoire reserve_polygones;
    TypeRappelAjoutepolygone ajoute_polygone;
    TypeRappelAjouteTousLesPolygones ajoute_tous_les_polygones;

    TypeRappelReserveMemoire reserve_coin;
    TypeRappelReserveCoinsPolygone reserve_coins_polygone;
    TypeRappelAjouteCoinPolygone ajoute_coin_polygone;
    TypeRappelAjouteTousLesCoins ajoute_tous_les_coins;
};

struct ConvertisseuseSubD {
    void *donnees;

    TypeRappelReserveMemoire reserve_points;
    TypeRappelAjouteUnPoint ajoute_un_point;
    TypeRappelAjouteTousLesPoints ajoute_tous_les_points;

    TypeRappelReserveMemoire reserve_polygones;
    TypeRappelAjoutepolygone ajoute_polygone;
    TypeRappelAjouteTousLesPolygones ajoute_tous_les_polygones;

    TypeRappelReserveMemoire reserve_coin;
    TypeRappelReserveCoinsPolygone reserve_coins_polygone;
    TypeRappelAjouteCoinPolygone ajoute_coin_polygone;
    TypeRappelAjouteTousLesCoins ajoute_tous_les_coins;

    TypeRappelReserveMemoire reserve_trous;
    TypeRappelMarquePolygoneTrou marque_polygone_trou;

    TypeRappelReserveMemoire reserve_plis_sommets;
    TypeRappelMarquePlisVertex marque_plis_vertex;

    TypeRappelReserveMemoire reserve_plis_aretes;
    TypeRappelMarquePlisAretes marque_plis_aretes;

    TypeRappelMarqueSchemaSubdivision marque_schema_subdivision;
    TypeRappelMarquePropagationCoinsFaceVarying marque_propagation_coins_face_varying;
    TypeRappelMarqueInterpolationFrontiereFaceVarying marque_interpolation_frontiere_face_varying;
    TypeRappelMarqueInterpolationFrontiere marque_interpolation_frontiere;
};

struct ConvertisseusePoints {
    void *donnees;

    TypeRappelReserveMemoire reserve_points;
    TypeRappelAjouteUnPoint ajoute_un_point;
    TypeRappelAjouteTousLesPoints ajoute_tous_les_points;

    TypeRappelReserveMemoire reserve_index;
    TypeRappelAjouteIndexPoint ajoute_index_point;
};

struct ConvertisseuseCourbes {
    void *donnees;
};

struct ConvertisseuseNurbs {
    void *donnees;
};

struct ConvertisseuseXform {
    void *donnees;
};

struct ConvertisseuseFaceSet {
    void *donnees;
};

struct ConvertisseuseLumiere {
    void *donnees;
};

struct ConvertisseuseCamera {
    void *donnees;
};

struct ConvertisseuseMateriau {
    void *donnees;
};

struct ContexteLectureCache {
    void (*initialise_convertisseuse_polymesh)(struct ConvertisseusePolyMesh *);

    void (*initialise_convertisseuse_subd)(struct ConvertisseuseSubD *);

    void (*initialise_convertisseuse_points)(struct ConvertisseusePoints *);

    void (*initialise_convertisseuse_courbes)(struct ConvertisseuseCourbes *);

    void (*initialise_convertisseuse_nurbs)(struct ConvertisseuseNurbs *);

    void (*initialise_convertisseuse_xform)(struct ConvertisseuseXform *);

    void (*initialise_convertisseuse_face_set)(struct ConvertisseuseFaceSet *);

    void (*initialise_convertisseuse_lumiere)(struct ConvertisseuseLumiere *);

    void (*initialise_convertisseuse_camera)(struct ConvertisseuseCamera *);

    void (*initialise_convertisseuse_materiau)(struct ConvertisseuseMateriau *);
};

typedef enum eTypeObjetAbc {
    CAMERA,
    COURBES,
    FACE_SET,
    LUMIERE,
    MATERIAU,
    NURBS,
    POINTS,
    POLY_MESH,
    SUBD,
    XFORM,
} eTypeObjetAbc;

/* ------------------------------------------------------------------------- */

typedef enum eAbcDomaineAttribut {
    AUCUNE,
    /* kConstantScope */
    OBJET,
    /* kVertexScope */
    POINT,
    /* kUniformScope */
    PRIMITIVE,
    /* kFacevaryingScope ou kVarying. */
    POINT_PRIMITIVE,
} eAbcDomaineAttribut;

#define NOMBRE_DE_DOMAINES (POINT_PRIMITIVE + 1)

typedef enum eTypeDoneesAttributAbc {
    ATTRIBUT_TYPE_BOOL = 0,
    ATTRIBUT_TYPE_N8 = 1,
    ATTRIBUT_TYPE_Z8 = 2,
    ATTRIBUT_TYPE_N16 = 3,
    ATTRIBUT_TYPE_Z16 = 4,
    ATTRIBUT_TYPE_N32 = 5,
    ATTRIBUT_TYPE_Z32 = 6,
    ATTRIBUT_TYPE_N64 = 7,
    ATTRIBUT_TYPE_Z64 = 8,
    ATTRIBUT_TYPE_R16 = 9,
    ATTRIBUT_TYPE_R32 = 10,
    ATTRIBUT_TYPE_R64 = 11,
    ATTRIBUT_TYPE_CHAINE = 12,
    ATTRIBUT_TYPE_WSTRING = 13,

    ATTRIBUT_TYPE_VEC2_Z16 = 14,
    ATTRIBUT_TYPE_VEC2_Z32 = 15,
    ATTRIBUT_TYPE_VEC2_R32 = 16,
    ATTRIBUT_TYPE_VEC2_R64 = 17,

    ATTRIBUT_TYPE_VEC3_Z16 = 18,
    ATTRIBUT_TYPE_VEC3_Z32 = 19,
    ATTRIBUT_TYPE_VEC3_R32 = 20,
    ATTRIBUT_TYPE_VEC3_R64 = 21,

    ATTRIBUT_TYPE_POINT2_Z16 = 22,
    ATTRIBUT_TYPE_POINT2_Z32 = 23,
    ATTRIBUT_TYPE_POINT2_R32 = 24,
    ATTRIBUT_TYPE_POINT2_R64 = 25,

    ATTRIBUT_TYPE_POINT3_Z16 = 26,
    ATTRIBUT_TYPE_POINT3_Z32 = 27,
    ATTRIBUT_TYPE_POINT3_R32 = 28,
    ATTRIBUT_TYPE_POINT3_R64 = 29,

    ATTRIBUT_TYPE_BOITE2_Z16 = 30,
    ATTRIBUT_TYPE_BOITE2_Z32 = 31,
    ATTRIBUT_TYPE_BOITE2_R32 = 32,
    ATTRIBUT_TYPE_BOITE2_R64 = 33,

    ATTRIBUT_TYPE_BOITE3_Z16 = 34,
    ATTRIBUT_TYPE_BOITE3_Z32 = 35,
    ATTRIBUT_TYPE_BOITE3_R32 = 36,
    ATTRIBUT_TYPE_BOITE3_R64 = 37,

    ATTRIBUT_TYPE_MAT3X3_R32 = 38,
    ATTRIBUT_TYPE_MAT3X3_R64 = 39,

    ATTRIBUT_TYPE_MAT4X4_R32 = 40,
    ATTRIBUT_TYPE_MAT4X4_R64 = 41,

    ATTRIBUT_TYPE_QUAT_R32 = 42,
    ATTRIBUT_TYPE_QUAT_R64 = 43,

    ATTRIBUT_TYPE_COULEUR_RGB_R16 = 44,
    ATTRIBUT_TYPE_COULEUR_RGB_R32 = 45,
    ATTRIBUT_TYPE_COULEUR_RGB_N8 = 46,

    ATTRIBUT_TYPE_COULEUR_RGBA_R16 = 47,
    ATTRIBUT_TYPE_COULEUR_RGBA_R32 = 48,
    ATTRIBUT_TYPE_COULEUR_RGBA_N8 = 49,

    ATTRIBUT_TYPE_NORMAL2_R32 = 50,
    ATTRIBUT_TYPE_NORMAL2_R64 = 51,

    ATTRIBUT_TYPE_NORMAL3_R32 = 52,
    ATTRIBUT_TYPE_NORMAL3_R64 = 53,
} eTypeDoneesAttributAbc;

/* ------------------------------------------------------------------------- */

/** Struct contenant des fonctions de rappels pour exporter des attributs vers Alembic. */
struct AbcExportriceAttribut {
    /** Doit retourner le nombre d'attributs à exporter. Les \a données sont celles de la
     * ConvertisseuseExport qui a été utilisée pour initialiser cette structure. */
    int (*donne_nombre_attributs_a_exporter)(void *donnees);

    /** Doit retourner un pointeur pour l'attribut à l'index, ainsi que remplir les informations
     * sur le nom de l'attribut, son domaine et son type de données. L'index est entre 0 et la
     * valeur retourner par `donne_nombre_attributs_a_exporter`.
     * Si un pointeur nul est retourner l'attribut n'est pas lu. */
    void *(*donne_attribut_pour_index)(void *donnees,
                                       int index,
                                       char **r_nom,
                                       int64_t *r_taille_nom,
                                       enum eAbcDomaineAttribut *r_domaine,
                                       enum eTypeDoneesAttributAbc *r_type_des_donnees);

    /** Fonctions pour lire les valeurs d'un attribut à l'index donné. L'attribut est celui
     * retourné par `donne_attribut_pour_index`. */
    void (*donne_valeur_bool)(void *attribut, int index, bool *r_valeur);
    void (*donne_valeur_n8)(void *attribut, int index, uint8_t *r_valeur);
    void (*donne_valeur_z8)(void *attribut, int index, int8_t *r_valeur);
    void (*donne_valeur_n16)(void *attribut, int index, uint16_t *r_valeur);
    void (*donne_valeur_z16)(void *attribut, int index, int16_t *r_valeur);
    void (*donne_valeur_n32)(void *attribut, int index, uint32_t *r_valeur);
    void (*donne_valeur_z32)(void *attribut, int index, int32_t *r_valeur);
    void (*donne_valeur_n64)(void *attribut, int index, uint64_t *r_valeur);
    void (*donne_valeur_z64)(void *attribut, int index, int64_t *r_valeur);
    void (*donne_valeur_r16)(void *attribut, int index, uint16_t *r_valeur);
    void (*donne_valeur_r32)(void *attribut, int index, float *r_valeur);
    void (*donne_valeur_r64)(void *attribut, int index, double *r_valeur);
    void (*donne_valeur_chaine)(void *attribut, int index, char **r_valeur, int64_t *r_taille);
    // void (*donne_valeur_wstring)(void *attribut, int index, wstring *r_valeur);

    void (*donne_valeur_vec2_z16)(void *attribut, int index, int16_t *r_valeur);
    void (*donne_valeur_vec2_z32)(void *attribut, int index, int32_t *r_valeur);
    void (*donne_valeur_vec2_r32)(void *attribut, int index, float *r_valeur);
    void (*donne_valeur_vec2_r64)(void *attribut, int index, double *r_valeur);

    void (*donne_valeur_vec3_z16)(void *attribut, int index, int16_t *r_valeur);
    void (*donne_valeur_vec3_z32)(void *attribut, int index, int32_t *r_valeur);
    void (*donne_valeur_vec3_r32)(void *attribut, int index, float *r_valeur);
    void (*donne_valeur_vec3_r64)(void *attribut, int index, double *r_valeur);

    void (*donne_valeur_point2_z16)(void *attribut, int index, int16_t *r_valeur);
    void (*donne_valeur_point2_z32)(void *attribut, int index, int32_t *r_valeur);
    void (*donne_valeur_point2_r32)(void *attribut, int index, float *r_valeur);
    void (*donne_valeur_point2_r64)(void *attribut, int index, double *r_valeur);

    void (*donne_valeur_point3_z16)(void *attribut, int index, int16_t *r_valeur);
    void (*donne_valeur_point3_z32)(void *attribut, int index, int32_t *r_valeur);
    void (*donne_valeur_point3_r32)(void *attribut, int index, float *r_valeur);
    void (*donne_valeur_point3_r64)(void *attribut, int index, double *r_valeur);

    void (*donne_valeur_boite2_z16)(void *attribut, int index, int16_t *r_valeur);
    void (*donne_valeur_boite2_z32)(void *attribut, int index, int32_t *r_valeur);
    void (*donne_valeur_boite2_r32)(void *attribut, int index, float *r_valeur);
    void (*donne_valeur_boite2_r64)(void *attribut, int index, double *r_valeur);

    void (*donne_valeur_boite3_z16)(void *attribut, int index, int16_t *r_valeur);
    void (*donne_valeur_boite3_z32)(void *attribut, int index, int32_t *r_valeur);
    void (*donne_valeur_boite3_r32)(void *attribut, int index, float *r_valeur);
    void (*donne_valeur_boite3_r64)(void *attribut, int index, double *r_valeur);

    void (*donne_valeur_mat3x3_r32)(void *attribut, int index, float *r_valeur);
    void (*donne_valeur_mat3x3_r64)(void *attribut, int index, double *r_valeur);

    void (*donne_valeur_mat4x4_r32)(void *attribut, int index, float *r_valeur);
    void (*donne_valeur_mat4x4_r64)(void *attribut, int index, double *r_valeur);

    void (*donne_valeur_quat_r32)(void *attribut, int index, float *r_valeur);
    void (*donne_valeur_quat_r64)(void *attribut, int index, double *r_valeur);

    void (*donne_valeur_couleur_rgb_r16)(void *attribut, int index, uint16_t *r_valeur);
    void (*donne_valeur_couleur_rgb_r32)(void *attribut, int index, float *r_valeur);
    void (*donne_valeur_couleur_rgb_n8)(void *attribut, int index, uint8_t *r_valeur);

    void (*donne_valeur_couleur_rgba_r16)(void *attribut, int index, uint16_t *r_valeur);
    void (*donne_valeur_couleur_rgba_r32)(void *attribut, int index, float *r_valeur);
    void (*donne_valeur_couleur_rgba_n8)(void *attribut, int index, uint8_t *r_valeur);

    void (*donne_valeur_normal2_r32)(void *attribut, int index, float *r_valeur);
    void (*donne_valeur_normal2_r64)(void *attribut, int index, double *r_valeur);

    void (*donne_valeur_normal3_r32)(void *attribut, int index, float *r_valeur);
    void (*donne_valeur_normal3_r64)(void *attribut, int index, double *r_valeur);
};

typedef struct AbcOptionsExport {
    /* décide si la hiérarchie doit être préservé */
    bool exporte_hierarchie;

    // À FAIRE : controle des objets exportés (visible, seulement les maillages, etc.)

    // À FAIRE : est-il possible, pour les calques, de n'exporter que les attributs ?
} AbcOptionsExport;

typedef enum eAbcIndexagePolygone {
    ABC_INDEXAGE_POLYGONE_HORAIRE,
    ABC_INDEXAGE_POLYGONE_ANTIHORAIRE,
} eAbcIndexagePolygone;

struct ConvertisseuseExportPolyMesh {
    void *donnees;
    uint64_t (*nombre_de_points)(struct ConvertisseuseExportPolyMesh *);
    void (*point_pour_index)(
        struct ConvertisseuseExportPolyMesh *, uint64_t, float *, float *, float *);

    enum eAbcIndexagePolygone (*donne_indexage_polygone)(struct ConvertisseuseExportPolyMesh *);

    uint64_t (*nombre_de_polygones)(struct ConvertisseuseExportPolyMesh *);
    int (*nombre_de_coins_polygone)(struct ConvertisseuseExportPolyMesh *, uint64_t);

    void (*coins_pour_polygone)(struct ConvertisseuseExportPolyMesh *, uint64_t, int *);

    /** Optionnel, doit fournir les limites géométriques du maillage. Si absent, les limites seront
     * calculer selon les points. \a r_min et \a r_max pointent vers des float[3]. */
    void (*donne_limites_geometriques)(struct ConvertisseuseExportPolyMesh *,
                                       float *r_min,
                                       float *r_max);

    /** Si ce rappel est présent, les attributs sont exportés, et ce rappel doit initialiser
     * l'exportrice d'attributs. */
    void (*initialise_exportrice_attribut)(struct ConvertisseuseExportPolyMesh *,
                                           struct AbcExportriceAttribut *);

    /** Doit retourner un pointeur vers l'attribut standard pour les UVs du maillage ainsi que les
     * métadonnées dudit attribut, s'il existe. Si le type de données n'est pas `VEC2`, l'attribut
     * ne sera pas exporté comme attribut standard.
     */
    void *(*donne_attribut_standard_uv)(struct ConvertisseuseExportPolyMesh *,
                                        char **r_nom,
                                        int64_t *r_taille_nom,
                                        enum eAbcDomaineAttribut *r_domaine,
                                        enum eTypeDoneesAttributAbc *r_type_des_donnees);

    /** Doit retourner un pointeur vers l'attribut standard pour la vélocité du maillage ainsi que
     * les métadonnées dudit attribut, s'il existe. Si le type de données n'est pas `VEC3`, et le
     * domaine `POINT`, l'attribut ne sera pas exporté comme attribut standard.
     */
    void *(*donne_attribut_standard_velocite)(struct ConvertisseuseExportPolyMesh *,
                                              char **r_nom,
                                              int64_t *r_taille_nom,
                                              enum eAbcDomaineAttribut *r_domaine,
                                              enum eTypeDoneesAttributAbc *r_type_des_donnees);

    /** Doit retourner un pointeur vers l'attribut standard pour les normaux du maillage ainsi que
     * les métadonnées dudit attribut, s'il existe. Si le type de données n'est pas `VEC3` ou
     * `NOR3`, et le domaine `POINT`, l'attribut ne sera pas exporté comme attribut standard.
     */
    void *(*donne_attribut_standard_normaux)(struct ConvertisseuseExportPolyMesh *,
                                             char **r_nom,
                                             int64_t *r_taille_nom,
                                             enum eAbcDomaineAttribut *r_domaine,
                                             enum eTypeDoneesAttributAbc *r_type_des_donnees);
};

struct ConvertisseuseExportSubD {
    void *donnees;

    uint64_t (*nombre_de_points)(struct ConvertisseuseExportSubD *);
    void (*point_pour_index)(
        struct ConvertisseuseExportSubD *, uint64_t, float *, float *, float *);

    enum eAbcIndexagePolygone (*donne_indexage_polygone)(struct ConvertisseuseExportSubD *);

    uint64_t (*nombre_de_polygones)(struct ConvertisseuseExportSubD *);
    int (*nombre_de_coins_polygone)(struct ConvertisseuseExportSubD *, uint64_t);

    void (*coins_pour_polygone)(struct ConvertisseuseExportSubD *, uint64_t, int *);

    /** Optionnel, doit fournir les limites géométriques du maillage. Si absent, les limites seront
     * calculer selon les points. \a r_min et \a r_max pointent vers des float[3]. */
    void (*donne_limites_geometriques)(struct ConvertisseuseExportSubD *,
                                       float *r_min,
                                       float *r_max);

    /** Si ce rappel est présent, les attributs sont exportés, et ce rappel doit initialiser
     * l'exportrice d'attributs. */
    void (*initialise_exportrice_attribut)(struct ConvertisseuseExportSubD *,
                                           struct AbcExportriceAttribut *);

    /** Doit retourner un pointeur vers l'attribut standard pour les UVs du maillage ainsi que les
     * métadonnées dudit attribut, s'il existe. Si le type de données n'est pas `VEC2`, l'attribut
     * ne sera pas exporté comme attribut standard.
     */
    void *(*donne_attribut_standard_uv)(struct ConvertisseuseExportSubD *,
                                        char **r_nom,
                                        int64_t *r_taille_nom,
                                        enum eAbcDomaineAttribut *r_domaine,
                                        enum eTypeDoneesAttributAbc *r_type_des_donnees);

    /** Doit retourner un pointeur vers l'attribut standard pour la vélocité du maillage ainsi que
     * les métadonnées dudit attribut, s'il existe. Si le type de données n'est pas `VEC3`, et le
     * domaine `POINT`, l'attribut ne sera pas exporté comme attribut standard.
     */
    void *(*donne_attribut_standard_velocite)(struct ConvertisseuseExportSubD *,
                                              char **r_nom,
                                              int64_t *r_taille_nom,
                                              enum eAbcDomaineAttribut *r_domaine,
                                              enum eTypeDoneesAttributAbc *r_type_des_donnees);
};

struct ConvertisseuseExportPoints {
    void *donnees;

    uint64_t (*nombre_de_points)(struct ConvertisseuseExportPoints *);

    void (*point_pour_index)(
        struct ConvertisseuseExportPoints *, uint64_t, float *, float *, float *);

    /** Optionnel, doit fournir les limites géométriques du maillage. Si absent, les limites seront
     * calculer selon les points. \a r_min et \a r_max pointent vers des float[3]. */
    void (*donne_limites_geometriques)(struct ConvertisseuseExportPoints *,
                                       float *r_min,
                                       float *r_max);

    /** Si ce rappel est présent, les attributs sont exportés, et ce rappel doit initialiser
     * l'exportrice d'attributs. */
    void (*initialise_exportrice_attribut)(struct ConvertisseuseExportPoints *,
                                           struct AbcExportriceAttribut *);

    /** Doit retourner un pointeur vers l'attribut standard pour les rayons des points ainsi que
     * les métadonnées dudit attribut, s'il existe. Si le type de données n'est pas `R32`,
     * l'attribut ne sera pas exporté comme attribut standard.
     */
    void *(*donne_attribut_standard_rayon)(struct ConvertisseuseExportPolyMesh *,
                                           char **r_nom,
                                           int64_t *r_taille_nom,
                                           enum eAbcDomaineAttribut *r_domaine,
                                           enum eTypeDoneesAttributAbc *r_type_des_donnees);

    /** Doit retourner un pointeur vers l'attribut standard pour la vélocité des points ainsi que
     * les métadonnées dudit attribut, s'il existe. Si le type de données n'est pas `VEC3`,
     * l'attribut ne sera pas exporté comme attribut standard.
     */
    void *(*donne_attribut_standard_velocite)(struct ConvertisseuseExportPoints *,
                                              char **r_nom,
                                              int64_t *r_taille_nom,
                                              enum eAbcDomaineAttribut *r_domaine,
                                              enum eTypeDoneesAttributAbc *r_type_des_donnees);

    /** Doit retourner un pointeur vers l'attribut standard pour les identifiants des points ainsi
     * que les métadonnées dudit attribut, s'il existe. Si le type de données n'est pas `N64`
     * l'attribut ne sera pas exporté comme attribut standard. Cette attribut ne doit être présent
     * que si les points ont changé de topologie, ou pour la première image.
     */
    void *(*donne_attribut_standard_ids)(struct ConvertisseuseExportPoints *,
                                         char **r_nom,
                                         int64_t *r_taille_nom,
                                         enum eAbcDomaineAttribut *r_domaine,
                                         enum eTypeDoneesAttributAbc *r_type_des_donnees);
};

struct ConvertisseuseExportCourbes {
    void *donnees;
};

struct ConvertisseuseExportNurbs {
    void *donnees;
};

/** Convertisseuse pour exporter une transformation vers Alembic. */
struct ConvertisseuseExportXform {
    /** Donne la matrice de transformation à écrire. L'adresse passée est celle d'une matrice de 4
     * lignes et de 4 colonnes représenter de manière contigüe en mémoire (double[16]). */
    void (*donne_matrice)(struct ConvertisseuseExportXform *, double *);

    /** Doit retouner vrai si la matrice écrite se concatène à celle de son parent. */
    bool (*doit_hériter_matrice_parent)(struct ConvertisseuseExportXform *);

    /** Données utilisateur de la convertisseuse. Les clients peuvent l'utiliser pour stocker leurs
     * données relatives à cette transformation. */
    void *donnees;
};

typedef enum eAbcExclusiviteGroupePolygone {
    ABC_EXCLUSIVITE_POLYGONE_EXCLUSIVE,
    ABC_EXCLUSIVITE_POLYGONE_NON_EXCLUSIVE,
} eAbcExclusiviteGroupePolygone;

struct ConvertisseuseExportFaceSet {
    void *donnees;

    /** Doit retourner l'exclusivité des polygones dans ce groupe. */
    enum eAbcExclusiviteGroupePolygone (*donne_exclusivite_polygones)(
        struct ConvertisseuseExportFaceSet *);

    /** Doit retourner le nombre de polygones dans groupe. */
    uint64_t (*nombre_de_polygones)(struct ConvertisseuseExportFaceSet *);

    /** Doit retourner l'index effectif du polygone pour l'index dans le groupe. */
    int (*donne_index_polygone)(struct ConvertisseuseExportFaceSet *, int index);

    /** Optionnel. Doit remplir tous les index des polygones d'un coup. Le tampon contient de
     * l'espace pour nombre_de_polygones de polygones. */
    void (*remplis_index_polygones)(struct ConvertisseuseExportFaceSet *, int *);
};

typedef struct AbcMillimetre {
    double valeur;
} AbcMillimetre;

typedef struct AbcCentimetre {
    double valeur;
} AbcCentimetre;

typedef struct AbcPourcentage {
    double valeur;
} AbcPourcentage;

typedef struct AbcTempsSeconde {
    double valeur;
} AbcTempsSeconde;

typedef struct AbcExportriceEchantillonCamera {
    /** Définis la longueur focale de la caméra. */
    void (*definis_longueur_focale)(struct AbcExportriceEchantillonCamera *,
                                    struct AbcMillimetre *);

    /** Définis l'ouverture horizontale et verticale de la caméra. */
    void (*definis_ouverture)(struct AbcExportriceEchantillonCamera *,
                              struct AbcCentimetre *,
                              struct AbcCentimetre *);

    /** Définis le décalage du senseur horizontal et vertical de la caméra. */
    void (*definis_decalage_senseur)(struct AbcExportriceEchantillonCamera *,
                                     struct AbcCentimetre *,
                                     struct AbcCentimetre *);

    /** Définis l'aspect (largeur / hauteur). */
    void (*definis_aspect_horizontal_sur_vertical)(struct AbcExportriceEchantillonCamera *,
                                                   double);

    /** Définis l'extension (overscan), en pourcentage relatif, de l'image. Les valeurs sont à
     * donner dans l'ordre : gauche, droite, haut, bas. */
    void (*definis_extension_image)(struct AbcExportriceEchantillonCamera *,
                                    struct AbcPourcentage *,
                                    struct AbcPourcentage *,
                                    struct AbcPourcentage *,
                                    struct AbcPourcentage *);

    /** Définis l'aspect (largeur / hauteur). */
    void (*definis_fstop)(struct AbcExportriceEchantillonCamera *, double);

    /** Définis la distance de la cible de la caméra, ce qui est focalisé. */
    void (*definis_distance_de_la_cible)(struct AbcExportriceEchantillonCamera *,
                                         struct AbcCentimetre *);

    /** Définis le temps, relatif à l'image, de l'ouverture et de la fermeture de l'obturateur. */
    void (*definis_temps_obturation)(struct AbcExportriceEchantillonCamera *,
                                     struct AbcTempsSeconde *,
                                     struct AbcTempsSeconde *);

    /** Définis la distance de visibilté du premier et de l'arrière plan respectivement. */
    void (*definis_avant_arriere_plan)(struct AbcExportriceEchantillonCamera *,
                                       struct AbcCentimetre *,
                                       struct AbcCentimetre *);
} AbcExportriceEchantillonCamera;

typedef struct AbcExportriceOperationSenseur {
    void (*ajoute_translation)(struct AbcExportriceOperationSenseur *, double *, char *, int64_t);
    void (*ajoute_taille)(struct AbcExportriceOperationSenseur *, double *, char *, int64_t);
    void (*ajoute_matrice)(struct AbcExportriceOperationSenseur *, double *, char *, int64_t);
} AbcExportriceOperationSenseur;

struct ConvertisseuseExportCamera {
    void *donnees;

    /** Optionnel. Doit donner la taille de la fenêtre dans l'ordre : haut, bas, gauche, droite. */
    void (*donne_taille_fenetre)(
        struct ConvertisseuseExportCamera *, double *, double *, double *, double *);

    /** Requis. Exporte les données de la caméra via une AbcExportriceEchantillonCamera. */
    void (*remplis_donnees_echantillon)(struct ConvertisseuseExportCamera *,
                                        struct AbcExportriceEchantillonCamera *);

    /** Optionnel. Doit fournir les limites géométriques de la caméra. */
    void (*donne_limites_geometriques_enfant)(struct ConvertisseuseExportCamera *,
                                              float *r_min,
                                              float *r_max);

    /** Optionnel. Ajoute des opérations de transformation pour le senseur de la caméra. */
    void (*ajoute_operations_senseur)(struct ConvertisseuseExportCamera *,
                                      struct AbcExportriceOperationSenseur *);
};

struct ConvertisseuseExportLumiere {
    void *donnees;

    /** Optionnel. Doit donner la taille de la fenêtre dans l'ordre : haut, bas, gauche, droite. */
    void (*donne_taille_fenetre)(
        struct ConvertisseuseExportLumiere *, double *, double *, double *, double *);

    /** Requis. Exporte les données de la caméra via une AbcExportriceEchantillonCamera. */
    void (*remplis_donnees_echantillon)(struct ConvertisseuseExportLumiere *,
                                        struct AbcExportriceEchantillonCamera *);

    /** Optionnel. Doit fournir les limites géométriques de la caméra. */
    void (*donne_limites_geometriques_enfant)(struct ConvertisseuseExportLumiere *,
                                              float *r_min,
                                              float *r_max);

    /** Optionnel. Ajoute des opérations de transformation pour le senseur de la caméra. */
    void (*ajoute_operations_senseur)(struct ConvertisseuseExportLumiere *,
                                      struct AbcExportriceOperationSenseur *);
};

struct ConvertisseuseExportMateriau {
    void *donnees;

    void (*nom_cible)(struct ConvertisseuseExportMateriau *, const char **, uint64_t *);
    void (*type_nuanceur)(struct ConvertisseuseExportMateriau *, const char **, uint64_t *);
    void (*nom_nuanceur)(struct ConvertisseuseExportMateriau *, const char **, uint64_t *);

    void (*nom_sortie_graphe)(struct ConvertisseuseExportMateriau *, const char **, uint64_t *);

    uint64_t (*nombre_de_noeuds)(struct ConvertisseuseExportMateriau *);

    void (*nom_noeud)(struct ConvertisseuseExportMateriau *, uint64_t, const char **, uint64_t *);
    void (*type_noeud)(struct ConvertisseuseExportMateriau *, uint64_t, const char **, uint64_t *);

    uint64_t (*nombre_entrees_noeud)(struct ConvertisseuseExportMateriau *, uint64_t);
    void (*nom_entree_noeud)(
        struct ConvertisseuseExportMateriau *, uint64_t, uint64_t, const char **, uint64_t *);

    uint64_t (*nombre_de_connexions)(struct ConvertisseuseExportMateriau *, uint64_t, uint64_t);
    void (*nom_connexion_entree)(struct ConvertisseuseExportMateriau *,
                                 uint64_t,
                                 uint64_t,
                                 uint64_t,
                                 const char **,
                                 uint64_t *);
    void (*nom_noeud_connexion)(struct ConvertisseuseExportMateriau *,
                                uint64_t,
                                uint64_t,
                                uint64_t,
                                const char **,
                                uint64_t *);
};

struct ContexteEcritureCache {
    void (*initialise_convertisseuse_polymesh)(struct ConvertisseuseExportPolyMesh *);

    void (*initialise_convertisseuse_subd)(struct ConvertisseuseExportSubD *);

    void (*initialise_convertisseuse_points)(struct ConvertisseuseExportPoints *);

    void (*initialise_convertisseuse_courbes)(struct ConvertisseuseExportCourbes *);

    void (*initialise_convertisseuse_nurbs)(struct ConvertisseuseExportNurbs *);

    void (*initialise_convertisseuse_xform)(struct ConvertisseuseExportXform *);

    void (*initialise_convertisseuse_face_set)(struct ConvertisseuseExportFaceSet *);

    void (*initialise_convertisseuse_lumiere)(struct ConvertisseuseExportLumiere *);

    void (*initialise_convertisseuse_camera)(struct ConvertisseuseExportCamera *);

    void (*initialise_convertisseuse_materiau)(struct ConvertisseuseExportMateriau *);
};

struct ConvertisseuseImportAttributs {
    bool (*lis_tous_les_attributs)(struct ConvertisseuseImportAttributs *);
    int (*nombre_attributs_requis)(struct ConvertisseuseImportAttributs *);

    void (*nom_attribut_requis)(struct ConvertisseuseImportAttributs *,
                                uint64_t,
                                const char **,
                                uint64_t *);

    void *(*ajoute_attribut)(struct ConvertisseuseImportAttributs *,
                             const char *,
                             uint64_t,
                             eAbcDomaineAttribut);

    void (*information_portee)(struct ConvertisseuseImportAttributs *,
                               int *points,
                               int *primitives,
                               int *points_primitives);

    // Ce n'est que pour un seul attribut
    void (*ajoute_bool)(void *, uint64_t, bool const *, int);
    void (*ajoute_n8)(void *, uint64_t, unsigned char const *, int);
    void (*ajoute_n16)(void *, uint64_t, unsigned short const *, int);
    void (*ajoute_n32)(void *, uint64_t, uint32_t const *, int);
    void (*ajoute_n64)(void *, uint64_t, uint64_t const *, int);
    void (*ajoute_z8)(void *, uint64_t, signed char const *, int);
    void (*ajoute_z16)(void *, uint64_t, short const *, int);
    void (*ajoute_z32)(void *, uint64_t, int const *, int);
    void (*ajoute_z64)(void *, uint64_t, int64_t const *, int);
    void (*ajoute_r16)(void *, uint64_t, r16 const *, int);
    void (*ajoute_r32)(void *, uint64_t, float const *, int);
    void (*ajoute_r64)(void *, uint64_t, double const *, int);
    void (*ajoute_chaine)(void *, uint64_t, char const *, uint64_t);
};

#ifdef __cplusplus
}
#endif
