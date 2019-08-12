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

#include "biblinternes/image/flux/lecture.h"
#include "biblinternes/image/operations/conversion.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wregister"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <OpenEXR/ImfArray.h>
#include <OpenEXR/ImfRgbaFile.h>
#include <OpenEXR/ImathBox.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfDeepTiledInputFile.h>
#include <OpenEXR/ImfDeepScanLineInputFile.h>
#include <OpenEXR/ImfDeepScanLineInputPart.h>
#pragma GCC diagnostic pop

#include "biblinternes/graphe/graphe.h"
#include "biblinternes/outils/chemin.hh"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/tableau.hh"

#include "coeur/base_de_donnees.hh"
#include "coeur/chef_execution.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/gestionnaire_fichier.hh"
#include "coeur/objet.h"
#include "coeur/operatrice_corps.h"
#include "coeur/operatrice_image.h"
#include "coeur/usine_operatrice.h"

#include "evaluation/reseau.hh"


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

static auto charge_exr(const char *chemin, std::any const &donnees)
{
	namespace openexr = OPENEXR_IMF_NAMESPACE;

	openexr::RgbaInputFile file(chemin);

	Imath::Box2i dw = file.dataWindow();

	auto width = static_cast<long>(dw.max.x - dw.min.x + 1);
	auto height = static_cast<long>(dw.max.y - dw.min.y + 1);

	dls::tableau<openexr::Rgba> pixels(width * height);

	file.setFrameBuffer(
				&pixels.front() - static_cast<size_t>(dw.min.x - dw.min.y) * static_cast<size_t>(width),
				1,
				static_cast<size_t>(width));

	file.readPixels(dw.min.y, dw.max.y);

	type_image img = type_image(
						 dls::math::Largeur(static_cast<int>(width)),
						 dls::math::Hauteur(static_cast<int>(height)));

	long idx(0);
	for (auto y(0); y < height; ++y) {
		for (auto x(0l), xe(width); x < xe; ++x, ++idx) {
			auto pixel = dls::image::PixelFloat();
			pixel.r = pixels[idx].r;
			pixel.g = pixels[idx].g;
			pixel.b = pixels[idx].b;
			pixel.a = pixels[idx].a;

			img[y][x] = pixel;
		}
	}

	auto ptr = std::any_cast<type_image *>(donnees);
	*ptr = img;
}

static auto charge_exr_tile(const char *chemin, std::any const &donnees)
{
	std::cerr << __func__ << '\n';
}

static auto imprime_entete(OPENEXR_IMF_NAMESPACE::Header const &entete)
{
	auto debut_entete = entete.begin();
	auto fin_entete   = entete.end();

	for (; debut_entete != fin_entete; ++debut_entete) {
		std::cerr << "Entete : " << debut_entete.name() << '\n';
	}
}

static auto imprime_canaux(OPENEXR_IMF_NAMESPACE::ChannelList const &entete)
{
	auto debut_entete = entete.begin();
	auto fin_entete   = entete.end();

	for (; debut_entete != fin_entete; ++debut_entete) {
		std::cerr << "Canal : " << debut_entete.name() << '\n';
	}
}

struct DonneesChargementImg {
	Image *image = nullptr;
	ChefExecution *chef = nullptr;

	DonneesChargementImg(DonneesChargementImg const &) = default;
	DonneesChargementImg &operator=(DonneesChargementImg const &) = default;
};

#define DBL_MEM

