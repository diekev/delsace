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

#include "visionneur_scene.h"

#include <GL/glew.h>

#include "biblinternes/math/transformation.hh"
#include "biblinternes/opengl/rendu_grille.h"
#include "biblinternes/opengl/rendu_texte.h"
#include "biblinternes/structures/flux_chaine.hh"
#include "biblinternes/vision/camera.h"

#include "../conversion_types.hh"

#include "rendu_brosse.h"
#include "rendu_maillage.h"
#include "rendu_rayon.h"
#include "rendu_seaux.hh"

VisionneurScene::VisionneurScene(VueCanevas3D *parent, KNB::Kanba &kanba)
    : m_parent(parent), m_kanba(kanba), m_caméra(kanba.donne_caméra()), m_rendu_brosse(nullptr),
      m_rendu_grille(nullptr), m_rendu_texte(nullptr), m_rendu_maillage(nullptr), m_pos_x(0),
      m_pos_y(0)
{
}

VisionneurScene::~VisionneurScene()
{
    delete m_rendu_maillage;
    delete m_rendu_texte;
    delete m_rendu_grille;
    delete m_rendu_brosse;
}

void VisionneurScene::initialise()
{
    glClearColor(0.5, 0.5, 0.5, 1.0);

    glEnable(GL_DEPTH_TEST);

    m_rendu_grille = new RenduGrille(20, 20);
    m_rendu_texte = new RenduTexte();
    m_rendu_brosse = new RenduBrosse;
    m_rendu_brosse->initialise();

    m_caméra.ajourne();

    m_chrono_rendu.commence();
}

void VisionneurScene::peint_opengl()
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_caméra.ajourne();

    /* Met en place le contexte. */
    auto const MV = convertis_matrice(m_caméra.MV());
    auto const P = convertis_matrice(m_caméra.P());
    auto const MVP = P * MV;

    m_contexte.vue(convertis_vecteur(m_caméra.donne_direction()));
    m_contexte.modele_vue(MV);
    m_contexte.projection(P);
    m_contexte.MVP(MVP);
    m_contexte.normal(dls::math::inverse_transpose(dls::math::mat3_depuis_mat4(MV)));
    m_contexte.matrice_objet(math::matf_depuis_matd(m_stack.sommet()));
    m_contexte.pour_surlignage(false);

    /* Peint la scene. */
    m_rendu_grille->dessine(m_contexte);

    auto maillage = m_kanba.donne_maillage();

    if (maillage && !m_rendu_maillage) {
        m_rendu_maillage = new RenduMaillage(maillage);
        m_rendu_maillage->initialise();
    }
    else if (m_rendu_maillage && m_rendu_maillage->maillage() != maillage) {
        delete m_rendu_maillage;
        m_rendu_maillage = new RenduMaillage(maillage);
        m_rendu_maillage->initialise();
    }

    if (m_rendu_maillage) {
        m_stack.ajoute(m_rendu_maillage->matrice());
        m_contexte.matrice_objet(math::matf_depuis_matd(m_stack.sommet()));

        m_rendu_maillage->dessine(m_contexte);

        m_stack.enleve_sommet();
    }

    if (m_affiche_brosse) {
        auto brosse = m_kanba.donne_pinceau();
        auto const &diametre = static_cast<float>(brosse.donne_diamètre());

        m_rendu_brosse->dessine(m_contexte,
                                diametre / static_cast<float>(m_caméra.donne_largeur()),
                                diametre / static_cast<float>(m_caméra.donne_hauteur()),
                                m_pos_x,
                                m_pos_y);
    }

    auto const fps = static_cast<int>(1.0 / m_chrono_rendu.arrete());

    dls::flux_chaine ss;
    ss << fps << " IPS";

    glEnable(GL_BLEND);

    m_rendu_texte->reinitialise();
    m_rendu_texte->dessine(m_contexte, ss.chn());

    glDisable(GL_BLEND);

    /* Peint les seaux au dessus du reste. */
    if (m_kanba.donne_dessine_seaux()) {
        glDisable(GL_DEPTH_TEST);
        if (!m_rendu_seaux) {
            m_rendu_seaux = new RenduSeaux(m_kanba);
        }

        m_rendu_seaux->initialise();
        m_contexte.modele_vue(dls::math::mat4x4f(1.0f));
        m_contexte.projection(dls::math::mat4x4f(1.0f));
        m_contexte.MVP(dls::math::mat4x4f(1.0f));
        m_rendu_seaux->dessine(m_contexte);
        glEnable(GL_DEPTH_TEST);
    }

    m_chrono_rendu.commence();
}

void VisionneurScene::redimensionne(int largeur, int hauteur)
{
    m_rendu_texte->etablie_dimension_fenetre(largeur, hauteur);
    m_caméra.définis_dimension_fenêtre(largeur, hauteur);
}

void VisionneurScene::position_souris(int x, int y)
{
    m_pos_x = static_cast<float>(x) / static_cast<float>(m_caméra.donne_largeur()) * 2.0f - 1.0f;
    m_pos_y = static_cast<float>(m_caméra.donne_hauteur() - y) /
                  static_cast<float>(m_caméra.donne_hauteur()) * 2.0f -
              1.0f;
}

void VisionneurScene::affiche_brosse(bool oui_ou_non)
{
    m_affiche_brosse = oui_ou_non;
}
