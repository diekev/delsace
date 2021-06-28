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
    void *(*reloge_memoire)(ContexteKuri *ctx,
                            void *ancien_pointeur,
                            unsigned long ancienne_taille,
                            unsigned long nouvelle_taille);
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

    /* Rappels pour les erreurs, afin de savoir ce qui s'est passé. */
    void (*erreur_aucun_chemin)(ContexteOuvertureArchive *ctx);
    void (*erreur_archive_invalide)(ContexteOuvertureArchive *ctx);

    /* Les données utilisateurs du contexte. */
    void *donnees_utilisateurs = nullptr;
};

// --------------------------------------------------------------
// Traversé de l'archive.

struct ContexteTraverseArchive {
    // Extraction du nom de l'objet courant.
    void (*extrait_nom_courant)(ContexteTraverseArchive *ctx,
                                const char *pointeur,
                                unsigned long taille);

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

using TypeRappelReserveMemoire = void(*)(void *, unsigned long);
using TypeRappelAjouteUnPoint = void(*)(void *, float, float, float);
using TypeRappelAjouteTousLesPoints = void(*)(void *, const float *, unsigned long);
using TypeRappelAjoutepolygone = void (*)(void *, unsigned long, const int *, int);
using TypeRappelAjouteTousLesPolygones = void (*)(void *, const int *, unsigned long);
using TypeRappelReserveCoinsPolygone = void (*)(void *, unsigned long, int);
using TypeRappelAjouteCoinPolygone = void (*)(void *, unsigned long, int);
using TypeRappelAjouteTousLesCoins = void (*)(void *, const int *, unsigned long);
using TypeRappelMarquePolygoneTrou = void (*)(void *, int);
using TypeRappelMarquePlisVertex = void (*)(void *, int, float);
using TypeRappelMarquePlisAretes = void (*)(void *, int, int, float);
using TypeRappelMarqueSchemaSubdivision = void (*)(void *, const char *, unsigned long);
using TypeRappelMarquePropagationCoinsFaceVarying = void (*)(void *, int);
using TypeRappelMarqueInterpolationFrontiereFaceVarying = void (*)(void *, int);
using TypeRappelMarqueInterpolationFrontiere = void (*)(void *, int);
using TypeRappelAjouteIndexPoint = void (*)(void*, unsigned long, unsigned long);

struct ConvertisseusePolyMesh {
    void *donnees = nullptr;

    TypeRappelReserveMemoire reserve_points = nullptr;
    TypeRappelAjouteUnPoint ajoute_un_point = nullptr;
    TypeRappelAjouteTousLesPoints ajoute_tous_les_points = nullptr;

    TypeRappelReserveMemoire reserve_polygones = nullptr;
    TypeRappelAjoutepolygone ajoute_polygone = nullptr;
    TypeRappelAjouteTousLesPolygones ajoute_tous_les_polygones = nullptr;

    TypeRappelReserveMemoire reserve_coin = nullptr;
    TypeRappelReserveCoinsPolygone reserve_coins_polygone = nullptr;
    TypeRappelAjouteCoinPolygone ajoute_coin_polygone = nullptr;
    TypeRappelAjouteTousLesCoins ajoute_tous_les_coins = nullptr;
};

struct ConvertisseuseSubD {
    void *donnees = nullptr;

    TypeRappelReserveMemoire reserve_points = nullptr;
    TypeRappelAjouteUnPoint ajoute_un_point = nullptr;
    TypeRappelAjouteTousLesPoints ajoute_tous_les_points = nullptr;

    TypeRappelReserveMemoire reserve_polygones = nullptr;
    TypeRappelAjoutepolygone ajoute_polygone = nullptr;
    TypeRappelAjouteTousLesPolygones ajoute_tous_les_polygones = nullptr;

    TypeRappelReserveMemoire reserve_coin = nullptr;
    TypeRappelReserveCoinsPolygone reserve_coins_polygone = nullptr;
    TypeRappelAjouteCoinPolygone ajoute_coin_polygone = nullptr;
    TypeRappelAjouteTousLesCoins ajoute_tous_les_coins = nullptr;

    TypeRappelReserveMemoire reserve_trous = nullptr;
    TypeRappelMarquePolygoneTrou marque_polygone_trou = nullptr;

    TypeRappelReserveMemoire reserve_plis_sommets = nullptr;
    TypeRappelMarquePlisVertex marque_plis_vertex = nullptr;

    TypeRappelReserveMemoire reserve_plis_aretes = nullptr;
    TypeRappelMarquePlisAretes marque_plis_aretes = nullptr;

    TypeRappelMarqueSchemaSubdivision marque_schema_subdivision = nullptr;
    TypeRappelMarquePropagationCoinsFaceVarying marque_propagation_coins_face_varying = nullptr;
    TypeRappelMarqueInterpolationFrontiereFaceVarying marque_interpolation_frontiere_face_varying = nullptr;
    TypeRappelMarqueInterpolationFrontiere marque_interpolation_frontiere = nullptr;
};

struct ConvertisseusePoints {
    void *donnees = nullptr;

    TypeRappelReserveMemoire reserve_points = nullptr;
    TypeRappelAjouteUnPoint ajoute_un_point = nullptr;
    TypeRappelAjouteTousLesPoints ajoute_tous_les_points = nullptr;

    TypeRappelReserveMemoire reserve_index = nullptr;
    TypeRappelAjouteIndexPoint ajoute_index_point = nullptr;
};

struct ConvertisseuseCourbes {
    void *donnees = nullptr;
};

struct ConvertisseuseNurbs {
    void *donnees = nullptr;
};

struct ConvertisseuseXform {
    void *donnees = nullptr;
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
