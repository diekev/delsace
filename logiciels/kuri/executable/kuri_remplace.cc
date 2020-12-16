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

#include <filesystem>
#include <fstream>
#include <iostream>

#include "biblinternes/json/json.hh"

#include "compilation/compilatrice.hh"
#include "compilation/lexeuse.hh"
#include "compilation/erreur.h"
#include "compilation/outils_lexemes.hh"
#include "compilation/modules.hh"

#include "options.hh"

/* petit outil pour remplacer des mot-clés dans les scripts préexistants afin de
 * faciliter l'évolution du langage
 * nous pourrions également avoir un sorte de fichier de configuration pour
 * définir comment changer les choses, ou formater le code
 */

struct Configuration {
	dls::tableau<dls::chaine> dossiers{};
	dls::tableau<dls::chaine> fichiers{};

	using paire = std::pair<dls::chaine, dls::chaine>;

	dls::tableau<paire> mots_cles{};
};

static void analyse_liste_chemin(
		tori::ObjetTableau *tableau,
		dls::tableau<dls::chaine> &chaines)
{
	for (auto objet : tableau->valeur) {
		if (objet->type != tori::type_objet::CHAINE) {
			std::cerr << "liste : l'objet n'est pas une chaine !\n";
			continue;
		}

		auto obj_chaine = extrait_chaine(objet.get());
		chaines.pousse(obj_chaine->valeur);
	}
}

static Configuration analyse_configuration(const char *chemin)
{
	auto config = Configuration{};
	auto obj = json::compile_script(chemin);

	if (obj == nullptr) {
		std::cerr << "La compilation du script a renvoyé un objet nul !\n";
		return config;
	}

	if (obj->type != tori::type_objet::DICTIONNAIRE) {
		std::cerr << "La compilation du script n'a pas produit de dictionnaire !\n";
		return config;
	}

	auto dico = tori::extrait_dictionnaire(obj.get());

	auto obj_fichiers = cherche_tableau(dico, "fichiers");

	if (obj_fichiers != nullptr) {
		analyse_liste_chemin(obj_fichiers, config.fichiers);
	}

	auto obj_dossiers = cherche_tableau(dico, "dossiers");

	if (obj_dossiers != nullptr) {
		analyse_liste_chemin(obj_dossiers, config.dossiers);
	}

	auto obj_change = cherche_dico(dico, "change");

	if (obj_change != nullptr) {
		auto obj_mots_cles = cherche_dico(obj_change, "mots-clés");

		if (obj_mots_cles == nullptr) {
			return config;
		}

		for (auto objet : obj_mots_cles->valeur) {
			auto const &nom_objet = objet.first;

			if (objet.second->type != tori::type_objet::CHAINE) {
				std::cerr << "mots-clés : la valeur l'objet '" << nom_objet << "' n'est pas une chaine !\n";
				continue;
			}

			auto obj_chaine = extrait_chaine(objet.second.get());
			config.mots_cles.pousse({ nom_objet, obj_chaine->valeur });

			//std::cerr << "Remplacement de '" << nom_objet << "' par '" << obj_chaine->valeur << "'\n";
		}
	}

	return config;
}

static void reecris_fichier(
		std::filesystem::path &chemin,
		Configuration const &config)
{
	std::cerr << "Réécriture du fichier " << chemin << "\n";

	try {
		if (chemin.is_relative()) {
			chemin = std::filesystem::absolute(chemin);
		}

		auto compilatrice = Compilatrice{};
		auto espace = compilatrice.demarre_un_espace_de_travail({}, "");
		auto donnees_fichier = compilatrice.sys_module->cree_fichier("", "");
		auto tampon = charge_fichier(chemin.c_str(), *espace, {});
		donnees_fichier->charge_tampon(lng::tampon_source(std::move(tampon)));

		auto lexeuse = Lexeuse(compilatrice, donnees_fichier, INCLUS_CARACTERES_BLANC | INCLUS_COMMENTAIRES);
		lexeuse.performe_lexage();

		auto os = std::ofstream(chemin);

		for (auto const &lexeme : donnees_fichier->lexemes) {
			if (!est_mot_cle(lexeme.genre)) {
				os << lexeme.chaine;
				continue;
			}

			auto trouve = false;

			for (auto const &paire : config.mots_cles) {
				if (paire.first != dls::chaine(lexeme.chaine)) {
					continue;
				}

				os << paire.second;
				trouve = true;

				break;
			}

			if (!trouve) {
				os << lexeme.chaine;
			}
		}
	}
	catch (const erreur::frappe &erreur_frappe) {
		std::cerr << erreur_frappe.message() << '\n';
	}
}

int main(int argc, char **argv)
{
	std::ios::sync_with_stdio(false);

	if (argc < 2) {
		std::cerr << "Utilisation " << argv[0] << " CONFIG.json\n";
		return 1;
	}

	auto config = analyse_configuration(argv[1]);

	if (config.dossiers.est_vide() && config.fichiers.est_vide()) {
		std::cerr << "Aucun fichier ni dossier précisé dans le fichier de configuration !\n";
		return 1;
	}

	if (config.mots_cles.est_vide()) {
		std::cerr << "Aucun mot-clé précisé !\n";
		return 1;
	}

	auto const &chemin_racine_kuri = getenv("RACINE_KURI");

	if (chemin_racine_kuri == nullptr) {
		std::cerr << "Impossible de trouver le chemin racine de l'installation de kuri !\n";
		std::cerr << "Possible solution : veuillez faire en sorte que la variable d'environnement 'RACINE_KURI' soit définie !\n";
		return 1;
	}

	for (auto const &chemin_dossier : config.dossiers) {
		auto chemin = std::filesystem::path(chemin_dossier.c_str());

		if (!std::filesystem::exists(chemin)) {
			std::cerr << "Le chemin " << chemin << " ne pointe vers rien !\n";
			continue;
		}

		if (!std::filesystem::is_directory(chemin)) {
			std::cerr << "Le chemin " << chemin << " ne pointe pas vers un dossier !\n";
			continue;
		}

		for (auto donnees : std::filesystem::recursive_directory_iterator(chemin)) {
			chemin = donnees.path();

			if (chemin.extension() != ".kuri") {
				continue;
			}

			reecris_fichier(chemin, config);
		}
	}

	for (auto const &chemin_fichier : config.fichiers) {
		auto chemin = std::filesystem::path(chemin_fichier.c_str());

		if (!std::filesystem::exists(chemin)) {
			std::cerr << "Le chemin " << chemin << " ne pointe vers rien !\n";
			continue;
		}

		if (!std::filesystem::is_regular_file(chemin)) {
			std::cerr << "Le chemin " << chemin << " ne pointe pas vers un fichier !\n";
			continue;
		}

		if (chemin.extension() != ".kuri") {
			std::cerr << "Le chemin " << chemin << " ne pointe pas vers un fichier kuri !\n";
			continue;
		}

		reecris_fichier(chemin, config);
	}

	return 0;
}
