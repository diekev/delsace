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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/phys/rayon.hh"
#include "biblinternes/phys/spectre.hh"

#include "pellicule.h"
#include "tache.h"

class GNA;

namespace kdo {

class ParametresRendu;

class MoteurRendu {
	Pellicule m_pellicule{};
	bool m_est_arrete = false;

public:
	void echantillone_scene(ParametresRendu const &parametres, dls::tableau<CarreauPellicule> const &carreaux, unsigned int echantillon);

	dls::math::matrice_dyn<dls::math::vec3d> const &pellicule();

	void reinitialise();

	Pellicule *pointeur_pellicule();

	bool est_arrete() const;
	void arrete();
};

Spectre calcul_spectre(GNA &gna, ParametresRendu const &parametres, dls::phys::rayond const &rayon, uint profondeur = 0);

/* ************************************************************************** */

class TacheRendu : public Tache {
public:
	explicit TacheRendu(Koudou const &koudou);

	void commence(Koudou const &koudou) override;
};

}  /* namespace kdo */
