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
    ERREUR_INCONNUE,
    IMAGE_NULLE,
    IMAGE_INEXISTANTE,
    TYPE_IMAGE_NON_SUPPORTE,
    PROXY_NON_SUPPORTE,
    AJOUT_CALQUE_IMPOSSIBLE,
    AJOUT_CANAL_IMPOSSIBLE,
    LECTURE_DONNEES_IMPOSSIBLE,
};

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

struct ImageIO {
    uint8_t *donnees;
    int64_t taille_donnees;

    int largeur;
    int hauteur;
    int nombre_composants;
    enum ImageIO_DataType format;
};

enum ResultatOperation IMG_ouvre_gif_depuis_fichier(const char *chemin, struct ImageIO *resultat);
enum ResultatOperation IMG_ouvre_gif_depuis_memoire(const void *donnees,
                                                    uint64_t taille,
                                                    struct ImageIO *resultat);

enum ResultatOperation IMG_ecris_image(const char *chemin, struct ImageIO *image);

void IMG_detruit_image(struct ImageIO *image);

void IMG_calcule_empreinte_floue_octet(unsigned char *image,
                                       int largeur,
                                       int hauteur,
                                       int nombre_canaux,
                                       int composant_x,
                                       int composant_y,
                                       char *resultat,
                                       int64_t *taille_resultat);

void IMG_calcule_empreinte_floue_reel(float *image,
                                      int largeur,
                                      int hauteur,
                                      int nombre_canaux,
                                      int composant_x,
                                      int composant_y,
                                      char *resultat,
                                      int64_t *taille_resultat);

uint8_t *IMG_decode_empreinte_floue(
    const char *empreinte, int largeur, int hauteur, int punch, int canaux);

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

struct ImageIO_Chaine IMG_donne_liste_extensions(void);

void IMG_donne_erreur(struct ImageIO_Chaine *résultat);

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

// ----------------------------------------------------------------------------
// Image SVG.

struct NSVGimage;
struct NSVGrasterizer;

typedef struct SVGImage {
    struct NSVGimage *image;
    struct NSVGrasterizer *ratisseuse;
    float width;
    float height;
} SVGImage;

bool SVG_parse_image_depuis_contenu(char *data, struct SVGImage *resultat);

void SVG_image_ratisse(struct SVGImage *image, uint8_t *sortie, int largeur, int hauteur);

void SVG_image_detruit(struct SVGImage *image);

// ----------------------------------------------------------------------------
// OIIO.

int64_t OIIO_AutoStride();

struct OIIO_StringView {
    const char *characters;
    uint64_t size;
};

#define OIIO_USTRING_SIZE 8
#define OIIO_USTRING_ALIGNMENT 8

struct OIIO_ustring {
    char data[OIIO_USTRING_SIZE];
} __attribute__((aligned(OIIO_USTRING_ALIGNMENT)));

const char *OIIO_ustring_c_str(struct OIIO_ustring *str);

uint64_t OIIO_ustring_size(struct OIIO_ustring *str);

struct OIIO_TypeDesc {
    unsigned char basetype;      ///< C data type at the heart of our type
    unsigned char aggregate;     ///< What kind of AGGREGATE is it?
    unsigned char vecsemantics;  ///< Hint: What does the aggregate represent?
    unsigned char reserved;      ///< Reserved for future expansion
    int arraylen;                ///< Array length, 0 = not array, -1 = unsized
};

