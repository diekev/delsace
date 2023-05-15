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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include <GL/glew.h>

#include "editrice_vue2d.h"

#include "biblinternes/ego/outils.h"
#include <cassert>
#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QApplication>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWheelEvent>
#pragma GCC diagnostic pop

#include "biblinternes/image/operations/operations.h"
#include "biblinternes/image/pixel.h"
#include "biblinternes/opengl/rendu_texte.h"
#include "biblinternes/outils/constantes.h"
#include "biblinternes/patrons_conception/commande.h"
#include "biblinternes/patrons_conception/repondant_commande.h"
#include "biblinternes/structures/flux_chaine.hh"

#include "coeur/jorjala.hh"

// #include "lcc/lcc.hh"

#include "opengl/rendu_image.h"
#include "opengl/rendu_manipulatrice_2d.h"

static dls::math::mat4x4f convertis_matrice(JJL::Mat4r matrice)
{
    return *reinterpret_cast<dls::math::mat4x4f *>(&matrice);
}

/* ************************************************************************** */

Visionneuse2D::Visionneuse2D(JJL::Jorjala &jorjala, EditriceVue2D *base, QWidget *parent)
    : QGLWidget(parent), m_jorjala(jorjala), m_base(base)
{
}

Visionneuse2D::~Visionneuse2D()
{
    memoire::deloge("RenduTexte", m_rendu_texte);
    memoire::deloge("RenduImage", m_rendu_image);
    memoire::deloge("RenduManipulatrice", m_rendu_manipulatrice);
}

void Visionneuse2D::initializeGL()
{
    glewExperimental = GL_TRUE;
    GLenum erreur = glewInit();

    if (erreur != GLEW_OK) {
        std::cerr << "Error: " << glewGetErrorString(erreur) << "\n";
        return;
    }

    m_rendu_image = memoire::loge<RenduImage>("RenduImage");
    m_rendu_texte = memoire::loge<RenduTexte>("RenduTexte");
    m_rendu_manipulatrice = memoire::loge<RenduManipulatrice2D>("RenduManipulatrice");
    m_chrono_rendu.commence();

    m_matrice_image = dls::math::mat4x4f(1.0);
    m_matrice_image[0][0] = 1.0;
    m_matrice_image[1][1] = static_cast<float>(720) / 1280;
}

void Visionneuse2D::paintGL()
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_BLEND);

    auto camera_2d = m_jorjala.caméra_2d();
    m_contexte.MVP(convertis_matrice(camera_2d.matrice()));
    m_contexte.matrice_objet(m_matrice_image);

    m_rendu_image->dessine(m_contexte);
    m_rendu_image->dessine_bordure(m_contexte);

    auto matrice_passe_partout = dls::math::mat4x4f(1.0f);
    matrice_passe_partout[0][0] = 1.0f;
    matrice_passe_partout[1][1] = 1080.0f / 1920.0f;

    m_contexte.matrice_objet(matrice_passe_partout);
    m_rendu_image->dessine_bordure(m_contexte);

    m_contexte.matrice_objet(dls::math::mat4x4f(1.0));

    /* À FAIRE */
    m_rendu_manipulatrice->dessine(m_contexte);

    auto const fps = static_cast<int>(1.0 / m_chrono_rendu.arrete());

    m_rendu_texte->reinitialise();

    dls::flux_chaine ss;
    ss << fps << " IPS";

    auto couleur_fps = dls::math::vec4f(1.0f);

    if (fps < 12) {
        couleur_fps = dls::math::vec4f(0.8f, 0.1f, 0.1f, 1.0f);
    }

    m_rendu_texte->dessine(m_contexte, ss.chn(), couleur_fps);

    // À FAIRE : stats programme
    ss.chn("");
    ss << "Mémoire allouée   : " << memoire::formate_taille(memoire::allouee());
    m_rendu_texte->dessine(m_contexte, ss.chn());

    ss.chn("");
    ss << "Mémoire consommée : " << memoire::formate_taille(memoire::consommee());
    m_rendu_texte->dessine(m_contexte, ss.chn());

    //	ss.chn("");
    //	ss << "Nombre commandes  : " << m_jorjala.usine_commandes().taille();
    //	m_rendu_texte->dessine(m_contexte, ss.chn());

    //	ss.chn("");
    //	ss << "Nombre noeuds     : " << m_jorjala.usine_operatrices().num_entries();
    //	m_rendu_texte->dessine(m_contexte, ss.chn());

    //	ss.chn("");
    //	ss << "Nombre fonctions  : " << m_jorjala.lcc->fonctions.table.taille();
    //	m_rendu_texte->dessine(m_contexte, ss.chn());

    glDisable(GL_BLEND);

    m_chrono_rendu.commence();
}

void Visionneuse2D::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);

    auto camera_2d = m_jorjala.caméra_2d();
    camera_2d.hauteur(h);
    camera_2d.largeur(w);
    camera_2d.ajourne_matrice();

    m_rendu_texte->etablie_dimension_fenetre(w, h);

    update();
}

