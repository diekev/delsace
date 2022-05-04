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
 * The Original Code is Copyright (C) 2022 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <optional>
#include <string>

#include "biblinternes/math/limites.hh"
#include "biblinternes/math/vecteur.hh"
#include "biblinternes/structures/tableau.hh"

using namespace dls;

#include "ipa.h"

inline std::string vers_std_string(const char *chemin, long taille_chemin)
{
    return std::string(chemin, static_cast<size_t>(taille_chemin));
}

namespace geo {

class Attribut : public AdaptriceAttribut {
  public:
    static Attribut enveloppe(AdaptriceAttribut *adaptrice)
    {
        return {*adaptrice};
    }
};

template <int TypeAttribut>
struct SelectriceClasseAttribut;

#define FLUX_VALEUR(type, nom)                                                                    \
    type lis_##nom(long index) const                                                              \
    {                                                                                             \
        type v;                                                                                   \
        this->lis_##nom##_pour_index(this->donnees_utilisateur, index, &v);                       \
        return v;                                                                                 \
    }                                                                                             \
    void ecris_##nom(long index, type v) const                                                    \
    {                                                                                             \
        this->ecris_##nom##_a_l_index(this->donnees_utilisateur, index, v);                       \
    }

#define FLUX_VECTEUR(type, nom)                                                                   \
    type lis_##nom(long index) const                                                              \
    {                                                                                             \
        type v;                                                                                   \
        this->lis_##nom##_pour_index(this->donnees_utilisateur, index, v);                        \
        return v;                                                                                 \
    }                                                                                             \
    void ecris_##nom(long index, type v) const                                                    \
    {                                                                                             \
        this->ecris_##nom##_a_l_index(this->donnees_utilisateur, index, v);                       \
    }

#define ATTRIBUT_VALEUR(nom_classe, type, nom, op, valeur_enum_attribut)                          \
    class nom_classe : public Attribut {                                                          \
      public:                                                                                     \
        static nom_classe enveloppe(AdaptriceAttribut *adaptrice)                                 \
        {                                                                                         \
            return {*adaptrice};                                                                  \
        }                                                                                         \
        op(type, nom);                                                                            \
        void copie(dls::tableau<type> const &valeurs)                                             \
        {                                                                                         \
            for (long i = 0; i < valeurs.taille(); i++) {                                         \
                ecris_##nom(i, valeurs[i]);                                                       \
            }                                                                                     \
        }                                                                                         \
    };                                                                                            \
    template <>                                                                                   \
    struct SelectriceClasseAttribut<valeur_enum_attribut> {                                       \
        using Type = nom_classe;                                                                  \
    };

ATTRIBUT_VALEUR(AttributBool, bool, bool, FLUX_VALEUR, TypeAttributGeo3D::BOOL)
ATTRIBUT_VALEUR(AttributEntier, int, entier, FLUX_VALEUR, TypeAttributGeo3D::Z32)
ATTRIBUT_VALEUR(AttributReel, float, reel, FLUX_VALEUR, TypeAttributGeo3D::R32)
ATTRIBUT_VALEUR(AttributVec2, math::vec2f, vec2, FLUX_VECTEUR, TypeAttributGeo3D::VEC2)
ATTRIBUT_VALEUR(AttributVec3, math::vec3f, vec3, FLUX_VECTEUR, TypeAttributGeo3D::VEC3)
ATTRIBUT_VALEUR(AttributVec4, math::vec4f, vec4, FLUX_VECTEUR, TypeAttributGeo3D::VEC4)
ATTRIBUT_VALEUR(AttributCouleur, math::vec4f, couleur, FLUX_VECTEUR, TypeAttributGeo3D::COULEUR)

#undef FLUX_VALEUR
#undef FLUX_VECTEUR
#undef ATTRIBUT_VALEUR

class Maillage : public AdaptriceMaillage {
  public:
    static Maillage enveloppe(AdaptriceMaillage *adaptrice)
    {
        return {*adaptrice};
    }

    /* Interface pour les points. */

    long nombreDePoints() const;

    math::vec3f pointPourIndex(long n) const;

    void ajoutePoints(float *points, long nombre) const;

    void reserveNombreDePoints(long nombre) const;

    void ajouteUnPoint(float x, float y, float z) const;

    void remplacePointALIndex(long n, math::vec3f const &point);

    template <TypeAttributGeo3D TypeAttribut>
    typename SelectriceClasseAttribut<TypeAttribut>::Type ajouteAttributPoint(
        const std::string &nom)
    {
        using ClasseAttribut = typename SelectriceClasseAttribut<TypeAttribut>::Type;
        AdaptriceAttribut adaptrice;
        if (!this->ajoute_attribut_sur_points) {
            return ClasseAttribut::enveloppe(&adaptrice);
        }
        this->ajoute_attribut_sur_points(
            this->donnees, TypeAttribut, nom.c_str(), static_cast<long>(nom.size()), &adaptrice);
        return ClasseAttribut::enveloppe(&adaptrice);
    }

    template <TypeAttributGeo3D TypeAttribut>
    auto accedeAttributPoint(const std::string &nom) const
        -> std::optional<typename SelectriceClasseAttribut<TypeAttribut>::Type>
    {
        using ClasseAttribut = typename SelectriceClasseAttribut<TypeAttribut>::Type;
        AdaptriceAttribut adaptrice;
        if (!this->accede_attribut_sur_points) {
            return ClasseAttribut::enveloppe(&adaptrice);
        }
        this->accede_attribut_sur_points(
            this->donnees, TypeAttribut, nom.c_str(), static_cast<long>(nom.size()), &adaptrice);
        if (!adaptrice) {
            return {};
        }
        return ClasseAttribut::enveloppe(&adaptrice);
    }

