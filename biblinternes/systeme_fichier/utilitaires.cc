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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "utilitaires.h"

#include <fstream>

namespace dls {
namespace systeme_fichier {

bool est_bibilotheque(const std::experimental::filesystem::path &p)
{
	return p.extension() == ".so";
}

std::experimental::filesystem::path chemin_repertoire_maison()
{
	const char *dir = getenv("HOME");

	if (dir != nullptr) {
		return dir;
	}

	return {};
}

std::experimental::filesystem::path chemin_repertoire_poubelle()
{
	std::experimental::filesystem::path trash_path;

	const char *xdg_data_home = getenv("XDG_DATA_HOME");

    if (xdg_data_home) {
        trash_path = xdg_data_home;
		trash_path /= "Trash";

		if (std::experimental::filesystem::exists(trash_path)) {
			return trash_path;
		}
    }

	std::experimental::filesystem::path home = chemin_repertoire_maison();

	trash_path = home / ".local/share/Trash";

	if (exists(trash_path)) {
		return trash_path;
	}

	trash_path = home / ".trash";

	if (std::experimental::filesystem::exists(trash_path)) {
		return trash_path;
	}

	return {};
}

void mettre_poubelle(const std::experimental::filesystem::path &chemin)
{
	auto chemin_poubelle = chemin_repertoire_poubelle();

	auto nom_fichier = chemin.filename();

	auto info_fichier_poubelle = chemin_poubelle / "info" / nom_fichier;
	info_fichier_poubelle.replace_extension("trashinfo");

	auto listes_fichiers_poubelle = chemin_poubelle / "files" / nom_fichier;
	int nr = 0;

	while (   std::experimental::filesystem::exists(info_fichier_poubelle)
		   || std::experimental::filesystem::exists(listes_fichiers_poubelle))
	{
		nom_fichier = chemin.filename().string() + "." + std::to_string(++nr);

		info_fichier_poubelle = chemin_poubelle / "info" / nom_fichier;
		info_fichier_poubelle.replace_extension("trashinfo");

		listes_fichiers_poubelle = chemin_poubelle / "files" / nom_fichier;
	}

	/* write info file to be able to restore the image */

	auto info_fichier = std::string{};
	info_fichier += "[Trash Info]\nPath=";
	info_fichier += chemin.string();
	info_fichier += "\nDeletionDate=";

	/* get deletion date and time */
	auto ftemps = std::experimental::filesystem::file_time_type::clock::now();
	auto temps = std::experimental::filesystem::file_time_type::clock::to_time_t(ftemps);
	char tampon[32];
	std::strftime(tampon, sizeof(tampon), "%Y-%m-%dT%H:%M:%S", std::localtime(&temps));

	info_fichier += tampon;
	info_fichier += "\n";

	rename(chemin, listes_fichiers_poubelle);

	std::ofstream infofile(info_fichier_poubelle.c_str());
	infofile << info_fichier;
}

std::experimental::filesystem::path chemin_repertoire_config()
{
	return chemin_repertoire_maison() / ".config";
}

}  /* namespace systeme_fichier */
}  /* namespace dls */
