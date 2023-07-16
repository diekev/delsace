/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

#include "outils.hh"

namespace KNB {

class Disposition;
class Éditrice;
class RégionInterface;

enum class TypeÉditrice : int;
enum class TypeRégion : int;

class InterfaceGraphique {
    Disposition *m_disposition = nullptr;

  public:
    ~InterfaceGraphique();

    static void initialise_interface_par_défaut(InterfaceGraphique &interface);

    void installe_disposition(Disposition *nouvelle_disposition);

    DEFINIS_ACCESSEUR_MEMBRE(Disposition *, disposition);
};

/** } */

/* ------------------------------------------------------------------------- */
/** \nom Disposition.
 * Une disposition contient des régions à disposer soit horizontalement, soit
 * verticalement.
 * \{ */

/* ATTENTION : valeurs écrites dans les fichier de sauvegarde, ne pas changer
 * l'ordre ou supprimer des valeurs. */
enum class DirectionDisposition : int {
    HORIZONTAL,
    VERTICAL,
};

class Disposition {
    DirectionDisposition m_direction{};
    dls::tableau<RégionInterface *> m_régions{};

  public:
    static Disposition *crée(DirectionDisposition direction);

    ~Disposition();

    RégionInterface *ajoute_région_conteneur_éditrice();
    RégionInterface *ajoute_région_conteneur_région(DirectionDisposition direction);

    DEFINIS_ACCESSEUR_MEMBRE(DirectionDisposition, direction);
    DEFINIS_ACCESSEUR_MEMBRE_TABLEAU(dls::tableau<RégionInterface *>, régions);
};

/** } */

/* ------------------------------------------------------------------------- */
/** \nom RégionInterface.
 * Une région contient soit des éditrices, soit une disposition contenant
 * d'autres régions.
 * \{ */

/* ATTENTION : valeurs écrites dans les fichier de sauvegarde, ne pas changer
 * l'ordre ou supprimer des valeurs. */
enum class TypeRégion : int {
    CONTENEUR_RÉGION,
    CONTENEUR_ÉDITRICE,
};

dls::chaine nom_pour_type_éditrice(TypeÉditrice type);

class RégionInterface {
    TypeRégion m_type{};
    Disposition *m_disposition = nullptr;
    dls::tableau<Éditrice *> m_éditrices{};

  public:
    static RégionInterface *crée_conteneur_éditrice();
    static RégionInterface *crée_conteneur_région(DirectionDisposition direction);

    ~RégionInterface();

    Éditrice *ajoute_une_éditrice(TypeÉditrice type);

    void supprime_éditrice_à_l_index(int index);

    DEFINIS_ACCESSEUR_MEMBRE(Disposition *, disposition);
    DEFINIS_ACCESSEUR_MEMBRE(TypeRégion, type);
    DEFINIS_ACCESSEUR_MEMBRE_TABLEAU(dls::tableau<Éditrice *>, éditrices);
};

/** } */

/* ------------------------------------------------------------------------- */
/** \nom Représentation d'une éditrice pour afficher ou modifier des données.
 * \{ */

/* ATTENTION : valeurs écrites dans les fichier de sauvegarde, ne pas changer
 * l'ordre ou supprimer des valeurs. */
enum class TypeÉditrice : int {
    /* Affiche les résultats des composites. */
    VUE_2D,
    /* Affiche les objets 3D. */
    VUE_3D,
    /* Affiche les paramètres de la brosse. */
    PARAMÈTRES_BROSSE,
    /* Affiche les calques. */
    CALQUES,
    /* Affiche les logs. */
    INFORMATIONS,
};

class Éditrice {
    TypeÉditrice m_type{};
    dls::chaine m_nom{};

  public:
    static Éditrice *crée(TypeÉditrice type);

    DEFINIS_ACCESSEUR_MEMBRE(TypeÉditrice, type);
    DEFINIS_ACCESSEUR_MEMBRE(dls::chaine, nom);
};

}  // namespace KNB
