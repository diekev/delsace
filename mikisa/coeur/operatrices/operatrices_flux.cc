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

#include "operatrices_flux.h"

#include <numero7/image/flux/lecture.h>
#include <numero7/image/operations/conversion.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wregister"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <OpenEXR/ImfRgbaFile.h>
#include <OpenEXR/ImathBox.h>
#pragma GCC diagnostic pop

#include "../operatrice_image.h"
#include "../usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

static void corrige_chemin_pour_temps(std::string &chemin, const int image)
{
	/* Trouve le dernier point. */
	auto pos_dernier_point = chemin.find_last_of('.');

	if (pos_dernier_point == std::string::npos || pos_dernier_point == 0) {
		//std::cerr << "Ne peut pas trouver le dernier point !\n";
		return;
	}

	//std::cerr << "Trouver le dernier point à la position : " << pos_dernier_point << '\n';

	/* Trouve le point précédent. */
	auto pos_point_precedent = pos_dernier_point - 1;

	while (pos_point_precedent > 0 && ::isdigit(chemin[pos_point_precedent])) {
		pos_point_precedent -= 1;
	}

	if (pos_point_precedent == std::string::npos || pos_point_precedent == 0) {
		//std::cerr << "Ne peut pas trouver le point précédent !\n";
		return;
	}

	if (chemin[pos_point_precedent] == '/') {
		//std::cerr << "Le chemin n'a pas de nom !\n";
		return;
	}

	//std::cerr << "Trouver l'avant dernier point à la position : " << pos_point_precedent << '\n';

	auto taille_nombre_image = pos_dernier_point - (pos_point_precedent + 1);

	//std::cerr << "Nombre de caractères pour l'image : " << taille_nombre_image << '\n';

	auto chaine_image = std::to_string(image);

	chaine_image.insert(0, taille_nombre_image - chaine_image.size(), '0');

	chemin.replace(pos_point_precedent + 1, chaine_image.size(), chaine_image);
	//std::cerr << "Nouveau nom " << chemin << '\n';
}

/* ************************************************************************** */

static type_image charge_exr(const char *chemin)
{
	namespace openexr = OPENEXR_IMF_NAMESPACE;

	openexr::RgbaInputFile file(chemin);

	Imath::Box2i dw = file.dataWindow();

	auto width = static_cast<size_t>(dw.max.x - dw.min.x + 1);
	auto height = static_cast<size_t>(dw.max.y - dw.min.y + 1);

	std::vector<openexr::Rgba> pixels(width * height);

	file.setFrameBuffer(&pixels[0] - static_cast<size_t>(dw.min.x - dw.min.y) * width, 1, width);
	file.readPixels(dw.min.y, dw.max.y);

	type_image img = type_image(
						 numero7::math::Largeur(static_cast<int>(width)),
						 numero7::math::Hauteur(static_cast<int>(height)));

	size_t idx(0);
	for (size_t y(0); y < height; ++y) {
		for (size_t x(0), xe(width); x < xe; ++x, ++idx) {
			auto pixel = numero7::image::PixelFloat();
			pixel.r = pixels[idx].r;
			pixel.g = pixels[idx].g;
			pixel.b = pixels[idx].b;
			pixel.a = pixels[idx].a;

			img[static_cast<int>(y)][x] = pixel;
		}
	}

	return img;
}

/* ************************************************************************** */

class OperatriceVisionnage : public OperatriceImage {
public:
	static constexpr auto NOM = "Visionneur";
	static constexpr auto AIDE = "Visionner le résultat du graphe.";

	explicit OperatriceVisionnage(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(1);
		sorties(0);
	}

