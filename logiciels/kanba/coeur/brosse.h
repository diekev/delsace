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

#include "melange.h"

#include "biblinternes/math/vecteur.hh"

#include "outils.hh"

namespace KNB {

struct Brosse {
  private:
    dls::math::vec4f m_couleur = dls::math::vec4f(1.0f, 0.0f, 1.0f, 1.0f);
    float m_opacité = 1.0f;
    int m_rayon = 35;

    TypeMelange m_mode_de_fusion{};

  public:
    DEFINIS_ACCESSEUR_MUTATEUR_MEMBRE(dls::math::vec4f, couleur);
    DEFINIS_ACCESSEUR_MUTATEUR_MEMBRE(float, opacité);
    DEFINIS_ACCESSEUR_MUTATEUR_MEMBRE(int, rayon);
    DEFINIS_ACCESSEUR_MUTATEUR_MEMBRE(TypeMelange, mode_de_fusion);

    int donne_diamètre() const
    {
        return m_rayon * 2;
    }
};

}  // namespace KNB
