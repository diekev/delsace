/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include <cstdint>
#include <iosfwd>
#include <variant>

#include "attente.hh"

struct NoeudExpression;
struct NoeudDéclarationType;

using Type = NoeudDéclarationType;

#define ENUMERE_TYPES_TRANSFORMATION                                                              \
    ENUMERE_TYPE_TRANSFORMATION_EX(INUTILE)                                                       \
    ENUMERE_TYPE_TRANSFORMATION_EX(IMPOSSIBLE)                                                    \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONSTRUIS_UNION)                                               \
    ENUMERE_TYPE_TRANSFORMATION_EX(EXTRAIT_UNION)                                                 \
    ENUMERE_TYPE_TRANSFORMATION_EX(EXTRAIT_UNION_SANS_VÉRIFICATION)                               \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONSTRUIS_EINI)                                                \
    ENUMERE_TYPE_TRANSFORMATION_EX(EXTRAIT_EINI)                                                  \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONSTRUIS_TRANCHE_OCTET)                                       \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONVERTIS_TABLEAU_FIXE_VERS_TRANCHE)                            \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONVERTIS_TABLEAU_DYNAMIQUE_VERS_TRANCHE)                       \
    ENUMERE_TYPE_TRANSFORMATION_EX(PRENDS_RÉFÉRENCE)                                               \
    ENUMERE_TYPE_TRANSFORMATION_EX(DÉRÉFERENCE)                                                   \
    ENUMERE_TYPE_TRANSFORMATION_EX(R16_VERS_R32)                                                  \
    ENUMERE_TYPE_TRANSFORMATION_EX(R16_VERS_R64)                                                  \
    ENUMERE_TYPE_TRANSFORMATION_EX(R32_VERS_R16)                                                  \
    ENUMERE_TYPE_TRANSFORMATION_EX(R64_VERS_R16)                                                  \
    ENUMERE_TYPE_TRANSFORMATION_EX(AUGMENTE_TAILLE_TYPE)                                          \
    ENUMERE_TYPE_TRANSFORMATION_EX(RÉDUIS_TAILLE_TYPE)                                            \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONVERTIS_VERS_BASE)                                            \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONVERTIS_VERS_DÉRIVÉ)                                          \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONVERTIS_ENTIER_CONSTANT)                                      \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONVERTIS_VERS_PTR_RIEN)                                        \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONVERTIS_VERS_TYPE_CIBLE)                                      \
    ENUMERE_TYPE_TRANSFORMATION_EX(ENTIER_VERS_RÉEL)                                              \
    ENUMERE_TYPE_TRANSFORMATION_EX(RÉEL_VERS_ENTIER)                                              \
    ENUMERE_TYPE_TRANSFORMATION_EX(ENTIER_VERS_POINTEUR)                                          \
    ENUMERE_TYPE_TRANSFORMATION_EX(POINTEUR_VERS_ENTIER)                                          \
    ENUMERE_TYPE_TRANSFORMATION_EX(PRENDS_RÉFÉRENCE_ET_CONVERTIS_VERS_BASE)

enum class TypeTransformation {
#define ENUMERE_TYPE_TRANSFORMATION_EX(type) type,
    ENUMERE_TYPES_TRANSFORMATION
#undef ENUMERE_TYPE_TRANSFORMATION_EX
};

const char *chaine_transformation(TypeTransformation type);
std::ostream &operator<<(std::ostream &os, TypeTransformation type);

struct TransformationType {
    TypeTransformation type{};
    /* Pour les transformations entre type base et type dérivé. */
    uint32_t décalage_type_base = uint32_t(-1);
    int64_t indice_rubrique = 0;
    Type const *type_cible = nullptr;

    TransformationType() = default;

    explicit TransformationType(TypeTransformation type_) : type(type_)
    {
    }

    TransformationType(TypeTransformation type_, Type const *type_cible_, int64_t indice_rubrique_)
        : type(type_), indice_rubrique(indice_rubrique_), type_cible(type_cible_)
    {
    }

    TransformationType(TypeTransformation type_, Type const *type_cible_)
        : type(type_), type_cible(type_cible_)
    {
    }

    static TransformationType vers_base(Type const *type_cible_, uint32_t décalage_type_base_)
    {
        auto résultat = TransformationType(TypeTransformation::CONVERTIS_VERS_BASE);
        résultat.type_cible = type_cible_;
        résultat.décalage_type_base = décalage_type_base_;
        return résultat;
    }

    static TransformationType prends_référence_vers_base(Type const *type_cible_,
                                                        uint32_t décalage_type_base_)
    {
        auto résultat = TransformationType(
            TypeTransformation::PRENDS_RÉFÉRENCE_ET_CONVERTIS_VERS_BASE);
        résultat.type_cible = type_cible_;
        résultat.décalage_type_base = décalage_type_base_;
        return résultat;
    }

    static TransformationType vers_dérivé(Type const *type_cible_, uint32_t décalage_type_base_)
    {
        auto résultat = TransformationType(TypeTransformation::CONVERTIS_VERS_DÉRIVÉ);
        résultat.type_cible = type_cible_;
        résultat.décalage_type_base = décalage_type_base_;
        return résultat;
    }

    bool operator==(TransformationType const &autre) const
    {
        if (this == &autre) {
            return true;
        }

        if (type != autre.type) {
            return false;
        }

        if (indice_rubrique != autre.indice_rubrique) {
            return false;
        }

        if (type_cible != autre.type_cible) {
            return false;
        }

        if (décalage_type_base != autre.décalage_type_base) {
            return false;
        }

        return false;
    }
};

std::ostream &operator<<(std::ostream &os, TransformationType type);

using RésultatTransformation = std::variant<TransformationType, Attente>;

RésultatTransformation cherche_transformation(Type const *type_de, Type const *type_vers);

RésultatTransformation cherche_transformation_pour_transtypage(Type const *type_de,
                                                               Type const *type_vers);

/* Représente une transformation et son poids associé. Le poids peut-être utilisé pour calculer le
 * poids d'appariement d'un opérateur ou d'une fonction. */
struct PoidsTransformation {
    TransformationType transformation;
    double poids;
};

using RésultatPoidsTransformation = std::variant<PoidsTransformation, Attente>;

// Vérifie la compatibilité de deux types pour un opérateur.
RésultatPoidsTransformation vérifie_compatibilité(Type const *type_vers,
                                                  Type const *type_de,
                                                  bool ignore_entier_constant);

// Vérifie la compatibilité de deux types pour passer une expressions à une expression d'appel.
RésultatPoidsTransformation vérifie_compatibilité(Type const *type_vers,
                                                  Type const *type_de,
                                                  NoeudExpression const *noeud,
                                                  bool ignore_entier_constant);
