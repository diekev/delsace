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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "brush.h"

Brush::Brush()
    : Brush(0.0f, 0.0f)
{}

Brush::Brush(const float radius, const float strength)
    : m_radius(radius)
    , m_inv_radius(1.0f / m_radius)
    , m_strength(strength)
    , m_mode(BRUSH_MODE_ADD)
    , m_tool(BRUSH_TOOL_DRAW)
{}

void Brush::radius(const float rad)
{
	m_radius = rad;
	m_inv_radius = 1.0f / m_radius;
}

float Brush::radius() const
{
	return m_radius;
}

void Brush::strength(const float s)
{
	m_strength = s;
}

float Brush::strength() const
{
	if (m_tool == BRUSH_TOOL_SMOOTH) {
		return -m_strength * 0.1f;
	}

	return ((m_mode == BRUSH_MODE_ADD) ? m_strength : -m_strength);
}

void Brush::mode(const int mode)
{
	m_mode = mode;
}

int Brush::tool() const
{
	return m_tool;
}

void Brush::tool(const int tool)
{
	m_tool = tool;
}
