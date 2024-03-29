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

#include "arbre_hbe.hh"
#include "biblinternes/math/limites.hh"

struct Corps;

struct DeleguePrim {
    Corps const &m_corps;

    DeleguePrim(Corps const &corps);

    long nombre_elements() const;

    void coords_element(int idx, dls::tableau<dls::math::vec3f> &cos) const;

    BoiteEnglobante boite_englobante(long idx) const;

    dls::phys::esectd intersecte_element(long idx, dls::phys::rayond const &r) const;

    DonneesPointPlusProche calcule_point_plus_proche(long idx, dls::math::point3d const &p) const;
};

struct delegue_arbre_octernaire {
    Corps const &corps;

    explicit delegue_arbre_octernaire(Corps const &c);

    long nombre_elements() const;

    limites3f limites_globales() const;

    limites3f calcule_limites(long idx) const;
};