static auto charge_exr_scanline(const char *chemin, std::any const &donnees)
{
	namespace openexr = OPENEXR_IMF_NAMESPACE;

	openexr::setGlobalThreadCount(8);

	auto fichier = openexr::DeepScanLineInputFile(chemin);

	auto entete = fichier.header();

	auto donnees_chrg = std::any_cast<DonneesChargementImg *>(donnees);

	auto chef = donnees_chrg->chef;

	chef->demarre_evaluation("lecture image profonde");

	auto image = donnees_chrg->image;
	image->est_profonde = true;

	auto dw = entete.dataWindow();

	auto hauteur = dw.max.y - dw.min.y + 1;
	auto largeur = dw.max.x - dw.min.x + 1;

	/* À FAIRE : prise en compte de la fenêtre d'affichage. */

#if 0
	auto ds = entete.displayWindow();

	std::cerr << "Chargement de '" << chemin << "'\n";
	std::cerr << "Les dimensions sont " << largeur << 'x' << hauteur << '\n';
	std::cerr << "La fenêtre de données est de "
			  << '(' << dw.min.x << ',' << dw.min.y << ')'
			  << " -> "
			  << '(' << dw.max.x << ',' << dw.max.y << ')'
			  << '\n';
	std::cerr << "La fenêtre d'affichage est de "
			  << '(' << ds.min.x << ',' << ds.min.y << ')'
			  << " -> "
			  << '(' << ds.max.x << ',' << ds.max.y << ')'
			  << '\n';
#endif

	auto S = image->ajoute_calque_profond("S", largeur, hauteur, wlk::type_grille::N32);
	auto R = image->ajoute_calque_profond("R", largeur, hauteur, wlk::type_grille::R32_PTR);
	auto G = image->ajoute_calque_profond("G", largeur, hauteur, wlk::type_grille::R32_PTR);
	auto B = image->ajoute_calque_profond("B", largeur, hauteur, wlk::type_grille::R32_PTR);
	auto A = image->ajoute_calque_profond("A", largeur, hauteur, wlk::type_grille::R32_PTR);
	auto Z = image->ajoute_calque_profond("Z", largeur, hauteur, wlk::type_grille::R32_PTR);

	auto tampon_S = dynamic_cast<wlk::grille_dense_2d<unsigned int> *>(S->tampon);
	auto tampon_R = dynamic_cast<wlk::grille_dense_2d<float *> *>(R->tampon);
	auto tampon_G = dynamic_cast<wlk::grille_dense_2d<float *> *>(G->tampon);
	auto tampon_B = dynamic_cast<wlk::grille_dense_2d<float *> *>(B->tampon);
	auto tampon_A = dynamic_cast<wlk::grille_dense_2d<float *> *>(A->tampon);
	auto tampon_Z = dynamic_cast<wlk::grille_dense_2d<float *> *>(Z->tampon);

	/* À FAIRE : ceci duplique la mémoire mais il y a un crash quand on utilise
	 * les pointeurs des grilles. */
#ifdef DBL_MEM
	auto compte_echantillons = dls::tableau<unsigned>(largeur * hauteur);
	auto aR = openexr::Array2D<float *>(hauteur, largeur);
	auto aG = openexr::Array2D<float *>(hauteur, largeur);
	auto aB = openexr::Array2D<float *>(hauteur, largeur);
	auto aA = openexr::Array2D<float *>(hauteur, largeur);
	auto aZ = openexr::Array2D<float *>(hauteur, largeur);

	auto ptr_S = compte_echantillons.donnees() - dw.min.x - dw.min.y * largeur;
	auto ptr_R = &aR[0][0] - dw.min.x - dw.min.y * largeur;
	auto ptr_G = &aG[0][0] - dw.min.x - dw.min.y * largeur;
	auto ptr_B = &aB[0][0] - dw.min.x - dw.min.y * largeur;
	auto ptr_A = &aA[0][0] - dw.min.x - dw.min.y * largeur;
	auto ptr_Z = &aZ[0][0] - dw.min.x - dw.min.y * largeur;
#else
	auto ptr_S = static_cast<unsigned *>(tampon_S->donnees()) - dw.min.x - dw.min.y * largeur;
	auto ptr_R = static_cast<float **>(tampon_R->donnees()) - dw.min.x - dw.min.y * largeur;
	auto ptr_G = static_cast<float **>(tampon_G->donnees()) - dw.min.x - dw.min.y * largeur;
	auto ptr_B = static_cast<float **>(tampon_B->donnees()) - dw.min.x - dw.min.y * largeur;
	auto ptr_A = static_cast<float **>(tampon_A->donnees()) - dw.min.x - dw.min.y * largeur;
	auto ptr_Z = static_cast<float **>(tampon_Z->donnees()) - dw.min.x - dw.min.y * largeur;
#endif

	auto tampon_frame = openexr::DeepFrameBuffer();
	tampon_frame.insertSampleCountSlice(openexr::DeepSlice(
											openexr::UINT,
											reinterpret_cast<char *>(ptr_S),
											sizeof(unsigned),
											sizeof(unsigned) * static_cast<unsigned>(largeur)));

	tampon_frame.insert("R", openexr::DeepSlice(
							openexr::FLOAT,
							reinterpret_cast<char *>(ptr_R),
							sizeof(float *),
							sizeof(float *) * static_cast<unsigned>(largeur),
							sizeof(float)));

	tampon_frame.insert("G", openexr::DeepSlice(
							openexr::FLOAT,
							reinterpret_cast<char *>(ptr_G),
							sizeof(float *),
							sizeof(float *) * static_cast<unsigned>(largeur),
							sizeof(float)));

	tampon_frame.insert("B", openexr::DeepSlice(
							openexr::FLOAT,
							reinterpret_cast<char *>(ptr_B),
							sizeof(float *),
							sizeof(float *) * static_cast<unsigned>(largeur),
							sizeof(float)));

	tampon_frame.insert("A", openexr::DeepSlice(
							openexr::FLOAT,
							reinterpret_cast<char *>(ptr_A),
							sizeof(float *),
							sizeof(float *) * static_cast<unsigned>(largeur),
							sizeof(float)));

	tampon_frame.insert("Z", openexr::DeepSlice(
							openexr::FLOAT,
							reinterpret_cast<char *>(ptr_Z),
							sizeof(float *),
							sizeof(float *) * static_cast<unsigned>(largeur),
							sizeof(float)));

	fichier.setFrameBuffer(tampon_frame);

	fichier.readPixelSampleCounts(dw.min.y, dw.max.y);

	chef->indique_progression(10.0f);

	for (auto i = 0; i < hauteur; ++i) {
		for (auto j = 0; j < largeur; ++j) {
			auto index = j + i * largeur;

#ifdef DBL_MEM
			auto const n = compte_echantillons[index];
#else
			auto const n = tampon_S->valeur(index);
#endif

			if (n == 0) {
				continue;
			}

#ifdef DBL_MEM
			tampon_S->valeur(index) = n;
#endif

			auto tR = memoire::loge_tableau<float>("deep_r", n);
			auto tG = memoire::loge_tableau<float>("deep_g", n);
			auto tB = memoire::loge_tableau<float>("deep_b", n);
			auto tA = memoire::loge_tableau<float>("deep_a", n);
			auto tZ = memoire::loge_tableau<float>("deep_z", n);

#ifdef DBL_MEM
			aR[i][j] = tR;
			aG[i][j] = tG;
			aB[i][j] = tB;
			aA[i][j] = tA;
			aZ[i][j] = tZ;
#endif

			tampon_R->valeur(index) = tR;
			tampon_G->valeur(index) = tG;
			tampon_B->valeur(index) = tB;
			tampon_A->valeur(index) = tA;
			tampon_Z->valeur(index) = tZ;
		}
	}

	chef->indique_progression(20.0f);

	auto progression = 20.0f;

	/* Utilise une boucle au lieu de fichier.readPixels(dw.min.y, dw.max.y) afin
	 * de pouvoir rapporter la progression via le chef.
	 */
	for (auto y = dw.min.y; y < dw.max.y; ++y) {
		fichier.readPixels(y);

		auto delta = static_cast<float>(y) / static_cast<float>(hauteur) * (100.0f - progression);
		chef->indique_progression(progression + delta);
	}

	chef->indique_progression(100.0f);
}

