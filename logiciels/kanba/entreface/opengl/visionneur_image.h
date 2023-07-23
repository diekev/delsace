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

#include "biblinternes/opengl/tampon_rendu.h"
#include "biblinternes/outils/definitions.h"

#include "biblinternes/math/matrice/matrice.hh"
#include "biblinternes/math/vecteur.hh"

namespace KNB {
struct Kanba;
}

class VueCanevas2D;

/**
 * La classe VisionneurImage contient la logique pour dessiner une image 2D avec
 * OpenGL dans une instance de VueCanevas.
 */
class VisionneurImage {
    VueCanevas2D *m_parent;

    std::unique_ptr<TamponRendu> m_tampon = nullptr;
    std::unique_ptr<TamponRendu> m_tampon_arêtes = nullptr;

    int m_hauteur = 0;
    int m_largeur = 0;

    KNB::Kanba &m_kanba;

  public:
    /**
     * Empêche la construction d'un visionneur sans VueCanevas.
     */
    VisionneurImage() = delete;

    EMPECHE_COPIE(VisionneurImage);

    /**
     * Construit un visionneur avec un pointeur vers le VueCanevas parent.
     */
    explicit VisionneurImage(VueCanevas2D *parent, KNB::Kanba &kanba);

    /**
     * Détruit le visionneur image. Les tampons de rendus sont détruits, et
     * utiliser cette instance après la destruction crashera le programme.
     */
    ~VisionneurImage() = default;

    /**
     * Crée les différents tampons de rendus OpenGL. Cette méthode est à appeler
     * dans un contexte OpenGL valide.
     */
    void initialise();

    /**
     * Dessine l'image avec OpenGL.
     */
    void peint_opengl();

    /**
     * Redimensionne le visionneur selon la largeur et la hauteur spécifiées.
     */
    void redimensionne(int largeur, int hauteur);

    void charge_image();
};
