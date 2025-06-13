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

#include "format.hh"

#include <iostream>

/* ************************************************************************** */

temps_seconde::temps_seconde(double v) : valeur(v)
{
}

std::ostream &operator<<(std::ostream &os, temps_seconde const &t)
{
    auto valeur = static_cast<int>(t.valeur * 1000);

    if (valeur < 1) {
        valeur = static_cast<int>(t.valeur * 1000000);
        if (valeur < 10) {
            os << ' ' << ' ' << ' ';
        }
        else if (valeur < 100) {
            os << ' ' << ' ';
        }
        else if (valeur < 1000) {
            os << ' ';
        }

        os << valeur << "ns";
    }
    else {
        if (valeur < 10) {
            os << ' ' << ' ' << ' ';
        }
        else if (valeur < 100) {
            os << ' ' << ' ';
        }
        else if (valeur < 1000) {
            os << ' ';
        }

        os << valeur << "ms";
    }

    return os;
}

/* ************************************************************************** */

pourcentage::pourcentage(double v) : valeur(v)
{
}

std::ostream &operator<<(std::ostream &os, pourcentage const &p)
{
    auto const valeur = static_cast<int>(p.valeur * 100) / 100.0;

    if (valeur < 10) {
        os << ' ' << ' ';
    }
    else if (valeur < 100) {
        os << ' ';
    }

    os << valeur << "%";

    return os;
}

/* ************************************************************************** */

taille_octet::taille_octet(size_t v) : valeur(v)
{
}

std::ostream &operator<<(std::ostream &os, taille_octet const &taille)
{
    if (taille.valeur > (1024 * 1024 * 1024)) {
        os << (taille.valeur / (1024 * 1024 * 1024)) << "Go";
    }
    else if (taille.valeur > (1024 * 1024)) {
        os << (taille.valeur / (1024 * 1024)) << "Mo";
    }
    else if (taille.valeur > 1024) {
        os << (taille.valeur / 1024) << "Ko";
    }
    else {
        os << (taille.valeur) << "o";
    }

    return os;
}
