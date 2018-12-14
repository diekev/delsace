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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "primitive.h"

class FenetrePrincipale;
class NodeFactory;
class Scene;
class UsineOperatrice;
class BaseEditrice;

namespace vision {

class Camera3D;

}  /* namespace vision */

enum {
	TIME_DIR_FORWARD = 0,
	TIME_DIR_BACKWARD = 1,
};

struct EvaluationContext {
	/** Whether we are currently editing the graph of an object. */
	bool edit_mode;

	/** Whether we are currently playing an animation. */
	bool animation;

	char time_direction;
};

struct Context {
	EvaluationContext *eval_ctx;
	Scene *scene;
	PrimitiveFactory *primitive_factory;
	UsineOperatrice &usine_operatrice;
	FenetrePrincipale *main_window;
	BaseEditrice *active_widget;
	vision::Camera3D *camera;

	Context(UsineOperatrice &usine)
		: eval_ctx(nullptr)
		, scene(nullptr)
		, primitive_factory(nullptr)
		, usine_operatrice(usine)
		, main_window(nullptr)
		, active_widget(nullptr)
		, camera(nullptr)
	{}
};
