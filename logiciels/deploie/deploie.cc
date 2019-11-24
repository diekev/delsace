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
#include "json/json.hh"

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
	commande += "html-minifier ";
	commande += "--collapse-whitespace ";
	commande += "--remove-comments ";
	commande += "--minify-css ";
	commande += "--minify-js ";
	/* doit être à la fin pour une raison qui m'échappe */
	commande += "--continue-on-parse-error ";
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

struct DonneesSite {
	struct DonneesFTP {
		dls::chaine hote{};
		dls::chaine mot_de_passe{};
		dls::chaine identifiant{};
		dls::chaine chemin{};
		int port = 0;
	};

	dls::chaine nom{};
	dls::chaine chemin{};
	DonneesFTP ftp{};

	dls::tableau<dls::vue_chaine> erreurs{};

	dls::tableau<std::pair<filesystem::path, filesystem::path>> chemins_fichiers{};

	bool valide()
	{
		if (nom == "") {
			ajoute_erreur("Le nom est vide");
		}

		if (chemin == "") {
			ajoute_erreur("Le chemin est vide");
		}
		else {
			if (!filesystem::exists(chemin.c_str())) {
				ajoute_erreur("Le chemin n'existe pas");
			}
		}

		if (ftp.hote == "") {
			ajoute_erreur("Le chemin de l'hôte est vide");
		}

		if (ftp.mot_de_passe == "") {
			ajoute_erreur("Le mot de passe de l'hôte est vide");
		}

		if (ftp.identifiant == "") {
			ajoute_erreur("L'identifiant de l'hôte est vide");
		}

		if (ftp.chemin == "") {
			ajoute_erreur("Le chemin sur l'hôte est vide");
		}

		if (ftp.port == 0) {
			ajoute_erreur("Le port est égal à zéro");
		}

		return erreurs.est_vide();
	}

	void ajoute_erreur(dls::vue_chaine const &message)
	{
		erreurs.pousse(message);
	}
};

static auto analyse_configuration(const char *chemin)
{
	auto donnees_sites = dls::tableau<DonneesSite>();
	auto obj = json::compile_script(chemin);

	if (obj == nullptr) {
		std::cerr << "La compilation du script a renvoyé un objet nul !\n";
		return donnees_sites;
	}

	if (obj->type != tori::type_objet::DICTIONNAIRE) {
		std::cerr << "La compilation du script n'a pas produit de dictionnaire !\n";
		return donnees_sites;
	}

	auto dico = tori::extrait_dictionnaire(obj.get());

	auto sites = cherche_tableau(dico, "sites");

	if (sites == nullptr) {
		return donnees_sites;
	}

	for (auto obj_site : sites->valeur) {
		donnees_sites.emplace_back();
		auto &donnees_site = donnees_sites.back();

		if (obj_site->type != tori::type_objet::DICTIONNAIRE) {
			std::cerr << "L'objet n'est pas un dictionnaire !\n";
			continue;
		}

		auto site = tori::extrait_dictionnaire(obj_site.get());

		auto nom_site = cherche_chaine(site, "nom");

		if (nom_site == nullptr) {
			continue;
		}

		donnees_site.nom = nom_site->valeur;

		auto chemin_site = cherche_chaine(site, "chemin");

		if (chemin_site == nullptr) {
			continue;
		}

		donnees_site.chemin = chemin_site->valeur;

		auto ftp = cherche_dico(site, "ftp");

		if (ftp == nullptr) {
			continue;
		}

		auto obj_hote = cherche_chaine(ftp, "hôte");
		auto obj_port = cherche_nombre_entier(ftp, "port");
		auto obj_identifiant = cherche_chaine(ftp, "identifiant");
		auto obj_mot_de_passe = cherche_chaine(ftp, "mot_de_passe");
		auto obj_chemin = cherche_chaine(ftp, "chemin");

		if (obj_hote == nullptr) {
			continue;
		}

		if (obj_port == nullptr) {
			continue;
		}

		if (obj_identifiant == nullptr) {
			continue;
		}

		if (obj_mot_de_passe == nullptr) {
			continue;
		}

		if (obj_chemin == nullptr) {
			continue;
		}

		donnees_site.ftp.hote = obj_hote->valeur;
		donnees_site.ftp.port = static_cast<int>(obj_port->valeur);
		donnees_site.ftp.identifiant = obj_identifiant->valeur;
		donnees_site.ftp.mot_de_passe = obj_mot_de_passe->valeur;
		donnees_site.ftp.chemin = obj_chemin->valeur;
	}

	return donnees_sites;
}

static bool besoin_ajournement(
		filesystem::path const &chemin_source,
		filesystem::path const &chemin_cible)
{
	if (!filesystem::exists(chemin_cible)) {
		return true;
	}

	struct stat etat_source;
	struct stat etat_cible;

	stat(chemin_source.c_str(), &etat_source);
	stat(chemin_cible.c_str(), &etat_cible);

	return etat_source.st_mtim.tv_sec > etat_cible.st_mtim.tv_sec;
}

