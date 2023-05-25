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
struct ContexteKuri;
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

typedef struct AbcOptionsExport {
    /* décide si la hiérarchie doit être préservé */
    bool exporte_hierarchie;

    // À FAIRE : controle des objets exportés (visible, seulement les maillages, etc.)

    // À FAIRE : est-il possible, pour les calques, de n'exporter que les attributs ?
} AbcOptionsExport;

struct ConvertisseuseExportPolyMesh {
    void *donnnees;
    uint64_t (*nombre_de_points)(struct ConvertisseuseExportPolyMesh *);
    void (*point_pour_index)(
        struct ConvertisseuseExportPolyMesh *, uint64_t, float *, float *, float *);

    uint64_t (*nombre_de_polygones)(struct ConvertisseuseExportPolyMesh *);
    int (*nombre_de_coins_polygone)(struct ConvertisseuseExportPolyMesh *, uint64_t);

    void (*coins_pour_polygone)(struct ConvertisseuseExportPolyMesh *, uint64_t, int *);
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

typedef enum eAbcPortee {
    AUCUNE,
    POINT,
    PRIMITIVE,
    POINT_PRIMITIVE,
    OBJECT,
} eAbcPortee;

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
                             eAbcPortee);

    void (*information_portee)(struct ConvertisseuseImportAttributs *,
                               int *points,
                               int *primitives,
                               int *points_primitives);

    // Ce n'est que pour un seul attribut
    void (*ajoute_bool)(void *, uint64_t, bool const *, int);
    void (*ajoute_n8)(void *, uint64_t, unsigned char const *, int);
    void (*ajoute_n16)(void *, uint64_t, unsigned short const *, int);
    void (*ajoute_n32)(void *, uint64_t, unsigned int const *, int);
    void (*ajoute_n64)(void *, uint64_t, uint64_t const *, int);
    void (*ajoute_z8)(void *, uint64_t, signed char const *, int);
    void (*ajoute_z16)(void *, uint64_t, short const *, int);
    void (*ajoute_z32)(void *, uint64_t, int const *, int);
    void (*ajoute_z64)(void *, uint64_t, long const *, int);
    void (*ajoute_r16)(void *, uint64_t, r16 const *, int);
    void (*ajoute_r32)(void *, uint64_t, float const *, int);
    void (*ajoute_r64)(void *, uint64_t, double const *, int);
    void (*ajoute_chaine)(void *, uint64_t, char const *, uint64_t);
};

#ifdef __cplusplus
}
#endif
