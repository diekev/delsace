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

#include "biblinternes/chrono/outils.hh"
#include "biblinternes/opengl/contexte_rendu.h"
#include "biblinternes/opengl/pile_matrice.h"

namespace vision {

class Camera3D;

}  // namespace vision

class Kanba;
class RenduBrosse;
class RenduGrille;
class RenduMaillage;
class RenduSeaux;
class RenduTexte;
class VueCanevas3D;

/**
 * La classe VisionneurScene contient la logique pour dessiner une scène 3D avec
 * OpenGL dans une instance de VueCanevas.
 */
class VisionneurScene {
    VueCanevas3D *m_parent;
    Kanba *m_kanba;

    vision::Camera3D *m_camera;
    RenduBrosse *m_rendu_brosse;
    RenduGrille *m_rendu_grille;
    RenduTexte *m_rendu_texte;
    RenduMaillage *m_rendu_maillage;
    RenduSeaux *m_rendu_seaux = nullptr;

    ContexteRendu m_contexte{};

    PileMatrice m_stack = {};

    float m_pos_x, m_pos_y;
    dls::chrono::metre_seconde m_chrono_rendu{};

    bool m_affiche_brosse = false;

  public:
    /**
     * Empêche la construction d'un visionneur sans VueCanevas.
     */
    VisionneurScene() = delete;

    /**
     * Construit un visionneur avec un pointeur vers le VueCanevas parent, et un
     * pointeur vers l'instance de Kanba du programme en cours.
     */
    VisionneurScene(VueCanevas3D *parent, Kanba *kanba);

    /**
     * Empêche la copie d'un visionneur.
     */
    EMPECHE_COPIE(VisionneurScene);

    /**
     * Détruit le visionneur scène. Les tampons de rendus sont détruits, et
     * utiliser cette instance après la destruction crashera le programme.
     */
    ~VisionneurScene();

    /**
     * Crée les différents tampons de rendus OpenGL. Cette méthode est à appeler
     * dans un contexte OpenGL valide.
     */
    void initialise();

    /**
     * Dessine la scène avec OpenGL.
     */
    void peint_opengl();

    /**
     * Redimensionne le visionneur selon la largeur et la hauteur spécifiées.
     */
    void redimensionne(int largeur, int hauteur);

    /**
     * Renseigne la position de la souris.
     */
    void position_souris(int x, int y);

    /**
     * Détermine si la brosse doit être affichée.
     */
    void affiche_brosse(bool oui_ou_non);
};
