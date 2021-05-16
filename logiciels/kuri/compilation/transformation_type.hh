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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/structures/vue_chaine_compacte.hh"

#include <variant>

struct EspaceDeTravail;
struct NoeudDeclarationEnteteFonction;
struct NoeudExpression;
struct Type;

struct ContexteValidationCode;

#define ENUMERE_TYPES_TRANSFORMATION                                                              \
    ENUMERE_TYPE_TRANSFORMATION_EX(INUTILE)                                                       \
    ENUMERE_TYPE_TRANSFORMATION_EX(IMPOSSIBLE)                                                    \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONSTRUIT_UNION)                                               \
    ENUMERE_TYPE_TRANSFORMATION_EX(EXTRAIT_UNION)                                                 \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONSTRUIT_EINI)                                                \
    ENUMERE_TYPE_TRANSFORMATION_EX(EXTRAIT_EINI)                                                  \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONSTRUIT_TABL_OCTET)                                          \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONVERTI_TABLEAU)                                              \
    ENUMERE_TYPE_TRANSFORMATION_EX(FONCTION)                                                      \
    ENUMERE_TYPE_TRANSFORMATION_EX(PREND_REFERENCE)                                               \
    ENUMERE_TYPE_TRANSFORMATION_EX(DEREFERENCE)                                                   \
    ENUMERE_TYPE_TRANSFORMATION_EX(AUGMENTE_TAILLE_TYPE)                                          \
    ENUMERE_TYPE_TRANSFORMATION_EX(REDUIT_TAILLE_TYPE)                                            \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONVERTI_VERS_BASE)                                            \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONVERTI_ENTIER_CONSTANT)                                      \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONVERTI_VERS_PTR_RIEN)                                        \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONVERTI_VERS_TYPE_CIBLE)                                      \
    ENUMERE_TYPE_TRANSFORMATION_EX(ENTIER_VERS_REEL)                                              \
    ENUMERE_TYPE_TRANSFORMATION_EX(REEL_VERS_ENTIER)                                              \
    ENUMERE_TYPE_TRANSFORMATION_EX(ENTIER_VERS_POINTEUR)                                          \
    ENUMERE_TYPE_TRANSFORMATION_EX(POINTEUR_VERS_ENTIER)                                          \
    ENUMERE_TYPE_TRANSFORMATION_EX(CONVERTI_REFERENCE_VERS_TYPE_CIBLE)

enum class TypeTransformation {
#define ENUMERE_TYPE_TRANSFORMATION_EX(type) type,
    ENUMERE_TYPES_TRANSFORMATION
#undef ENUMERE_TYPE_TRANSFORMATION_EX
};

const char *chaine_transformation(TypeTransformation type);
std::ostream &operator<<(std::ostream &os, TypeTransformation type);

struct TransformationType {
    TypeTransformation type{};
    NoeudDeclarationEnteteFonction const *fonction{};
    Type *type_cible = nullptr;
    long index_membre = 0;

    TransformationType() = default;

    TransformationType(TypeTransformation type_) : type(type_)
    {
    }

    TransformationType(TypeTransformation type_, Type *type_cible_, long index_membre_)
        : type(type_), type_cible(type_cible_), index_membre(index_membre_)
    {
    }

    TransformationType(TypeTransformation type_, Type *type_cible_)
        : type(type_), type_cible(type_cible_)
    {
    }

    TransformationType(NoeudDeclarationEnteteFonction const *fonction_)
        : type(TypeTransformation::FONCTION), fonction(fonction_)
    {
    }

    TransformationType(NoeudDeclarationEnteteFonction const *fonction_, Type *type_cible_)
        : type(TypeTransformation::FONCTION), fonction(fonction_), type_cible(type_cible_)
    {
    }

    bool operator==(TransformationType const &autre) const
    {
        if (this == &autre) {
            return true;
        }

        if (type != autre.type) {
            return false;
        }

        if (fonction != autre.fonction) {
            return false;
        }

        if (index_membre != autre.index_membre) {
            return false;
        }

        if (type_cible != autre.type_cible) {
            return false;
        }

        return false;
    }
};

struct Attente {
    Type *attend_sur_type = nullptr;
    const char *attend_sur_interface_kuri = nullptr;

    static Attente sur_type(Type *type)
    {
        auto attente = Attente{};
        attente.attend_sur_type = type;
        return attente;
    }

    static Attente sur_interface_kuri(const char *nom_fonction)
    {
        auto attente = Attente{};
        attente.attend_sur_interface_kuri = nom_fonction;
        return attente;
    }
};

using ResultatTransformation = std::variant<TransformationType, Attente>;

ResultatTransformation cherche_transformation(EspaceDeTravail &espace,
                                              ContexteValidationCode &contexte,
                                              Type *type_de,
                                              Type *type_vers);

ResultatTransformation cherche_transformation_pour_transtypage(EspaceDeTravail &espace,
                                                               ContexteValidationCode &contexte,
                                                               Type *type_de,
                                                               Type *type_vers);

/* Représente une transformation et son poids associé. Le poids peut-être utilisé pour calculer le
 * poids d'appariement d'un opérateur ou d'une fonction. */
struct PoidsTransformation {
    TransformationType transformation;
    double poids;
};

using ResultatPoidsTransformation = std::variant<PoidsTransformation, Attente>;

// Vérifie la compatibilité de deux types pour un opérateur.
ResultatPoidsTransformation verifie_compatibilite(EspaceDeTravail &espace,
                                                  ContexteValidationCode &contexte,
                                                  Type *type_arg,
                                                  Type *type_enf);

// Vérifie la compatibilité de deux types pour passer une expressions à une expression d'appel.
ResultatPoidsTransformation verifie_compatibilite(EspaceDeTravail &espace,
                                                  ContexteValidationCode &contexte,
                                                  Type *type_arg,
                                                  Type *type_enf,
                                                  NoeudExpression *enfant);
