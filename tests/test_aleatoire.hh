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

#pragma once

#include <functional>
#include <string>

namespace dls {
namespace test_aleatoire {

using t_fonction_initialisation = std::function<void(u_char *, size_t)>;
using t_fonction_entree_test = std::function<int(const u_char *, size_t)>;

/**
 * Classe aidant à performer des tests aléatoires.
 *
 * Des données aléatoires sont fournies à des fonctions, et si le programme
 * crash, les données sont écrit dans un fichier et un message d'erreur est
 * imprimé dans un flux spécifié.
 */
class Testeuse {
	struct FonctionsTest {
		std::string nom{};
		t_fonction_initialisation initialisation = nullptr;
		t_fonction_entree_test entree_test = nullptr;
	};

	std::vector<FonctionsTest> fonctions{};

public:
	/**
	 * Ajoute les fonctions spécifiés à la liste des fonctions à tester.
	 *
	 * La première fonction est une fonction qui peuple un tampon avec des
	 * données aléatoires, la deuxième, une qui teste lesdites données.
	 *
	 * La première fonction peut-être nulle, auquel cas des données aléatoires
	 * générées automatiquement seront fournies à la deuxième.
	 *
	 * Le nom passé en premier paramètre sera utilisé pour définir le nom de
	 * fichier où écrire les données si un test échou.
	 */
	void ajoute_tests(
			const std::string &nom,
			t_fonction_initialisation initialisation,
			t_fonction_entree_test entree_test);

	/**
	 * Performe les tests ajoutés, et imprime les messages d'erreur dans le flux
	 * spécifié.
	 */
	int performe_tests(std::ostream &os);
};

}  /* namespace test_aleatoire */
}  /* namespace dls */