static auto charge_exr_profonde(const char *chemin, std::any const &donnees)
{
	namespace openexr = OPENEXR_IMF_NAMESPACE;

	auto fichier = openexr::InputFile(chemin);
	auto entete = fichier.header();

	if (entete.type() == "deeptile") {
		charge_exr_tile(chemin, donnees);
	}
	else if (entete.type() == "deepscanline") {
		charge_exr_scanline(chemin, donnees);
	}
}

static auto charge_jpeg(const char *chemin, std::any const &donnees)
{
	auto const image_char = dls::image::flux::LecteurJPEG::ouvre(chemin);
	auto ptr = std::any_cast<type_image *>(donnees);
	*ptr = dls::image::operation::converti_en_float(image_char);
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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		entree(0)->requiers_copie_image(m_image, contexte, donnees_aval);
		m_image.nom_calque_actif(evalue_chaine("nom_calque"));
		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceLectureJPEG : public OperatriceImage {
	type_image m_image_chargee{};
	dls::chaine m_dernier_chemin = "";
	PoigneeFichier *m_poignee_fichier = nullptr;

public:
	static constexpr auto NOM = "Lecture Image";
	static constexpr auto AIDE = "Charge une image depuis le disque.";

	explicit OperatriceLectureJPEG(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(0);
		sorties(1);
	}

	OperatriceLectureJPEG(OperatriceLectureJPEG const &) = default;
	OperatriceLectureJPEG &operator=(OperatriceLectureJPEG const &) = default;

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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);
		m_image.reinitialise();

		auto nom_calque = evalue_chaine("nom_calque");

		if (nom_calque == "") {
			nom_calque = "image";
		}

		auto const &rectangle = contexte.resolution_rendu;
		auto tampon = m_image.ajoute_calque(nom_calque, rectangle);

		dls::chaine chemin = evalue_chaine("chemin");

		if (chemin.est_vide()) {
			ajoute_avertissement("Le chemin de fichier est vide !");
			return EXECUTION_ECHOUEE;
		}

		if (evalue_bool("est_animation")) {
			dls::corrige_chemin_pour_temps(chemin, contexte.temps_courant);
		}

		if (m_dernier_chemin != chemin) {
			m_poignee_fichier = contexte.gestionnaire_fichier->poignee_fichier(chemin);
			auto donnees = std::any(&m_image_chargee);

			if (chemin.trouve(".exr") != dls::chaine::npos) {
				m_poignee_fichier->lecture_chemin(charge_exr, donnees);
			}
			else {
				m_poignee_fichier->lecture_chemin(charge_jpeg, donnees);
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

		auto debut_x = std::max(0l, static_cast<long>(rectangle.x));
		auto fin_x = static_cast<long>(std::min(m_image_chargee.nombre_colonnes(), largeur));
		auto debut_y = std::max(0l, static_cast<long>(rectangle.y));
		auto fin_y = static_cast<long>(std::min(m_image_chargee.nombre_lignes(), hauteur));

		for (auto x = debut_x; x < fin_x; ++x) {
			for (auto y = debut_y; y < fin_y; ++y) {
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

class OpLectureImgProfonde : public OperatriceImage {
	type_image m_image_chargee{};
	dls::chaine m_dernier_chemin = "";
	PoigneeFichier *m_poignee_fichier = nullptr;

public:
	static constexpr auto NOM = "Lecture Image Profonde";
	static constexpr auto AIDE = "Charge une image depuis le disque.";

	explicit OpLectureImgProfonde(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(0);
		sorties(1);
	}

	OpLectureImgProfonde(OpLectureImgProfonde const &) = default;
	OpLectureImgProfonde &operator=(OpLectureImgProfonde const &) = default;

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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);

		auto nom_calque = evalue_chaine("nom_calque");

		if (nom_calque == "") {
			nom_calque = "image";
		}

		dls::chaine chemin = evalue_chaine("chemin");

		if (chemin.est_vide()) {
			ajoute_avertissement("Le chemin de fichier est vide !");
			return EXECUTION_ECHOUEE;
		}

		if (evalue_bool("est_animation")) {
			dls::corrige_chemin_pour_temps(chemin, contexte.temps_courant);
		}

		if (m_dernier_chemin != chemin) {
			m_poignee_fichier = contexte.gestionnaire_fichier->poignee_fichier(chemin);
			m_image.reinitialise();

			auto donnees_chrg = DonneesChargementImg{};
			donnees_chrg.chef = contexte.chef;
			donnees_chrg.image = &m_image;

			auto donnees = std::any(&donnees_chrg);

			if (chemin.trouve(".exr") != dls::chaine::npos) {
				m_poignee_fichier->lecture_chemin(charge_exr_profonde, donnees);
			}
			else {
				ajoute_avertissement("L'extension est invalide !");
				return EXECUTION_ECHOUEE;
			}

			m_dernier_chemin = chemin;
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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		auto const value = evalue_entier("prise");
		entree(static_cast<size_t>(value))->requiers_copie_image(m_image, contexte, donnees_aval);
		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceCommutationCorps : public OperatriceCorps {
public:
	static constexpr auto NOM = "Commutation Corps";
	static constexpr auto AIDE = "";

	explicit OperatriceCommutationCorps(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_commutation_corps.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();

		auto const condition = evalue_enum("condition");
		auto const valeur = evalue_entier("valeur_condition");
		auto resultat = false;

		if (condition == "tps_scn_egl") {
			resultat = (contexte.temps_courant == valeur);
		}
		else if (condition == "tps_scn_sup") {
			resultat = (contexte.temps_courant > valeur);
		}
		else if (condition == "tps_scn_inf") {
			resultat = (contexte.temps_courant < valeur);
		}
		else {
			ajoute_avertissement("Condition invalide !");
			return EXECUTION_ECHOUEE;
		}

		if (resultat) {
			entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);
		}
		else {
			entree(1)->requiers_copie_corps(&m_corps, contexte, donnees_aval);
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceEntreeGraphe : public OperatriceCorps {
public:
	static constexpr auto NOM = "Entrée Graphe";
	static constexpr auto AIDE = "";

	explicit OperatriceEntreeGraphe(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(0);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_entree_simulation.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(contexte);
		INUTILISE(donnees_aval);

		m_corps.reinitialise();

		if (m_graphe_parent.entrees.est_vide()) {
			ajoute_avertissement("Le graphe n'a aucune entrée !");
			return EXECUTION_ECHOUEE;
		}

		auto index_entree = evalue_entier("index_entrée");

		if (index_entree >= m_graphe_parent.entrees.taille()) {
			ajoute_avertissement("L'index de l'entrée est hors de portée !");
			return EXECUTION_ECHOUEE;
		}

		auto corps = std::any_cast<Corps *>(m_graphe_parent.entrees[index_entree]);
		corps->copie_vers(&m_corps);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceImportObjet : public OperatriceCorps {
	dls::chaine m_nom_objet = "";
	Objet *m_objet = nullptr;

public:
	static constexpr auto NOM = "Import Objet";
	static constexpr auto AIDE = "";

	explicit OperatriceImportObjet(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(0);
	}

	OperatriceImportObjet(OperatriceImportObjet const &) = default;
	OperatriceImportObjet &operator=(OperatriceImportObjet const &) = default;

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_import_objet.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	Objet *trouve_objet(ContexteEvaluation const &contexte)
	{
		auto nom_objet = evalue_chaine("nom_objet");

		if (nom_objet.est_vide()) {
			return nullptr;
		}

		if (nom_objet != m_nom_objet) {
			m_nom_objet = nom_objet;
			m_objet = contexte.bdd->objet(nom_objet);
		}

		return m_objet;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		auto nom_objet = evalue_chaine("nom_objet");

		if (nom_objet.est_vide()) {
			this->ajoute_avertissement("Aucun objet sélectionné");
			return EXECUTION_ECHOUEE;
		}

		m_objet = trouve_objet(contexte);

		if (m_objet == nullptr) {
			this->ajoute_avertissement("Aucun objet de ce nom n'existe");
			return EXECUTION_ECHOUEE;
		}

		m_objet->donnees.accede_lecture([this](DonneesObjet const *donnees)
		{
			auto &_corps_ = extrait_corps(donnees);
			_corps_.copie_vers(&m_corps);
		});

		return EXECUTION_REUSSIE;
	}

	void renseigne_dependance(ContexteEvaluation const &contexte, CompilatriceReseau &compilatrice, NoeudReseau *noeud) override
	{
		if (m_objet == nullptr) {
			m_objet = trouve_objet(contexte);

			if (m_objet == nullptr) {
				return;
			}
		}

		compilatrice.ajoute_dependance(noeud, m_objet);
	}

	void obtiens_liste(
			ContexteEvaluation const &contexte,
			dls::chaine const &raison,
			dls::tableau<dls::chaine> &liste) override
	{
		if (raison == "nom_objet") {
			for (auto &objet : contexte.bdd->objets()) {
				liste.pousse(objet->nom);
			}
		}
	}
};

/* ************************************************************************** */

#undef OP_INFINIE

#ifdef OP_INFINIE
/* utilisée pour tester le chef d'exécution */
class OperatriceInfinie : public OperatriceCorps {
public:
	static constexpr auto NOM = "Infinie";
	static constexpr auto AIDE = "";

	explicit OperatriceInfinie(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);

		m_corps.reinitialise();

		auto chef = contexte.chef;

		chef->demarre_evaluation("Infinie");

		auto i = 0.0f;

		while (true) {
			if (chef->interrompu()) {
				break;
			}

			chef->indique_progression((i + 1.0f) / 1000.0f);

			i += 1.0f;

			if (i >= 100000.0f) {
				i = 0.0f;
			}
		}

		return EXECUTION_REUSSIE;
	}
};
#endif

/* ************************************************************************** */

void enregistre_operatrices_flux(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceCommutation>());
	usine.enregistre_type(cree_desc<OperatriceCommutationCorps>());
	usine.enregistre_type(cree_desc<OperatriceVisionnage>());
	usine.enregistre_type(cree_desc<OperatriceLectureJPEG>());
	usine.enregistre_type(cree_desc<OpLectureImgProfonde>());
	usine.enregistre_type(cree_desc<OperatriceEntreeGraphe>());
	usine.enregistre_type(cree_desc<OperatriceImportObjet>());

#ifdef OP_INFINIE
	usine.enregistre_type(cree_desc<OperatriceInfinie>());
#endif
}

#pragma clang diagnostic pop
