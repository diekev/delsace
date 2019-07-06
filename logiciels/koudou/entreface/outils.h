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

class AssembleurControles;
class Persona;
class QLayout;

/**
 * Supprime tous les controles d'un agencement.
 */
void vide_agencement(QLayout *agencement);

/**
 * Crée des controles dans l'assembleur selon les propriétés du Persona.
 */
void cree_controles(AssembleurControles &assembleur, Persona *persona);

/**
 * Ajourne les contrôles de l'assembleur selon les données du Persona.
 */
void ajourne_controles(AssembleurControles &assembleur, Persona *persona);
