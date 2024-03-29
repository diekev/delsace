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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/math/transformation.hh"

#include "biblinternes/structures/liste.hh"

#include "accesseuses.hh"
#include "attribut.h"
#include "groupes.h"
#include "listes.h"

class Attribut;

struct Sphere;

/**
 * La structure Corps représente une partie constituante d'un objet. Le Corps
 * peut-être un maillage, ou un volume, ou autre (voir énumération ci-dessus).
 * Cette structure n'est qu'une base, définissant les propriétés générales de
 * tous les types de corps.
 */
struct Corps {
    /* transformation */
    math::transformation transformation = math::transformation();
    dls::math::point3f pivot = dls::math::point3f(0.0f);
    dls::math::point3f position = dls::math::point3f(0.0f);
    dls::math::point3f echelle = dls::math::point3f(1.0f);
    dls::math::point3f rotation = dls::math::point3f(0.0f);
    float echelle_uniforme = 1.0f;

    /* boîte englobante */
    dls::math::point3f min = dls::math::point3f(0.0f);
    dls::math::point3f max = dls::math::point3f(0.0f);
    dls::math::point3f taille = dls::math::point3f(0.0f);

    /* autres propriétés */
    dls::chaine nom = "corps";

    using plage_attributs = dls::outils::plage_iterable_liste<dls::liste<Attribut>::iteratrice>;
    using plage_const_attributs =
        dls::outils::plage_iterable_liste<dls::liste<Attribut>::const_iteratrice>;

    Corps() = default;
    virtual ~Corps();

    bool possede_attribut(dls::chaine const &nom_attribut);

    void ajoute_attribut(Attribut *attr);

    Attribut *ajoute_attribut(dls::chaine const &nom_attribut,
                              type_attribut type_,
                              int dimensions = 1,
                              portee_attr portee = portee_attr::POINT,
                              bool force_vide = false);

    void supprime_attribut(dls::chaine const &nom_attribut);

    Attribut *attribut(dls::chaine const &nom_attribut);

    Attribut const *attribut(dls::chaine const &nom_attribut) const;

    void ajoute_primitive(Primitive *p);

    void copie_points(Corps const autre);

    /**
     * Copie la liste de points de ce corps avant de la rendre unique à lui,
     * pour la copie sur écriture, et retourne le nouveau pointeur.
     */
    AccesseusePointEcriture points_pour_ecriture();
    friend struct AccesseusePointEcriture;

    /**
     * Retourne un pointeur vers la liste de point de ce corps. Les points ne
     * sont pas modifiables.
     */
    AccesseusePointLecture points_pour_lecture() const;

    ListePrimitives *prims();

    const ListePrimitives *prims() const;

    /* polygones */

    Polygone *ajoute_polygone(type_polygone type_poly, long nombre_sommets = 0);

    long ajoute_sommet(Polygone *p, long idx_point);

    long nombre_sommets() const;

    /* sphères */

    Sphere *ajoute_sphere(long idx_point, float rayon);

    /* autres */

    void reinitialise();

    Corps *copie() const;

    void copie_vers(Corps *corps) const;

    plage_attributs attributs();

    plage_const_attributs attributs() const;

    /* Groupes points. */

    using plage_grp_pnts = dls::outils::plage_iterable_liste<dls::liste<GroupePoint>::iteratrice>;
    using plage_const_grp_pnts =
        dls::outils::plage_iterable_liste<dls::liste<GroupePoint>::const_iteratrice>;

    GroupePoint *ajoute_groupe_point(dls::chaine const &nom_groupe);

    GroupePoint *groupe_point(const dls::chaine &nom_groupe) const;

    plage_grp_pnts groupes_points();

    plage_const_grp_pnts groupes_points() const;

    /* Groupes primitives. */

    using plage_grp_prims =
        dls::outils::plage_iterable_liste<dls::liste<GroupePrimitive>::iteratrice>;
    using plage_const_grp_prims =
        dls::outils::plage_iterable_liste<dls::liste<GroupePrimitive>::const_iteratrice>;

    GroupePrimitive *ajoute_groupe_primitive(dls::chaine const &nom_groupe);

    GroupePrimitive *groupe_primitive(dls::chaine const &nom_groupe) const;

    plage_grp_prims groupes_prims();

    plage_const_grp_prims groupes_prims() const;

  protected:
    dls::liste<Attribut> m_attributs{};

  private:
    void redimensionne_attributs(portee_attr portee);

    ListePoints3D m_points{};
    ListePrimitives m_prims{};

    dls::liste<GroupePoint> m_groupes_points{};
    dls::liste<GroupePrimitive> m_groupes_prims{};

    long m_nombre_sommets = 0;
};

bool possede_volume(Corps const &corps);

bool possede_sphere(Corps const &corps);
