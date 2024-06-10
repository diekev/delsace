/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#else
typedef unsigned char bool;
#endif

enum ResultatOperation {
    OK,
    IMAGE_INEXISTANTE,
    TYPE_IMAGE_NON_SUPPORTE,
    AJOUT_CALQUE_IMPOSSIBLE,
    AJOUT_CANAL_IMPOSSIBLE,
    LECTURE_DONNEES_IMPOSSIBLE,
};

struct ImageIO {
    float *donnees;
    int64_t taille_donnees;

    int largeur;
    int hauteur;
    int nombre_composants;
};

enum ResultatOperation IMG_ouvre_image(const char *chemin, struct ImageIO *image);

enum ResultatOperation IMG_ecris_image(const char *chemin, struct ImageIO *image);

void IMG_detruit_image(struct ImageIO *image);

void IMG_calcul_empreinte_floue(const char *chemin,
                                int composant_x,
                                int composant_y,
                                char *resultat,
                                int64_t *taille_resultat);

/** Structure pour décrire la résolution d'une image.
 */
typedef struct DescriptionImage {
    int hauteur;
    int largeur;
} DescriptionImage;

/**
 * Structure pour définir la fenêtre de données d'une image.
 */
typedef struct IMG_Fenetre {
    int min_x;
    int max_x;
    int min_y;
    int max_y;
} IMG_Fenetre;

#define ENUMERE_DECLARATION_ENUM_IPA(nom_ipa, nom_natif) nom_ipa,

#define ENUMERE_TRANSLATION_ENUM_NATIF_VERS_IPA(nom_ipa, nom_natif)                               \
    case nom_natif:                                                                               \
    {                                                                                             \
        return nom_ipa;                                                                           \
    }

#define ENUM_IMAGEIO_DATATYPE(O)                                                                  \
    O(IMAGEIO_DATATYPE_UNKNOWN, OIIO::TypeDesc::UNKNOWN)                                          \
    O(IMAGEIO_DATATYPE_NONE, OIIO::TypeDesc::NONE)                                                \
    O(IMAGEIO_DATATYPE_UINT8, OIIO::TypeDesc::UINT8)                                              \
    O(IMAGEIO_DATATYPE_INT8, OIIO::TypeDesc::INT8)                                                \
    O(IMAGEIO_DATATYPE_UINT16, OIIO::TypeDesc::UINT16)                                            \
    O(IMAGEIO_DATATYPE_INT16, OIIO::TypeDesc::INT16)                                              \
    O(IMAGEIO_DATATYPE_UINT32, OIIO::TypeDesc::UINT32)                                            \
    O(IMAGEIO_DATATYPE_INT32, OIIO::TypeDesc::INT32)                                              \
    O(IMAGEIO_DATATYPE_UINT64, OIIO::TypeDesc::UINT64)                                            \
    O(IMAGEIO_DATATYPE_INT64, OIIO::TypeDesc::INT64)                                              \
    O(IMAGEIO_DATATYPE_HALF, OIIO::TypeDesc::HALF)                                                \
    O(IMAGEIO_DATATYPE_FLOAT, OIIO::TypeDesc::FLOAT)                                              \
    O(IMAGEIO_DATATYPE_DOUBLE, OIIO::TypeDesc::DOUBLE)                                            \
    O(IMAGEIO_DATATYPE_STRING, OIIO::TypeDesc::STRING)                                            \
    O(IMAGEIO_DATATYPE_PTR, OIIO::TypeDesc::PTR)

enum ImageIO_DataType { ENUM_IMAGEIO_DATATYPE(ENUMERE_DECLARATION_ENUM_IPA) };

#define ENUM_IMAGEIO_AGGREGATETYPE(O)                                                             \
    O(IMAGEIO_AGGREGATETYPE_SCALAR, OIIO::TypeDesc::SCALAR)                                       \
    O(IMAGEIO_AGGREGATETYPE_VEC2, OIIO::TypeDesc::VEC2)                                           \
    O(IMAGEIO_AGGREGATETYPE_VEC3, OIIO::TypeDesc::VEC3)                                           \
    O(IMAGEIO_AGGREGATETYPE_VEC4, OIIO::TypeDesc::VEC4)                                           \
    O(IMAGEIO_AGGREGATETYPE_MATRIX33, OIIO::TypeDesc::MATRIX33)                                   \
    O(IMAGEIO_AGGREGATETYPE_MATRIX44, OIIO::TypeDesc::MATRIX44)

enum ImageIO_AggregateType { ENUM_IMAGEIO_AGGREGATETYPE(ENUMERE_DECLARATION_ENUM_IPA) };

struct ImageIO_Chaine {
    const char *caractères;
    uint64_t taille;
};

void IMG_detruit_chaine(struct ImageIO_Chaine *chn);

