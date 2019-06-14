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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include <filesystem>
#include <iostream>

#include "../chrono/chronometre_de_portee.hh"
#include "../../biblexternes/docopt/docopt.hh"

#include "definitions.h"

OUVRE_GARDE_INCLUSION_OPENCV
#	include <opencv2/opencv.hpp>
#	include <opencv2/objdetect.hpp>
FERME_GARDE_INCLUSION_OPENCV

#include "auteur_video.h"
#include "lecteur_video.h"

namespace std_filesystem = std::experimental::filesystem;

static const char utilisation[] = R"(
OpenCV.

Usage:
	opencv --hote=<hotebd> --usager=<usagerbd> --motpasse=<mdpbd> --schema=<schemabd> [--chemin_image=<chmimg> --chemin_cascade=<chccsd>]
	opencv (-h | --help)
	opencv --version

Options:
	-h, --help                 Montre cette écran.
	--version                  Montre la version.
	--hote=<hotebd>            Nom de l'hôte de la base de données.
	--usager=<usagerbd>        Utilisateur de la base de données.
	--motpasse=<mdpbd>         Mot de passe de la base de données.
	--schema=<schemabd>        Schéma de la base de données.
	--chemin_image=<chmimg>    Chemin vers l'image à traiter.
	--chemin_cascade=<chccsd>  Chemin vers la cascade de Haar à utiliser.
)";

cv::Rect trouve_cadre_plus_proche(const std::vector<cv::Rect> &cadres, const cv::Rect &cadre)
{
	const auto dist_min = std::numeric_limits<int>::max();
	cv::Rect min;

	for (const cv::Rect &c : cadres) {
		const auto dist = c.tl().dot(cadre.tl());

		if (dist < dist_min) {
			min = c;
		}
	}

	return min;
}

#if 1

#include "enveloppe_sql.h"
#include "objets_communs.h"
#include "reconnaissant_facial.h"

#include <cppconn/exception.h>

static void entraine_modele(
		ObjetsCommuns &objets_communs,
		std::ostream &os = std::cerr)
{
	std::vector<cv::Mat> visages;
	std::vector<int> etiquettes;

	{
		CHRONOMETRE_PORTEE("Chargement des images.", os);

		/* Requiert la liste des fichiers vidéos à partir desquelles les visages
		 * sont extraits. */

		auto requete = "SELECT chemin, etiquette FROM videos;";

		DeclarationSQL declaration(objets_communs.connection());
		ResultatsSQL res = declaration.requiert(requete);

		cv::Mat image;
		cv::Mat luminance;

		while (res.suivant()) {
			const auto etiquette = std::stoi(res.valeur("etiquette"));
			const auto chemin = res.valeur("chemin");

			os << "Traitement de la vidéo : '" << chemin << "'\n";

			lecteur_video lecteur(chemin);

			const auto nombre_images_max = 100;
			const auto nombre_images_video = lecteur.nombre_image();
			auto nombre_images_lues = 0;

			lecteur.delta(nombre_images_video / nombre_images_max);

			visages.reserve(visages.size() + nombre_images_video);
			etiquettes.reserve(etiquettes.size() + nombre_images_video);

			while (lecteur.image_suivante(image) && nombre_images_lues < nombre_images_max) {
				cv::cvtColor(image, luminance, cv::COLOR_RGB2GRAY);

				visages.push_back(luminance);
				etiquettes.push_back(etiquette);
				++nombre_images_lues;
			}
		}
	}

	if (visages.empty() | etiquettes.empty()) {
		return;
	}

	reconnaissant_facial reconnaissant;

	{
		CHRONOMETRE_PORTEE("Entrainement du modèle.", os);

		/* Entrainement du modèle. */
		reconnaissant.entraine(visages, etiquettes);
	}

	{
		CHRONOMETRE_PORTEE("Sauvegarde du modèle.", os);

		/* Entrainement du modèle. */
		reconnaissant.sauvegarde(objets_communs.chemin_racine() / "modèles/visages.xml");
	}
}

