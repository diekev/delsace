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

class CompileuseGraphe;

enum {
	NOEUD_ENTREE,
	NOEUD_SORTIE,
	NOEUD_SATURATION,
	NOEUD_MELANGE,            /* À FAIRE : plusieurs entrées */
	NOEUD_FUSIONNAGE,         /* À FAIRE : plusieurs entrées */
	NOEUD_BRUITAGE,           /* À FAIRE : génératrice nombre aléatoire */
	NOEUD_CONSTANTE,          /* À FAIRE */
	NOEUD_DEGRADE,            /* À FAIRE : passer rampe couleur */
	NOEUD_NUAGE,              /* À FAIRE */
	NOEUD_ETALONNAGE,         /* À FAIRE */
	NOEUD_CORRECTION_GAMMA,   /* À FAIRE */
	NOEUD_MAPPAGE_TONAL,      /* À FAIRE */
	NOEUD_CORRECTION_COULEUR, /* À FAIRE */
	NOEUD_INVERSEMENT,        /* À FAIRE */
	NOEUD_INCRUSTATION,       /* À FAIRE */
	NOEUD_PREMULTIPLICATION,  /* À FAIRE */
	NOEUD_NORMALISATION,      /* À FAIRE */
	NOEUD_CONTRASTE,          /* À FAIRE */
	NOEUD_COURBE_COULEUR,     /* À FAIRE : passer courbe couleur */
	NOEUD_TRADUCTION,         /* À FAIRE */
	NOEUD_MIN_MAX,            /* À FAIRE */
	NOEUD_DALTONISME,         /* À FAIRE */
};

class OperatricePixel : public OperatriceImage {
public:
	explicit OperatricePixel(Graphe &graphe_parent, Noeud *node);

	virtual int type() const override;

	virtual void evalue_entrees(int temps) = 0;

	virtual dls::image::Pixel<float> evalue_pixel(
			dls::image::Pixel<float> const &pixel,
			const float x,
			const float y) = 0;

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override;

	virtual void compile(CompileuseGraphe &compileuse, int temps);
};
