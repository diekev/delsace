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

#include "lumiere.h"

#include "nuanceur.h"

Lumiere::Lumiere(const math::transformation &transform)
	: transformation(transform)
{}

Lumiere::~Lumiere()
{
	delete nuanceur;
}

LumierePoint::LumierePoint(const math::transformation &transform, Spectre spectre, double intensite)
	: Lumiere(transform)
{
	this->type = LUMIERE_POINT;
	this->spectre = spectre;
	this->intensite = intensite;

	transform(numero7::math::point3d(0.0), &this->pos);
}

LumiereDistante::LumiereDistante(const math::transformation &transform, Spectre spectre, double intensite)
	: Lumiere(transform)
{
	this->type = LUMIERE_DISTANTE;
	this->spectre = spectre;
	this->intensite = intensite;

	transform(numero7::math::vec3d(0.0, 0.0, -1.0), &this->dir);
	this->dir = numero7::math::normalise(this->dir);
}
