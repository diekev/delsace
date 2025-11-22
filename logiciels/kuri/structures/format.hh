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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <iosfwd>
#include <type_traits>

#include "chaine.hh"
#include "enchaineuse.hh"

/**
 * Petite bibliothèque de typage pour mieux formatter les messages.
 */

/* ************************************************************************** */

struct temps_seconde {
    double valeur;

    explicit temps_seconde(double v);
};

std::ostream &operator<<(std::ostream &os, temps_seconde const &t);

/* ************************************************************************** */

struct pourcentage {
    double valeur;

    explicit pourcentage(double v);
};

std::ostream &operator<<(std::ostream &os, pourcentage const &p);

/* ************************************************************************** */

struct taille_octet {
    size_t valeur;

    explicit taille_octet(size_t v);
};

std::ostream &operator<<(std::ostream &os, taille_octet const &taille);

/* ************************************************************************** */

template <typename T>
auto formatte_nombre_entier(T nombre)
{
    auto résultat_tmp = enchaine(nombre);

    const auto taille = résultat_tmp.taille();

    if (taille <= 3) {
        return résultat_tmp;
    }

    const auto reste = taille % 3;
    auto résultat = Enchaineuse();

    for (auto i = 0l; i < reste; ++i) {
        résultat.ajoute_caractère(résultat_tmp[i]);
    }

    for (auto i = reste; i < taille; i += 3) {
        if (reste != 0 || i != reste) {
            résultat.ajoute_caractère(' ');
        }

        résultat.ajoute_caractère(résultat_tmp[i + 0]);
        résultat.ajoute_caractère(résultat_tmp[i + 1]);
        résultat.ajoute_caractère(résultat_tmp[i + 2]);
    }

    return résultat.chaine();
}

template <typename T>
struct integral_repr;

#ifdef HALF_SUPPORT
template <>
struct integral_repr<half> {
    using type = short;
};
#endif

template <>
struct integral_repr<float> {
    using type = int;
};

template <>
struct integral_repr<double> {
    using type = long;
};

template <typename T>
auto formatte_nombre_décimal(T nombre)
{
    using type_entier = typename integral_repr<T>::type;
    auto part_entière = formatte_nombre_entier(static_cast<type_entier>(nombre));

    Enchaineuse enchaineuse;
    enchaineuse.format_réel_court = true;
    enchaineuse << nombre;
    auto part_décimale = enchaineuse.chaine();

    auto pos_point = part_décimale.trouve('.');

    if (pos_point != -1) {
        part_entière = enchaine(part_entière, part_décimale.sous_chaine(pos_point));
    }

    return part_entière;
}

template <typename T>
auto formatte_nombre(T nombre)
{
    if constexpr (std::is_integral_v<T>) {
        return formatte_nombre_entier(nombre);
    }
    else if constexpr (std::is_floating_point_v<T>) {
        return formatte_nombre_décimal(nombre);
    }

    return kuri::chaine("pas un nombre");
}
