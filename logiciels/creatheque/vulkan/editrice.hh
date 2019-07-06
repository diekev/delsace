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

#include <delsace/math/vecteur.hh>

struct Evenement;

class Editrice {
protected:
	dls::math::vec2d pos{};
	dls::math::vec2d taille_norm = dls::math::vec2d{1.0, 1.0};
	dls::math::vec2d taille      = dls::math::vec2d{800.0, 600.0};

public:
	virtual ~Editrice() = default;

	bool accepte_evenement(Evenement const &evenement);

	virtual void souris_bougee(Evenement const &) = 0;

	virtual void souris_pressee(Evenement const &) = 0;

	virtual void souris_relachee(Evenement const &) = 0;

	virtual void cle_pressee(Evenement const &) = 0;

	virtual void cle_relachee(Evenement const &) = 0;

	virtual void cle_repetee(Evenement const &) = 0;

	virtual void roulette(Evenement const &) = 0;

	virtual void double_clic(Evenement const &) = 0;

	virtual void dessine() = 0;
};
