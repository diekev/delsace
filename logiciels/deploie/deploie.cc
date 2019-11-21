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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include <iostream>
#include <filesystem>

#include "biblinternes/structures/chaine.hh"

#include "client_ftp.hh"

namespace filesystem = std::filesystem;

static auto chemin_relatif(
		filesystem::path const &chemin,
		filesystem::path const &dossier)
{
	auto chn_dossier = dossier.string();
	return filesystem::path(chemin.string().substr(chn_dossier.size() + (chn_dossier.back() != '/')));
}

static int minimise_js(
		filesystem::path const &chemin_source,
		filesystem::path const &chemin_cible)
{
	auto commande = dls::chaine();
	commande += "uglifyjs --compress --mangle -o ";
	commande += chemin_cible.string();
	commande += " -- ";
	commande += chemin_source.string();

	//std::cout << "\t-- Minimisation de " << chemin_cible << '\n';
	//std::cout << "\t-- commande : " << commande << '\n';

	auto err = system(commande.c_str());

	if (err != 0) {
		std::cerr << "Ne peut pas minimiser le fichier " << chemin_cible << '\n';
		return 1;
	}

	return 0;
}

static int minimise_jsx(
		filesystem::path const &chemin_source,
		filesystem::path const &chemin_cible)
{
	/* À FAIRE : appel babel */
	auto commande = dls::chaine();
	commande += "uglifyjs --compress --mangle -o ";
	commande += chemin_cible.string();
	commande += " -- ";
	commande += chemin_source.string();

	//std::cout << "\t-- Minimisation de " << chemin_cible << '\n';
	//std::cout << "\t-- commande : " << commande << '\n';

	auto err = system(commande.c_str());

	if (err != 0) {
		std::cerr << "Ne peut pas minimiser le fichier " << chemin_cible << '\n';
		return 1;
	}

	return 0;
}

static int minimise_css(
		filesystem::path const &chemin_source,
		filesystem::path const &chemin_cible)
{
	auto commande = dls::chaine();
	commande += "cssnano ";
	commande += chemin_source.string();
	commande += chemin_cible.string();

	//std::cout << "\t-- Minimisation de " << chemin_cible << '\n';
	//std::cout << "\t-- commande : " << commande << '\n';

	auto err = system(commande.c_str());

	if (err != 0) {
		std::cerr << "Ne peut pas minimiser le fichier " << chemin_cible << '\n';
		return 1;
	}

	return 0;
}

static int minimise_html(
		filesystem::path const &chemin_source,
		filesystem::path const &chemin_cible)
{
	auto commande = dls::chaine();
	commande += "html-minifier --collapse-whitespace ";
	commande += chemin_source.string();
	commande += " -o  ";
	commande += chemin_cible.string();

	//std::cout << "\t-- Minimisation de " << chemin_cible << '\n';
	//std::cout << "\t-- commande : " << commande << '\n';

	auto err = system(commande.c_str());

	if (err != 0) {
		std::cerr << "Ne peut pas minimiser le fichier " << chemin_cible << '\n';
		return 1;
	}

	return 0;
}

/**
 * Dépendances :
 * npm install uglify-js html-minifier cssnano
 * sudo apt-get install libcurl4-openssl-dev
 */
int main(int argc, char **argv)
{
#if 1
	if (argc != 2) {
		std::cerr << "Utilisation : " << argv[0] << " DOSSIER\n";
		return 1;
	}

	auto chemin_dossier = filesystem::path(argv[1]);

	if (!filesystem::is_directory(chemin_dossier)) {
		std::cerr << "Le chemin " << chemin_dossier << " ne pointe pas vers un dossier !\n";
		return 1;
	}

	auto nom_dossier = chemin_dossier.stem();
	std::cout << "Déploiement de " << nom_dossier << '\n';

	auto dossier_cible = filesystem::path("/home/kevin/deploie/") / nom_dossier;

	if (!filesystem::exists(dossier_cible)) {
		filesystem::create_directories(dossier_cible);
		std::cout << "Création du dossier cible : " << dossier_cible << '\n';
	}

	std::cout << "Traitement des fichiers :" << std::endl;

	for (auto const &noeud : filesystem::recursive_directory_iterator(chemin_dossier)) {
		auto chemin_source = noeud.path();
		auto extension = chemin_source.extension();

		if (extension == ".pyc") {
			continue;
		}

		auto ch_relatif = chemin_relatif(chemin_source, chemin_dossier);
		auto chemin_cible = (dossier_cible / ch_relatif);

		filesystem::create_directories(chemin_cible.parent_path());

		//std::cout << "Copie de " << chemin_source << "\n\t-- cible : " << chemin_cible << '\n';

		if (extension == ".html") {
			if (minimise_html(chemin_source, chemin_cible) == 1) {
				return 1;
			}
		}
		else if (extension == ".js") {
			if (minimise_js(chemin_source, chemin_cible) == 1) {
				return 1;
			}
		}
		else if (extension == ".jsx") {
			if (minimise_jsx(chemin_source, chemin_cible) == 1) {
				return 1;
			}
		}
		else if (extension == ".css") {
			if (minimise_css(chemin_source, chemin_cible) == 1) {
				return 1;
			}
		}
		else {
			filesystem::copy(chemin_source, chemin_cible, filesystem::copy_options::overwrite_existing);
		}

		std::cout << '.' << std::flush;
	}

	std::cout << '\n';
#else
	auto client_ftp = CFTPClient(nullptr);

	auto session_ouverte = client_ftp.InitSession(
				"",
				0,
				"",
				"",
				CFTPClient::FTP_PROTOCOL::FTP,
				CFTPClient::NO_FLAGS);

	if (!session_ouverte) {
		std::cerr << "Impossible d'ouvrir une session curl\n";
		return 1;
	}

	auto liste = std::string();
	auto res = client_ftp.List("www/www", liste);

	if (!res) {
		std::cerr << "Impossible d'obtenir la liste des fichiers\n";
		return 1;
	}

	std::cerr << "Liste :\n" << liste << '\n';

#endif

	return 0;
}
