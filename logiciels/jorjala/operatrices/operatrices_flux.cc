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

#include <png.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wregister"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#	include <opencv/cv.hpp>

#include <OpenEXR/ImfArray.h>
#include <OpenEXR/ImfRgbaFile.h>
#include <OpenEXR/ImathBox.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfDeepTiledInputFile.h>
#include <OpenEXR/ImfDeepScanLineInputFile.h>
#include <OpenEXR/ImfDeepScanLineInputPart.h>
#pragma GCC diagnostic pop

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

static auto desc_depuis_exr(Imath::Box2i const &ds, Imath::Box2i const &dw)
{
	/**
	 * défintion d'un système de coordonnées pour pouvoir fusionner correctement
	 * les pixels
	 *
	 * la fenêtre d'affichage définie les coordonnées globales
	 * l'image est centrée sur le point (0.0, 0.0)
	 * la taille des pixels est originellement de 1.0
	 */

	auto const taille_ds_x = static_cast<float>(ds.max.x - ds.min.x + 1);
	auto const taille_ds_y = static_cast<float>(ds.max.y - ds.min.y + 1);

	auto const moitie_ds_x = taille_ds_x * 0.5f;
	auto const moitie_ds_y = taille_ds_y * 0.5f;

	auto const min_x = static_cast<float>(dw.min.x) - moitie_ds_x;
	auto const min_y = static_cast<float>(dw.min.y) - moitie_ds_y;

	auto const max_x = static_cast<float>(dw.max.x + 1) - moitie_ds_x;
	auto const max_y = static_cast<float>(dw.max.y + 1) - moitie_ds_y;

#if 0
	std::cerr << "---------------------------------------------\n";
	std::cerr << "étendue grille : "
			  << '(' << min_x << ',' << min_y << ')'
			  << " -> "
			  << '(' << max_x << ',' << max_y << ')'
			  << '\n';
	std::cerr << "résolution grille : "
			  << '(' << max_x - min_x << ',' << max_y - min_y << ')'
			  << " orig : "
			  << '(' << largeur << ',' << hauteur << ')'
			  << '\n';
#endif

	auto desc = wlk::desc_grille_2d{};
	desc.taille_pixel = 1.0;
	desc.etendue.min = dls::math::vec2f{ min_x, min_y };
	desc.etendue.max = dls::math::vec2f{ max_x, max_y };
	desc.fenetre_donnees = desc.etendue;

	return desc;
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

static auto charge_exr(const char *chemin, std::any const &donnees)
{
	namespace openexr = OPENEXR_IMF_NAMESPACE;

	auto fichier = openexr::InputFile(chemin);

	auto const &entete = fichier.header();
	auto const &dw = entete.dataWindow();
	auto const &ds = entete.displayWindow();

	auto const largeur = static_cast<long>(dw.max.x - dw.min.x + 1);
	auto const hauteur = static_cast<long>(dw.max.y - dw.min.y + 1);

	auto R = dls::tableau<float>();
	auto V = dls::tableau<float>();
	auto B = dls::tableau<float>();
	auto A = dls::tableau<float>();
	auto Z = dls::tableau<float>();

	auto possede_z = entete.channels().find("Z") != entete.channels().end();

	auto tampon_frame = openexr::FrameBuffer();

	R.redimensionne(largeur * hauteur);
	V.redimensionne(largeur * hauteur);
	B.redimensionne(largeur * hauteur);
	A.redimensionne(largeur * hauteur);

	tampon_frame.insert("R", openexr::Slice(
							openexr::FLOAT,
							reinterpret_cast<char *>(&R[0] - dw.min.x - dw.min.y * largeur),
							sizeof(float),
							sizeof(float) * static_cast<size_t>(largeur)));

	tampon_frame.insert("G", openexr::Slice(
							openexr::FLOAT,
							reinterpret_cast<char *>(&V[0] - dw.min.x - dw.min.y * largeur),
							sizeof(float),
							sizeof(float) * static_cast<size_t>(largeur)));

	tampon_frame.insert("B", openexr::Slice(
							openexr::FLOAT,
							reinterpret_cast<char *>(&B[0] - dw.min.x - dw.min.y * largeur),
							sizeof(float),
							sizeof(float) * static_cast<size_t>(largeur)));

	tampon_frame.insert("A", openexr::Slice(
							openexr::FLOAT,
							reinterpret_cast<char *>(&A[0] - dw.min.x - dw.min.y * largeur),
							sizeof(float),
							sizeof(float) * static_cast<size_t>(largeur)));

	if (possede_z) {
		Z.redimensionne(largeur * hauteur);

		tampon_frame.insert("Z", openexr::Slice(
								openexr::FLOAT,
								reinterpret_cast<char *>(&Z[0] - dw.min.x - dw.min.y * largeur),
								sizeof(float),
								sizeof(float) * static_cast<size_t>(largeur)));
	}

	fichier.setFrameBuffer(tampon_frame);
	fichier.readPixels(dw.min.y, dw.max.y);

	auto desc = desc_depuis_exr(ds, dw);

	auto ptr_image = std::any_cast<Image *>(donnees);
	auto calque = ptr_image->ajoute_calque("image", desc, wlk::type_grille::COULEUR);
	auto tampon = extrait_grille_couleur(calque);

	auto idx = 0l;
	for (auto y = 0; y < hauteur; ++y) {
		for (auto x = 0; x < largeur; ++x, ++idx) {
			auto pixel = dls::phys::couleur32();
			pixel.r = R[idx];
			pixel.v = V[idx];
			pixel.b = B[idx];
			pixel.a = A[idx];

			tampon->valeur(idx) = pixel;
		}
	}
}

static auto charge_exr_tile(const char *chemin, std::any const &donnees)
{
	INUTILISE(chemin);
	INUTILISE(donnees);
	std::cerr << __func__ << '\n';
}

struct DonneesChargementImg {
	Image *image = nullptr;
	ChefExecution *chef = nullptr;

	DonneesChargementImg() = default;
	DonneesChargementImg(DonneesChargementImg const &) = default;
	DonneesChargementImg &operator=(DonneesChargementImg const &) = default;
};

#define DBL_MEM

static auto charge_exr_scanline(const char *chemin, std::any const &donnees)
{
	namespace openexr = OPENEXR_IMF_NAMESPACE;

	auto fichier = openexr::DeepScanLineInputFile(chemin);

	auto entete = fichier.header();

	auto donnees_chrg = std::any_cast<DonneesChargementImg *>(donnees);

	auto chef = donnees_chrg->chef;

	chef->demarre_evaluation("lecture image profonde");

	auto image = donnees_chrg->image;
	image->est_profonde = true;

	auto ds = entete.displayWindow();
	auto dw = entete.dataWindow();

	auto hauteur = dw.max.y - dw.min.y + 1;
	auto largeur = dw.max.x - dw.min.x + 1;

	/* À FAIRE : prise en compte de la fenêtre d'affichage. */

#if 0
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

	auto desc = desc_depuis_exr(ds, dw);

	auto S = image->ajoute_calque_profond("S", desc, wlk::type_grille::N32);
	auto R = image->ajoute_calque_profond("R", desc, wlk::type_grille::R32_PTR);
	auto G = image->ajoute_calque_profond("G", desc, wlk::type_grille::R32_PTR);
	auto B = image->ajoute_calque_profond("B", desc, wlk::type_grille::R32_PTR);
	auto A = image->ajoute_calque_profond("A", desc, wlk::type_grille::R32_PTR);
	auto Z = image->ajoute_calque_profond("Z", desc, wlk::type_grille::R32_PTR);

	auto tampon_S = dynamic_cast<wlk::grille_dense_2d<unsigned int> *>(S->tampon());
	auto tampon_R = dynamic_cast<wlk::grille_dense_2d<float *> *>(R->tampon());
	auto tampon_G = dynamic_cast<wlk::grille_dense_2d<float *> *>(G->tampon());
	auto tampon_B = dynamic_cast<wlk::grille_dense_2d<float *> *>(B->tampon());
	auto tampon_A = dynamic_cast<wlk::grille_dense_2d<float *> *>(A->tampon());
	auto tampon_Z = dynamic_cast<wlk::grille_dense_2d<float *> *>(Z->tampon());

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

	auto echantillons_totals = 0l;

	for (auto i = 0; i < compte_echantillons.taille(); ++i) {
		echantillons_totals += compte_echantillons[i];
	}

	R->echantillons.redimensionne(echantillons_totals);
	G->echantillons.redimensionne(echantillons_totals);
	B->echantillons.redimensionne(echantillons_totals);
	A->echantillons.redimensionne(echantillons_totals);
	Z->echantillons.redimensionne(echantillons_totals);

	auto decalage = 0l;
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

			auto tR = &R->echantillons[decalage];
			auto tG = &G->echantillons[decalage];
			auto tB = &B->echantillons[decalage];
			auto tA = &A->echantillons[decalage];
			auto tZ = &Z->echantillons[decalage];

			decalage += n;

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
	auto tmp = dls::image::operation::converti_en_float(image_char);

	auto largeur = tmp.nombre_colonnes();
	auto hauteur = tmp.nombre_lignes();

	auto desc = wlk::desc_depuis_hauteur_largeur(hauteur, largeur);

	auto ptr_image = std::any_cast<Image *>(donnees);
	auto calque = ptr_image->ajoute_calque("image", desc, wlk::type_grille::COULEUR);
	auto tampon = extrait_grille_couleur(calque);

	auto index = 0l;
	for (auto y = 0; y < hauteur; ++y) {
		for (auto x = 0; x < largeur; ++x, ++index) {
			auto const &v = tmp[y][x];

			auto pixel = dls::phys::couleur32();
			pixel.r = v.r;
			pixel.v = v.g;
			pixel.b = v.b;
			pixel.a = 1.0f;

			tampon->valeur(index) = pixel;
		}
	}
}

static auto charge_png(const char *chemin, std::any const &donnees)
{
	auto ptr_image = std::any_cast<Image *>(donnees);

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

	if (!png) {
		return;
	}

	png_infop info = png_create_info_struct(png);

	if (!info) {
		return;
	}

	if (setjmp(png_jmpbuf(png))) {
		return;
	}

	FILE *file = std::fopen(chemin, "rb");

	if (file == nullptr) {
		return;
	}

	png_init_io(png, file);

	png_read_info(png, info);

	auto const hauteur    = png_get_image_height(png, info);
	auto const largeur    = png_get_image_width(png, info);
	auto const color_type = png_get_color_type(png, info);
	auto const bit_depth  = png_get_bit_depth(png, info);

	if (bit_depth == 16) {
		png_set_strip_16(png);
	}

	if (color_type == PNG_COLOR_TYPE_PALETTE){
		png_set_palette_to_rgb(png);
	}

	// PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8){
		png_set_expand_gray_1_2_4_to_8(png);
	}

	if (png_get_valid(png, info, PNG_INFO_tRNS)){
		png_set_tRNS_to_alpha(png);
	}

	// These color_type don't have an alpha channel then fill it with 0xff.
	if (color_type == PNG_COLOR_TYPE_RGB ||
			color_type == PNG_COLOR_TYPE_GRAY ||
			color_type == PNG_COLOR_TYPE_PALETTE)
	{
		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
	}

	if (color_type == PNG_COLOR_TYPE_GRAY ||
			color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		png_set_gray_to_rgb(png);
	}


	png_read_update_info(png, info);

	auto row_pointers = dls::tableau<png_bytep>(hauteur);

	for (auto y = 0; y < static_cast<int>(hauteur); y++) {
		row_pointers[y] = static_cast<png_byte *>(malloc(png_get_rowbytes(png,info)));
	}

	png_read_image(png, &row_pointers[0]);

	auto desc = wlk::desc_depuis_hauteur_largeur(static_cast<int>(hauteur), static_cast<int>(largeur));
	auto calque = ptr_image->ajoute_calque("image", desc, wlk::type_grille::COULEUR);
	auto tampon = extrait_grille_couleur(calque);

	for (auto y = 0; y < static_cast<int>(hauteur); y++) {
		auto row = row_pointers[y];

		for (auto x = 0; x < static_cast<int>(largeur); x++) {
			auto px = &(row[x * 4]);

			auto clr = dls::phys::couleur32();
			clr.r = px[0] / 255.0f;
			clr.v = px[1] / 255.0f;
			clr.b = px[2] / 255.0f;
			clr.a = px[3] / 255.0f;

			tampon->valeur(dls::math::vec2i(x, y)) = clr;
		}
	}

	for (auto y = 0; y < static_cast<int>(hauteur); y++) {
		free(row_pointers[y]);
	}

	png_destroy_read_struct(&png, &info, nullptr);

	std::fclose(file);
}