/// BASETYPE is a simple enum describing the base data types that
/// correspond (mostly) to the C/C++ built-in types.
enum OIIO_TYPEDESC_BASETYPE {
    OIIO_TYPEDESC_BASETYPE_UNKNOWN,  ///< unknown type
    OIIO_TYPEDESC_BASETYPE_NONE,     ///< void/no type
    OIIO_TYPEDESC_BASETYPE_UINT8,    ///< 8-bit unsigned int values ranging from 0..255,
                                     ///<   (C/C++ `unsigned char`).
    OIIO_TYPEDESC_BASETYPE_UCHAR = OIIO_TYPEDESC_BASETYPE_UINT8,
    OIIO_TYPEDESC_BASETYPE_INT8,  ///< 8-bit int values ranging from -128..127,
                                  ///<   (C/C++ `char`).
    OIIO_TYPEDESC_BASETYPE_CHAR = OIIO_TYPEDESC_BASETYPE_INT8,
    OIIO_TYPEDESC_BASETYPE_UINT16,  ///< 16-bit int values ranging from 0..65535,
                                    ///<   (C/C++ `unsigned short`).
    OIIO_TYPEDESC_BASETYPE_USHORT = OIIO_TYPEDESC_BASETYPE_UINT16,
    OIIO_TYPEDESC_BASETYPE_INT16,  ///< 16-bit int values ranging from -32768..32767,
                                   ///<   (C/C++ `short`).
    OIIO_TYPEDESC_BASETYPE_SHORT = OIIO_TYPEDESC_BASETYPE_INT16,
    OIIO_TYPEDESC_BASETYPE_UINT32,  ///< 32-bit unsigned int values (C/C++ `unsigned int`).
    OIIO_TYPEDESC_BASETYPE_UINT = OIIO_TYPEDESC_BASETYPE_UINT32,
    OIIO_TYPEDESC_BASETYPE_INT32,  ///< signed 32-bit int values (C/C++ `int`).
    OIIO_TYPEDESC_BASETYPE_INT = OIIO_TYPEDESC_BASETYPE_INT32,
    OIIO_TYPEDESC_BASETYPE_UINT64,  ///< 64-bit unsigned int values (C/C++
                                    ///<   `unsigned long long` on most architectures).
    OIIO_TYPEDESC_BASETYPE_ULONGLONG = OIIO_TYPEDESC_BASETYPE_UINT64,
    OIIO_TYPEDESC_BASETYPE_INT64,  ///< signed 64-bit int values (C/C++ `long long`
                                   ///<   on most architectures).
    OIIO_TYPEDESC_BASETYPE_LONGLONG = OIIO_TYPEDESC_BASETYPE_INT64,
    OIIO_TYPEDESC_BASETYPE_HALF,         ///< 16-bit IEEE floating point values (OpenEXR `half`).
    OIIO_TYPEDESC_BASETYPE_FLOAT,        ///< 32-bit IEEE floating point values, (C/C++ `float`).
    OIIO_TYPEDESC_BASETYPE_DOUBLE,       ///< 64-bit IEEE floating point values, (C/C++ `double`).
    OIIO_TYPEDESC_BASETYPE_STRING,       ///< Character string.
    OIIO_TYPEDESC_BASETYPE_PTR,          ///< A pointer value.
    OIIO_TYPEDESC_BASETYPE_USTRINGHASH,  ///< A uint64 that is the hash of a ustring.
    OIIO_TYPEDESC_BASETYPE_LASTBASE
};

/// AGGREGATE describes whether our TypeDesc is a simple scalar of one
/// of the BASETYPE's, or one of several simple aggregates.
///
/// Note that aggregates and arrays are different. A `TypeDesc(FLOAT,3)`
/// is an array of three floats, a `TypeDesc(FLOAT,VEC3)` is a single
/// 3-component vector comprised of floats, and `TypeDesc(FLOAT,3,VEC3)`
/// is an array of 3 vectors, each of which is comprised of 3 floats.
enum OIIO_TYPEDESC_AGGREGATE {
    OIIO_TYPEDESC_AGGREGATE_SCALAR = 1,    ///< A single scalar value (such as a raw `int` or
                                           ///<   `float` in C).  This is the default.
    OIIO_TYPEDESC_AGGREGATE_VEC2 = 2,      ///< 2 values representing a 2D vector.
    OIIO_TYPEDESC_AGGREGATE_VEC3 = 3,      ///< 3 values representing a 3D vector.
    OIIO_TYPEDESC_AGGREGATE_VEC4 = 4,      ///< 4 values representing a 4D vector.
    OIIO_TYPEDESC_AGGREGATE_MATRIX33 = 9,  ///< 9 values representing a 3x3 matrix.
    OIIO_TYPEDESC_AGGREGATE_MATRIX44 = 16  ///< 16 values representing a 4x4 matrix.
};

/// VECSEMANTICS gives hints about what the data represent (for example,
/// if a spatial vector quantity should transform as a point, direction
/// vector, or surface normal).
enum OIIO_TYPEDESC_VECSEMANTICS {
    OIIO_TYPEDESC_VECSEMANTICS_NOXFORM = 0,      ///< No semantic hints.
    OIIO_TYPEDESC_VECSEMANTICS_NOSEMANTICS = 0,  ///< No semantic hints.
    OIIO_TYPEDESC_VECSEMANTICS_COLOR,            ///< Color
    OIIO_TYPEDESC_VECSEMANTICS_POINT,            ///< Point: a spatial location
    OIIO_TYPEDESC_VECSEMANTICS_VECTOR,           ///< Vector: a spatial direction
    OIIO_TYPEDESC_VECSEMANTICS_NORMAL,           ///< Normal: a surface normal
    OIIO_TYPEDESC_VECSEMANTICS_TIMECODE,  ///< indicates an `int[2]` representing the standard
                                          ///<   4-byte encoding of an SMPTE timecode.
    OIIO_TYPEDESC_VECSEMANTICS_KEYCODE,   ///< indicates an `int[7]` representing the standard
                                          ///<   28-byte encoding of an SMPTE keycode.
    OIIO_TYPEDESC_VECSEMANTICS_RATIONAL,  ///< A VEC2 representing a rational number `val[0] /
                                          ///< val[1]`
    OIIO_TYPEDESC_VECSEMANTICS_BOX,  ///< A VEC2[2] or VEC3[2] that represents a 2D or 3D bounds
                                     ///< (min/max)
};

uint64_t OIIO_TypeDesc_basesize(struct OIIO_TypeDesc *type_desc);

#define OIIO_PARAMVALUE_SIZE 40
#define OIIO_PARAMVALUE_ALIGNMENT 8

