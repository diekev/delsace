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

#include "visionneur_image.h"

#include "biblinternes/ego/outils.h"
#include "biblinternes/image/pixel.h"
#include "biblinternes/opengl/contexte_rendu.h"

#include "../editeur_canevas.h"

#include "coeur/kanba.h"
#include "coeur/maillage.h"

#include "tampons_rendu.hh"
#include "textures.hh"

/* ************************************************************************** */

VisionneurImage::VisionneurImage(VueCanevas2D *parent, KNB::Kanba *kanba)
    : m_parent(parent), m_kanba(kanba)
{
}

void VisionneurImage::initialise()
{
    m_tampon = crée_tampon_image();
    charge_image();
}

void VisionneurImage::peint_opengl()
{
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_BLEND);
    if (m_tampon) {
        m_tampon->dessine({});
    }
    glDisable(GL_BLEND);
    dls::ego::util::GPU_check_errors("Erreur lors du dessin de la texture");
}

void VisionneurImage::redimensionne(int largeur, int hauteur)
{
    glViewport(0, 0, largeur, hauteur);
}

void VisionneurImage::charge_image()
{
    if (!m_tampon) {
        return;
    }

    auto maillage = m_kanba->maillage;
    if (maillage == nullptr) {
        return;
    }

    auto canal_fusionné = maillage->donne_canal_fusionné();

    GLint size[] = {GLint(canal_fusionné.largeur), GLint(canal_fusionné.hauteur)};

    if (m_largeur != size[0] || m_hauteur != size[1]) {
        m_hauteur = size[0];
        m_largeur = size[1];
    }

    génère_texture(
        m_tampon->texture(), reinterpret_cast<float *>(canal_fusionné.tampon_diffusion), size);

    dls::ego::util::GPU_check_errors("Unable to create image texture");
}
