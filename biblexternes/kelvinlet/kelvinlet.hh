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

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include "brushAffine.h"
#include "brushBase.h"
#include "brushGrab.h"
#include "brushGrabBase.h"
#include "brushGrabBiLaplacian.h"
#include "brushGrabBiScale.h"
#include "brushGrabCusp.h"
#include "brushGrabCuspBiLaplacian.h"
#include "brushGrabCuspLaplacian.h"
#include "brushGrabLaplacian.h"
#include "brushGrabTriScale.h"
#include "dynaBase.h"
#include "dynaPulseAffine.h"
#include "dynaPulseBase.h"
#include "dynaPulseGrab.h"
#include "dynaPushAffine.h"
#include "dynaPushBase.h"
#include "dynaPushGrab.h"
#pragma GCC diagnostic pop
