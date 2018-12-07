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

#include "creation_arbre.h"

#include "bibliotheques/outils/constantes.h"

#include "arbre.h"

void parametres_tremble(Parametres *parametres)
{
	parametres->Shape = FORME_FLAME_TEND;
	parametres->BaseSize = 0.4f;
	parametres->Scale = 13;
	parametres->ScaleV = 3;
	parametres->ZScale = 1;
	parametres->ZScaleV = 0;
	parametres->Levels = 3;
	parametres->Ratio = 0.015f;
	parametres->RatioPower = 1.2f;
	parametres->Lobes = 5;
	parametres->LobeDepth = 0.07f;
	parametres->Flare = 0.6f;

	parametres->_0Scale = 1;
	parametres->_0ScaleV = 0;
	parametres->_0Length = 1;
	parametres->_0LengthV = 0;
	parametres->_0BaseSplits = 0;
	parametres->_0SegSplits = 0;
	parametres->_0SplitAngle = 0;
	parametres->_0SplitAngleV = 0;
	parametres->_0CurveRes = 3;
	parametres->_0Curve = 0;
	parametres->_0CurveBack = 0;
	parametres->_0CurveV = 20;

	parametres->_1DownAngle = 60;
	parametres->_1DownAngleV = -50;
	parametres->_1Rotate = 140;
	parametres->_1RotateV = 0;
	parametres->_1Branches = 50;
	parametres->_1Length = 0.3f;
	parametres->_1LengthV = 0;
	parametres->_1Taper = 1;
	parametres->_1SegSplits = 0;
	parametres->_1SplitAngle = 0;
	parametres->_1SplitAngleV = 0;
	parametres->_1CurveRes = 5;
	parametres->_1Curve = -40;
	parametres->_1CurveBack = 0;
	parametres->_1CurveV = 50;

	parametres->_2DownAngle = 45;
	parametres->_2DownAngleV = 10;
	parametres->_2Rotate = 140;
	parametres->_2RotateV = 0;
	parametres->_2Branches = 30;
	parametres->_2Length = 0.6f;
	parametres->_2LengthV = 0;
	parametres->_2Taper = 1;
	parametres->_2SegSplits = 0;
	parametres->_2SplitAngle = 0;
	parametres->_2SplitAngleV = 0;
	parametres->_2CurveRes = 3;
	parametres->_2Curve = -40;
	parametres->_2CurveBack = 0;
	parametres->_2CurveV = 75;

	parametres->_3DownAngle = 45;
	parametres->_3DownAngleV = 10;
	parametres->_3Rotate = 77;
	parametres->_3RotateV = 0;
	parametres->_3Branches = 10;
	parametres->_3Length = 0;
	parametres->_3LengthV = 0;
	parametres->_3Taper = 1;
	parametres->_3SegSplits = 0;
	parametres->_3SplitAngle = 0;
	parametres->_3SplitAngleV = 0;
	parametres->_3CurveRes = 1;
	parametres->_3Curve = 0;
	parametres->_3CurveBack = 0;
	parametres->_3CurveV = 0;

	parametres->Leaves = 25;
	parametres->LeafShape = 0;
	parametres->LeafScale = 0.17f;
	parametres->LeafScaleX = 1;
	parametres->AttractionUp = 0.5f;
	parametres->PruneRatio = 0;
	parametres->PruneWidth = 0.5f;
	parametres->PruneWidthPeak = 0.5f;
	parametres->PrunePowerLow = 0.5f;
	parametres->PrunePowerHigh = 0.5f;
}

float ShapeRatio(int shape, float ratio)
{
	switch (shape) {
		case FORME_CONIQUE:
			return 0.2f + 0.8f * ratio;
		case FORME_SPHERIQUE:
			return 0.2f + 0.8f * std::sin(static_cast<float>(PI) * ratio);
		case FORME_HEMISPHERIQUE:
			return 0.2f + 0.8f * std::sin(0.5f * static_cast<float>(PI) * ratio);
		case FORME_CYLINDRIQUE:
			return 1.0f;
		case FORME_CYLINDRIQUE_TAPERED:
			return 0.5f + 0.5f * ratio;
		case FORME_FLAME:
			if (ratio <= 0.7f) {
				return ratio / 0.7f;
			}

			return (1.0f - ratio) / 0.3f;
		case FORME_CONIQUE_INVERSE:
			return 1.0f - 0.8f * ratio;
		case FORME_FLAME_TEND:
			if (ratio <= 0.7f) {
				return 0.5f + 0.5f * ratio / 0.7f;
			}

			return 0.5f + 0.5f * (1.0f - ratio) / 0.3f;
		default:
		case FORME_ENVELOPPE:
			/* À FAIRE : utiliser l'enveloppe de taille */
			return 1.0f;
	}
}

void cree_arbre(Arbre *arbre)
{
	arbre->reinitialise();

	Parametres *params = arbre->parametres();

	/* Tronc */
	auto taille_tronc = params->Scale * params->_0Scale;
	auto taille_segment = taille_tronc / params->_0CurveRes;

	auto origine = dls::math::vec3f(0.0f, 0.0f, 0.0f);

	for (int i = 0; i < static_cast<int>(params->_0CurveRes); ++i) {
		arbre->ajoute_sommet(origine);
		origine.y += taille_segment;
	}

	for (int i = 0; i < static_cast<int>(params->_0CurveRes) - 1; ++i) {
		arbre->ajoute_arrete(i, i + 1);
	}
}