    /* Interface pour les polygones. */

    long nombreDePolygones() const;

    long nombreDeSommetsPolygone(long n) const;

    void pointPourSommetPolygones(long n, long v, math::vec3f &pos) const;

    math::vec3f normalPolygone(long i) const;

    void reserveNombreDePolygones(long nombre) const;

    void ajouteListePolygones(const int *sommets,
                              const int *sommets_par_polygones,
                              long nombre_polygones);

    void ajouteUnPolygone(const int *sommets, int taille) const;

    void indexPointsSommetsPolygone(long n, int *index) const;

    void rafinePolygone(long i, const RafineusePolygone &rafineuse) const;

    template <TypeAttributGeo3D TypeAttribut>
    typename SelectriceClasseAttribut<TypeAttribut>::Type ajouteAttributPolygone(
        const std::string &nom)
    {
        using ClasseAttribut = typename SelectriceClasseAttribut<TypeAttribut>::Type;
        AdaptriceAttribut adaptrice;
        if (!this->ajoute_attribut_sur_polygones) {
            return ClasseAttribut::enveloppe(&adaptrice);
        }
        this->ajoute_attribut_sur_polygones(
            this->donnees, TypeAttribut, nom.c_str(), static_cast<long>(nom.size()), &adaptrice);
        return ClasseAttribut::enveloppe(&adaptrice);
    }

    template <TypeAttributGeo3D TypeAttribut>
    auto accedeAttributPolygone(const std::string &nom) const
        -> std::optional<typename SelectriceClasseAttribut<TypeAttribut>::Type>
    {
        AdaptriceAttribut adaptrice;
        using ClasseAttribut = typename SelectriceClasseAttribut<TypeAttribut>::Type;
        if (!this->accede_attribut_sur_polygones) {
            return ClasseAttribut::enveloppe(&adaptrice);
        }
        this->accede_attribut_sur_polygones(
            this->donnees, TypeAttribut, nom.c_str(), static_cast<long>(nom.size()), &adaptrice);
        if (!adaptrice) {
            return {};
        }
        return ClasseAttribut::enveloppe(&adaptrice);
    }

    template <TypeAttributGeo3D TypeAttribut>
    typename SelectriceClasseAttribut<TypeAttribut>::Type ajouteAttributSommetsPolygone(
        const std::string &nom)
    {
        using ClasseAttribut = typename SelectriceClasseAttribut<TypeAttribut>::Type;
        AdaptriceAttribut adaptrice;
        if (!this->ajoute_attribut_sur_sommets_polygones) {
            return ClasseAttribut::enveloppe(&adaptrice);
        }
        this->ajoute_attribut_sur_sommets_polygones(
            this->donnees, TypeAttribut, nom.c_str(), static_cast<long>(nom.size()), &adaptrice);
        return ClasseAttribut::enveloppe(&adaptrice);
    }

    template <TypeAttributGeo3D TypeAttribut>
    auto accedeAttributSommetsPolygone(const std::string &nom) const
        -> std::optional<typename SelectriceClasseAttribut<TypeAttribut>::Type>
    {
        using ClasseAttribut = typename SelectriceClasseAttribut<TypeAttribut>::Type;
        AdaptriceAttribut adaptrice;
        if (!this->accede_attribut_sur_sommets_polygones) {
            return ClasseAttribut::enveloppe(&adaptrice);
        }
        this->accede_attribut_sur_sommets_polygones(
            this->donnees, TypeAttribut, nom.c_str(), static_cast<long>(nom.size()), &adaptrice);
        if (!adaptrice) {
            return {};
        }
        return ClasseAttribut::enveloppe(&adaptrice);
    }

    /* Interface pour les groupes. */

    void *creeUnGroupeDePoints(const std::string &nom) const;

    void *creeUnGroupeDePolygones(const std::string &nom) const;

    void ajouteAuGroupe(void *poignee_groupe, long index) const;

    void ajoutePlageAuGroupe(void *poignee_groupe, long index_debut, long index_fin) const;

    bool groupePolygonePossedePoint(const void *poignee_groupe, long index) const;

    template <TypeAttributGeo3D TypeAttribut>
    typename SelectriceClasseAttribut<TypeAttribut>::Type ajouteAttributMaillage(
        const std::string &nom)
    {
        using ClasseAttribut = typename SelectriceClasseAttribut<TypeAttribut>::Type;
        AdaptriceAttribut adaptrice;
        if (!this->ajoute_attribut_sur_maillage) {
            return ClasseAttribut::enveloppe(&adaptrice);
        }
        this->ajoute_attribut_sur_maillage(
            this->donnees, TypeAttribut, nom.c_str(), static_cast<long>(nom.size()), &adaptrice);
        return ClasseAttribut::enveloppe(&adaptrice);
    }

    template <TypeAttributGeo3D TypeAttribut>
    auto accedeAttributMaillage(const std::string &nom) const
        -> std::optional<typename SelectriceClasseAttribut<TypeAttribut>::Type>
    {
        AdaptriceAttribut adaptrice;
        using ClasseAttribut = typename SelectriceClasseAttribut<TypeAttribut>::Type;
        if (!this->accede_attribut_sur_maillage) {
            return ClasseAttribut::enveloppe(&adaptrice);
        }
        this->accede_attribut_sur_maillage(
            this->donnees, TypeAttribut, nom.c_str(), static_cast<long>(nom.size()), &adaptrice);
        if (!adaptrice) {
            return {};
        }
        return ClasseAttribut::enveloppe(&adaptrice);
    }

    /* Outils. */

    limites<math::vec3f> boiteEnglobante() const;
};

}  // namespace geo