static auto copie_fichiers(DonneesSite &donnees)
{
	auto chemin_dossier = filesystem::path(donnees.chemin.c_str());

	if (!filesystem::is_directory(chemin_dossier)) {
		std::cerr << "Le chemin " << chemin_dossier << " ne pointe pas vers un dossier !\n";
		return false;
	}

	auto nom_dossier = chemin_dossier.stem();
	std::cout << "Déploiement de " << donnees.nom << " (" << nom_dossier << ")\n";

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

		std::cout << '.' << std::flush;

		if (!besoin_ajournement(chemin_source, chemin_cible)) {
			continue;
		}

		if (!filesystem::is_directory(chemin_source)) {
			donnees.chemins_fichiers.pousse({ chemin_cible, ch_relatif });
		}

		//std::cout << "Copie de " << chemin_source << "\n\t-- cible : " << chemin_cible << '\n';

		if (extension == ".html") {
			if (minimise_html(chemin_source, chemin_cible) == 1) {
				return false;
			}
		}
		else if (extension == ".js") {
			if (minimise_js(chemin_source, chemin_cible) == 1) {
				return false;
			}
		}
		else if (extension == ".jsx") {
			if (minimise_jsx(chemin_source, chemin_cible) == 1) {
				return false;
			}
		}
		else if (extension == ".css") {
			if (minimise_css(chemin_source, chemin_cible) == 1) {
				return false;
			}
		}
		else {
			filesystem::copy(chemin_source, chemin_cible, filesystem::copy_options::overwrite_existing);
		}
	}

	std::cout << '\n';

	return true;
}

static auto log_client_ftp(const std::string &chn)
{
	std::cerr << '\n' << __func__ << chn << '\n';
}

static bool envoie_fichiers(DonneesSite &donnees)
{
	if (donnees.chemins_fichiers.est_vide()) {
		return true;
	}

	auto client_ftp = CFTPClient(log_client_ftp);

	auto session_ouverte = client_ftp.InitSession(
				donnees.ftp.hote.c_str(),
				static_cast<unsigned>(donnees.ftp.port),
				donnees.ftp.identifiant.c_str(),
				donnees.ftp.mot_de_passe.c_str(),
				CFTPClient::FTP_PROTOCOL::FTP);

	if (!session_ouverte) {
		std::cerr << "Impossible d'ouvrir une session curl\n";
		return false;
	}

	auto chemin_ftp = filesystem::path(donnees.ftp.chemin.c_str());

	std::cout << "Envoie des fichiers :" << std::endl;

	for (auto const &paire : donnees.chemins_fichiers) {
		auto chemin_cible = chemin_ftp / paire.second;

		std::cout << '.' << std::endl;

		auto ok = client_ftp.UploadFile(paire.first.c_str(), chemin_cible.c_str());

		if (!ok) {
			std::cerr << "Ne peut pas téléverser le fichier " << paire.first << '\n';
			return false;
		}
	}

	client_ftp.CleanupSession();

	return true;
}

static auto deploie_sites(dls::tableau<DonneesSite> &donnees_sites)
{
	for (auto &donnees_site : donnees_sites) {
		if (!donnees_site.valide()) {
			std::cerr << "Impossible de déployer « " << donnees_site.nom << " »\n";

			if (donnees_site.erreurs.taille() == 1) {
				std::cerr << "Une erreur est survenue :\n";
			}
			else {
				std::cerr << "Plusieurs erreurs sont survenues :\n";
			}

			for (auto const &erreur : donnees_site.erreurs) {
				std::cerr << '\t' << erreur << '\n';
			}

			continue;
		}

		if (!copie_fichiers(donnees_site)) {
			std::cerr << "Impossible de traiter les fichier pour « " << donnees_site.nom << " »\n";
			continue;
		}

		if (!envoie_fichiers(donnees_site)) {
			std::cerr << "Impossible d'envoyer les fichiers pour « " << donnees_site.nom << " »\n";
			continue;
		}
	}
}

/**
 * Dépendances :
 * npm install uglify-js html-minifier cssnano
 * sudo apt-get install libcurl4-openssl-dev
 */
int main(int argc, char **argv)
{
	if (argc != 2) {
		std::cerr << "Utilisation : " << argv[0] << " FICHIER\n";
		return 1;
	}

	if (!filesystem::exists(argv[1])) {
		std::cerr << "Le fichier \"" << argv[1] << "\" n'existe pas !\n";
		return 1;
	}

	if (!filesystem::is_regular_file(argv[1])) {
		std::cerr << "\"" << argv[1] << "\" n'est pas un fichier !\n";
		return 1;
	}

	auto donnees_sites = analyse_configuration(argv[1]);

	if (donnees_sites.est_vide()) {
		return 1;
	}

	deploie_sites(donnees_sites);

#if 0


#endif

	return 0;
}
