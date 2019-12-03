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

#ifdef DEBOGUE_EXPRESSION
static constexpr auto g_log_expression = true;
#else
static constexpr auto g_log_expression = false;
#endif

#define DEB_LOG_EXPRESSION if (g_log_expression) { std::cerr
#define FIN_LOG_EXPRESSION '\n';}

/**
 * Limitation du nombre récursif de sous-expressions (par exemple :
 * f(g(h(i(j()))))).
 */
static constexpr auto PROFONDEUR_EXPRESSION_MAX = 32;

/* Tabulations utilisées au début des logs. */
static const char *tabulations[PROFONDEUR_EXPRESSION_MAX] = {
	"",
	" ",
	"  ",
	"   ",
	"    ",
	"     ",
	"      ",
	"       ",
	"        ",
	"         ",
	"          ",
	"           ",
	"            ",
	"             ",
	"              ",
	"               ",
	"                ",
	"                 ",
	"                  ",
	"                   ",
	"                    ",
	"                     ",
	"                      ",
	"                       ",
	"                        ",
	"                         ",
	"                          ",
	"                           ",
	"                            ",
	"                             ",
	"                              ",
	"                               ",
};
