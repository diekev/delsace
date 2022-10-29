/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#ifdef __cplusplus
extern "C" {
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
    long taille_donnees;

    int largeur;
    int hauteur;
    int nombre_composants;
};

enum ResultatOperation IMG_ouvre_image(const char *chemin, struct ImageIO *image);

enum ResultatOperation IMG_ecris_image(const char *chemin, struct ImageIO *image);

void IMG_detruit_image(struct ImageIO *image);

void IMG_calcul_empreinte_floue(
    const char *chemin, int composant_x, int composant_y, char *resultat, long *taille_resultat);

/** Structure pour décrire la résolution d'une image.
 */
typedef struct DescriptionImage {
    int hauteur;
    int largeur;
} DescriptionImage;

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
    void *(*cree_calque)(struct AdaptriceImage *, const char *nom, long taille_nom);

    /** Rappel pour obtenir le nom du calque passé en paramètre. */
    void (*nom_calque)(const struct AdaptriceImage *,
                       const void *calque,
                       char **nom,
                       long *taille_nom);

    /** Rappel pour obtenir le nom du canal passé en paramètre. */
    void (*nom_canal)(const struct AdaptriceImage *,
                      const void *canal,
                      char **nom,
                      long *taille_nom);

    /** Rappel pour ajouter un canal dans un calque retourner par `cree_calque`.
     *  Ceci doit retourner le pointeur vers le canal créé.
     */
    void *(*ajoute_canal)(struct AdaptriceImage *, void *calque, const char *nom, long taille_nom);

    /* Accès aux calques et canaux, et aux données de l'image. */

    /** Rappel pour remplir la description de l'image. */
    void (*decris_image)(const struct AdaptriceImage *, struct DescriptionImage *);

    /** Rappel pour accéder au nombre de calques dans l'image. */
    int (*nombre_de_calques)(const struct AdaptriceImage *);

    /** Rappel pour accéder au calque à l'index donné. L'index est dans [0, nombre_de_calques), où
     * nombre_de_calques est la valeur retournée par `nombre_de_calques`. */
    const void *(*calque_pour_index)(const struct AdaptriceImage *, long index);

    /** Rappel pour accéder au nombre de canal dans le calque donné. */
    int (*nombre_de_canaux)(const struct AdaptriceImage *, const void *calque);

    /** Rappel pour accéder au canal du calque à l'index donné.
     * L'index est dans [0, nombre_de_canaux), où nombre_de_canaux est la valeur retournée par
     * `nombre_de_canaux`. */
    const void *(*canal_pour_index)(const struct AdaptriceImage *, const void *calque, long index);

    /** Rappel pour accéder aux données en lecture du canal. */
    const float *(*donnees_canal_pour_lecture)(const struct AdaptriceImage *, const void *canal);

    /** Rappel pour accéder aux données en écriture du canal. */
    float *(*donnees_canal_pour_ecriture)(const struct AdaptriceImage *, const void *canal);
};

enum ResultatOperation IMG_ouvre_image_avec_adaptrice(const char *chemin,
                                                      long taille_chemin,
                                                      struct AdaptriceImage *image);

enum ResultatOperation IMG_ecris_image_avec_adaptrice(const char *chemin,
                                                      long taille_chemin,
                                                      struct AdaptriceImage *image);

// ----------------------------------------------------------------------------
// Simumlation de grain sur image

struct ParametresSimulationGrain {
    /** Graine pour générer des nombres aléatoires et rendre le résultat unique. */
    unsigned int graine;

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
                          const float *rayon_flou_par_pixel);

#ifdef __cplusplus
}
#endif
