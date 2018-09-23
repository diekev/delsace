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

#include "rectangle.h"


Rectangle Rectangle::depuis_centre(float x, float y, float largeur, float hauteur)
{
	Rectangle rectangle;
	rectangle.hauteur = hauteur;
	rectangle.largeur = largeur;
	rectangle.x = x - largeur * 0.5f;
	rectangle.y = y - hauteur * 0.5f;

	return rectangle;
}

Rectangle Rectangle::depuis_coord(float x, float y, float largeur, float hauteur)
{
	Rectangle rectangle;
	rectangle.hauteur = hauteur;
	rectangle.largeur = largeur;
	rectangle.x = x;
	rectangle.y = y;

	return rectangle;
}

bool Rectangle::contiens(float pos_x, float pos_y) const
{
	return pos_x > x && pos_y > y && pos_x < (x + largeur) && pos_y < (y + hauteur);
}
