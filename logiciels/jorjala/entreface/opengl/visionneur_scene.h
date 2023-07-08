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

namespace JJL {
class Jorjala;
class Camera3D;
}  // namespace JJL

class MoteurRendu;
class RenduManipulatriceEchelle;
class RenduManipulatricePosition;
class RenduManipulatriceRotation;
class RenduTexte;
class TamponRendu;
class VueCanevas3D;

/**
 * La classe VisionneurScene contient la logique pour dessiner une scène 3D avec
 * OpenGL dans une instance de VueCanevas.
 */
class VisionneurScene {
    VueCanevas3D *m_parent;
    JJL::Jorjala &m_jorjala;

    RenduTexte *m_rendu_texte;
    RenduManipulatricePosition *m_rendu_manipulatrice_pos;
    RenduManipulatriceRotation *m_rendu_manipulatrice_rot;
    RenduManipulatriceEchelle *m_rendu_manipulatrice_ech;

    ContexteRendu m_contexte{};

    PileMatrice m_stack = {};

    TamponRendu *m_tampon_image = nullptr;
    dls::chaine m_nom_rendu = "rendu";
    float *m_tampon = nullptr;

    float m_pos_x, m_pos_y;
    dls::chrono::metre_seconde m_chrono_rendu{};

    MoteurRendu *m_moteur_rendu;

  public:
    /**
     * Empêche la construction d'un visionneur sans VueCanevas.
     */
    VisionneurScene() = delete;

    /**
     * Construit un visionneur avec un pointeur vers le VueCanevas parent, et un
     * pointeur vers l'instance de Kanba du programme en cours.
     */
    VisionneurScene(VueCanevas3D *parent, JJL::Jorjala &jorjala);

    /**
     * Empêche la copie d'un visionneur.
     */
    VisionneurScene(VisionneurScene const &visionneur) = delete;
    VisionneurScene &operator=(VisionneurScene const &) = delete;

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

    void change_moteur_rendu(const dls::chaine &id);
};
