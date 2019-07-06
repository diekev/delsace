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

#include <iostream>

#include <delsace/math/vecteur.hh>

enum class evenement_fenetre : short {
	NUL,
	SOURIS_BOUGEE,
	SOURIS_PRESSEE,
	SOURIS_RELACHEE,
	SOURIS_ROULETTE,
	CLE_PRESSEE,
	CLE_RELACHEE,
	CLE_REPETEE,
	DOUBLE_CLIC,
	REDIMENSION,
};

enum class type_cle : short {
	INCONNU       = -1,
	ESPACE        = 32,
	APOSTROPHE    = 39,  /* ' */
	VIRGULE       = 44,  /* , */
	MOINS         = 45,  /* - */
	POINT         = 46,  /* . */
	SLASH         = 47,  /* / */
	n0            = 48,
	n1            = 49,
	n2            = 50,
	n3            = 51,
	n4            = 52,
	n5            = 53,
	n6            = 54,
	n7            = 55,
	n8            = 56,
	n9            = 57,
	POINT_VIRGULE = 59,  /* ; */
	EGAL          = 61,  /* = */
	A             = 65,
	B             = 66,
	C             = 67,
	D             = 68,
	E             = 69,
	F             = 70,
	G             = 71,
	H             = 72,
	I             = 73,
	J             = 74,
	K             = 75,
	L             = 76,
	M             = 77,
	N             = 78,
	O             = 79,
	P             = 80,
	Q             = 81,
	R             = 82,
	S             = 83,
	T             = 84,
	U             = 85,
	V             = 86,
	W             = 87,
	X             = 88,
	Y             = 89,
	Z             = 90,
	LEFT_BRACKET  = 91,  /* [ */
	BACKSLASH     = 92,  /* \ */
	RIGHT_BRACKET = 93,  /* ] */
	ACCENT_GRAVE  = 96,  /* ` */
	WORLD_1       = 161, /* non-US #1 */
	WORLD_2       = 162, /* non-US #2 */

	/* Function keys */
	ESCAPE        = 256,
	ENTREE        = 257,
	TAB           = 258,
	RETOUR        = 259,
	INSERT        = 260,
	DELETE        = 261,
	RIGHT         = 262,
	LEFT          = 263,
	DOWN          = 264,
	UP            = 265,
	PAGE_UP       = 266,
	PAGE_DOWN     = 267,
	HOME          = 268,
	END           = 269,
	CAPS_LOCK     = 280,
	SCROLL_LOCK   = 281,
	NUM_LOCK      = 282,
	PRINT_SCREEN  = 283,
	PAUSE         = 284,
	F1            = 290,
	F2            = 291,
	F3            = 292,
	F4            = 293,
	F5            = 294,
	F6            = 295,
	F7            = 296,
	F8            = 297,
	F9            = 298,
	F10           = 299,
	F11           = 300,
	F12           = 301,
	F13           = 302,
	F14           = 303,
	F15           = 304,
	F16           = 305,
	F17           = 306,
	F18           = 307,
	F19           = 308,
	F20           = 309,
	F21           = 310,
	F22           = 311,
	F23           = 312,
	F24           = 313,
	F25           = 314,
	KP_0          = 320,
	KP_1          = 321,
	KP_2          = 322,
	KP_3          = 323,
	KP_4          = 324,
	KP_5          = 325,
	KP_6          = 326,
	KP_7          = 327,
	KP_8          = 328,
	KP_9          = 329,
	KP_DECIMAL    = 330,
	KP_DIVIDE     = 331,
	KP_MULTIPLY   = 332,
	KP_SUBTRACT   = 333,
	KP_ADD        = 334,
	KP_ENTER      = 335,
	KP_EQUAL      = 336,
	LEFT_SHIFT    = 340,
	LEFT_CONTROL  = 341,
	LEFT_ALT      = 342,
	LEFT_SUPER    = 343,
	RIGHT_SHIFT   = 344,
	RIGHT_CONTROL = 345,
	RIGHT_ALT     = 346,
	RIGHT_SUPER   = 347,
	MENU          = 348,
};

enum class bouton_souris : short {
	_1      = 0,
	_2      = 1,
	_3      = 2,
	_4      = 3,
	_5      = 4,
	_6      = 5,
	_7      = 6,
	_8      = 7,
	DERNIER = _8,
	GAUCHE  = _1,
	DROITE  = _2,
	MILIEU  = _3,
};

enum class type_mod : short {
	AUCUN   = 0x0000,
	SHIFT   = 0x0001,
	CONTROL = 0x0002,
	ALT     = 0x0004,
	SUPER   = 0x0008,
};

struct Evenement {
	evenement_fenetre type = evenement_fenetre::NUL;
	type_mod mods = type_mod::AUCUN;
	type_cle cle = type_cle::INCONNU;
	bouton_souris souris = bouton_souris::_1;

	/* position de la souris */
	dls::math::vec2d pos{};

	/* delta de la roulette */
	dls::math::vec2d delta{};
};

std::ostream &operator<<(std::ostream &os, Evenement const &evenement);
