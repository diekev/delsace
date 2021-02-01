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

#include "environnement.hh"

#include "options.hh"

/* À FAIRE(r16) : il faudra proprement gérer les architectures pour les r16, ou trouver des algorithmes pour supprimer les tables */
void precompile_objet_r16(const std::filesystem::path &chemin_racine_kuri)
{
	// objet pour la liaison statique de la bibliothèque
	{
		auto chemin_fichier = chemin_racine_kuri / "fichiers/r16_tables.cc";
		auto chemin_objet = "/tmp/r16_tables_x64.o";

		if (!std::filesystem::exists(chemin_objet)) {
			auto commande = dls::chaine("/usr/bin/g++-9 -c ");
			commande += chemin_fichier.c_str();
			commande += " -o ";
			commande += chemin_objet;

			std::cout << "Compilation des tables de conversion R16...\n";
			std::cout << "Exécution de la commande " << commande << std::endl;

			auto err = system(commande.c_str());

			if (err != 0) {
				std::cerr << "Impossible de compiler les tables de conversion R16 !\n";
				return;
			}

			std::cout << "Compilation du fichier statique réussie !" << std::endl;
		}
	}

	// objet pour la liaison dynamique de la bibliothèque, pour les métaprogrammes
	{
		auto chemin_fichier = chemin_racine_kuri / "fichiers/r16_tables.cc";
		auto chemin_objet = "/tmp/r16_tables_x64.so";

		if (!std::filesystem::exists(chemin_objet)) {
			auto commande = dls::chaine("/usr/bin/g++-9 -shared -fPIC ");
			commande += chemin_fichier.c_str();
			commande += " -o ";
			commande += chemin_objet;

			std::cout << "Compilation des tables de conversion R16...\n";
			std::cout << "Exécution de la commande " << commande << std::endl;

			auto err = system(commande.c_str());

			if (err != 0) {
				std::cerr << "Impossible de compiler les tables de conversion R16 !\n";
				return;
			}

			std::cout << "Compilation du fichier dynamique réussie !" << std::endl;
		}
	}
}

void compile_objet_r16(const std::filesystem::path &chemin_racine_kuri, ArchitectureCible architecture_cible)
{
	if (architecture_cible == ArchitectureCible::X64) {
		// nous devrions déjà l'avoir
		return;
	}

	{
		auto chemin_fichier = chemin_racine_kuri / "fichiers/r16_tables.cc";
		auto chemin_objet = "/tmp/r16_tables_x86.o";

		if (!std::filesystem::exists(chemin_objet)) {
			auto commande = dls::chaine("/usr/bin/g++-9 -c -m32 ");
			commande += chemin_fichier.c_str();
			commande += " -o ";
			commande += chemin_objet;

			std::cout << "Compilation des tables de conversion R16...\n";
			std::cout << "Exécution de la commande " << commande << std::endl;

			auto err = system(commande.c_str());

			if (err != 0) {
				std::cerr << "Impossible de compiler les tables de conversion R16 !\n";
				return;
			}

			std::cout << "Compilation du fichier statique réussie !" << std::endl;
		}
	}
}
