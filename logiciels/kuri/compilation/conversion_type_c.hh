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

#include "biblinternes/structures/vue_chaine.hh"

#include "donnees_type.h"

void converti_type_C(
		ContexteGenerationCode &contexte,
		dls::vue_chaine const &nom_variable,
		type_plage_donnees_type donnees,
		dls::flux_chaine &os,
		bool echappe = false,
		bool echappe_struct = false,
		bool echappe_tableau_fixe = false);

void cree_typedef(
		ContexteGenerationCode &contexte,
		DonneesTypeFinal &donnees,
		dls::flux_chaine &os);
