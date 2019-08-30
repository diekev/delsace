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

#include "operatrice_image.h"

class OperatricePixel : public OperatriceImage {
public:
	OperatricePixel(Graphe &graphe_parent, Noeud &noeud);

	virtual int type() const override;

	virtual void evalue_entrees(int temps) = 0;

	virtual dls::phys::couleur32 evalue_pixel(
			dls::phys::couleur32 const &pixel,
			const float x,
			const float y) = 0;

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override;
};