cv::Mat extrait_visage_image(
			const std_filesystem::path &chemin_image,
			const std_filesystem::path &chemin_cascade)
{
	auto face_cascade = cv::CascadeClassifier(chemin_cascade.string());

	auto image = cv::imread(chemin_image.string());

	cv::Mat gray;
	cv::cvtColor(image, gray, cv::COLOR_RGB2GRAY);

	std::vector<cv::Rect> visages;

	face_cascade.detectMultiScale(
				gray,
				visages,
				/* scaleFactor = */ 1.1,
				/* minNeighbours = */ 5,
				/* flags  = */ CV_HAAR_SCALE_IMAGE);

	std::cerr << "Trouvé " << visages.size() << " visage(s) !\n";

	return image(visages[0]);
}

static void extrait_images_videos(ObjetsCommuns &objets_communs, std::ostream &os = std::cerr)
{
	/* Requiert la liste des fichiers vidéos à partir desquelles les visages
	 * sont extraits. */

	auto requete = "SELECT chemin, etiquette FROM videos;";

	DeclarationSQL declaration(objets_communs.connection());
	ResultatsSQL res = declaration.requiert(requete);

	cv::Mat image;
	cv::Mat luminance;

	while (res.suivant()) {
		const auto etiquette = res.valeur("etiquette");
		const auto chemin = res.valeur("chemin");

		os << "Traitement de la vidéo : '" << chemin << "'\n";

		lecteur_video lecteur(chemin);

		auto nombre_images_lues = 0;

		auto chemin_images = objets_communs.chemin_racine() / "images";
		auto chemin_entrainement = chemin_images / "entrainement";
		auto chemin_test = chemin_images / "test";

		auto dossier_entrainement = chemin_entrainement / etiquette;
		auto dossier_test = chemin_test / etiquette;

		std::experimental::filesystem::create_directory(dossier_entrainement);
		std::experimental::filesystem::create_directory(dossier_test);

		while (lecteur.image_suivante(image)) {
			cv::cvtColor(image, luminance, cv::COLOR_RGB2GRAY);
			auto nom_fichier = std::string("image_") + std::to_string(nombre_images_lues) + ".png";


			if ((nombre_images_lues % 5) == 0) {
				auto chemin_ecriture = dossier_test / nom_fichier;
				cv::imwrite(chemin_ecriture.string(), luminance);
			}
			else {
				auto chemin_ecriture = dossier_entrainement / nom_fichier;
				cv::imwrite(chemin_ecriture.string(), luminance);
			}

			++nombre_images_lues;
		}
	}
}

static void cree_fichier_csv(
		ObjetsCommuns &objets_communs,
		const std::string &nom_dossier,
		std::ostream &os = std::cerr)
{
	using directory_entry = std::experimental::filesystem::directory_entry;
	using directory_iterator = std::experimental::filesystem::directory_iterator;

	auto chemin_fichier = objets_communs.chemin_racine() / "csv";
	chemin_fichier /= "images_" + nom_dossier + ".csv";

	os << "Écriture du fichier '" << chemin_fichier << "'\n";

	std::ofstream fichier_csv(chemin_fichier.c_str());

	auto dossier = objets_communs.chemin_racine().append("images") / nom_dossier;
	auto iterateur = directory_iterator(dossier);

	for (const directory_entry &entree : iterateur) {
		if (!std::experimental::filesystem::is_directory(entree.path())) {
			continue;
		}

		auto nom_dossier = entree.path().stem();
		std::string nom;

		for (const directory_entry &sous_entree : directory_iterator(entree.path())) {
			if (!std::experimental::filesystem::is_regular_file(sous_entree.path())) {
				continue;
			}

			nom = sous_entree.path().string() + ";" + nom_dossier.string();

			fichier_csv << nom << '\n';
		}
	}
}

static void cree_fichiers_csv(ObjetsCommuns &objets_communs)
{
	cree_fichier_csv(objets_communs, "entrainement");
	cree_fichier_csv(objets_communs, "test");
}

