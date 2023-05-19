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

#include "moteur_rendu_opengl.hh"

#include "biblinternes/ego/outils.h"
#include <GL/glew.h>

#include <set>

#include "biblinternes/opengl/contexte_rendu.h"
#include "biblinternes/opengl/pile_matrice.h"
#include "biblinternes/opengl/rendu_camera.h"
#include "biblinternes/opengl/rendu_grille.h"

#include "coeur/conversion_types.hh"
#include "coeur/jorjala.hh"
#include "coeur/objet.h"

#include "rendu_corps.h"
#include "rendu_lumiere.h"

/* ************************************************************************** */

MoteurRenduOpenGL::~MoteurRenduOpenGL()
{
    memoire::deloge("RenduGrille", m_rendu_grille);

    for (auto paire : m_rendus_corps) {
        memoire::deloge("RendusCorps", paire.second);
    }
}

const char *MoteurRenduOpenGL::id() const
{
    return "opengl";
}

void MoteurRenduOpenGL::calcule_rendu(
    StatistiquesRendu &stats, float *tampon, int hauteur, int largeur, bool rendu_final)
{
    auto contexte = crée_contexte_rendu();

    ajourne_objets(contexte);

    /* initialise données pour le rendu */

    GLuint fbo, render_buf, tampon_prof;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

    glGenRenderbuffers(1, &render_buf);
    glBindRenderbuffer(GL_RENDERBUFFER, render_buf);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, largeur, hauteur);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, render_buf);

    glGenRenderbuffers(1, &tampon_prof);
    glBindRenderbuffer(GL_RENDERBUFFER, tampon_prof);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, largeur, hauteur);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, tampon_prof);

    /* ****************************************************************** */

    int taille_original[4];
    glGetIntegerv(GL_VIEWPORT, taille_original);

    glViewport(0, 0, largeur, hauteur);

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.5, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* ****************************************************************** */

    auto pile = PileMatrice{};

    contexte.matrice_objet(math::matf_depuis_matd(pile.sommet()));

    /* Peint la grille. */
    if (!rendu_final) {
        if (m_rendu_grille == nullptr) {
            /* Simule une grille infini en en dessinant une aussi grande que la
             * caméra peut voir. */
            auto taille_grille = static_cast<int>(m_camera.plan_éloigné() * 2.0f);
            m_rendu_grille = memoire::loge<RenduGrille>(
                "RenduGrille", taille_grille, taille_grille);
        }

        m_rendu_grille->dessine(contexte);
    }

    stats.nombre_objets = m_objets_à_rendre.taille();

    for (auto objet_à_rendre : m_objets_à_rendre) {
        auto objet_rendu = m_delegue->objet(objet_à_rendre.index_délégué);
        // auto objet = objet_rendu.objet;

        /* À FAIRE : matrice pour chaque objet */
        // pile.ajoute(objet->transformation.matrice());
        pile.ajoute(dls::math::mat4x4d(1.0));

        if (objet_rendu.matrices.taille() == 0) {
            /* À FAIRE : matrice pour chaque corps */
            // pile.ajoute(corps.transformation.matrice());
            pile.ajoute(dls::math::mat4x4d(1.0));
        }

        contexte.matrice_objet(math::matf_depuis_matd(pile.sommet()));

        RenduCorps *rendu_corps = objet_à_rendre.rendu_corps;
        rendu_corps->dessine(stats, contexte);

        if (objet_rendu.matrices.taille() == 0) {
            pile.enleve_sommet();
        }

        pile.enleve_sommet();
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    /* ****************************************************************** */

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, largeur, hauteur, GL_RGBA, GL_FLOAT, tampon);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &render_buf);
    glDeleteRenderbuffers(1, &tampon_prof);

    glViewport(taille_original[0], taille_original[1], taille_original[2], taille_original[3]);

    /* Inverse la direction de l'image */
    for (int x = 0; x < largeur * 4; x += 4) {
        for (int y = 0; y < hauteur / 2; ++y) {
            auto idx0 = x + largeur * y * 4;
            auto idx1 = x + largeur * (hauteur - y - 1) * 4;

            std::swap(tampon[idx0 + 0], tampon[idx1 + 0]);
            std::swap(tampon[idx0 + 1], tampon[idx1 + 1]);
            std::swap(tampon[idx0 + 2], tampon[idx1 + 2]);
            std::swap(tampon[idx0 + 3], tampon[idx1 + 3]);
        }
    }
}

