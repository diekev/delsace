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

#include "biblinternes/structures/tableau.hh"

#include <map>

#include "moteur_rendu.hh"

class ContexteRendu;
class RenduGrille;
class RenduCorps;
class TamponRendu;

class MoteurRenduOpenGL final : public MoteurRendu {
    RenduGrille *m_rendu_grille = nullptr;

    dls::tableau<TamponRendu *> m_tampons{};

    std::map<unsigned long, RenduCorps *> m_rendus_corps{};

    struct ObjetÀRendre {
        /* Index dans le délégué_scène. */
        long index_délégué = 0;
        RenduCorps *rendu_corps = nullptr;
    };

    dls::tableau<ObjetÀRendre> m_objets_à_rendre{};

  public:
    MoteurRenduOpenGL() = default;

    ~MoteurRenduOpenGL() override;

    MoteurRenduOpenGL(MoteurRenduOpenGL const &) = delete;
    MoteurRenduOpenGL &operator=(MoteurRenduOpenGL const &) = delete;

    const char *id() const override;

    void calcule_rendu(StatistiquesRendu &stats,
                       float *tampon,
                       int hauteur,
                       int largeur,
                       bool rendu_final) override;

  private:
    ContexteRendu crée_contexte_rendu();

    void ajourne_objets(ContexteRendu &contexte);
};
