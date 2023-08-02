/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

#include <optional>
#include <string>

#include "biblinternes/math/limites.hh"
#include "biblinternes/math/vecteur.hh"

#include "structures/tableau.hh"

using namespace dls;

#include "ipa.h"

inline std::string vers_std_string(const char *chemin, int64_t taille_chemin)
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
    type lis_##nom(int64_t index) const                                                           \
    {                                                                                             \
        type v;                                                                                   \
        this->lis_##nom##_pour_index(this->donnees_utilisateur, index, &v);                       \
        return v;                                                                                 \
    }                                                                                             \
    void ecris_##nom(int64_t index, type v) const                                                 \
    {                                                                                             \
        this->ecris_##nom##_a_l_index(this->donnees_utilisateur, index, v);                       \
    }

#define FLUX_VECTEUR(type, nom)                                                                   \
    type lis_##nom(int64_t index) const                                                           \
    {                                                                                             \
        type v;                                                                                   \
        this->lis_##nom##_pour_index(this->donnees_utilisateur, index, v);                        \
        return v;                                                                                 \
    }                                                                                             \
    void ecris_##nom(int64_t index, type v) const                                                 \
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
        void copie(kuri::tableau<type> const &valeurs)                                            \
        {                                                                                         \
            for (int64_t i = 0; i < valeurs.taille(); i++) {                                      \
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

    int64_t nombreDePoints() const;

    math::vec3f pointPourIndex(int64_t n) const;

    void ajoutePoints(float *points, int64_t nombre) const;

    void reserveNombreDePoints(int64_t nombre) const;

    void ajouteUnPoint(float x, float y, float z) const;
    void ajouteUnPoint(math::vec3f xyz) const;

    void remplacePointALIndex(int64_t n, math::vec3f const &point);

    template <TypeAttributGeo3D TypeAttribut>
    typename SelectriceClasseAttribut<TypeAttribut>::Type ajouteAttributPoint(
        const std::string &nom)
    {
        using ClasseAttribut = typename SelectriceClasseAttribut<TypeAttribut>::Type;
        AdaptriceAttribut adaptrice;
        if (!this->ajoute_attribut_sur_points) {
            return ClasseAttribut::enveloppe(&adaptrice);
        }
        this->ajoute_attribut_sur_points(this->donnees,
                                         TypeAttribut,
                                         nom.c_str(),
                                         static_cast<int64_t>(nom.size()),
                                         &adaptrice);
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
        this->accede_attribut_sur_points(this->donnees,
                                         TypeAttribut,
                                         nom.c_str(),
                                         static_cast<int64_t>(nom.size()),
                                         &adaptrice);
        if (!adaptrice) {
            return {};
        }
        return ClasseAttribut::enveloppe(&adaptrice);
    }

    /* Interface pour les polygones. */

    int64_t nombreDePolygones() const;

    int64_t nombreDeSommetsPolygone(int64_t n) const;

    void pointPourSommetPolygones(int64_t n, int64_t v, math::vec3f &pos) const;

    math::vec3f normalPolygone(int64_t i) const;

    void reserveNombreDePolygones(int64_t nombre) const;

    void ajouteListePolygones(const int *sommets,
                              const int *sommets_par_polygones,
                              int64_t nombre_polygones);

    void ajouteUnPolygone(const int *sommets, int taille) const;

    void indexPointsSommetsPolygone(int64_t n, int *index) const;

    void rafinePolygone(int64_t i, const RafineusePolygone &rafineuse) const;

    template <TypeAttributGeo3D TypeAttribut>
    typename SelectriceClasseAttribut<TypeAttribut>::Type ajouteAttributPolygone(
        const std::string &nom)
    {
        using ClasseAttribut = typename SelectriceClasseAttribut<TypeAttribut>::Type;
        AdaptriceAttribut adaptrice;
        if (!this->ajoute_attribut_sur_polygones) {
            return ClasseAttribut::enveloppe(&adaptrice);
        }
        this->ajoute_attribut_sur_polygones(this->donnees,
                                            TypeAttribut,
                                            nom.c_str(),
                                            static_cast<int64_t>(nom.size()),
                                            &adaptrice);
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
        this->accede_attribut_sur_polygones(this->donnees,
                                            TypeAttribut,
                                            nom.c_str(),
                                            static_cast<int64_t>(nom.size()),
                                            &adaptrice);
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
        this->ajoute_attribut_sur_sommets_polygones(this->donnees,
                                                    TypeAttribut,
                                                    nom.c_str(),
                                                    static_cast<int64_t>(nom.size()),
                                                    &adaptrice);
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
        this->accede_attribut_sur_sommets_polygones(this->donnees,
                                                    TypeAttribut,
                                                    nom.c_str(),
                                                    static_cast<int64_t>(nom.size()),
                                                    &adaptrice);
        if (!adaptrice) {
            return {};
        }
        return ClasseAttribut::enveloppe(&adaptrice);
    }

    /* Interface pour les groupes. */

    void *creeUnGroupeDePoints(const std::string &nom) const;

    void *creeUnGroupeDePolygones(const std::string &nom) const;

    void ajouteAuGroupe(void *poignee_groupe, int64_t index) const;

    void ajoutePlageAuGroupe(void *poignee_groupe, int64_t index_debut, int64_t index_fin) const;

    bool groupePolygonePossedePoint(const void *poignee_groupe, int64_t index) const;

    template <TypeAttributGeo3D TypeAttribut>
    typename SelectriceClasseAttribut<TypeAttribut>::Type ajouteAttributMaillage(
        const std::string &nom)
    {
        using ClasseAttribut = typename SelectriceClasseAttribut<TypeAttribut>::Type;
        AdaptriceAttribut adaptrice;
        if (!this->ajoute_attribut_sur_maillage) {
            return ClasseAttribut::enveloppe(&adaptrice);
        }
        this->ajoute_attribut_sur_maillage(this->donnees,
                                           TypeAttribut,
                                           nom.c_str(),
                                           static_cast<int64_t>(nom.size()),
                                           &adaptrice);
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
        this->accede_attribut_sur_maillage(this->donnees,
                                           TypeAttribut,
                                           nom.c_str(),
                                           static_cast<int64_t>(nom.size()),
                                           &adaptrice);
        if (!adaptrice) {
            return {};
        }
        return ClasseAttribut::enveloppe(&adaptrice);
    }

    /* Outils. */

    limites<math::vec3f> boiteEnglobante() const;
};

}  // namespace geo
