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

#include "structures/chaine.hh"

// Ces structures doivent être tenues synchronisées avec celles dans compilateur.kuri
enum class TypeCoulisse : int {
	C,
	LLVM,
	ASM,
};

enum class ArchitectureCible : int {
	X64,
	X86,
};

enum class NiveauOptimisation : int {
	AUCUN,
	O0,
	O1,
	O2,
	Os,
	Oz,
	O3,
};

enum class OptionsLangage : unsigned int {
	ACTIVE_INTROSPECTION = (1 << 0),
	ACTIVE_COROUTINE     = (1 << 1),

	TOUT = (ACTIVE_COROUTINE | ACTIVE_INTROSPECTION)
};

enum class ObjetGenere : int {
	Executable,
	FichierObjet,
	Rien,
};

struct OptionsCompilation {
	kuri::chaine nom_sortie = kuri::chaine("a.out");
	TypeCoulisse type_coulisse = TypeCoulisse::C;
	NiveauOptimisation niveau_optimisation = NiveauOptimisation::AUCUN;
	ArchitectureCible architecture_cible = ArchitectureCible::X64;
	OptionsLangage options_langage = OptionsLangage::TOUT;
	ObjetGenere objet_genere = ObjetGenere::Executable;

	bool emets_metriques = true;
};
