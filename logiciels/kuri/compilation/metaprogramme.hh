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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

struct DonneesExecution;
struct NoeudBloc;
struct NoeudDeclarationEnteteFonction;
struct NoeudDirectiveExecution;
struct NoeudDirectiveExecution;
struct NoeudStruct;
struct UniteCompilation;

struct MetaProgramme {
	enum class ResultatExecution : int {
		NON_INITIALISE,
		ERREUR,
		SUCCES,
	};

	/* non-nul pour les directives d'exécutions (exécute, corps texte, etc.) */
	NoeudDirectiveExecution *directive = nullptr;

	/* non-nuls pour les corps-textes */
	NoeudBloc *corps_texte = nullptr;
	NoeudDeclarationEnteteFonction *corps_texte_pour_fonction = nullptr;
	NoeudStruct *corps_texte_pour_structure = nullptr;

	/* la fonction qui sera exécutée */
	NoeudDeclarationEnteteFonction *fonction = nullptr;

	UniteCompilation *unite = nullptr;

	bool fut_execute = false;

	ResultatExecution resultat{};

	DonneesExecution *donnees_execution = nullptr;
};