int main(int argc, char *argv[])
{
	auto args = docopt::docopt(utilisation, { argv + 1, argv + argc }, true, "OpenCV 0.1");

	const auto hote = docopt::get_string(args, "--hote");
	const auto usager = docopt::get_string(args, "--usager");
	const auto motpasse = docopt::get_string(args, "--motpasse");
	const auto schema = docopt::get_string(args, "--schema");
	const auto chemin_image = docopt::get_string(args, "--chemin_image");
	const auto chemin_cascade = docopt::get_string(args, "--chemin_cascade");

	std::ostream &os = std::cout;

	try {
		ObjetsCommuns objets_communs;
		objets_communs.initialise_dossiers();
		objets_communs.initialise_base_de_donnees(hote, usager, motpasse, schema);

		cree_fichiers_csv(objets_communs);

		return 0;

		extrait_images_videos(objets_communs);

		auto chemin_modeles = objets_communs.chemin_racine() / "modèles";

		if (!std_filesystem::exists(chemin_modeles / "visages.xml")) {
			std_filesystem::create_directory(chemin_modeles);

			entraine_modele(objets_communs);
		}
		else if (!chemin_image.empty() && !chemin_cascade.empty()) {
			reconnaissant_facial reconnaissant;
			reconnaissant.charge(chemin_modeles / "visages2.xml");

			auto visage = extrait_visage_image(chemin_image, chemin_cascade);
			auto resultats = reconnaissant.reconnaissance(visage);

			std::ofstream file;
			file.open((chemin_modeles / "predictions.txt").c_str());

			for (const std::pair<int, double> &resultat : resultats) {
				file << "Etiquette : " << resultat.first
					 << ", confiance : " << resultat.second << '\n';
			}

			reconnaissant.sauvegarde_eigenvectors();
		}
	}
	catch (const sql::SQLException &e) {
		os << e.what();

		os << "# ERR: SQLException in " << __FILE__;
		os << "(" << __func__ << ") on line " << __LINE__ << '\n';
		os << "# ERR: " << e.what();
		os << " (MySQL error code: " << e.getErrorCode();
		os << ", SQLState: " << e.getSQLState() << " )" << '\n';
	}
}
#else
int main(int argc, char *argv[])
{
	std::ostream &os = std::cerr;

	if (argc < 3) {
		os << "Utilisation : nom chemin_video chemin_cascade\n";
		return 1;
	}

	try {
		auto chemin_video = std_filesystem::path(argv[1]);

		if (!std_filesystem::exists(chemin_video)) {
			os << "Le chemin_video pointe vers un fichier nonexistant!\n";
			return 1;
		}

		auto chemin_cascade = std_filesystem::path(argv[2]);

		if (!std_filesystem::exists(chemin_cascade)) {
			os << "Le chemin_cascade pointe vers un fichier nonexistant!\n";
			return 1;
		}

		auto nom_sortie = chemin_video.stem().concat("_visage_extrait");
		nom_sortie.replace_extension(chemin_video.extension());

		auto chemin_sortie = chemin_video;
		chemin_sortie.replace_filename(nom_sortie);

		CHRONOMETRE_PORTEE("Détection de visage, écriture vidéo", os);

		auto face_cascade = cv::CascadeClassifier(chemin_cascade.string());

		lecteur_video lecteur(chemin_video);

		std::vector<cv::Rect> visages;
		cv::Mat sortie;
		cv::Mat image;
		cv::Mat gris;
		auto taille_sortie = cv::Size(256, 256);

		auteur_video auteur(chemin_sortie,
							lecteur.cadence(),
							taille_sortie,
							CV_FOURCC('M','J','P','G'));

		cv::Rect ancien_cadre;
		bool premier = true;

		const auto nombre_images_video = static_cast<double>(lecteur.nombre_image());

		int nombre_images = 0;

		while (lecteur.image_suivante(image)) {
			nombre_images += 100;

			visages.clear();

			cv::cvtColor(image, gris, cv::COLOR_BGR2GRAY);

			face_cascade.detectMultiScale(
						gris,
						visages,
						/* scaleFactor = */ 1.3,
						/* minNeighbours = */ 5,
						/* flags  = */ CV_HAAR_SCALE_IMAGE);

			if (visages.empty()) {
				continue;
			}

			cv::Rect cadre_visage;

			if (!premier && visages.size() != 1) {
				cadre_visage = trouve_cadre_plus_proche(visages, ancien_cadre);
			}
			else {
				cadre_visage = visages[0];
			}

			ancien_cadre = cadre_visage;
			premier = false;

			/* Découpe le visage de l'image et redimensionne le à la taille
			 * souhaitée. */
			cv::Mat decoupe = image(cadre_visage);
			cv::resize(decoupe, sortie, taille_sortie, 0.0, 0.0, cv::INTER_LINEAR);

			auteur.ecrit(sortie);

			os << "Progrès (%) : "
			   << static_cast<double>(nombre_images) / nombre_images_video
			   << '\r';
		}

		os << '\n';
	}
	catch (const char *quoi) {
		os << "Erreur : " << quoi << '\n';
	}
}
#endif
