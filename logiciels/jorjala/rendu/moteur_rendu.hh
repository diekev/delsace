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

#pragma once

#include "biblinternes/math/matrice.hh"
#include "biblinternes/structures/tableau.hh"

#include "coeur/jorjala.hh"

class Scene;

struct deleguee_scene;
struct StatistiquesRendu;

/* ************************************************************************** */

struct ObjetRendu {
    JJL::Objet objet = JJL::Objet({});

    /* matrices pour définir où instancier l'objet */
    dls::tableau<dls::math::mat4x4f> matrices{};
};

/* Concernant ce déléguée_scène :
 * La finalité du MoteurRendu est d'abstraire différents moteurs de rendus
 * (traçage de rayon, ratissage, OpenGL, etc.) dans un système où il y a
 * plusieurs moteurs de rendu, et plusieurs représentation scénique différentes,
 * opérants en même temps. La Déléguée de scène servira de pont entre les
 * différentes représentations scéniques et les différents moteurs de rendus.
 * L'idée est similaire à celle présente dans Hydra de Pixar.
 */
struct deleguee_scene {
    dls::tableau<ObjetRendu> objets{};

    long nombre_objets() const;

    ObjetRendu const &objet(long idx) const;
};

/* ************************************************************************** */

class MoteurRendu {
  protected:
    JJL::EnveloppeCaméra3D m_camera = JJL::EnveloppeCaméra3D({});
    deleguee_scene *m_delegue = nullptr;

  public:
    MoteurRendu();

    virtual ~MoteurRendu();

    MoteurRendu(MoteurRendu const &) = delete;
    MoteurRendu &operator=(MoteurRendu const &) = delete;

    void camera(JJL::EnveloppeCaméra3D camera);

    deleguee_scene *delegue();

    virtual const char *id() const = 0;

    virtual void calcule_rendu(
        StatistiquesRendu &stats, float *tampon, int hauteur, int largeur, bool rendu_final) = 0;

    virtual void construit_scene();
};
