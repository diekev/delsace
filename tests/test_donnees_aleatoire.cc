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

#include <iostream>
#include <random>

#include <llvm/IR/Module.h>

#include "analyseuse_grammaire.h"
#include "contexte_generation_code.h"
#include "decoupeuse.h"

static int test_entree_aleatoire(const u_char *donnees, int taille)
{
	try {
		auto donnees_char = reinterpret_cast<const char *>(donnees);

		std::string texte;
		texte.reserve(static_cast<size_t>(taille) + 1);

		for (auto i = 0; i < taille; ++i) {
			texte.push_back(donnees_char[i]);
		}

		auto tampon = TamponSource(texte);

		decoupeuse_texte decoupeuse(tampon);
		decoupeuse.genere_morceaux();

		auto contexte = ContexteGenerationCode{tampon};
		auto assembleuse = assembleuse_arbre();
		auto analyseuse = analyseuse_grammaire(contexte, decoupeuse.morceaux(), tampon, &assembleuse);
		analyseuse.lance_analyse();
	}
	catch (...) {

	}

	return 0;
}

int main()
{
	std::random_device device{};
	std::uniform_int_distribution<u_char> rng{0, 255};

	std::uniform_int_distribution<int> rng_taille{0, 20 * 1024};

	for (auto n = 0; n < 1000; ++n) {
		const auto taille = rng_taille(device);

		std::vector<u_char> tampon(static_cast<size_t>(taille));

		for (auto i = 0; i < taille; ++i) {
			tampon[static_cast<size_t>(i)] = rng(device);
		}

		std::cerr << "Lancement du test aléatoire pour " << taille << " caractères.\n";

		test_entree_aleatoire(tampon.data(), taille);
	}

	return 0;
}
