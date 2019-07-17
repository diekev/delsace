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
#include <OpenEXR/ImfRgbaFile.h>
#include <OpenEXR/ImathBox.h>
#pragma GCC diagnostic pop

#include "biblinternes/graphe/graphe.h"
#include "biblinternes/outils/chemin.hh"
#include "biblinternes/outils/definitions.h"

#include "biblinternes/structures/tableau.hh"

#include "../evaluation/reseau.hh"

#include "../base_de_donnees.hh"

#include "../chef_execution.hh"
#include "../contexte_evaluation.hh"
#include "../gestionnaire_fichier.hh"
#include "../objet.h"
#include "../operatrice_corps.h"
#include "../operatrice_image.h"
#include "../usine_operatrice.h"


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
		entree(0)->requiers_image(m_image, contexte, donnees_aval);
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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		auto const value = evalue_entier("prise");
		entree(static_cast<size_t>(value))->requiers_image(m_image, contexte, donnees_aval);
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

		m_objet->corps.accede_lecture([this](Corps const &_corps_)
		{
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
	usine.enregistre_type(cree_desc<OperatriceEntreeGraphe>());
	usine.enregistre_type(cree_desc<OperatriceImportObjet>());

#ifdef OP_INFINIE
	usine.enregistre_type(cree_desc<OperatriceInfinie>());
#endif
}

#pragma clang diagnostic pop
