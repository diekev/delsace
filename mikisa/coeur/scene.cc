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

#include "scene.h"

#include <algorithm>

void Scene::reinitialise()
{
	m_objets.clear();
	m_camera = nullptr;
}

void Scene::ajoute_objet(Objet *objet)
{
	m_objets.pousse(objet);
}

void Scene::enleve_objet(Objet *objet)
{
	auto iter = std::find(m_objets.debut(), m_objets.fin(), objet);
	m_objets.erase(iter);
}

const dls::tableau<Objet *> &Scene::objets()
{
	return m_objets;
}

void Scene::camera(vision::Camera3D *camera)
{
	m_camera = camera;
}

vision::Camera3D *Scene::camera()
{
	return m_camera;
}
