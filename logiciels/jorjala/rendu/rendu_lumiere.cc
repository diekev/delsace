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

#include "rendu_lumiere.h"

#include <numeric>

#include "biblinternes/outils/fichier.hh"

#include "coeur/objet.h"

RenduLumiere::RenduLumiere(Lumiere const *lumiere) : m_lumiere(lumiere)
{
}

void RenduLumiere::initialise()
{
    if (m_tampon != nullptr) {
        return;
    }

    auto source = crée_sources_glsl_depuis_fichier("nuanceurs/simple.vert",
                                                   "nuanceurs/simple.frag");
    if (!source.has_value()) {
        std::cerr << __func__ << " erreur : les sources sont invalides !\n";
        return;
    }

    m_tampon = TamponRendu::crée_unique(source.value());

    auto programme = m_tampon->programme();
    programme->active();
    programme->uniforme("couleur",
                        m_lumiere->spectre.r,
                        m_lumiere->spectre.v,
                        m_lumiere->spectre.b,
                        m_lumiere->spectre.a);
    programme->desactive();

    dls::tableau<dls::math::vec3f> sommets;

    if (m_lumiere->type == LUMIERE_POINT) {
        sommets.redimensionne(6);

        sommets[0] = dls::math::vec3f(-1.0f, 0.0f, 0.0f);
        sommets[1] = dls::math::vec3f(1.0f, 0.0f, 0.0f);
        sommets[2] = dls::math::vec3f(0.0f, -1.0f, 0.0f);
        sommets[3] = dls::math::vec3f(0.0f, 1.0f, 0.0f);
        sommets[4] = dls::math::vec3f(0.0f, 0.0f, -1.0f);
        sommets[5] = dls::math::vec3f(0.0f, 0.0f, 1.0f);
    }
    else {
        sommets.redimensionne(6);

        sommets[0] = dls::math::vec3f(0.0f, 0.1f, 0.0f);
        sommets[1] = dls::math::vec3f(0.0f, 0.1f, -1.0f);
        sommets[2] = dls::math::vec3f(0.1f, -0.1f, 0.0f);
        sommets[3] = dls::math::vec3f(0.1f, -0.1f, -1.0f);
        sommets[4] = dls::math::vec3f(-0.1f, -0.1f, 0.0f);
        sommets[5] = dls::math::vec3f(-0.1f, -0.1f, -1.0f);
    }

    dls::tableau<unsigned int> indices(sommets.taille());
    std::iota(indices.debut(), indices.fin(), 0);

    remplis_tampon_principal(m_tampon.get(), "sommets", sommets, indices);

    ParametresDessin parametres_dessin;
    parametres_dessin.type_dessin(GL_LINES);

    m_tampon->parametres_dessin(parametres_dessin);
}

void RenduLumiere::dessine(ContexteRendu const &contexte)
{
    m_tampon->dessine(contexte);
}
