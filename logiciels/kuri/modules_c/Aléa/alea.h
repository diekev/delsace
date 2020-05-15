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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GNA GNA;

GNA *ALEA_cree_gna(unsigned graine);
void ALEA_reseme_gna(GNA *gna, unsigned graine);
void ALEA_detruit_gna(GNA *gna);

float ALEA_uniforme_r32(GNA *gna, float min, float max);
double ALEA_uniforme_r64(GNA *gna, double min, double max);

float ALEA_normale_r32(GNA *gna, float moyenne, float ecart);
double ALEA_normale_r64(GNA *gna, double moyenne, double ecart);

#ifdef __cplusplus
}
#endif
