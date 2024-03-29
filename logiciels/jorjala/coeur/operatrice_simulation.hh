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

#include "corps/corps.h"

#include "operatrice_corps.h"

class OperatriceSimulation final : public OperatriceCorps {
    int m_dernier_temps = 0;
    int pad = 0;

    Corps m_corps1{};
    Corps m_corps2{};

  public:
    static constexpr auto NOM = "Simulation";
    static constexpr auto AIDE = "Ajoute un noeud de simulation physique";

    OperatriceSimulation(Graphe &graphe_parent, Noeud &noeud_);

    virtual const char *nom_classe() const override;

    virtual const char *texte_aide() const override;

    ResultatCheminEntreface chemin_entreface() const override;

    int type() const override;

    res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override;

    bool depend_sur_temps() const override;

    void amont_change(PriseEntree *entree) override;

    void renseigne_dependance(ContexteEvaluation const &contexte,
                              CompilatriceReseau &compilatrice,
                              NoeudReseau *noeud) override;
};
