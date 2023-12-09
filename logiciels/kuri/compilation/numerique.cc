/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "numerique.hh"

#include "arbre_syntaxique/noeud_expression.hh"

bool est_hors_des_limites(int64_t valeur, Type *type)
{
    if (type->est_type_entier_naturel()) {
        if (type->taille_octet == 1) {
            return valeur >= std::numeric_limits<unsigned char>::max();
        }

        if (type->taille_octet == 2) {
            return valeur > std::numeric_limits<unsigned short>::max();
        }

        if (type->taille_octet == 4) {
            return valeur > std::numeric_limits<uint32_t>::max();
        }

        // À FAIRE : trouve une bonne manière de détecter ceci
        return false;
    }

    if (type->taille_octet == 1) {
        return valeur < std::numeric_limits<char>::min() ||
               valeur > std::numeric_limits<char>::max();
    }

    if (type->taille_octet == 2) {
        return valeur < std::numeric_limits<short>::min() ||
               valeur > std::numeric_limits<short>::max();
    }

    if (type->taille_octet == 4) {
        return valeur < std::numeric_limits<int>::min() ||
               valeur > std::numeric_limits<int>::max();
    }

    // À FAIRE : trouve une bonne manière de détecter ceci
    return false;
}

int64_t valeur_min(Type *type)
{
    if (type->est_type_entier_naturel()) {
        if (type->taille_octet == 1) {
            return std::numeric_limits<unsigned char>::min();
        }

        if (type->taille_octet == 2) {
            return std::numeric_limits<unsigned short>::min();
        }

        if (type->taille_octet == 4) {
            return std::numeric_limits<uint32_t>::min();
        }

        return std::numeric_limits<uint64_t>::min();
    }

    if (type->taille_octet == 1) {
        return std::numeric_limits<char>::min();
    }

    if (type->taille_octet == 2) {
        return std::numeric_limits<short>::min();
    }

    if (type->taille_octet == 4) {
        return std::numeric_limits<int>::min();
    }

    return std::numeric_limits<int64_t>::min();
}

uint64_t valeur_max(Type *type)
{
    if (type->est_type_entier_naturel()) {
        if (type->taille_octet == 1) {
            return std::numeric_limits<unsigned char>::max();
        }

        if (type->taille_octet == 2) {
            return std::numeric_limits<unsigned short>::max();
        }

        if (type->taille_octet == 4) {
            return std::numeric_limits<uint32_t>::max();
        }

        return std::numeric_limits<uint64_t>::max();
    }

    if (type->taille_octet == 1) {
        return std::numeric_limits<char>::max();
    }

    if (type->taille_octet == 2) {
        return std::numeric_limits<short>::max();
    }

    if (type->taille_octet == 4) {
        return std::numeric_limits<int>::max();
    }

    return std::numeric_limits<int64_t>::max();
}

int nombre_de_bits_pour_type(Type *type)
{
    while (type->est_type_opaque()) {
        type = type->comme_type_opaque()->type_opacifie;
    }

    /* Utilisation de unsigned car signed enlève 1 bit pour le signe. */

    if (type->taille_octet == 1) {
        return std::numeric_limits<unsigned char>::digits;
    }

    if (type->taille_octet == 2) {
        return std::numeric_limits<unsigned short>::digits;
    }

    if (type->taille_octet == 4 || type->est_type_entier_constant()) {
        return std::numeric_limits<uint32_t>::digits;
    }

    return std::numeric_limits<uint64_t>::digits;
}