/* ************************************************************************** */

class OperatriceVisionnage final : public OperatriceImage {
public:
	static constexpr auto NOM = "Visionneur";
	static constexpr auto AIDE = "Visionner le résultat du graphe.";

	OperatriceVisionnage(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
	{
		entrees(1);
		sorties(0);
	}

	ResultatCheminEntreface chemin_entreface() const override
	{
		return CheminFichier{"entreface/operatrice_visionnage.jo"};
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		entree(0)->requiers_copie_image(m_image, contexte, donnees_aval);
		m_image.nom_calque_actif(evalue_chaine("nom_calque"));
		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceLectureJPEG final : public OperatriceImage {
	grille_couleur m_image_chargee{};
	dls::chaine m_dernier_chemin = "";
	PoigneeFichier *m_poignee_fichier = nullptr;

public:
	static constexpr auto NOM = "Lecture Image";
	static constexpr auto AIDE = "Charge une image depuis le disque.";

	OperatriceLectureJPEG(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
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

	ResultatCheminEntreface chemin_entreface() const override
	{
		return CheminFichier{"entreface/operatrice_lecture_fichier.jo"};
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);

		auto nom_calque = evalue_chaine("nom_calque");

		if (nom_calque == "") {
			nom_calque = "image";
		}

		dls::chaine chemin = evalue_chaine("chemin");

		if (chemin.est_vide()) {
			ajoute_avertissement("Le chemin de fichier est vide !");
			return res_exec::ECHOUEE;
		}

		if (evalue_bool("est_animation")) {
			dls::corrige_chemin_pour_temps(chemin, contexte.temps_courant);
		}

		if (m_dernier_chemin != chemin || this->cache_est_invalide) {
			m_image.reinitialise();

			m_poignee_fichier = contexte.gestionnaire_fichier->poignee_fichier(chemin);
			auto donnees = std::any(&m_image);

			if (chemin.trouve(".exr") != dls::chaine::npos) {
				m_poignee_fichier->lecture_chemin(charge_exr, donnees);
			}
			else if (chemin.trouve(".png") != dls::chaine::npos) {
				m_poignee_fichier->lecture_chemin(charge_png, donnees);
			}
			else {
				m_poignee_fichier->lecture_chemin(charge_jpeg, donnees);
			}

			m_dernier_chemin = chemin;
		}

		return res_exec::REUSSIE;
	}

	bool depend_sur_temps() const override
	{
		return evalue_bool("est_animation");
	}
};

/* ************************************************************************** */

class OperatriceLectureVideo final : public OperatriceImage {
	dls::chaine m_dernier_chemin = "";
	PoigneeFichier *m_poignee_fichier = nullptr;
	cv::VideoCapture m_video{};
	int m_derniere_image = -1;
	int m_nombre_images = 0;

public:
	static constexpr auto NOM = "Lecture Vidéo";
	static constexpr auto AIDE = "Charge une vidéo depuis le disque.";

	OperatriceLectureVideo(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
	{
		entrees(0);
		sorties(1);
	}

	OperatriceLectureVideo(OperatriceLectureVideo const &) = default;
	OperatriceLectureVideo &operator=(OperatriceLectureVideo const &) = default;

	~OperatriceLectureVideo() override
	{
		m_video.release();
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	ResultatCheminEntreface chemin_entreface() const override
	{
		return CheminFichier{"entreface/operatrice_lecture_video.jo"};
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);
		auto nom_calque = evalue_chaine("nom_calque");

		if (nom_calque == "") {
			nom_calque = "image";
		}

		dls::chaine chemin = evalue_chaine("chemin");

		if (chemin.est_vide()) {
			ajoute_avertissement("Le chemin de fichier est vide !");
			return res_exec::ECHOUEE;
		}

		if (m_dernier_chemin != chemin || m_poignee_fichier == nullptr) {
			if (m_video.isOpened()) {
				m_video.release();
			}

			m_poignee_fichier = contexte.gestionnaire_fichier->poignee_fichier(chemin);
			m_dernier_chemin = chemin;

			if (!m_video.open(chemin.c_str())) {
				this->ajoute_avertissement("Impossible d'ouvrir la vidéo");
				return res_exec::ECHOUEE;
			}

			m_nombre_images = static_cast<int>(m_video.get(CV_CAP_PROP_FRAME_COUNT));
		}

		if (m_poignee_fichier == nullptr) {
			this->ajoute_avertissement("Impossible d'obtenir une poignée sur le fichier");
			return res_exec::ECHOUEE;
		}

		if (contexte.temps_courant == contexte.temps_debut) {
			m_video.set(CV_CAP_PROP_POS_FRAMES, 0);
		}
		else if (m_derniere_image != -1 && contexte.temps_courant != m_derniere_image + 1) {
			return res_exec::REUSSIE;
		}

		m_image.reinitialise();

		auto mat = cv::Mat();
		if (!m_video.read(mat)) {
			this->ajoute_avertissement("Impossible de lire une image depuis la vidéo");
			return res_exec::ECHOUEE;
		}

		auto largeur = static_cast<int>(m_video.get(CV_CAP_PROP_FRAME_WIDTH));
		auto hauteur = static_cast<int>(m_video.get(CV_CAP_PROP_FRAME_HEIGHT));

		auto desc = wlk::desc_depuis_hauteur_largeur(hauteur, largeur);
		auto calque = m_image.ajoute_calque("image", desc, wlk::type_grille::COULEUR);
		auto tampon = extrait_grille_couleur(calque);

		for (auto y = 0; y < hauteur; ++y) {
			for (auto x = 0; x < largeur; ++x) {
				auto pixel = mat.at<cv::Vec3b>(y, x);

				auto res = dls::phys::couleur32();
				res.r = static_cast<float>(pixel[2]) / 255.0f;
				res.v = static_cast<float>(pixel[1]) / 255.0f;
				res.b = static_cast<float>(pixel[0]) / 255.0f;
				res.a = 1.0f;

				tampon->valeur(dls::math::vec2i(x, y)) = res;
			}
		}

		m_derniere_image = contexte.temps_courant;

		return res_exec::REUSSIE;
	}

	bool depend_sur_temps() const override
	{
		return true;
	}
};

/* ************************************************************************** */

class OpLectureImgProfonde final : public OperatriceImage {
	grille_couleur m_image_chargee{};
	dls::chaine m_dernier_chemin = "";
	PoigneeFichier *m_poignee_fichier = nullptr;

public:
	static constexpr auto NOM = "Lecture Image Profonde";
	static constexpr auto AIDE = "Charge une image depuis le disque.";

	OpLectureImgProfonde(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
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

	ResultatCheminEntreface chemin_entreface() const override
	{
		return CheminFichier{"entreface/operatrice_lecture_fichier.jo"};
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);

		auto nom_calque = evalue_chaine("nom_calque");

		if (nom_calque == "") {
			nom_calque = "image";
		}

		dls::chaine chemin = evalue_chaine("chemin");

		if (chemin.est_vide()) {
			ajoute_avertissement("Le chemin de fichier est vide !");
			return res_exec::ECHOUEE;
		}

		if (evalue_bool("est_animation")) {
			dls::corrige_chemin_pour_temps(chemin, contexte.temps_courant);
		}

		if (m_dernier_chemin != chemin || this->cache_est_invalide) {
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
				return res_exec::ECHOUEE;
			}

			m_dernier_chemin = chemin;
		}

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

template <int O>
struct desc_operatrice_commutation;

template <>
struct desc_operatrice_commutation<0> {
	static constexpr auto NOM = "Commutation Image";
	static constexpr auto AIDE = "";
	static constexpr auto chemin_entreface = "entreface/operatrice_commutateur.jo";
	static constexpr auto type_prises = type_prise::IMAGE;
	static constexpr int type_operatrice = OPERATRICE_IMAGE;
};

template <>
struct desc_operatrice_commutation<1> {
	static constexpr auto NOM = "Commutation Corps";
	static constexpr auto AIDE = "";
	static constexpr auto chemin_entreface = "entreface/operatrice_commutation_corps.jo";
	static constexpr auto type_prises = type_prise::CORPS;
	static constexpr int type_operatrice = OPERATRICE_CORPS;
};

template <int O>
class OperatriceCommutation final : public OperatriceCorps {
public:
	static constexpr auto NOM = desc_operatrice_commutation<O>::NOM;
	static constexpr auto AIDE = desc_operatrice_commutation<O>::AIDE;

	OperatriceCommutation(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
	}

	ResultatCheminEntreface chemin_entreface() const override
	{
		return CheminFichier{desc_operatrice_commutation<O>::chemin_entreface};
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int type() const override
	{
		return desc_operatrice_commutation<O>::type_operatrice;
	}

	type_prise type_entree(int) const override
	{
		return desc_operatrice_commutation<O>::type_prises;
	}

	type_prise type_sortie(int) const override
	{
		return desc_operatrice_commutation<O>::type_prises;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		auto chef = contexte.chef;

		if (O == 0) {
			auto const value = evalue_entier("prise");
			entree(value)->requiers_copie_image(m_image, contexte, donnees_aval);
			chef->demarre_evaluation("commutation image");
		}
		else if (O == 1) {
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
				return res_exec::ECHOUEE;
			}

			if (resultat) {
				entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);
			}
			else {
				entree(1)->requiers_copie_corps(&m_corps, contexte, donnees_aval);
			}

			chef->demarre_evaluation("commutation corps");
		}

		chef->indique_progression(100.0f);

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

template <int O>
struct desc_operatrice_cache_memoire;

template <>
struct desc_operatrice_cache_memoire<0> {
	static constexpr auto NOM = "Cache Image Mémoire";
	static constexpr auto AIDE = "";
	static constexpr auto type_prises = type_prise::IMAGE;
	static constexpr int type_operatrice = OPERATRICE_IMAGE;
};

template <>
struct desc_operatrice_cache_memoire<1> {
	static constexpr auto NOM = "Cache Corps Mémoire";
	static constexpr auto AIDE = "";
	static constexpr auto type_prises = type_prise::CORPS;
	static constexpr int type_operatrice = OPERATRICE_CORPS;
};

template <int O>
class OpCacheMemoire final : public OperatriceCorps {
public:
	static constexpr auto NOM = desc_operatrice_cache_memoire<O>::NOM;
	static constexpr auto AIDE = desc_operatrice_cache_memoire<O>::AIDE;

	OpCacheMemoire(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(1);
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

	int type() const override
	{
		return desc_operatrice_commutation<O>::type_operatrice;
	}

	type_prise type_entree(int) const override
	{
		return desc_operatrice_commutation<O>::type_prises;
	}

	type_prise type_sortie(int) const override
	{
		return desc_operatrice_commutation<O>::type_prises;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);

		auto chef = contexte.chef;

		if (O == 0) {
			m_image.reinitialise();

			entree(0)->requiers_copie_image(m_image, contexte, donnees_aval);

			chef->demarre_evaluation("cache image");
		}
		else if (O == 1) {
			m_corps.reinitialise();

			entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

			chef->demarre_evaluation("cache corps");
		}

		entree(0)->signale_cache(chef);
		chef->indique_progression(100.0f);

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceEntreeGraphe final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Entrée Graphe";
	static constexpr auto AIDE = "";

	OperatriceEntreeGraphe(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(0);
	}

	ResultatCheminEntreface chemin_entreface() const override
	{
		return CheminFichier{"entreface/operatrice_entree_simulation.jo"};
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(contexte);
		INUTILISE(donnees_aval);

		m_corps.reinitialise();

		if (m_graphe_parent.entrees.est_vide()) {
			ajoute_avertissement("Le graphe n'a aucune entrée !");
			return res_exec::ECHOUEE;
		}

		auto index_entree = evalue_entier("index_entrée");

		if (index_entree >= m_graphe_parent.entrees.taille()) {
			ajoute_avertissement("L'index de l'entrée est hors de portée !");
			return res_exec::ECHOUEE;
		}

		auto corps = std::any_cast<Corps *>(m_graphe_parent.entrees[index_entree]);
		corps->copie_vers(&m_corps);

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OpReferenceObjet final : public OperatriceCorps {
	dls::chaine m_nom_objet = "";
	Objet *m_objet = nullptr;

public:
	static constexpr auto NOM = "Référence Objet";
	static constexpr auto AIDE = "";

	OpReferenceObjet(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(0);
	}

	OpReferenceObjet(OpReferenceObjet const &) = default;
	OpReferenceObjet &operator=(OpReferenceObjet const &) = default;

	ResultatCheminEntreface chemin_entreface() const override
	{
		return CheminFichier{"entreface/operatrice_import_objet.jo"};
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		auto nom_objet = evalue_chaine("nom_objet");

		if (nom_objet.est_vide()) {
			this->ajoute_avertissement("Aucun objet sélectionné");
			return res_exec::ECHOUEE;
		}

		m_objet = trouve_objet(contexte);

		if (m_objet == nullptr) {
			this->ajoute_avertissement("Aucun objet de ce nom n'existe");
			return res_exec::ECHOUEE;
		}

		m_objet->donnees.accede_lecture([this](DonneesObjet const *donnees)
		{
			auto &_corps_ = extrait_corps(donnees);
			_corps_.copie_vers(&m_corps);
		});

		return res_exec::REUSSIE;
	}

	void renseigne_dependance(ContexteEvaluation const &contexte, CompilatriceReseau &compilatrice, NoeudReseau *noeud_reseau) override
	{
		if (m_objet == nullptr) {
			m_objet = trouve_objet(contexte);

			if (m_objet == nullptr) {
				return;
			}
		}

		compilatrice.ajoute_dependance(noeud_reseau, m_objet->noeud);
	}

	void obtiens_liste(
			ContexteEvaluation const &contexte,
			dls::chaine const &raison,
			dls::tableau<dls::chaine> &liste) override
	{
		if (raison == "nom_objet") {
			for (auto &objet : contexte.bdd->objets()) {
				liste.ajoute(objet->noeud->nom);
			}
		}
	}
};

/* ************************************************************************** */

#undef OP_INFINIE

#ifdef OP_INFINIE
/* utilisée pour tester le chef d'exécution */
class OperatriceInfinie final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Infinie";
	static constexpr auto AIDE = "";

	OperatriceInfinie(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(1);
	}

	ResultatCheminEntreface chemin_entreface() const override
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
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

		return res_exec::REUSSIE;
	}
};
#endif

/* ************************************************************************** */

void enregistre_operatrices_flux(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceCommutation<0>>());
	usine.enregistre_type(cree_desc<OperatriceCommutation<1>>());
	usine.enregistre_type(cree_desc<OpCacheMemoire<0>>());
	usine.enregistre_type(cree_desc<OpCacheMemoire<1>>());
	usine.enregistre_type(cree_desc<OperatriceVisionnage>());
	usine.enregistre_type(cree_desc<OperatriceLectureJPEG>());
	usine.enregistre_type(cree_desc<OperatriceLectureVideo>());
	usine.enregistre_type(cree_desc<OpLectureImgProfonde>());
	usine.enregistre_type(cree_desc<OperatriceEntreeGraphe>());
	usine.enregistre_type(cree_desc<OpReferenceObjet>());

#ifdef OP_INFINIE
	usine.enregistre_type(cree_desc<OperatriceInfinie>());
#endif
}

#pragma clang diagnostic pop
