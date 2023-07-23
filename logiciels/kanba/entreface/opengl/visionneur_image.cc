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

#include "tampons_rendu.hh"
#include "textures.hh"

/* ------------------------------------------------------------------------- */
/** \name Tampon pour le rendu des arêtes.
 * \{ */

static std::unique_ptr<TamponRendu> crée_tampon_arêtes(KNB::Maillage maillage)
{
    auto const canaux = maillage.donne_canaux_texture();
    auto const largeur = canaux.donne_largeur();
    auto const hauteur = canaux.donne_hauteur();

    std::cerr << "Taille tampon canal : " << largeur << "x" << hauteur << '\n';

    dls::tableau<dls::math::vec3f> sommets;
    sommets.reserve(maillage.donne_nombre_polygones() * 4);

    dls::tableau<int> index;
    index.reserve(maillage.donne_nombre_polygones() * 4 * 2);

    for (int i = 0; i < maillage.donne_nombre_polygones(); i++) {
        auto quad = maillage.donne_quadrilatère(i);

        auto x = float(quad.donne_x()) / float(largeur);
        auto y = float(quad.donne_y()) / float(hauteur);

        auto l = float(quad.donne_res_u()) / float(largeur);
        auto h = float(quad.donne_res_v()) / float(hauteur);

        auto px0 = x;
        auto px1 = x + l;
        auto py0 = y;
        auto py1 = y + h;

        px0 = px0 * 2.0f - 1.0f;
        px1 = px1 * 2.0f - 1.0f;
        /* Le 0 des seaux est en haut de l'écran, celuis d'OpenGL en bas. */
        py0 = (1.0f - py0) * 2.0f - 1.0f;
        py1 = (1.0f - py1) * 2.0f - 1.0f;

        auto p0 = dls::math::vec3f(px0, py0, 0.0f);
        auto p1 = dls::math::vec3f(px1, py0, 0.0f);
        auto p2 = dls::math::vec3f(px1, py1, 0.0f);
        auto p3 = dls::math::vec3f(px0, py1, 0.0f);

        auto decalage_sommets = int(sommets.taille());

        sommets.ajoute(p0);
        sommets.ajoute(p1);
        sommets.ajoute(p2);
        sommets.ajoute(p3);

        index.ajoute(decalage_sommets + 0);
        index.ajoute(decalage_sommets + 1);

        index.ajoute(decalage_sommets + 1);
        index.ajoute(decalage_sommets + 2);

        index.ajoute(decalage_sommets + 2);
        index.ajoute(decalage_sommets + 3);

        index.ajoute(decalage_sommets + 3);
        index.ajoute(decalage_sommets + 0);
    }

    auto tampon = crée_tampon_nuanceur_simple(dls::phys::couleur32(0.0f, 1.0f, 0.0f, 1.0f));
    remplis_tampon_principal(tampon.get(), "sommets", sommets, index);

    ParametresDessin parametres_dessin;
    parametres_dessin.type_dessin(GL_LINES);
    parametres_dessin.taille_ligne(1.0f);
    tampon->parametres_dessin(parametres_dessin);

    return tampon;
}

/** \} */

/* ************************************************************************** */

VisionneurImage::VisionneurImage(VueCanevas2D *parent, KNB::Kanba &kanba)
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
    if (m_tampon_arêtes) {
        ContexteRendu m_contexte;
        m_contexte.modele_vue(dls::math::mat4x4f(1.0f));
        m_contexte.projection(dls::math::mat4x4f(1.0f));
        m_contexte.MVP(dls::math::mat4x4f(1.0f));
        m_contexte.matrice_objet(dls::math::mat4x4f(1.0f));
        m_tampon_arêtes->dessine(m_contexte);
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

    auto maillage = m_kanba.donne_maillage();
    if (maillage == nullptr) {
        return;
    }

    if (!m_tampon_arêtes) {
        m_tampon_arêtes = crée_tampon_arêtes(maillage);
    }

    auto canal_fusionné = maillage.donne_canal_fusionné();

    GLint size[] = {GLint(canal_fusionné.donne_largeur()), GLint(canal_fusionné.donne_hauteur())};

    if (m_largeur != size[0] || m_hauteur != size[1]) {
        m_hauteur = size[0];
        m_largeur = size[1];
    }

    génère_texture(m_tampon->texture(),
                   reinterpret_cast<float *>(canal_fusionné.donne_tampon_diffusion()),
                   size);

    dls::ego::util::GPU_check_errors("Unable to create image texture");
}
