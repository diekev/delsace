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

class Arbre;

enum {
	FORME_CONIQUE = 0,
	FORME_SPHERIQUE = 1,
	FORME_HEMISPHERIQUE = 2,
	FORME_CYLINDRIQUE = 3,
	FORME_CYLINDRIQUE_TAPERED = 4,
	FORME_FLAME = 5,
	FORME_CONIQUE_INVERSE = 6,
	FORME_FLAME_TEND = 7,
	FORME_ENVELOPPE = 8,
};

struct Parametres {
	int Shape;
	float BaseSize;
	float Scale,ScaleV,ZScale,ZScaleV;
	int Levels;
	float Ratio,RatioPower;
	int Lobes;
	float LobeDepth;
	float Flare;

	float _0Scale, _0ScaleV;
	float _0Length, _0LengthV, _0Taper;
	float _0BaseSplits;
	float _0SegSplits,_0SplitAngle,_0SplitAngleV;
	float _0CurveRes,_0Curve,_0CurveBack,_0CurveV;

	float _1DownAngle,_1DownAngleV;
	float _1Rotate,_1RotateV,_1Branches;
	float _1Length,_1LengthV,_1Taper;
	float _1SegSplits,_1SplitAngle,_1SplitAngleV;
	float _1CurveRes,_1Curve,_1CurveBack,_1CurveV;

	float _2DownAngle,_2DownAngleV;
	float _2Rotate,_2RotateV,_2Branches;
	float _2Length,_2LengthV, _2Taper;
	float _2SegSplits,_2SplitAngle,_2SplitAngleV;
	float _2CurveRes,_2Curve,_2CurveBack,_2CurveV;

	float _3DownAngle,_3DownAngleV;
	float _3Rotate,_3RotateV,_3Branches;
	float _3Length,_3LengthV, _3Taper;
	float _3SegSplits,_3SplitAngle,_3SplitAngleV;
	float _3CurveRes,_3Curve,_3CurveBack,_3CurveV;

	float Leaves,LeafShape;
	float LeafScale,LeafScaleX;
	float AttractionUp;
	float PruneRatio;
	float PruneWidth,PruneWidthPeak;
	float PrunePowerLow,PrunePowerHigh;

	Parametres() = default;
	Parametres(Parametres const &autre) = default;
};

void parametres_tremble(Parametres *parametres);

float ShapeRatio(int shape, float ratio);

void cree_arbre(Arbre *arbre);