enum ImageIO_Options_Lecture {
    /* Lis les pixels de l'image. */
    IMAGEIO_OPTIONS_LECTURE_LIS_PIXELS = 1,
    /* Lis les attributs de l'image. */
    IMAGEIO_OPTIONS_LECTURE_LIS_ATTRIBUTS = 2,
};

/** Structure de rappel pour créer des calques et des canaux dans une image, ou pour accéder à
 * ceux-ci.
 * Les applications clientes doivent dériver cette structure afin de placer leurs données
 * spécifiques dans la structure dérivée.
 */
struct AdaptriceImage {
    /* Création de calques et canaux. */

    /** Rappel pour initialiser les données de l'image. */
    void (*initialise_image)(struct AdaptriceImage *, const DescriptionImage *desc);

    /** Rappel pour créer un calque dans l'image. Ceci doit retourner le pointeur vers le
     * nouveau calque créer. */
    void *(*cree_calque)(struct AdaptriceImage *, const char *nom, int64_t taille_nom);

    /** Rappel pour obtenir le nom du calque passé en paramètre. */
    void (*nom_calque)(const struct AdaptriceImage *,
                       const void *calque,
                       char **nom,
                       int64_t *taille_nom);

    /** Rappel pour obtenir le nom du canal passé en paramètre. */
    void (*nom_canal)(const struct AdaptriceImage *,
                      const void *canal,
                      char **nom,
                      int64_t *taille_nom);

    /** Rappel pour ajouter un canal dans un calque retourner par `cree_calque`.
     *  Ceci doit retourner le pointeur vers le canal créé.
     */
    void *(*ajoute_canal)(struct AdaptriceImage *,
                          void *calque,
                          const char *nom,
                          int64_t taille_nom);

    /* Accès aux calques et canaux, et aux données de l'image. */

    /** Rappel pour remplir la description de l'image. */
    void (*decris_image)(const struct AdaptriceImage *, struct DescriptionImage *);

    /** Rappel pour remplir la fenêtre de l'image. */
    void (*fenetre_image)(const struct AdaptriceImage *, struct IMG_Fenetre *);

    /** Rappel pour accéder au nombre de calques dans l'image. */
    int (*nombre_de_calques)(const struct AdaptriceImage *);

    /** Rappel pour accéder au calque à l'index donné. L'index est dans [0, nombre_de_calques), où
     * nombre_de_calques est la valeur retournée par `nombre_de_calques`. */
    const void *(*calque_pour_index)(const struct AdaptriceImage *, int64_t index);

    /** Rappel pour accéder au nombre de canal dans le calque donné. */
    int (*nombre_de_canaux)(const struct AdaptriceImage *, const void *calque);

    /** Rappel pour accéder au canal du calque à l'index donné.
     * L'index est dans [0, nombre_de_canaux), où nombre_de_canaux est la valeur retournée par
     * `nombre_de_canaux`. */
    const void *(*canal_pour_index)(const struct AdaptriceImage *,
                                    const void *calque,
                                    int64_t index);

    /** Rappel pour accéder aux données en lecture du canal. */
    const float *(*donnees_canal_pour_lecture)(const struct AdaptriceImage *, const void *canal);

    /** Rappel pour accéder aux données en écriture du canal. */
    float *(*donnees_canal_pour_ecriture)(const struct AdaptriceImage *, const void *canal);

    void (*ajoute_attribut)(const struct AdaptriceImage *,
                            struct ImageIO_Chaine *nom,
                            enum ImageIO_DataType type,
                            enum ImageIO_AggregateType aggregate,
                            const void *donnees,
                            int nombre_valeur);
};

struct ImageIO_RappelsProgression {
    bool (*rappel_progression)(struct ImageIO_RappelsProgression *, float);
};

enum ResultatOperation IMG_ouvre_image_avec_adaptrice(const char *chemin,
                                                      int64_t taille_chemin,
                                                      struct AdaptriceImage *image,
                                                      struct ImageIO_RappelsProgression *rappels,
                                                      enum ImageIO_Options_Lecture options);

enum ResultatOperation IMG_ecris_image_avec_adaptrice(const char *chemin,
                                                      int64_t taille_chemin,
                                                      struct AdaptriceImage *image,
                                                      struct ImageIO_RappelsProgression *rappels);

struct ImageIO_Chaine IMG_donne_filtre_extensions();

// ----------------------------------------------------------------------------
// Simumlation de grain sur image

struct ParametresSimulationGrain {
    /** Graine pour générer des nombres aléatoires et rendre le résultat unique. */
    uint32_t graine;

    /** Le nombre d'itérations de Monte-Carlo à effectuer pour générer le grain. */
    int iterations;

    /** Rayon des grains générés. Ceci est également la distance minimale à préserver entre les
     * grains. */
    float rayon_r;
    float rayon_v;
    float rayon_b;

    /** Variation du rayon des grains. Ceci est en fait l'écart-type d'une distribution normale où
     * le rayon des grains est la moyenne. Cette valeur peut donc être supérieure à 1. */
    float sigma_r;
    float sigma_v;
    float sigma_b;

