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

#include <vector>

struct Objet;

namespace vision {
class Camera3D;
}  /* namespace vision */

class Scene {
	std::vector<Objet *> m_objets{};
	vision::Camera3D *m_camera = nullptr;

public:
	Scene() = default;
	~Scene() = default;

	Scene(Scene const &) = default;
	Scene &operator=(Scene const &) = default;

	void reinitialise();

	void ajoute_objet(Objet *objet);

	void enleve_objet(Objet *objet);

	const std::vector<Objet *> &objets();

	void camera(vision::Camera3D *camera);

	vision::Camera3D *camera();
};