void Visionneuse2D::charge_image(grille_couleur const &image)
{
    if ((image.desc().resolution.x == 0) || (image.desc().resolution.y == 0)) {
        m_matrice_image = dls::math::mat4x4f(1.0);
        m_matrice_image[0][0] = 1.0;
        m_matrice_image[1][1] = static_cast<float>(720) / 1280;
        return;
    }

    GLint size[2] = {image.desc().resolution.x, image.desc().resolution.y};

    m_matrice_image = dls::math::mat4x4f(1.0f);
    m_matrice_image[0][0] = static_cast<float>(size[0]) / 1920.0f;
    m_matrice_image[1][1] = static_cast<float>(size[1]) / 1920.0f;

    /* calcul de la translation puisque l'image n'est pas forcément centrée
     * À FAIRE : pour les images EXR il faut préserver la fenêtre d'affichage */
    auto moitie_x = -static_cast<float>(size[0]) * 0.5f;
    auto moitie_y = -static_cast<float>(size[1]) * 0.5f;

    auto min_x = image.desc().etendue.min.x;
    auto min_y = image.desc().etendue.min.y;

    auto trans_x = (moitie_x - min_x) / 1920.0f;
    auto trans_y = (moitie_y - min_y) / 1920.0f;

    m_matrice_image[3][0] = trans_x;
    m_matrice_image[3][1] = trans_y;

    /* À FAIRE : il y a des crashs lors du démarrage, il faudrait réviser la
     * manière d'initialiser les éditeurs quand ils sont ajoutés */
    if (m_rendu_image != nullptr) {
        m_rendu_image->charge_image(image);
    }
}

void Visionneuse2D::charge_composite(JJL::Composite composite)
{
    auto fenetre = composite.fenêtre();

    GLint size[2] = {
        fenetre.x_max() - fenetre.x_min() + 1,
        fenetre.y_max() - fenetre.y_min() + 1,
    };

    if ((size[0] == 0) || (size[1] == 0)) {
        m_matrice_image = dls::math::mat4x4f(1.0);
        m_matrice_image[0][0] = 1.0;
        m_matrice_image[1][1] = static_cast<float>(720) / 1280;
        return;
    }

    m_matrice_image = dls::math::mat4x4f(1.0f);
    m_matrice_image[0][0] = static_cast<float>(size[0]) / 1920.0f;
    m_matrice_image[1][1] = static_cast<float>(size[1]) / 1920.0f;

    /* calcul de la translation puisque l'image n'est pas forcément centrée
     * À FAIRE : pour les images EXR il faut préserver la fenêtre d'affichage */
    auto moitie_x = -static_cast<float>(size[0]) * 0.5f;
    auto moitie_y = -static_cast<float>(size[1]) * 0.5f;

    auto min_x = static_cast<float>(fenetre.x_min());
    auto min_y = static_cast<float>(fenetre.y_min());

    auto trans_x = (moitie_x - min_x) / 1920.0f;
    auto trans_y = (moitie_y - min_y) / 1920.0f;

    m_matrice_image[3][0] = trans_x;
    m_matrice_image[3][1] = trans_y;

    /* À FAIRE : il y a des crashs lors du démarrage, il faudrait réviser la
     * manière d'initialiser les éditeurs quand ils sont ajoutés */
    if (m_rendu_image != nullptr) {
        m_rendu_image->charge_composite(composite);
    }
}

void Visionneuse2D::mousePressEvent(QMouseEvent *event)
{
    m_base->mousePressEvent(event);
}

void Visionneuse2D::mouseMoveEvent(QMouseEvent *event)
{
    m_base->mouseMoveEvent(event);
}

void Visionneuse2D::mouseReleaseEvent(QMouseEvent *event)
{
    m_base->mouseReleaseEvent(event);
}

void Visionneuse2D::wheelEvent(QWheelEvent *event)
{
    m_base->wheelEvent(event);
}

/* ************************************************************************** */

static JJL::Composite accède_composite(JJL::Noeud noeud_racine_composite)
{
    if (noeud_racine_composite == nullptr) {
        return nullptr;
    }

    auto graphe_composite_ = noeud_racine_composite.accède_sous_graphe();
    if (graphe_composite_ == nullptr) {
        return nullptr;
    }

    auto graphe_composite = transtype<JJL::GrapheComposite>(graphe_composite_);

    auto noeud_sortie = graphe_composite.noeud_sortie();
    if (noeud_sortie == nullptr) {
        return nullptr;
    }

    return noeud_sortie.composite();
}

EditriceVue2D::EditriceVue2D(JJL::Jorjala &jorjala, QWidget *parent)
    : BaseEditrice("vue_2d", jorjala, parent), m_vue(new Visionneuse2D(jorjala, this))
{
    m_main_layout->addWidget(m_vue);
}

void EditriceVue2D::ajourne_etat(int evenement)
{
    auto chargement = evenement ==
                      static_cast<int>(JJL::TypeEvenement::IMAGE | JJL::TypeEvenement::TRAITÉ);
    chargement |= (evenement ==
                   static_cast<int>(JJL::TypeEvenement::TEMPS | JJL::TypeEvenement::MODIFIÉ));
    chargement |= (evenement == static_cast<int>(JJL::TypeEvenement::RAFRAICHISSEMENT));

    if (chargement) {
        auto graphe_cmp = m_jorjala.trouve_graphe_pour_chemin("/cmp");
        auto noeud_actif = graphe_cmp.noeud_actif();

        if (noeud_actif == nullptr) {
            return;
        }

        auto composite = accède_composite(noeud_actif);

        if (composite == nullptr) {
            /* Charge une image vide, les dimensions sont à peu près celle d'une
             * image de 1280x720. */
            auto desc = wlk::desc_grille_2d();
            desc.etendue.min = dls::math::vec2f(-1.7f);
            desc.etendue.max = dls::math::vec2f(1.0f);
            desc.fenetre_donnees = desc.etendue;
            desc.taille_pixel = 1.0;

            auto pixel = dls::phys::couleur32(0.0f);
            pixel.a = 1.0f;

            auto image_vide = grille_couleur(desc, pixel);

            m_vue->charge_image(image_vide);
        }
        else {
            m_vue->charge_composite(composite);
        }
    }

    m_vue->update();
}

QPointF EditriceVue2D::transforme_position_evenement(QPoint pos)
{
    return QPointF(m_vue->size().width() - pos.x(), pos.y());
}