    /** Variation du flou des grains. Ceci est en fait l'écart-type d'une distribution normale où
     * le rayon des grains est la moyenne. Cette valeur peut donc être supérieure à 1. */
    float sigma_filtre_r;
    float sigma_filtre_v;
    float sigma_filtre_b;
};

/**
 * Simule du grain sur l'image d'entrée.
 * Le grain est simulé pour tous les calques de l'image d'entrée.
 * Pour chaque calque, jusque trois canaux sont utilisés pour la simulation; ces canaux sont les
 * premiers canaux de l'image.
 * Le résultat sera place dans l'image de sortie. Des calques et canaux avec des noms similaires à
 * ceux de l'image d'entrée seront créés dans l'image de sortie. Seuls les canaux résultats seront
 * dans l'image de sortie : aucune copie des autres canaux de l'image d'entrée ne sera faite.
 */
void IMG_simule_grain_image(const struct ParametresSimulationGrain *params,
                            const struct AdaptriceImage *image_entree,
                            struct AdaptriceImage *image_sortie);

// ----------------------------------------------------------------------------
// Filtrage de l'image

enum IMG_TypeFiltre {
    TYPE_FILTRE_BOITE,
    TYPE_FILTRE_TRIANGULAIRE,
    TYPE_FILTRE_QUADRATIC,
    TYPE_FILTRE_CUBIC,
    TYPE_FILTRE_GAUSSIEN,
    TYPE_FILTRE_MITCHELL,
    TYPE_FILTRE_CATROM,
};

struct IMG_ParametresFiltrageImage {
    enum IMG_TypeFiltre filtre;
    float rayon;
};

void IMG_filtre_image(const struct IMG_ParametresFiltrageImage *params,
                      const struct AdaptriceImage *entree,
                      struct AdaptriceImage *sortie);

// ----------------------------------------------------------------------------
// Affinage de l'image.

struct IMG_ParametresAffinageImage {
    enum IMG_TypeFiltre filtre;
    float rayon;
    float poids;
};

void IMG_affine_image(const struct IMG_ParametresAffinageImage *params,
                      const struct AdaptriceImage *entree,
                      struct AdaptriceImage *sortie);

// ----------------------------------------------------------------------------
// Dilatation de l'image.

struct IMG_ParametresDilatationImage {
    int rayon;
};

void IMG_dilate_image(const struct IMG_ParametresDilatationImage *params,
                      const struct AdaptriceImage *entree,
                      struct AdaptriceImage *sortie);

// ----------------------------------------------------------------------------
// Érosion d'image.

void IMG_erode_image(const struct IMG_ParametresDilatationImage *params,
                     const struct AdaptriceImage *entree,
                     struct AdaptriceImage *sortie);

// ----------------------------------------------------------------------------
// Filtrage médian de l'image.

struct IMG_ParametresMedianImage {
    int rayon;
};

void IMG_filtre_median_image(const struct IMG_ParametresMedianImage *params,
                             const struct AdaptriceImage *entree,
                             struct AdaptriceImage *sortie);

// ----------------------------------------------------------------------------
// Filtrage bilatéral de l'image.

struct IMG_ParametresFiltreBilateralImage {
    int rayon;
    float sigma_s;
    float sigma_i;
};

void IMG_filtre_bilateral_image(const struct IMG_ParametresFiltreBilateralImage *params,
                                const struct AdaptriceImage *entree,
                                struct AdaptriceImage *sortie);

// ----------------------------------------------------------------------------
// Champs de distance de l'image.

enum IMG_TypeChampsDeDistance {
    BALAYAGE_RAPIDE,
    NAVIGATION_ESTIME,
    DISTANCE_EUCLIDIENNE_SIGNEE_SEQUENTIELLE
};

struct IMG_ParametresChampsDeDistance {
    float iso;
    enum IMG_TypeChampsDeDistance methode;
    int emets_gradients;
};

void IMG_genere_champs_de_distance(const struct IMG_ParametresChampsDeDistance *params,
                                   const struct AdaptriceImage *entree,
                                   struct AdaptriceImage *sortie);

// ----------------------------------------------------------------------------
// Défocalisation de l'image.

void IMG_defocalise_image(const struct AdaptriceImage *image_entree,
                          struct AdaptriceImage *image_sortie,
                          struct IMG_Fenetre *fenetre,
                          const float *rayon_flou_par_pixel);

// ----------------------------------------------------------------------------
// Rééchantillonnage de l'image.

typedef struct IMG_ParametresReechantillonnage {
    enum IMG_TypeFiltre type_filtre;
    int taille_filtre;
    int nouvelle_largeur;
    int nouvelle_hauteur;
} IMG_ParametresReechantillonnage;

void IMG_reechantillonne_image(const struct IMG_ParametresReechantillonnage *params,
                               const struct AdaptriceImage *entree,
                               struct AdaptriceImage *sortie);

#ifdef __cplusplus
}
#endif
