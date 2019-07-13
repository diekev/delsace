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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/structures/chaine.hh"

class ProprieteEnumerante;
class Propriete;
class AssembleurControles;
class QString;

/**
 * @brief int_param Add a UI parameter for an integer property.
 *
 * @param assembleur   The callback used to create the parameter.
 * @param prop The pointer to the property.
 */
void controle_int(AssembleurControles &assembleur, Propriete *prop);

/**
 * @brief float_param Add a UI parameter for a float property.
 *
 * @param assembleur   The callback used to create the parameter.
 * @param prop The pointer to the property.
 */
void controle_float(AssembleurControles &assembleur, Propriete *prop);

/**
 * @brief enum_param Add a UI parameter for an enumeration property.
 *
 * @param assembleur   The callback used to create the parameter.
 * @param prop The pointer to the property.
 */
void controle_enum(AssembleurControles &assembleur, Propriete *prop);

/**
 * @brief enum_param Add a UI parameter for an enumeration property.
 *
 * @param assembleur   The callback used to create the parameter.
 * @param prop The pointer to the property.
 */
void bitfield_param(AssembleurControles &assembleur, Propriete *prop);

/**
 * @brief string_param Add a UI parameter for a string property.
 *
 * @param assembleur   The callback used to create the parameter.
 * @param prop The pointer to the property.
 */
void controle_chaine_caractere(AssembleurControles &assembleur, Propriete *prop);

/**
 * @brief bool_param Add a UI parameter for a boolean property.
 *
 * @param assembleur   The callback used to create the parameter.
 * @param prop The pointer to the property.
 */
void controle_bool(AssembleurControles &assembleur, Propriete *prop);

/**
 * @brief xyz_param Add a UI parameter for a vector property.
 *
 * @param assembleur   The callback used to create the parameter.
 * @param prop The pointer to the property.
 */
void controle_vec3(AssembleurControles &assembleur, Propriete *prop);

/**
 * @brief color_param Add a UI parameter for a color property.
 *
 * @param assembleur   The callback used to create the parameter.
 * @param prop The pointer to the property.
 */
void controle_couleur(AssembleurControles &assembleur, Propriete *prop);

/**
 * @brief file_param Add a UI parameter for displaying a file selector.
 *
 * @param assembleur   The callback used to create the parameter.
 * @param prop The pointer to the property.
 */
void file_param(AssembleurControles &assembleur, Propriete *prop);

/**
 * @brief file_param Add a UI parameter for displaying a file selector.
 *
 * @param assembleur   The callback used to create the parameter.
 * @param prop The pointer to the property.
 */
void controle_fichier_entree(AssembleurControles &assembleur, Propriete *prop);

/**
 * @brief file_param Add a UI parameter for displaying a file selector.
 *
 * @param assembleur   The callback used to create the parameter.
 * @param prop The pointer to the property.
 */
void controle_fichier_sortie(AssembleurControles &assembleur, Propriete *prop);

/**
 * @brief param_tooltip Set the tooltip for the last added parameter.
 *
 * @param assembleur The callback used to create the parameter.
 * @param tooltip The parameters tooltip.
 */
void infobulle_controle(AssembleurControles &assembleur, const char *tooltip);

/**
 * @brief list_selection_param Add a UI parameter for displaying a list selector.
 *
 * @param assembleur   The callback used to create the parameter.
 * @param prop The pointer to the property.
 */
void controle_liste(AssembleurControles &assembleur, Propriete *prop);
