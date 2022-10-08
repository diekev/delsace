/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#ifdef __cplusplus
extern "C" {
#else
typedef char bool;
#endif

/**
 * Ensemble de fonctions de rappel utilisées par les routines de compression ou de décompression
 * afin de déléguer certaines actions à l'application Kuri.
 */
struct ContexteCompression {
    /**
     * Définis la taille du stockage temporaire qui sera utilisé par la bibliothèque.
     */
    unsigned int (*taille_stockage_temporaire)(struct ContexteCompression *);

    /**
     * Retourne un pointeur vers le stockage temporaire à utiliser. Chaque invocation de ce rappel
     * doit retourner un stockage différent.
     */
    unsigned char *(*cree_stockage_temporaire)(struct ContexteCompression *, unsigned int taille);

    /**
     * Détruit le stockage temporaire créé via cree_stockage_temporaire.
     */
    void (*detruit_stockage_temporaire)(struct ContexteCompression *,
                                        unsigned char *,
                                        unsigned int);

    /**
     * Un pointeur vers des données utilisateur possibles, qui peut être mis en place par
     * l'application cliente.
     */
    void *donnees_utilisateur;
};

/**
 * Ensemble de fonctions de rappel abstrayant un flux de données, soit pour l'entrée, soit la
 * sortie.
 */
struct FluxCompression {
    /**
     * Lis max quantité de données depuis le flux, et retourne la quantité lue.
     */
    unsigned int (*lis)(struct FluxCompression *, unsigned int max, unsigned char *donnees);

    /**
     * Écris max quantité de données depuis le flux, et retourne la quantité lue.
     */
    unsigned int (*ecris)(struct FluxCompression *, unsigned int max, unsigned char *donnees);

    /**
     * Doit retourner vrai si le flux possede une erreur.
     */
    bool (*possede_erreur)(struct FluxCompression *);

    /**
     * Retourne vrai si nous avons dépasser la fin du flux.
     */
    bool (*fin_de_flux)(struct FluxCompression *);

    /**
     * Un pointeur vers des données utilisateur possibles, qui peut être mis en place par
     * l'application cliente.
     */
    void *donnees_utilisateur;
};

/**
 * Compresse les données d'un flux d'entrée vers un flux de sortie au format Zlib.
 */
int ZLIB_compresse(struct ContexteCompression *ctx,
                   struct FluxCompression *flux_entree,
                   struct FluxCompression *flux_sortie,
                   int niveau);

/**
 * Décompresse les données d'un flux d'entrée vers un flux de sortie au format Zlib.
 */
int ZLIB_decompresse(struct ContexteCompression *ctx,
                     struct FluxCompression *flux_entree,
                     struct FluxCompression *flux_sortie);

#ifdef __cplusplus
}
#endif
