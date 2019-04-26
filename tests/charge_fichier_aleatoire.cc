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

#include <fstream>
#include <iostream>
#include <vector>

#include "analyseuse_grammaire.h"
#include "contexte_generation_code.h"
#include "decoupeuse.h"
#include "modules.hh"

int main(int argc, char *argv[])
{
	if (argc != 2) {
		std::cerr << "Usage : charge_fichier_aleatoire FICHIER\n";
		return 1;
	}

	std::ifstream fichier(argv[1]);

	fichier.seekg(0, fichier.end);
	auto const taille_fichier = static_cast<size_t>(fichier.tellg());
	fichier.seekg(0, fichier.beg);

	char *donnees = new char[taille_fichier];

	fichier.read(donnees, static_cast<long>(taille_fichier));

	auto donnees_morceaux = reinterpret_cast<const id_morceau *>(donnees);
	auto nombre_morceaux = taille_fichier / sizeof(id_morceau);

	std::vector<DonneesMorceaux> morceaux;
	morceaux.reserve(nombre_morceaux);

	for (size_t i = 0; i < nombre_morceaux; ++i) {
		auto dm = DonneesMorceaux{};
		dm.identifiant = donnees_morceaux[i];
		/* rétabli une chaîne car nous une décharge de la mémoire, donc les
		 * pointeurs sont mauvais. */
		dm.chaine = "texte_test";
		dm.ligne_pos = 0ul;
		dm.module = 0;
		morceaux.push_back(dm);
	}

	std::cerr << "Il y a " << nombre_morceaux << " morceaux.\n";

	try {
		auto contexte = ContexteGenerationCode{};
		auto module = contexte.cree_module("", "");
		module->tampon = lng::tampon_source("texte_test");
		module->morceaux = morceaux;
		auto assembleuse = assembleuse_arbre(contexte);
		contexte.assembleuse = &assembleuse;
		auto analyseuse = analyseuse_grammaire(contexte, module, "");

		std::ostream os(nullptr);
		analyseuse.lance_analyse(os);
	}
	catch (...) {

	}

	delete [] donnees;

	return 0;
}