struct OIIO_ParamValue {
    char data[OIIO_PARAMVALUE_SIZE];
} __attribute__((aligned(OIIO_PARAMVALUE_ALIGNMENT)));

struct OIIO_TypeDesc OIIO_ParamValue_type(struct OIIO_ParamValue *param);

struct OIIO_StringView OIIO_ParamValue_name(struct OIIO_ParamValue *param);

const void *OIIO_ParamValue_data(struct OIIO_ParamValue *param);

int OIIO_ParamValue_nvavlues(struct OIIO_ParamValue *param);

#define OIIO_IMAGESPEC_SIZE 160
#define OIIO_IMAGESPEC_ALIGNMENT 8

struct OIIO_ImageSpec {
    char data[OIIO_IMAGESPEC_SIZE];
} __attribute__((aligned(OIIO_IMAGESPEC_ALIGNMENT)));

void OIIO_ImageSpec_init(struct OIIO_ImageSpec *spec);

int OIIO_ImageSpec_donne_width(struct OIIO_ImageSpec *spec);

void OIIO_ImageSpec_definis_width(struct OIIO_ImageSpec *spec, int width);

int OIIO_ImageSpec_donne_height(struct OIIO_ImageSpec *spec);

void OIIO_ImageSpec_definis_height(struct OIIO_ImageSpec *spec, int height);

int OIIO_ImageSpec_donne_nchannels(struct OIIO_ImageSpec *spec);

void OIIO_ImageSpec_definis_nchannels(struct OIIO_ImageSpec *spec, int nchannels);

struct OIIO_TypeDesc OIIO_ImageSpec_donne_format(struct OIIO_ImageSpec *spec);

void OIIO_ImageSpec_definis_format(struct OIIO_ImageSpec *spec, struct OIIO_TypeDesc format);

struct OIIO_ParamValue *OIIO_ImageSpec_donne_extra_attribs(struct OIIO_ImageSpec *spec);

uint64_t OIIO_ImageSpec_donne_extra_attribs_size(struct OIIO_ImageSpec *spec);

struct OIIO_Filesystem_IOProxy;

struct OIIO_Filesystem_IOProxy *OIIO_Filesytem_IOMemReader_new(void *buf, uint64_t size);

void OIIO_Filesytem_IOMemReader_delete(struct OIIO_Filesystem_IOProxy *proxy);

struct OIIO_ImageInput;

struct OIIO_ImageInput *OIIO_ImageInput_open(struct OIIO_StringView chemin,
                                             struct OIIO_ImageSpec *config,
                                             struct OIIO_Filesystem_IOProxy *ioproxy);

bool OIIO_ImageInput_close(struct OIIO_ImageInput *image);

void OIIO_ImageInput_delete(struct OIIO_ImageInput *image);

struct OIIO_ImageSpec *OIIO_ImageInput_spec(struct OIIO_ImageInput *image);

bool OIIO_ImageInput_supports(struct OIIO_ImageInput *image, struct OIIO_StringView feature);

typedef bool (*OIIO_ProgressCallback)(void *data, float progress);

bool OIIO_ImageInput_read_image(struct OIIO_ImageInput *image,
                                int subimage,
                                int miplevel,
                                int chbegin,
                                int chend,
                                struct OIIO_TypeDesc format,
                                void *data,
                                int64_t xstride,
                                int64_t ystride,
                                int64_t zstride,
                                OIIO_ProgressCallback progress_callback,
                                void *progress_callback_data);

struct OIIO_ImageOutput;

struct OIIO_ImageOutput *OIIO_ImageOutput_create(struct OIIO_StringView filename,
                                                 struct OIIO_Filesystem_IOProxy *ioproxy,
                                                 struct OIIO_StringView plugin_searchpath);

bool OIIO_ImageOutput_close(struct OIIO_ImageOutput *output);

void OIIO_ImageOutput_delete(struct OIIO_ImageOutput *output);

enum OIIO_ImageOutput_OpenMode {
    OIIO_IMAGEOUTPUT_OPENMODE_CREATE,
    OIIO_IMAGEOUTPUT_OPENMODE_APPEND_SUB_IMAGE,
    OIIO_IMAGEOUTPUT_OPENMODE_APPEND_MIP_LEVEL,
};

bool OIIO_ImageOutput_open(struct OIIO_ImageOutput *output,
                           struct OIIO_StringView filename,
                           struct OIIO_ImageSpec *newspec,
                           enum OIIO_ImageOutput_OpenMode open_mode);

bool OIIO_ImageOutput_supports(struct OIIO_ImageOutput *output, struct OIIO_StringView feature);

bool OIIO_ImageOutput_write_image(struct OIIO_ImageOutput *output,
                                  struct OIIO_TypeDesc format,
                                  const void *data,
                                  int64_t xstride,
                                  int64_t ystride,
                                  int64_t zstride,
                                  OIIO_ProgressCallback progress_callback,
                                  void *progress_callback_data);

#ifdef __cplusplus
}
#endif
