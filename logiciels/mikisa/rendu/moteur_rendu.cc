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

#include "moteur_rendu.hh"

/* ************************************************************************** */

long deleguee_scene::nombre_objets() const
{
	return objets.taille();
}

Objet *deleguee_scene::objet(long idx) const
{
	return objets[idx];
}

/* ************************************************************************** */

MoteurRendu::MoteurRendu()
	: m_delegue(memoire::loge<deleguee_scene>("Délégué Scène"))
{}

MoteurRendu::~MoteurRendu()
{
	memoire::deloge("Délégué Scène", m_delegue);
}

void MoteurRendu::camera(vision::Camera3D *camera)
{
	m_camera = camera;
}

deleguee_scene *MoteurRendu::delegue()
{
	return m_delegue;
}

void MoteurRendu::construit_scene()
{
}
