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

#ifdef __cplusplus
}
#endif