	int type() const override
	{
		return OPERATRICE_SORTIE_IMAGE;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_visionnage.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(Rectangle const &rectangle, const int temps) override
	{
		entree(0)->requiers_image(m_image, rectangle, temps);
		m_image.nom_calque_actif(evalue_chaine("nom_calque"));
		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

enum class type_fichier : char {
	IMAGE,
	FILM,
	OBJET
};

struct DonneesFichier {
	std::list<OperatriceImage *> operatrices{};
	type_fichier type = type_fichier::IMAGE;
	bool entree = true;
	char pad[6];
};

#include <algorithm>

class GestionnaireFichier {
	std::unordered_map<std::string, DonneesFichier> m_tableau;

public:
	using plage = plage_iterable<std::unordered_map<std::string, DonneesFichier>::iterator>;
	using plage_const = plage_iterable<std::unordered_map<std::string, DonneesFichier>::const_iterator>;

	void ajoute_chemin(OperatriceImage *operatrice, std::string const &chemin, type_fichier type_fichier, bool entree)
	{
		auto iter = m_tableau.find(chemin);

		if (iter != m_tableau.end()) {
			DonneesFichier &donnees = iter->second;

			auto iter_operatrice = std::find(donnees.operatrices.begin(),
											 donnees.operatrices.end(),
											 operatrice);

			if (iter_operatrice == donnees.operatrices.end()) {
				donnees.operatrices.push_back(operatrice);
			}

			return;
		}

		DonneesFichier donnees;
		donnees.entree = entree;
		donnees.type = type_fichier;
		donnees.operatrices.push_back(operatrice);

		m_tableau.insert(std::make_pair(chemin, donnees));
	}

	void supprime_chemin(OperatriceImage *operatrice, std::string const &chemin)
	{
		auto iter = m_tableau.find(chemin);

		if (iter != m_tableau.end()) {
			DonneesFichier &donnees = iter->second;

			auto iter_operatrice = std::find(donnees.operatrices.begin(),
											 donnees.operatrices.end(),
											 operatrice);

			if (iter_operatrice == donnees.operatrices.end()) {
				donnees.operatrices.erase(iter_operatrice);
			}

			if (donnees.operatrices.empty()) {
				m_tableau.erase(iter);
			}
		}
	}

	plage donnees_fichiers()
	{
		return { m_tableau.begin(), m_tableau.end() };
	}

	plage_const donnees_fichiers() const
	{
		return { m_tableau.cbegin(), m_tableau.cend() };
	}
};

class OperatriceLectureJPEG : public OperatriceImage {
	type_image m_image_chargee{};
	std::string m_dernier_chemin = "";

public:
	static constexpr auto NOM = "Lecture Image";
	static constexpr auto AIDE = "Charge une image depuis le disque.";

	explicit OperatriceLectureJPEG(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(0);
		sorties(1);
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_lecture_fichier.jo";
	}

	int execute(Rectangle const &rectangle, const int temps) override
	{
		m_image.reinitialise();

		auto nom_calque = evalue_chaine("nom_calque");

		if (nom_calque == "") {
			nom_calque = "image";
		}

		auto tampon = m_image.ajoute_calque(nom_calque, rectangle);

		std::string chemin = evalue_chaine("chemin");

		if (chemin.empty()) {
			ajoute_avertissement("Le chemin de fichier est vide !");
			return EXECUTION_ECHOUEE;
		}

		if (evalue_bool("est_animation")) {
			corrige_chemin_pour_temps(chemin, temps);
		}

		if (m_dernier_chemin != chemin) {
			if (chemin.find(".exr") != std::string::npos) {
				m_image_chargee = charge_exr(chemin.c_str());
			}
			else {
				auto const image_char = numero7::image::flux::LecteurJPEG::ouvre(chemin.c_str());
				m_image_chargee = numero7::image::operation::converti_en_float(image_char);
			}

			m_dernier_chemin = chemin;
		}

		/* copie dans l'image de l'opérateur. */
		auto largeur = static_cast<int>(rectangle.largeur);
		auto hauteur = static_cast<int>(rectangle.hauteur);

		/* À FAIRE : un neoud dédié pour les textures. */
		if (evalue_bool("est_texture")) {
			largeur = m_image_chargee.nombre_colonnes();
			hauteur = m_image_chargee.nombre_lignes();

			tampon->tampon = type_image(m_image_chargee.dimensions());
		}

		auto debut_x = std::max(0ul, static_cast<size_t>(rectangle.x));
		auto fin_x = static_cast<size_t>(std::min(m_image_chargee.nombre_colonnes(), largeur));
		auto debut_y = std::max(0ul, static_cast<size_t>(rectangle.y));
		auto fin_y = static_cast<size_t>(std::min(m_image_chargee.nombre_lignes(), hauteur));

		for (size_t x = debut_x; x < fin_x; ++x) {
			for (size_t y = debut_y; y < fin_y; ++y) {
				/* À FAIRE : alpha. */
				auto pixel = m_image_chargee[static_cast<int>(y)][x];
				pixel.a = 1.0f;

				tampon->valeur(x, y, pixel);
			}
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceCommutation : public OperatriceImage {
public:
	static constexpr auto NOM = "Commutateur";
	static constexpr auto AIDE = "";

	explicit OperatriceCommutation(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_commutateur.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(Rectangle const &rectangle, const int temps) override
	{
		auto const value = evalue_entier("prise");
		entree(static_cast<size_t>(value))->requiers_image(m_image, rectangle, temps);
		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_flux(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceCommutation>());
	usine.enregistre_type(cree_desc<OperatriceVisionnage>());
	usine.enregistre_type(cree_desc<OperatriceLectureJPEG>());
}

#pragma clang diagnostic pop
