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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

extern "C" {

struct ContexteKuri {
    void *(*loge_memoire)(ContexteKuri *ctx, unsigned long taille);
    void *(*reloge_memoire)(ContexteKuri *ctx, void *ancien_pointeur, unsigned long ancienne_taille, unsigned long nouvelle_taille);
    void (*deloge_memoire)(ContexteKuri *ctx, void *ancien_pointeur, unsigned long taille);
};

struct ArchiveCache;
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
    int (*nombre_de_chemins)(ContexteOuvertureArchive *ctx);
    /* Accède au chemin pour index données. */
    void (*chemin)(ContexteOuvertureArchive *ctx, int i, char **pointeur, unsigned long *taille);

    /* Pour les erreurs venant d'Alembic. */
    eAbcPoliceErreur (*police_erreur)(ContexteOuvertureArchive *ctx);

    /* Pour Ogawa. */
    int (*nombre_de_flux_ogawa_desires)(ContexteOuvertureArchive *ctx);
    eAbcStrategieLectureOgawa (*strategie_lecture_ogawa)(ContexteOuvertureArchive *ctx);

    /* Rappels pour les erreurs, afin de savoir ce qui sait passé. */
    void (*erreur_aucun_chemin)(ContexteOuvertureArchive *ctx);
    void (*erreur_archive_invalide)(ContexteOuvertureArchive *ctx);

    /* Les données utilisateurs du contexte. */
    void *donnees_utilisateurs = nullptr;
};

// --------------------------------------------------------------
// Traversé de l'archive.

struct ContexteTraverseArchive {
    // Extraction du nom de l'objet courant.
    void (*extrait_nom_courant)(ContexteTraverseArchive *ctx, const char *pointeur, unsigned long taille);

    // Certaines tâches peuvent prendre du temps, ce rappel sers à annuler l'opération en cours.
    bool (*annule)(ContexteTraverseArchive *ctx);

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

    void *donnees_utilisateur = nullptr;
};

// --------------------------------------------------------------
// Lecture des objets.

struct ConvertisseusePolyMesh {
    void *donnees;

    void (*reserve_points)(void *, unsigned long);
    void (*ajoute_un_point)(void *, float, float, float);
    void (*ajoute_tous_les_points)(void *, const float *, unsigned long);

    void (*reserve_polygones)(void *, unsigned long);
    void (*ajoute_polygone)(void *, unsigned long, const int *, int);
    void (*ajoute_tous_les_polygones)(void *, const int *, unsigned long);

    void (*reserve_coin)(void *, unsigned long);
    void (*reserve_coins_polygone)(void *, unsigned long, int);
    void (*ajoute_coin_polygone)(void *, unsigned long, int);
    void (*ajoute_tous_les_coins)(void *, const int *, unsigned long);
};

struct ConvertisseuseSubD {
    void *donnees;

    void (*reserve_points)(void *, unsigned long);
    void (*ajoute_un_point)(void *, float, float, float);
    void (*ajoute_tous_les_points)(void *, const float *, unsigned long);

    void (*reserve_polygones)(void *, unsigned long);
    void (*ajoute_polygone)(void *, unsigned long, const int *, int);
    void (*ajoute_tous_les_polygones)(void *, const int *, unsigned long);

    void (*reserve_coin)(void *, unsigned long);
    void (*reserve_coins_polygone)(void *, unsigned long, int);
    void (*ajoute_coin_polygone)(void *, unsigned long, int);
    void (*ajoute_tous_les_coins)(void *, const int *, unsigned long);
};

struct ConvertisseusePoints {
    void *donnees;

    void (*reserve_points)(void *, unsigned long);
    void (*ajoute_un_point)(void *, float, float, float);
    void (*ajoute_tous_les_points)(void *, const float *, unsigned long);
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

struct ContexteLectureCache {
    void (*initialise_convertisseuse_polymesh)(ConvertisseusePolyMesh *);

    void (*initialise_convertisseuse_subd)(ConvertisseuseSubD *);

    void (*initialise_convertisseuse_points)(ConvertisseusePoints *);

    void (*initialise_convertisseuse_courbes)(ConvertisseuseCourbes *);

    void (*initialise_convertisseuse_nurbs)(ConvertisseuseNurbs *);

    void (*initialise_convertisseuse_xform)(ConvertisseuseXform *);
};

}
