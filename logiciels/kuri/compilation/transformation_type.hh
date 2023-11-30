/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include <cstdint>
#include <iosfwd>
#include <variant>

#include "attente.hh"

struct NoeudExpression;
struct Type;

#define ENUMERE_TYPES_TRANSFORMATION                                                              \
    ENUMERE_TYPE_TRANSFORMATION_EX(INUTILE)                                                       \
    ENUMERE_TYPE_TRANSFORMATION_EX(IMPOSSIBLE)                                                    \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONSTRUIT_UNION)                                               \
    ENUMERE_TYPE_TRANSFORMATION_EX(EXTRAIT_UNION)                                                 \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONSTRUIT_EINI)                                                \
    ENUMERE_TYPE_TRANSFORMATION_EX(EXTRAIT_EINI)                                                  \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONSTRUIT_TABL_OCTET)                                          \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONVERTI_TABLEAU)                                              \
    ENUMERE_TYPE_TRANSFORMATION_EX(PREND_REFERENCE)                                               \
    ENUMERE_TYPE_TRANSFORMATION_EX(DEREFERENCE)                                                   \
    ENUMERE_TYPE_TRANSFORMATION_EX(R16_VERS_R32)                                                  \
    ENUMERE_TYPE_TRANSFORMATION_EX(R16_VERS_R64)                                                  \
    ENUMERE_TYPE_TRANSFORMATION_EX(R32_VERS_R16)                                                  \
    ENUMERE_TYPE_TRANSFORMATION_EX(R64_VERS_R16)                                                  \
    ENUMERE_TYPE_TRANSFORMATION_EX(AUGMENTE_TAILLE_TYPE)                                          \
    ENUMERE_TYPE_TRANSFORMATION_EX(REDUIT_TAILLE_TYPE)                                            \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONVERTI_VERS_BASE)                                            \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONVERTI_VERS_DÉRIVÉ)                                          \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONVERTI_ENTIER_CONSTANT)                                      \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONVERTI_VERS_PTR_RIEN)                                        \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONVERTI_VERS_TYPE_CIBLE)                                      \
    ENUMERE_TYPE_TRANSFORMATION_EX(ENTIER_VERS_REEL)                                              \
    ENUMERE_TYPE_TRANSFORMATION_EX(REEL_VERS_ENTIER)                                              \
    ENUMERE_TYPE_TRANSFORMATION_EX(ENTIER_VERS_POINTEUR)                                          \
    ENUMERE_TYPE_TRANSFORMATION_EX(POINTEUR_VERS_ENTIER)                                          \
    ENUMERE_TYPE_TRANSFORMATION_EX(PREND_REFERENCE_ET_CONVERTIS_VERS_BASE)

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
    uint32_t décalage_type_base = -1u;
    int64_t index_membre = 0;
    Type const *type_cible = nullptr;

    TransformationType() = default;

    TransformationType(TypeTransformation type_) : type(type_)
    {
    }

    TransformationType(TypeTransformation type_, Type const *type_cible_, int64_t index_membre_)
        : type(type_), index_membre(index_membre_), type_cible(type_cible_)
    {
    }

    TransformationType(TypeTransformation type_, Type const *type_cible_)
        : type(type_), type_cible(type_cible_)
    {
    }

    static TransformationType vers_base(Type const *type_cible_, uint32_t décalage_type_base_)
    {
        auto résultat = TransformationType(TypeTransformation::CONVERTI_VERS_BASE);
        résultat.type_cible = type_cible_;
        résultat.décalage_type_base = décalage_type_base_;
        return résultat;
    }

    static TransformationType prend_référence_vers_base(Type const *type_cible_,
                                                        uint32_t décalage_type_base_)
    {
        auto résultat = TransformationType(
            TypeTransformation::PREND_REFERENCE_ET_CONVERTIS_VERS_BASE);
        résultat.type_cible = type_cible_;
        résultat.décalage_type_base = décalage_type_base_;
        return résultat;
    }

    static TransformationType vers_dérivé(Type const *type_cible_, uint32_t décalage_type_base_)
    {
        auto résultat = TransformationType(TypeTransformation::CONVERTI_VERS_DÉRIVÉ);
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

        if (index_membre != autre.index_membre) {
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

using ResultatTransformation = std::variant<TransformationType, Attente>;

ResultatTransformation cherche_transformation(Type const *type_de, Type const *type_vers);

ResultatTransformation cherche_transformation_pour_transtypage(Type const *type_de,
                                                               Type const *type_vers);

/* Représente une transformation et son poids associé. Le poids peut-être utilisé pour calculer le
 * poids d'appariement d'un opérateur ou d'une fonction. */
struct PoidsTransformation {
    TransformationType transformation;
    double poids;
};

using ResultatPoidsTransformation = std::variant<PoidsTransformation, Attente>;

// Vérifie la compatibilité de deux types pour un opérateur.
ResultatPoidsTransformation vérifie_compatibilité(Type const *type_vers, Type const *type_de);

// Vérifie la compatibilité de deux types pour passer une expressions à une expression d'appel.
ResultatPoidsTransformation vérifie_compatibilité(Type const *type_vers,
                                                  Type const *type_de,
                                                  NoeudExpression const *noeud);