ContexteRendu MoteurRenduOpenGL::crée_contexte_rendu()
{
    m_camera.ajourne();

    auto résultat = ContexteRendu{};

    auto const &MV = convertis_matrice(m_camera.MV());
    auto const &P = convertis_matrice(m_camera.P());
    auto const &MVP = P * MV;

    résultat.vue(convertis_vecteur(m_camera.direction()));
    résultat.modele_vue(MV);
    résultat.projection(P);
    résultat.MVP(MVP);
    résultat.normal(dls::math::inverse_transpose(dls::math::mat3_depuis_mat4(MV)));
    résultat.pour_surlignage(false);

    return résultat;
}

void MoteurRenduOpenGL::ajourne_objets(ContexteRendu &contexte)
{
    m_objets_à_rendre.efface();

    std::set<RenduCorps *> rendus_utilisés;

    for (auto i = 0; i < m_delegue->nombre_objets(); ++i) {
        auto objet_rendu = m_delegue->objet(i);
        auto objet = objet_rendu.objet;

        /* À FAIRE : drapeaux pour savoir si l'objet est à rendre. */
        //		if (!objet->rendu_scene) {
        //			continue;
        //		}

        /* À FAIRE : types d'objets */
        //		if (objet->type != type_objet::CORPS && rendu_final) {
        //			continue;
        //      }

        /* À FAIRE : mutex pour accéder aux données de l'objet. */
        // objet->donnees.accede_lecture
        auto corps = objet->accède_corps();
        if (!corps) {
            continue;
        }

        RenduCorps *rendu_corps = nullptr;
        auto iter_rendu_corps = m_rendus_corps.find(corps.uuid());
        if (iter_rendu_corps == m_rendus_corps.end()) {
            // std::cerr << "Création d'un nouveau rendu corps...\n";
            rendu_corps = memoire::loge<RenduCorps>("RenduCorps", corps);
            /* À FAIRE : invalide si les matrices ne sont pas les mêmes. */
            rendu_corps->initialise(contexte, objet_rendu.matrices);

            m_rendus_corps.insert({corps.uuid(), rendu_corps});
        }
        else {
            // std::cerr << "Réutilisation d'un ancien rendu corps...\n";
            rendu_corps = iter_rendu_corps->second;
        }

        rendus_utilisés.insert(rendu_corps);
        m_objets_à_rendre.ajoute({i, rendu_corps});

#if 0
        objet->donnees.accede_lecture([&objet, &pile, &contexte, &stats, &objet_rendu](DonneesObjet const *donnees)
        {
            if (objet->type == type_objet::CAMERA) {
                auto const &camera = extrait_camera(donnees);

                /* la rotation de la caméra est appliquée aux points dans
                 * RenduCamera, donc on recrée une matrice sans rotation, et dont
                 * la taille dans la scène est de 1.0 (en mettant à l'échelle
                 * avec un facteur de 1.0 / distance éloignée. */
                auto matrice = dls::math::mat4x4d(1.0);
                matrice = dls::math::translation(matrice, dls::math::vec3d(camera.pos()));
                matrice = dls::math::dimension(matrice, dls::math::vec3d(static_cast<double>(1.0f / camera.eloigne())));
                pile.ajoute(matrice);
                contexte.matrice_objet(math::matf_depuis_matd(pile.sommet()));

                RenduCamera rendu_camera(const_cast<vision::Camera3D *>(&camera));
                rendu_camera.initialise();
                rendu_camera.dessine(contexte);

                pile.enleve_sommet();
            }
            else if (objet->type == type_objet::LUMIERE) {
                auto const &lumiere = extrait_lumiere(donnees);

                contexte.matrice_objet(math::matf_depuis_matd(pile.sommet()));

                auto rendu_lumiere = RenduLumiere(&lumiere);
                rendu_lumiere.initialise();
                rendu_lumiere.dessine(contexte);
            }
            else {
                auto const &corps = extrait_corps(donnees);

                if (objet_rendu.matrices.taille() == 0) {
                    pile.ajoute(corps.transformation.matrice());
                }

                contexte.matrice_objet(math::matf_depuis_matd(pile.sommet()));

                RenduCorps rendu_corps(&corps);
                rendu_corps.initialise(contexte, stats, objet_rendu.matrices);
                rendu_corps.dessine(contexte);

                if (objet_rendu.matrices.taille() == 0) {
                    pile.enleve_sommet();
                }
            }
        });
#endif
    }

    std::map<unsigned long, RenduCorps *> rendus_corps;
    for (auto paire : m_rendus_corps) {
        if (rendus_utilisés.find(paire.second) == rendus_utilisés.end()) {
            memoire::deloge("RendusCorps", paire.second);
            continue;
        }

        rendus_corps.insert({paire.first, paire.second});
    }

    m_rendus_corps = rendus_corps;
}
