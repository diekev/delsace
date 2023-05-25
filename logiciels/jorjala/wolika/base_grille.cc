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

#include "base_grille.hh"

#include "biblinternes/memoire/logeuse_memoire.hh"

#include "grille_dense.hh"

namespace wlk {

void deloge_grille(base_grille_2d *&tampon)
{
    if (tampon == nullptr) {
        return;
    }

    auto const &desc = tampon->desc();

    switch (desc.type_donnees) {
        case type_grille::N32:
        {
            auto grille = dynamic_cast<grille_dense_2d<unsigned int> *>(tampon);
            memoire::deloge("grille_dense", grille);
            break;
        }
        case type_grille::Z8:
        {
            auto grille = dynamic_cast<grille_dense_2d<char> *>(tampon);
            memoire::deloge("grille_dense", grille);
            break;
        }
        case type_grille::Z32:
        {
            auto grille = dynamic_cast<grille_dense_2d<int> *>(tampon);
            memoire::deloge("grille_dense", grille);
            break;
        }
        case type_grille::R32:
        {
            auto grille = dynamic_cast<grille_dense_2d<float> *>(tampon);
            memoire::deloge("grille_dense", grille);
            break;
        }
        case type_grille::R32_PTR:
        {
            auto grille = dynamic_cast<grille_dense_2d<float *> *>(tampon);
            memoire::deloge("grille_dense", grille);
            break;
        }
        case type_grille::R64:
        {
            auto grille = dynamic_cast<grille_dense_2d<double> *>(tampon);
            memoire::deloge("grille_dense", grille);
            break;
        }
        case type_grille::VEC2:
        {
            auto grille = dynamic_cast<grille_dense_2d<dls::math::vec2f> *>(tampon);
            memoire::deloge("grille_dense", grille);
            break;
        }
        case type_grille::VEC3:
        {
            auto grille = dynamic_cast<grille_dense_2d<dls::math::vec3f> *>(tampon);
            memoire::deloge("grille_dense", grille);
            break;
        }
        case type_grille::VEC3_R64:
        {
            auto grille = dynamic_cast<grille_dense_2d<dls::math::vec3d> *>(tampon);
            memoire::deloge("grille_dense", grille);
            break;
        }
        case type_grille::COULEUR:
        {
            auto grille = dynamic_cast<grille_dense_2d<dls::phys::couleur32> *>(tampon);
            memoire::deloge("grille_dense", grille);
            break;
        }
        case type_grille::COURBE_PAIRE_TEMPS:
        {
            auto grille = dynamic_cast<grille_dense_2d<wlk::type_courbe> *>(tampon);
            memoire::deloge("grille_dense", grille);
            break;
        }
    }

    tampon = nullptr;
}

desc_grille_2d desc_depuis_hauteur_largeur(int hauteur, int largeur)
{
    auto moitie_x = static_cast<float>(largeur) * 0.5f;
    auto moitie_y = static_cast<float>(hauteur) * 0.5f;

    auto desc = wlk::desc_grille_2d();
    desc.etendue.min = dls::math::vec2f(-moitie_x, -moitie_y);
    desc.etendue.max = dls::math::vec2f(moitie_x, moitie_y);
    desc.fenetre_donnees = desc.etendue;
    desc.taille_voxel = 1.0;
    desc.resolution.x = hauteur;
    desc.resolution.y = largeur;

    return desc;
}

} /* namespace wlk */
