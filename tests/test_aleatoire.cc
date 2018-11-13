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

#include "test_aleatoire.hh"

#include <fstream>
#include <iostream>
#include <random>
#include <sys/wait.h>
#include <unistd.h>

#include "../chrono/outils.hh"

namespace dls {
namespace test_aleatoire {

void Testeuse::ajoute_tests(
		const std::string &nom,
		t_fonction_initialisation initialisation,
		t_fonction_entree_test entree_test)
{
	FonctionsTest foncs;
	foncs.nom = nom;
	foncs.initialisation = initialisation;
	foncs.entree_test = entree_test;

	fonctions.push_back(foncs);
}

int Testeuse::performe_tests(std::ostream &os)
{
	auto chemin = std::string("/tmp/test_");

	std::random_device device{};
	std::uniform_int_distribution<u_char> rng{0, 255};
	std::uniform_int_distribution<size_t> rng_taille{32 * 1024, 64 * 1024};

	u_char tampon[64 * 1024];

	for (const auto &foncs : this->fonctions) {
		os << "Début test : " << foncs.nom << '\n';
		for (auto n = 0; n < 100; ++n) {
			os << '.';

			size_t taille = rng_taille(device);

			if (foncs.initialisation) {
				foncs.initialisation(tampon, taille);
			}
			else {
				for (auto i = 0ul; i < taille; ++i) {
					tampon[i] = rng(device);
				}
			}

			auto pid = fork();

			if (pid == 0) {
				return foncs.entree_test(tampon, taille);
			}
			else if (pid > 0) {
				auto debut = chrono::maintenant();

				while (true) {
					int status;
					pid_t result = waitpid(pid, &status, WNOHANG);

					if (result == 0) {
						/* L'enfant est toujours en vie, continue. */
					}
					else if (result == -1) {
						os << "Erreur lors de l'attente\n";
						break;
					}
					else {
						if (!WIFEXITED(status)) {
							auto chemin_test = chemin + foncs.nom + std::to_string(n) + ".bin";
							std::ofstream of;
							of.open(chemin_test.c_str());
							of.write(reinterpret_cast<const char *>(tampon), static_cast<long>(taille));

							os << "\nÉchec test : écriture du fichier'" << chemin_test << "'...\n";
						}

						break;
					}

					auto temps = chrono::delta(debut);

					if (temps > 25.0) {
						auto chemin_test = chemin + foncs.nom + "_boucle_infini" + std::to_string(n) + ".bin";
						std::ofstream of;
						of.open(chemin_test.c_str());
						of.write(reinterpret_cast<const char *>(tampon), static_cast<long>(taille));

						kill(pid, SIGKILL);

						os << "\nÉchec test : écriture du fichier'" << chemin_test << "'...\n";
						break;
					}
				}
			}
		}

		os << '\n';
	}

	return 0;
}

}  /* namespace test_aleatoire */
}  /* namespace dls */
