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

namespace lcc {

enum class code_inst : int {
	/* constructions de types natifs */
	CONSTRUIT_TABLEAU,

	IN_INSERT_TABLEAU,
	IN_EXTRAIT_TABLEAU,
	FN_TAILLE_TABLEAU,
	IN_BRANCHE,
	IN_BRANCHE_CONDITION,
	IN_INCREMENTE,

	/* opérateurs */
	ASSIGNATION,
	NIE,
	FN_AJOUTE,
	FN_SOUSTRAIT,
	FN_MULTIPLIE,
	FN_MULTIPLIE_MAT,
	FN_DIVISE,
	FN_MODULO,
	FN_EGALITE,
	FN_INEGALITE,
	FN_SUPERIEUR,
	FN_INFERIEUR,
	FN_SUPERIEUR_EGAL,
	FN_INFERIEUR_EGAL,
	FN_COMP_OU,
	FN_COMP_ET,
	FN_COMP_OUX,

	/* fonctions */
	FN_BASE_ORTHONORMALE,
	FN_COMBINE_VEC2,
	FN_COMBINE_VEC3,
	FN_SEPARE_VEC2,
	FN_SEPARE_VEC3,
	FN_TRADUIT,
	FN_PRODUIT_SCALAIRE_VEC3,
	FN_PRODUIT_CROIX_VEC3,
	FN_NORMALISE_VEC3,
	FN_LONGUEUR_VEC3,
	FN_COMPLEMENT,
	FN_FRESNEL,
	FN_REFRACTE,
	FN_REFLECHI,
	FN_ALEA_UNI,
	FN_ALEA_NRM,
	FN_ECHANTILLONE_SPHERE,
	FN_RESTREINT,
	FN_ENLIGNE,
	FN_HERMITE1,
	FN_HERMITE2,
	FN_HERMITE3,
	FN_HERMITE4,
	FN_HERMITE5,
	FN_HERMITE6,

	/* fonctions mathématiques paramètre simple */
	FN_COSINUS,
	FN_SINUS,
	FN_TANGEANTE,
	FN_ARCCOSINUS,
	FN_ARCSINUS,
	FN_ARCTANGEANTE,
	FN_ABSOLU,
	FN_RACINE_CARREE,
	FN_EXPONENTIEL,
	FN_LOGARITHME,
	FN_FRACTION,
	FN_PLAFOND,
	FN_SOL,
	FN_ARRONDIS,
	FN_INVERSE,

	/* fonctions mathématiques paramètres doubles */
	FN_ARCTAN2,
	FN_MAX,
	FN_MIN,
	FN_PLUS_GRAND_QUE,
	FN_PLUS_PETIT_QUE,
	FN_PUISSANCE,

	/* fonctions colorimétriques */
	FN_SATURE,
	FN_LUMINANCE,
	FN_CONTRASTE,
	FN_CORPS_NOIR,
	FN_LONGUEUR_ONDE,

	/* conversion */
	ENT_VERS_DEC,
	DEC_VERS_ENT,
	ENT_VERS_VEC2,
	DEC_VERS_VEC2,
	ENT_VERS_VEC3,
	DEC_VERS_VEC3,
	ENT_VERS_VEC4,
	DEC_VERS_VEC4,
	DEC_VERS_COULEUR,
	VEC3_VERS_COULEUR,
	COULEUR_VERS_VEC3,

	/* fonctions topologiques */
	FN_AJOUTE_POINT,
	FN_AJOUTE_PRIMITIVE,
	FN_AJOUTE_PRIMITIVE_SOMMETS,
	FN_AJOUTE_SOMMET,
	FN_AJOUTE_SOMMETS,
	FN_AJOUTE_LIGNE,
	FN_POINTS_VOISINS,
	FN_POINT,

	/* fonctions bruit */
	FN_BRUIT_CELLULE,
	FN_BRUIT_FLUX,
	FN_BRUIT_FOURIER,
	FN_BRUIT_ONDELETTE,
	FN_BRUIT_PERLIN,
	FN_BRUIT_SIMPLEX,
	FN_BRUIT_VALEUR,
	FN_BRUIT_VORONOI_F1,
	FN_BRUIT_VORONOI_F2,
	FN_BRUIT_VORONOI_F3,
	FN_BRUIT_VORONOI_F4,
	FN_BRUIT_VORONOI_F1F2,
	FN_BRUIT_VORONOI_CR,
	FN_EVALUE_BRUIT,
	FN_EVALUE_BRUIT_TURBULENCE,

	/* fonctions images */
	FN_ECHANTILLONE_IMAGE,
	FN_ECHANTILLONE_TRIPLAN,
	FN_PROJECTION_SPHERIQUE,
	FN_PROJECTION_CYLINDRIQUE,
	FN_PROJECTION_CAMERA,

	FN_EVALUE_COURBE_VALEUR,
	FN_EVALUE_COURBE_COULEUR,
	FN_EVALUE_RAMPE_COULEUR,

	/* autres */
	TERMINE,
};

const char *chaine_code_inst(code_inst inst);

bool est_fonction_mathematique(code_inst inst);

}  /* namespace lcc */
