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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301) USA.
 *
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "code_inst.hh"

namespace lcc {

const char *chaine_code_inst(code_inst inst)
{
#define CHAINE_CAS(x) \
	case x: \
	{ \
		return #x; \
	}

	switch (inst) {
		CHAINE_CAS(code_inst::TERMINE)
		CHAINE_CAS(code_inst::ASSIGNATION)
		CHAINE_CAS(code_inst::NIE)
		CHAINE_CAS(code_inst::FN_TRADUIT)
		CHAINE_CAS(code_inst::FN_BASE_ORTHONORMALE)
		CHAINE_CAS(code_inst::FN_COMBINE_VEC2)
		CHAINE_CAS(code_inst::FN_COMBINE_VEC3)
		CHAINE_CAS(code_inst::FN_SEPARE_VEC2)
		CHAINE_CAS(code_inst::FN_SEPARE_VEC3)
		CHAINE_CAS(code_inst::FN_NORMALISE_VEC3)
		CHAINE_CAS(code_inst::FN_COMPLEMENT)
		CHAINE_CAS(code_inst::FN_PRODUIT_SCALAIRE_VEC3)
		CHAINE_CAS(code_inst::FN_PRODUIT_CROIX_VEC3)
		CHAINE_CAS(code_inst::FN_FRESNEL)
		CHAINE_CAS(code_inst::FN_REFLECHI)
		CHAINE_CAS(code_inst::FN_REFRACTE)
		CHAINE_CAS(code_inst::FN_ALEA_UNI)
		CHAINE_CAS(code_inst::FN_ALEA_NRM)
		CHAINE_CAS(code_inst::FN_AJOUTE)
		CHAINE_CAS(code_inst::FN_SOUSTRAIT)
		CHAINE_CAS(code_inst::FN_MULTIPLIE)
		CHAINE_CAS(code_inst::FN_MULTIPLIE_MAT)
		CHAINE_CAS(code_inst::FN_DIVISE)
		CHAINE_CAS(code_inst::FN_MODULO)
		CHAINE_CAS(code_inst::FN_COSINUS)
		CHAINE_CAS(code_inst::FN_SINUS)
		CHAINE_CAS(code_inst::FN_TANGEANTE)
		CHAINE_CAS(code_inst::FN_ARCCOSINUS)
		CHAINE_CAS(code_inst::FN_ARCSINUS)
		CHAINE_CAS(code_inst::FN_ARCTANGEANTE)
		CHAINE_CAS(code_inst::FN_ABSOLU)
		CHAINE_CAS(code_inst::FN_RACINE_CARREE)
		CHAINE_CAS(code_inst::FN_EXPONENTIEL)
		CHAINE_CAS(code_inst::FN_LOGARITHME)
		CHAINE_CAS(code_inst::FN_FRACTION)
		CHAINE_CAS(code_inst::FN_PLAFOND)
		CHAINE_CAS(code_inst::FN_SOL)
		CHAINE_CAS(code_inst::FN_ARRONDIS)
		CHAINE_CAS(code_inst::FN_INVERSE)
		CHAINE_CAS(code_inst::FN_ARCTAN2)
		CHAINE_CAS(code_inst::FN_MAX)
		CHAINE_CAS(code_inst::FN_MIN)
		CHAINE_CAS(code_inst::FN_PLUS_GRAND_QUE)
		CHAINE_CAS(code_inst::FN_PLUS_PETIT_QUE)
		CHAINE_CAS(code_inst::FN_PUISSANCE)
		CHAINE_CAS(code_inst::ENT_VERS_DEC)
		CHAINE_CAS(code_inst::DEC_VERS_ENT)
		CHAINE_CAS(code_inst::ENT_VERS_VEC2)
		CHAINE_CAS(code_inst::DEC_VERS_VEC2)
		CHAINE_CAS(code_inst::ENT_VERS_VEC3)
		CHAINE_CAS(code_inst::DEC_VERS_VEC3)
		CHAINE_CAS(code_inst::ENT_VERS_VEC4)
		CHAINE_CAS(code_inst::DEC_VERS_VEC4)
		CHAINE_CAS(code_inst::DEC_VERS_COULEUR)
		CHAINE_CAS(code_inst::VEC3_VERS_COULEUR)
		CHAINE_CAS(code_inst::COULEUR_VERS_VEC3)
		CHAINE_CAS(code_inst::FN_RESTREINT)
		CHAINE_CAS(code_inst::FN_ENLIGNE)
		CHAINE_CAS(code_inst::FN_HERMITE1)
		CHAINE_CAS(code_inst::FN_HERMITE2)
		CHAINE_CAS(code_inst::FN_HERMITE3)
		CHAINE_CAS(code_inst::FN_HERMITE4)
		CHAINE_CAS(code_inst::FN_HERMITE5)
		CHAINE_CAS(code_inst::FN_HERMITE6)
		CHAINE_CAS(code_inst::FN_AJOUTE_POINT)
		CHAINE_CAS(code_inst::FN_AJOUTE_PRIMITIVE)
		CHAINE_CAS(code_inst::FN_AJOUTE_PRIMITIVE_SOMMETS)
		CHAINE_CAS(code_inst::FN_AJOUTE_SOMMET)
		CHAINE_CAS(code_inst::FN_AJOUTE_SOMMETS)
		CHAINE_CAS(code_inst::FN_POINTS_VOISINS)
		CHAINE_CAS(code_inst::FN_POINT)
		CHAINE_CAS(code_inst::FN_SATURE)
		CHAINE_CAS(code_inst::FN_LUMINANCE)
		CHAINE_CAS(code_inst::FN_CONTRASTE)
		CHAINE_CAS(code_inst::FN_CORPS_NOIR)
		CHAINE_CAS(code_inst::FN_LONGUEUR_ONDE)
		CHAINE_CAS(code_inst::CONSTRUIT_TABLEAU)
		CHAINE_CAS(code_inst::IN_INSERT_TABLEAU)
		CHAINE_CAS(code_inst::IN_EXTRAIT_TABLEAU)
		CHAINE_CAS(code_inst::FN_TAILLE_TABLEAU)
		CHAINE_CAS(code_inst::FN_EGALITE)
		CHAINE_CAS(code_inst::FN_INEGALITE)
		CHAINE_CAS(code_inst::FN_SUPERIEUR)
		CHAINE_CAS(code_inst::FN_INFERIEUR)
		CHAINE_CAS(code_inst::FN_SUPERIEUR_EGAL)
		CHAINE_CAS(code_inst::FN_INFERIEUR_EGAL)
		CHAINE_CAS(code_inst::FN_COMP_ET)
		CHAINE_CAS(code_inst::FN_COMP_OU)
		CHAINE_CAS(code_inst::FN_COMP_OUX)
		CHAINE_CAS(code_inst::IN_BRANCHE)
		CHAINE_CAS(code_inst::IN_BRANCHE_CONDITION)
		CHAINE_CAS(code_inst::IN_INCREMENTE)
		CHAINE_CAS(code_inst::FN_LONGUEUR_VEC3)
		CHAINE_CAS(code_inst::FN_AJOUTE_LIGNE)
		CHAINE_CAS(code_inst::FN_ECHANTILLONE_SPHERE)
		CHAINE_CAS(code_inst::FN_ECHANTILLONE_IMAGE)
		CHAINE_CAS(code_inst::FN_ECHANTILLONE_TRIPLAN)
		CHAINE_CAS(code_inst::FN_PROJECTION_SPHERIQUE)
		CHAINE_CAS(code_inst::FN_PROJECTION_CYLINDRIQUE)
		CHAINE_CAS(code_inst::FN_PROJECTION_CAMERA)
		CHAINE_CAS(code_inst::FN_BRUIT_CELLULE)
		CHAINE_CAS(code_inst::FN_BRUIT_FLUX)
		CHAINE_CAS(code_inst::FN_BRUIT_FOURIER)
		CHAINE_CAS(code_inst::FN_BRUIT_ONDELETTE)
		CHAINE_CAS(code_inst::FN_BRUIT_PERLIN)
		CHAINE_CAS(code_inst::FN_BRUIT_SIMPLEX)
		CHAINE_CAS(code_inst::FN_BRUIT_VALEUR)
		CHAINE_CAS(code_inst::FN_BRUIT_VORONOI_F1)
		CHAINE_CAS(code_inst::FN_BRUIT_VORONOI_F2)
		CHAINE_CAS(code_inst::FN_BRUIT_VORONOI_F3)
		CHAINE_CAS(code_inst::FN_BRUIT_VORONOI_F4)
		CHAINE_CAS(code_inst::FN_BRUIT_VORONOI_F1F2)
		CHAINE_CAS(code_inst::FN_BRUIT_VORONOI_CR)
		CHAINE_CAS(code_inst::FN_EVALUE_BRUIT)
		CHAINE_CAS(code_inst::FN_EVALUE_BRUIT_TURBULENCE)
		CHAINE_CAS(code_inst::FN_EVALUE_COURBE_COULEUR)
		CHAINE_CAS(code_inst::FN_EVALUE_COURBE_VALEUR)
		CHAINE_CAS(code_inst::FN_EVALUE_RAMPE_COULEUR)
		CHAINE_CAS(code_inst::FN_TAILLE_CHAINE)
		CHAINE_CAS(code_inst::FN_MORCELLE_CHAINE)
		CHAINE_CAS(code_inst::FN_EXTRAIT_CHAINE_TABL)
	}

	return "erreur : code_inst inconnu";
}

bool est_fonction_mathematique(code_inst inst)
{
	switch (inst) {
		default:
		{
			return false;
		}
		case code_inst::FN_AJOUTE:
		case code_inst::FN_SOUSTRAIT:
		case code_inst::FN_MULTIPLIE:
		case code_inst::FN_MULTIPLIE_MAT:
		case code_inst::FN_DIVISE:
		case code_inst::FN_MODULO:
		case code_inst::FN_COSINUS:
		case code_inst::FN_SINUS:
		case code_inst::FN_TANGEANTE:
		case code_inst::FN_ARCCOSINUS:
		case code_inst::FN_ARCSINUS:
		case code_inst::FN_ARCTANGEANTE:
		case code_inst::FN_ABSOLU:
		case code_inst::FN_RACINE_CARREE:
		case code_inst::FN_EXPONENTIEL:
		case code_inst::FN_LOGARITHME:
		case code_inst::FN_FRACTION:
		case code_inst::FN_PLAFOND:
		case code_inst::FN_SOL:
		case code_inst::FN_ARRONDIS:
		case code_inst::FN_INVERSE:
		case code_inst::FN_ARCTAN2:
		case code_inst::FN_MAX:
		case code_inst::FN_MIN:
		case code_inst::FN_PLUS_GRAND_QUE:
		case code_inst::FN_PLUS_PETIT_QUE:
		case code_inst::FN_PUISSANCE:
		{
			return true;
		}
	}
}

}  /* namespace lcc */
