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

enum class NiveauOptimisation : char {
	Aucun,
	O0,
	O1,
	O2,
	Os,
	Oz,
	O3,
};

struct OptionsCompilation {
	const char *chemin_fichier = nullptr;
	const char *chemin_sortie = "a.out";
	bool emet_fichier_objet = true;
	bool emet_code_intermediaire = false;
	bool emet_arbre = false;
	bool imprime_taille_memoire_objet = false;
	bool imprime_temps = false;
	bool imprime_version = false;
	bool imprime_aide = false;
	bool erreur = false;
	bool bit32 = false;

	NiveauOptimisation optimisation = NiveauOptimisation::Aucun;
	char pad[6];
};

OptionsCompilation genere_options_compilation(int argc, char **argv);
