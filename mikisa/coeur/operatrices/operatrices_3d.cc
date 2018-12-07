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

#include "operatrices_3d.h"

#include <random>

#include "bibliotheques/objets/creation.h"
#include "bibliotheques/objets/import_objet.h"
#include "bibliotheques/outils/constantes.h"
#include "bibliotheques/outils/definitions.hh"
#include "bibliotheques/texture/texture.h"
#include "bibliotheques/vision/camera.h"

#include "../corps/adaptrice_creation_corps.h"

#include "../attribut.h"
#include "../manipulatrice.h"
#include "../objet.h"
#include "../operatrice_corps.h"
#include "../operatrice_image.h"
#include "../operatrice_objet.h"
#include "../operatrice_scene.h"
#include "../usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

static constexpr auto NOM_SORTIE_CORPS = "Sortie Corps";
static constexpr auto AIDE_SORTIE_CORPS = "Crée une sortie d'un graphe de corps.";

class OperatriceSortieCorps final : public OperatriceCorps {
public:
	explicit OperatriceSortieCorps(Noeud *noeud)
		: OperatriceCorps(noeud)
	{
		inputs(1);
		outputs(0);
	}

	int type() const override
	{
		return OPERATRICE_SORTIE_CORPS;
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *class_name() const override
	{
		return NOM_SORTIE_CORPS;
	}

	const char *help_text() const override
	{
		return AIDE_SORTIE_CORPS;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		m_corps.reinitialise();
		input(0)->requiers_copie_corps(&m_corps, rectangle, temps);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static void ajourne_portee_attr_normaux(Corps *corps)
{
	auto attr_normaux = corps->attribut("N");

	if (attr_normaux != nullptr) {
		if (attr_normaux->taille() == corps->polys()->taille()) {
			attr_normaux->portee = ATTR_PORTEE_POLYGONE;
		}
		else if (attr_normaux->taille() == corps->points()->taille()) {
			attr_normaux->portee = ATTR_PORTEE_POINT;
		}
	}
}

static constexpr auto NOM_CREATION_CORPS = "Création Corps";
static constexpr auto AIDE_CREATION_CORPS = "Crée un corps.";

class OperatriceCreationCorps final : public OperatriceCorps {
public:
	explicit OperatriceCreationCorps(Noeud *noeud)
		: OperatriceCorps(noeud)
	{
		inputs(1);
		outputs(1);
	}

	int type_entree(int) const override
	{
		return OPERATRICE_IMAGE;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_objet.jo";
	}

	const char *class_name() const override
	{
		return NOM_CREATION_CORPS;
	}

	const char *help_text() const override
	{
		return AIDE_CREATION_CORPS;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		INUTILISE(rectangle);

		m_corps.reinitialise();

		AdaptriceCreationCorps adaptrice;
		adaptrice.corps = &m_corps;

		auto forme = evalue_enum("forme");

		if (forme == "plan") {
			objets::cree_grille(&adaptrice, 1.0f, 1.0f, 2, 2);
		}
		else if (forme == "cylindre") {
			objets::cree_cylindre(&adaptrice, 32, 1.0f, 1.0f, 1.0f);
		}
		else if (forme == "cube") {
			objets::cree_boite(&adaptrice, 1.0f, 1.0f, 1.0f);
		}
		else if (forme == "sphère") {
			objets::cree_sphere_uv(&adaptrice, 1.0f);
		}

		ajourne_portee_attr_normaux(&m_corps);

		auto position = evalue_vecteur("position", temps);
		auto rotation = evalue_vecteur("rotation", temps);
		auto taille = evalue_vecteur("taille", temps);

		auto transformation = math::transformation();
		transformation *= math::translation(position.x, position.y, position.z);
		transformation *= math::echelle(taille.x, taille.y, taille.z);
		transformation *= math::rotation_x(rotation.x * static_cast<float>(POIDS_DEG_RAD));
		transformation *= math::rotation_y(rotation.y * static_cast<float>(POIDS_DEG_RAD));
		transformation *= math::rotation_z(rotation.z * static_cast<float>(POIDS_DEG_RAD));

		m_corps.transformation = transformation;

//		m_manipulatrice_position.pos(dls::math::point3f(position.x, position.y, position.z));
//		m_manipulatrice_rotation.pos(dls::math::point3f(position.x, position.y, position.z));
//		m_manipulatrice_echelle.pos(dls::math::point3f(position.x, position.y, position.z));

//		if (input(0)->connectee() == false) {
//			ajoute_avertissement("Aucune texture connectée");
//		}

//		auto texture = input(0)->requiers_texture(rectangle, temps);

//		maillage->texture(texture);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static constexpr auto NOM_CAMERA = "Caméra";
static constexpr auto AIDE_CAMERA = "Crée une caméra.";

class OperatriceCamera final : public OperatriceImage {
	vision::Camera3D m_camera;
	ManipulatricePosition3D m_manipulatrice_position{};
	ManipulatriceRotation3D m_manipulatrice_rotation{};

public:
	explicit OperatriceCamera(Noeud *noeud)
		: OperatriceImage(noeud)
		, m_camera(0, 0)
	{
		inputs(0);
		outputs(1);
	}

	int type() const override
	{
		return OPERATRICE_CAMERA;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CAMERA;
	}

	vision::Camera3D *camera() override
	{
		return &m_camera;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_camera.jo";
	}

	const char *class_name() const override
	{
		return NOM_CAMERA;
	}

	const char *help_text() const override
	{
		return AIDE_CAMERA;
	}

	bool possede_manipulatrice_3d(int type) const override
	{
		return type == MANIPULATION_POSITION || type == MANIPULATION_ROTATION;
	}

	Manipulatrice3D *manipulatrice_3d(int type) override
	{
		if (type == MANIPULATION_POSITION) {
			return &m_manipulatrice_position;
		}

		if (type == MANIPULATION_ROTATION) {
			return &m_manipulatrice_rotation;
		}

		return nullptr;
	}

	void ajourne_selon_manipulatrice_3d(int type, const int temps) override
	{
		dls::math::vec3f position;

		if (type == MANIPULATION_POSITION) {
			auto p = m_manipulatrice_position.pos();
			position = dls::math::vec3f(p.x, p.y, p.z);
			m_camera.position(dls::math::vec3f(position.x, position.y, position.z));
			m_camera.ajourne_pour_operatrice();

			valeur_vecteur("position", dls::math::vec3f(position.x, position.y, position.z));
		}
		else if (type == MANIPULATION_ROTATION) {
			auto rotation = m_manipulatrice_rotation.rotation();
			m_camera.rotation(dls::math::vec3f(rotation.x, rotation.y, rotation.z));
			m_camera.ajourne_pour_operatrice();

			position = evalue_vecteur("position", temps);

			valeur_vecteur("rotation", dls::math::vec3f(rotation.x, rotation.y, rotation.z) * static_cast<float>(POIDS_RAD_DEG));
		}

		m_manipulatrice_position.pos(dls::math::point3f(position.x, position.y, position.z));
		m_manipulatrice_rotation.pos(dls::math::point3f(position.x, position.y, position.z));
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		INUTILISE(rectangle);

		const auto largeur = evalue_entier("largeur");
		const auto hauteur = evalue_entier("hauteur");
		const auto longueur_focale = evalue_decimal("longueur_focale");
		const auto largeur_senseur = evalue_decimal("largeur_senseur");
		const auto proche = evalue_decimal("proche");
		const auto eloigne = evalue_decimal("éloigné");
		const auto projection = evalue_enum("projection");
		const auto position = evalue_vecteur("position", temps);
		const auto rotation = evalue_vecteur("rotation", temps);

		if (projection == "perspective") {
			m_camera.projection(vision::TypeProjection::PERSPECTIVE);
		}
		else if (projection == "orthographique") {
			m_camera.projection(vision::TypeProjection::ORTHOGRAPHIQUE);
		}

		m_camera.redimensionne(largeur, hauteur);
		m_camera.longueur_focale(longueur_focale);
		m_camera.largeur_senseur(largeur_senseur);
		m_camera.profondeur(proche, eloigne);
		m_camera.position(position);
		m_camera.rotation(rotation * dls::math::vec3f(static_cast<float>(POIDS_DEG_RAD)));
		m_camera.ajourne_pour_operatrice();

		m_manipulatrice_position.pos(dls::math::point3f(position.x, position.y, position.z));
		m_manipulatrice_rotation.pos(dls::math::point3f(position.x, position.y, position.z));

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static constexpr auto NOM_TEXTURE = "Texture";
static constexpr auto AIDE_TEXTURE = "Crée une texture.";

class OperatriceTexture final : public OperatriceImage {
	vision::Camera3D *m_camera = nullptr;
	TextureImage m_texture{};

public:
	explicit OperatriceTexture(Noeud *noeud)
		: OperatriceImage(noeud)
	{
		inputs(2);
		outputs(1);
	}

	OperatriceTexture(OperatriceTexture const &) = default;
	OperatriceTexture &operator=(OperatriceTexture const &) = default;

	int type() const override
	{
		return OPERATRICE_IMAGE;
	}

	int type_entree(int n) const override
	{
		if (n == 0) {
			return OPERATRICE_IMAGE;
		}

		return OPERATRICE_CAMERA;
	}

	const char *nom_entree(int n) override
	{
		if (n == 0) {
			return "image";
		}

		return "caméra";
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_IMAGE;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_texture.jo";
	}

	const char *class_name() const override
	{
		return NOM_TEXTURE;
	}

	const char *help_text() const override
	{
		return AIDE_TEXTURE;
	}

	TextureImage *texture() override
	{
		return &m_texture;
	}

	vision::Camera3D *camera() override
	{
		return m_camera;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		if (input(0)->connectee() == false) {
			ajoute_avertissement("Aucune image connectée pour la texture");
			return EXECUTION_ECHOUEE;
		}

		input(0)->requiers_image(m_image, rectangle, temps);
		auto tampon = m_image.calque("image");

		if (tampon == nullptr) {
			ajoute_avertissement("Impossible de trouver un calque nommé 'image'");
			return EXECUTION_ECHOUEE;
		}

		m_texture.charge_donnees(tampon->tampon);

		auto entrepolation = evalue_enum("entrepolation");

		if (entrepolation == "linéaire") {
			m_texture.entrepolation(ENTREPOLATION_LINEAIRE);
		}
		else if (entrepolation == "proximité") {
			m_texture.entrepolation(ENTREPOLATION_VOISINAGE_PROCHE);
		}

		auto enveloppage = evalue_enum("enveloppage");

		if (enveloppage == "répétition") {
			m_texture.enveloppage(ENVELOPPAGE_REPETITION);
		}
		else if (enveloppage == "répétition_mirroir") {
			m_texture.enveloppage(ENVELOPPAGE_REPETITION_MIRROIR);
		}
		else if (enveloppage == "restriction") {
			m_texture.enveloppage(ENVELOPPAGE_RESTRICTION);
		}

		auto projection = evalue_enum("projection");

		if (projection == "planaire") {
			m_texture.projection(PROJECTION_PLANAIRE);
		}
		else if (projection == "triplanaire") {
			m_texture.projection(PROJECTION_TRIPLANAIRE);
		}
		else if (projection == "caméra") {
			m_texture.projection(PROJECTION_CAMERA);
			m_camera = input(1)->requiers_camera(rectangle, temps);

			if (m_camera == nullptr) {
				ajoute_avertissement("Aucune caméra trouvée pour la projection caméra !");
			}

			m_texture.camera(m_camera);
		}
		else if (projection == "cubique") {
			m_texture.projection(PROJECTION_CUBIQUE);
		}
		else if (projection == "cylindrique") {
			m_texture.projection(PROJECTION_CYLINDRIQUE);
		}
		else if (projection == "sphèrique") {
			m_texture.projection(PROJECTION_SPHERIQUE);
		}
		else if (projection == "uv") {
			m_texture.projection(PROJECTION_UV);
		}

		auto taille_texture = evalue_vecteur("taille_texture", temps);

		m_texture.taille(dls::math::vec3f(taille_texture.x,
											  taille_texture.y,
											  taille_texture.z));
		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static constexpr auto NOM_LECTURE_OBJET = "Lecture Objet";
static constexpr auto AIDE_LECTURE_OBJET = "Charge un objet depuis un fichier externe.";

class OperatriceLectureObjet final : public OperatriceCorps {
	vision::Camera3D *m_camera = nullptr;
	ManipulatricePosition3D m_manipulatrice_position{};
	ManipulatriceEchelle3D m_manipulatrice_echelle{};
	ManipulatriceRotation3D m_manipulatrice_rotation{};

	math::transformation m_transformation{};

	std::string m_dernier_chemin = "";

public:
	explicit OperatriceLectureObjet(Noeud *noeud)
		: OperatriceCorps(noeud)
	{
		inputs(1);
		outputs(1);
	}

	OperatriceLectureObjet(OperatriceLectureObjet const &) = default;
	OperatriceLectureObjet &operator=(OperatriceLectureObjet const &) = default;

	int type_entree(int n) const override
	{
		INUTILISE(n);
		return OPERATRICE_IMAGE;
	}

	const char *nom_entree(int n) override
	{
		INUTILISE(n);
		return "image";
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_lecture_objet.jo";
	}

	const char *class_name() const override
	{
		return NOM_LECTURE_OBJET;
	}

	const char *help_text() const override
	{
		return AIDE_LECTURE_OBJET;
	}

	bool possede_manipulatrice_3d(int type) const override
	{
		return type == MANIPULATION_POSITION
				|| type == MANIPULATION_ECHELLE
				|| type == MANIPULATION_ROTATION;
	}

	Manipulatrice3D *manipulatrice_3d(int type) override
	{
		if (type == MANIPULATION_POSITION) {
			return &m_manipulatrice_position;
		}

		if (type == MANIPULATION_ECHELLE) {
			return &m_manipulatrice_echelle;
		}

		if (type == MANIPULATION_ROTATION) {
			return &m_manipulatrice_rotation;
		}

		return nullptr;
	}

	void ajourne_selon_manipulatrice_3d(int type, const int temps) override
	{
		dls::math::vec3f position, rotation, taille;

		if (type == MANIPULATION_POSITION) {
			auto p = m_manipulatrice_position.pos();
			position = dls::math::vec3f(p.x, p.y, p.z);
			rotation = evalue_vecteur("rotation", temps) * static_cast<float>(POIDS_DEG_RAD);
			taille = evalue_vecteur("taille", temps);

			valeur_vecteur("position", dls::math::vec3f(position.x, position.y, position.z));
		}
		else if (type == MANIPULATION_ECHELLE) {
			position = evalue_vecteur("position", temps);
			rotation = evalue_vecteur("rotation", temps) * static_cast<float>(POIDS_DEG_RAD);
			auto t = m_manipulatrice_echelle.taille();
			taille = dls::math::vec3f(t.x, t.y, t.z);

			valeur_vecteur("taille", dls::math::vec3f(taille.x, taille.y, taille.z));
		}
		else if (type == MANIPULATION_ROTATION) {
			position = evalue_vecteur("position", temps);
			auto r = m_manipulatrice_rotation.rotation();
			rotation = dls::math::vec3f(r.x, r.y, r.z);
			taille = evalue_vecteur("taille", temps);

			valeur_vecteur("rotation", dls::math::vec3f(rotation.x, rotation.y, rotation.z) * static_cast<float>(POIDS_RAD_DEG));
		}
		else {
			return;
		}

		m_transformation = math::transformation();
		m_transformation *= math::translation(position.x, position.y, position.z);
		m_transformation *= math::echelle(taille.x, taille.y, taille.z);
		m_transformation *= math::rotation_x(rotation.x);
		m_transformation *= math::rotation_y(rotation.y);
		m_transformation *= math::rotation_z(rotation.z);

		m_manipulatrice_position.pos(dls::math::point3f(position.x, position.y, position.z));
		m_manipulatrice_rotation.pos(dls::math::point3f(position.x, position.y, position.z));
		m_manipulatrice_echelle.pos(dls::math::point3f(position.x, position.y, position.z));
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		INUTILISE(rectangle);

		auto chemin = evalue_fichier_entree("chemin");

		if (chemin == "") {
			ajoute_avertissement("Le chemin de fichier est vide !");
			return EXECUTION_ECHOUEE;
		}

		if (m_dernier_chemin != chemin) {
			m_corps.reinitialise();

			AdaptriceCreationCorps adaptrice;
			adaptrice.corps = &m_corps;

			if (chemin.find(".obj") != std::string::npos) {
				objets::charge_fichier_OBJ(&adaptrice, chemin);
			}
			else if (chemin.find(".stl") != std::string::npos) {
				objets::charge_fichier_STL(&adaptrice, chemin);
			}

			ajourne_portee_attr_normaux(&m_corps);

			m_dernier_chemin = chemin;
		}

		auto position = evalue_vecteur("position", temps);
		auto rotation = evalue_vecteur("rotation", temps);
		auto taille = evalue_vecteur("taille", temps);
		auto echelle_uniforme = evalue_decimal("echelle_uniforme", temps);

		m_transformation = math::transformation();
		m_transformation *= math::translation(position.x, position.y, position.z);
		m_transformation *= math::echelle(taille.x * echelle_uniforme,
										  taille.y * echelle_uniforme,
										  taille.z * echelle_uniforme);
		m_transformation *= math::rotation_x(rotation.x * static_cast<float>(POIDS_DEG_RAD));
		m_transformation *= math::rotation_y(rotation.y * static_cast<float>(POIDS_DEG_RAD));
		m_transformation *= math::rotation_z(rotation.z * static_cast<float>(POIDS_DEG_RAD));

		m_corps.transformation = m_transformation;

		m_manipulatrice_position.pos(dls::math::point3f(position.x, position.y, position.z));
		m_manipulatrice_rotation.pos(dls::math::point3f(position.x, position.y, position.z));
		m_manipulatrice_echelle.pos(dls::math::point3f(position.x, position.y, position.z));

//		if (input(0)->connectee() == false) {
//			ajoute_avertissement("Aucune texture connectée");
//		}

//		auto texture = input(0)->requiers_texture(rectangle, temps);

//		maillage->texture(texture);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static constexpr auto NOM_CREATION_NORMAUX = "Création Normaux";
static constexpr auto AIDE_CREATION_NORMAUX = "Crée des normaux pour les maillages d'entrée.";

class OperatriceCreationNormaux final : public OperatriceCorps {
public:
	explicit OperatriceCreationNormaux(Noeud *noeud)
		: OperatriceCorps(noeud)
	{
		inputs(1);
		outputs(1);
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_normaux.jo";
	}

	const char *class_name() const override
	{
		return NOM_CREATION_NORMAUX;
	}

	const char *help_text() const override
	{
		return AIDE_CREATION_NORMAUX;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		m_corps.reinitialise();
		input(0)->requiers_copie_corps(&m_corps, rectangle, temps);

		auto type = evalue_enum("type_normaux");
		auto inverse_normaux = evalue_bool("inverse_direction");

		auto attr_normaux = m_corps.ajoute_attribut("N", type_attribut::ATTRIBUT_VEC3, 0);

		if (attr_normaux->taille() != 0ul) {
			attr_normaux->reinitialise();
		}

		auto liste_polys = m_corps.polys();
		auto nombre_polygones = liste_polys->taille();

		if (nombre_polygones == 0ul) {
			ajoute_avertissement("Aucun polygone trouvé pour calculer les vecteurs normaux");
			return EXECUTION_ECHOUEE;
		}

		auto liste_points = m_corps.points();

		if (type == "plats") {
			attr_normaux->reserve(nombre_polygones);
			attr_normaux->portee = ATTR_PORTEE_POLYGONE;

			for (Polygone *poly : liste_polys->polys()) {
				if (poly->type != POLYGONE_FERME || poly->nombre_sommets() < 3) {
					attr_normaux->pousse_vec3(dls::math::vec3f(0.0f));
					continue;
				}

				const auto &v0 = liste_points->point(poly->index_point(0));
				const auto &v1 = liste_points->point(poly->index_point(1));
				const auto &v2 = liste_points->point(poly->index_point(2));

				const auto e1 = v1 - v0;
				const auto e2 = v2 - v0;

				const auto nor = normalise(produit_croix(e1, e2));

				if (inverse_normaux) {
					attr_normaux->pousse_vec3(-nor);
				}
				else {
					attr_normaux->pousse_vec3(nor);
				}
			}
		}
		else {
			auto nombre_sommets = m_corps.points()->taille();
			attr_normaux->redimensionne(nombre_sommets);
			attr_normaux->portee = ATTR_PORTEE_POINT;

			/* calcul le normal de chaque polygone */
			for (Polygone *poly : liste_polys->polys()) {
				if (poly->type != POLYGONE_FERME || poly->nombre_sommets() < 3) {
					poly->nor = dls::math::vec3f(0.0f);
					continue;
				}

				const auto &v0 = liste_points->point(poly->index_point(0));
				const auto &v1 = liste_points->point(poly->index_point(1));
				const auto &v2 = liste_points->point(poly->index_point(2));

				const auto e1 = v1 - v0;
				const auto e2 = v2 - v0;

				poly->nor = normalise(produit_croix(e1, e2));
			}

			/* accumule les normaux pour chaque sommets */
			std::vector<int> poids(nombre_sommets, 0);

			for (Polygone *poly : liste_polys->polys()) {
				if (poly->type == POLYGONE_OUVERT || poly->nombre_sommets() < 3) {
					continue;
				}

				for (size_t i = 0; i < poly->nombre_segments(); ++i) {
					const auto index_sommet = poly->index_point(i);

					if (poids[index_sommet] != 0) {
						auto nor = attr_normaux->vec3(index_sommet);
						nor += poly->nor;
						attr_normaux->vec3(index_sommet, nor);
					}
					else {
						attr_normaux->vec3(index_sommet, poly->nor);
					}

					poids[index_sommet] += 1;
				}
			}

			/* normalise les normaux */
			for (size_t n = 0; n < nombre_sommets; ++n) {
				auto nor = attr_normaux->vec3(n);
				nor /= static_cast<float>(poids[n]);
				nor = normalise(nor);

				if (inverse_normaux) {
					attr_normaux->vec3(n, -nor);
				}
				else {
					attr_normaux->vec3(n, nor);
				}
			}
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wcast-function-type"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#include <opensubdiv/far/primvarRefiner.h>
#include <opensubdiv/far/topologyDescriptor.h>
#pragma GCC diagnostic pop

static constexpr auto NOM_OPENSUBDIV = "OpenSubDiv";
static constexpr auto AIDE_OPENSUBDIV = "Sousdivise les maillages d'entrée en utilisant la bibliothèque OpenSubDiv.";

struct SommetOSD {
	SommetOSD() = default;

	SommetOSD(SommetOSD const & src)
	{
		_position[0] = src._position[0];
		_position[1] = src._position[1];
		_position[2] = src._position[2];
	}

	void Clear( void * = nullptr )
	{
		_position[0] = _position[1] = _position[2] = 0.0f;
	}

	void AddWithWeight(SommetOSD const & src, float weight)
	{
		_position[0] += weight * src._position[0];
		_position[1] += weight * src._position[1];
		_position[2] += weight * src._position[2];
	}

	// Public entreface ------------------------------------
	void SetPosition(float x, float y, float z)
	{
		_position[0] = x;
		_position[1] = y;
		_position[2] = z;
	}

	const float *GetPosition() const
	{
		return _position;
	}

private:
	float _position[3];
};

class OperatriceOpenSubDiv final : public OperatriceCorps {
public:
	explicit OperatriceOpenSubDiv(Noeud *noeud)
		: OperatriceCorps(noeud)
	{
		inputs(1);
		outputs(1);
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_opensubdiv.jo";
	}

	const char *class_name() const override
	{
		return NOM_OPENSUBDIV;
	}

	const char *help_text() const override
	{
		return AIDE_OPENSUBDIV;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		m_corps.reinitialise();
		input(0)->requiers_copie_corps(&m_corps, rectangle, temps);

		/* peuple un descripteur avec nos données crues */
		using Descripteur   = OpenSubdiv::Far::TopologyDescriptor;
		using Refineur      = OpenSubdiv::Far::TopologyRefiner;
		using UsineRafineur = OpenSubdiv::Far::TopologyRefinerFactory<Descripteur>;

		auto niveau_max = evalue_entier("niveau_max");

		auto schema = evalue_enum("schéma");
		OpenSubdiv::Sdc::SchemeType type_subdiv;

		if (schema == "catmark") {
			type_subdiv = OpenSubdiv::Sdc::SCHEME_CATMARK;
		}
		else if (schema == "bilineaire") {
			type_subdiv = OpenSubdiv::Sdc::SCHEME_BILINEAR;
		}
		else if (schema == "boucle") {
			/* À FAIRE : CRASH */
			//type_subdiv = OpenSubdiv::Sdc::SCHEME_LOOP;
			type_subdiv = OpenSubdiv::Sdc::SCHEME_CATMARK;
		}
		else {
			ajoute_avertissement("Type de schéma invalide !");
			return EXECUTION_ECHOUEE;
		}

		OpenSubdiv::Sdc::Options options;

		auto entrep_bord = evalue_enum("entrep_bord");

		if (entrep_bord == "aucune") {
			options.SetVtxBoundaryInterpolation(OpenSubdiv::Sdc::Options::VTX_BOUNDARY_NONE);
		}
		else if (entrep_bord == "segment") {
			options.SetVtxBoundaryInterpolation(OpenSubdiv::Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);
		}
		else if (entrep_bord == "segment_coin") {
			options.SetVtxBoundaryInterpolation(OpenSubdiv::Sdc::Options::VTX_BOUNDARY_EDGE_AND_CORNER);
		}
		else {
			ajoute_avertissement("Type d'entrepolation bordure sommet invalide !");
		}

		auto entrep_fvar = evalue_enum("entrep_fvar");

		if (entrep_fvar == "aucune") {
			options.SetFVarLinearInterpolation(OpenSubdiv::Sdc::Options::FVAR_LINEAR_NONE);
		}
		else if (entrep_fvar == "coins_seuls") {
			options.SetFVarLinearInterpolation(OpenSubdiv::Sdc::Options::FVAR_LINEAR_CORNERS_ONLY);
		}
		else if (entrep_fvar == "coins_p1") {
			options.SetFVarLinearInterpolation(OpenSubdiv::Sdc::Options::FVAR_LINEAR_CORNERS_PLUS1);
		}
		else if (entrep_fvar == "coins_p2") {
			options.SetFVarLinearInterpolation(OpenSubdiv::Sdc::Options::FVAR_LINEAR_CORNERS_PLUS2);
		}
		else if (entrep_fvar == "bordures") {
			options.SetFVarLinearInterpolation(OpenSubdiv::Sdc::Options::FVAR_LINEAR_BOUNDARIES);
		}
		else if (entrep_fvar == "tout") {
			options.SetFVarLinearInterpolation(OpenSubdiv::Sdc::Options::FVAR_LINEAR_ALL);
		}
		else {
			ajoute_avertissement("Type d'entrepolation bordure sommet invalide !");
		}

		auto pliage = evalue_enum("pliage");

		if (pliage == "uniforme") {
			options.SetCreasingMethod(OpenSubdiv::Sdc::Options::CREASE_UNIFORM);
		}
		else if (pliage == "chaikin") {
			options.SetCreasingMethod(OpenSubdiv::Sdc::Options::CREASE_CHAIKIN);
		}
		else {
			ajoute_avertissement("Type de pliage invalide !");
		}

		auto sousdivision_triangle = evalue_enum("sousdivision_triangle");

		if (sousdivision_triangle == "catmark") {
			options.SetTriangleSubdivision(OpenSubdiv::Sdc::Options::TRI_SUB_CATMARK);
		}
		else if (sousdivision_triangle == "lisse") {
			options.SetTriangleSubdivision(OpenSubdiv::Sdc::Options::TRI_SUB_SMOOTH);
		}
		else {
			ajoute_avertissement("Type de sousdivision triangulaire invalide !");
		}

		auto nombre_sommets = m_corps.points()->taille();
		auto nombre_polygones = m_corps.polys()->taille();

		Descripteur desc;
		desc.numVertices = static_cast<int>(nombre_sommets);
		desc.numFaces    = static_cast<int>(nombre_polygones);

		std::vector<int> nombre_sommets_par_poly;
		nombre_sommets_par_poly.reserve(nombre_polygones);

		std::vector<int> index_sommets_polys;
		index_sommets_polys.reserve(nombre_sommets * nombre_polygones);

		for (Polygone *poly : m_corps.polys()->polys()) {
			nombre_sommets_par_poly.push_back(static_cast<int>(poly->nombre_sommets()));

			for (size_t i = 0; i < poly->nombre_sommets(); ++i) {
				index_sommets_polys.push_back(static_cast<int>(poly->index_point(i)));
			}
		}

		desc.numVertsPerFace = &nombre_sommets_par_poly[0];
		desc.vertIndicesPerFace = &index_sommets_polys[0];

		/* Crée un rafineur depuis le descripteur. */
		auto rafineur = UsineRafineur::Create(
							desc, UsineRafineur::Options(type_subdiv, options));

		/* Rafine uniformément la topologie jusque 'niveau_max'. */
		rafineur->RefineUniform(Refineur::UniformOptions(niveau_max));

		/* Alloue un tampon pouvant contenir le nombre total de sommets à
			 * 'niveau_max' de rafinement. */
		std::vector<SommetOSD> sommets_osd(static_cast<size_t>(rafineur->GetNumVerticesTotal()));
		SommetOSD *sommets = &sommets_osd[0];

		/* Initialise les positions du maillage grossier. */
		auto index_point = 0;
		for (Point3D *point : m_corps.points()->points()) {
			sommets[index_point++].SetPosition(point->x, point->y, point->z);
		}

		/* Entrepole les sommets */
		OpenSubdiv::Far::PrimvarRefiner rafineur_primvar(*rafineur);

		SommetOSD *src_sommets = sommets;
		for (int niveau = 1; niveau <= niveau_max; ++niveau) {
			auto dst_sommets = src_sommets + rafineur->GetLevel(niveau - 1).GetNumVertices();
			rafineur_primvar.Interpolate(niveau, src_sommets, dst_sommets);
			src_sommets = dst_sommets;
		}

		{
			const auto ref_der_niv = rafineur->GetLevel(niveau_max);
			nombre_sommets = static_cast<size_t>(ref_der_niv.GetNumVertices());
			nombre_polygones = static_cast<size_t>(ref_der_niv.GetNumFaces());

			auto premier_sommet = static_cast<size_t>(rafineur->GetNumVerticesTotal()) - nombre_sommets;

			m_corps.reinitialise();
			m_corps.points()->reserve(nombre_sommets);
			m_corps.polys()->reserve(nombre_polygones);

			auto liste_points = m_corps.points();

			for (size_t vert = 0; vert < nombre_sommets; ++vert) {
				float const * pos = sommets[premier_sommet + vert].GetPosition();
				auto p3d = new Point3D;
				p3d->x = pos[0];
				p3d->y = pos[1];
				p3d->z = pos[2];
				liste_points->pousse(p3d);
			}

			for (size_t face = 0; face < nombre_polygones; ++face) {
				auto fverts = ref_der_niv.GetFaceVertices(static_cast<int>(face));

				auto poly = Polygone::construit(&m_corps, POLYGONE_FERME, static_cast<size_t>(fverts.size()));

				for (int i = 0; i < fverts.size(); ++i) {
					poly->ajoute_sommet(static_cast<size_t>(fverts[i]));
				}
			}
		}

		delete rafineur;

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static constexpr auto NOM_CREATION_ATTRIBUT = "Création Attribut";
static constexpr auto AIDE_CREATION_ATTRIBUT = "Crée un attribut.";

class OperatriceCreationAttribut final : public OperatriceCorps {
public:
	explicit OperatriceCreationAttribut(Noeud *noeud)
		: OperatriceCorps(noeud)
	{
		inputs(1);
		outputs(1);
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_creation_attribut.jo";
	}

	const char *class_name() const override
	{
		return NOM_CREATION_ATTRIBUT;
	}

	const char *help_text() const override
	{
		return AIDE_CREATION_ATTRIBUT;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		m_corps.reinitialise();
		input(0)->requiers_copie_corps(&m_corps, rectangle, temps);

		const auto nom_attribut = evalue_chaine("nom_attribut");
		const auto chaine_type = evalue_enum("type_attribut");
		const auto chaine_portee = evalue_enum("portee_attribut");

		if (nom_attribut == "") {
			ajoute_avertissement("Le nom de l'attribut est vide !");
			return EXECUTION_ECHOUEE;
		}

		type_attribut type;

		if (chaine_type == "ent8") {
			type = type_attribut::ATTRIBUT_ENT8;
		}
		else if (chaine_type == "ent32") {
			type = type_attribut::ATTRIBUT_ENT32;
		}
		else if (chaine_type == "décimal") {
			type = type_attribut::ATTRIBUT_DECIMAL;
		}
		else if (chaine_type == "chaine") {
			type = type_attribut::ATTRIBUT_CHAINE;
		}
		else if (chaine_type == "vec2") {
			type = type_attribut::ATTRIBUT_VEC2;
		}
		else if (chaine_type == "vec3") {
			type = type_attribut::ATTRIBUT_VEC3;
		}
		else if (chaine_type == "vec4") {
			type = type_attribut::ATTRIBUT_VEC4;
		}
		else if (chaine_type == "mat3") {
			type = type_attribut::ATTRIBUT_MAT3;
		}
		else if (chaine_type == "mat4") {
			type = type_attribut::ATTRIBUT_MAT4;
		}
		else {
			ajoute_avertissement("Type d'attribut invalide !");
			return EXECUTION_ECHOUEE;
		}

		int portee;

		if (chaine_portee == "point") {
			portee = ATTR_PORTEE_POINT;
		}
		else if (chaine_portee == "polygone") {
			portee = ATTR_PORTEE_POLYGONE;
		}
		else if (chaine_portee == "poly_point") {
			portee = ATTR_PORTEE_POLYGONE_POINT;
		}
		else if (chaine_portee == "segment") {
			portee = ATTR_PORTEE_SEGMENT;
		}
		else if (chaine_portee == "segment_point") {
			portee = ATTR_PORTEE_SEGMENT_POINT;
		}
		else {
			ajoute_avertissement("Portée d'attribut invalide !");
			return EXECUTION_ECHOUEE;
		}

		auto liste_points = m_corps.points();
		auto liste_polygones = m_corps.polys();

		size_t taille_attrib = 0ul;

		switch (portee) {
			case ATTR_PORTEE_POINT:
				taille_attrib = liste_points->taille();
				break;
			case ATTR_PORTEE_POLYGONE:
				taille_attrib = liste_polygones->taille();
				break;
			case ATTR_PORTEE_POLYGONE_POINT:
				for (Polygone *poly : liste_polygones->polys()) {
					taille_attrib += poly->nombre_sommets();
				}
				break;
			case ATTR_PORTEE_SEGMENT:
				for (Polygone *poly : liste_polygones->polys()) {
					taille_attrib += poly->nombre_segments();
				}
				break;
			case ATTR_PORTEE_SEGMENT_POINT:
				for (Polygone *poly : liste_polygones->polys()) {
					taille_attrib += poly->nombre_segments() * 2;
				}
				break;
			default:
				break;
		}

		m_corps.ajoute_attribut(nom_attribut, type, portee, taille_attrib);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static constexpr auto NOM_SUPPRESSION_ATTRIBUT = "Suppression Attribut";
static constexpr auto AIDE_SUPPRESSION_ATTRIBUT = "Supprime un attribut.";

class OperatriceSuppressionAttribut final : public OperatriceCorps {
public:
	explicit OperatriceSuppressionAttribut(Noeud *noeud)
		: OperatriceCorps(noeud)
	{
		inputs(1);
		outputs(1);
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_suppression_attribut.jo";
	}

	const char *class_name() const override
	{
		return NOM_SUPPRESSION_ATTRIBUT;
	}

	const char *help_text() const override
	{
		return AIDE_SUPPRESSION_ATTRIBUT;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		m_corps.reinitialise();
		input(0)->requiers_copie_corps(&m_corps, rectangle, temps);

		const auto nom_attribut = evalue_chaine("nom_attribut");

		if (nom_attribut == "") {
			ajoute_avertissement("Le nom de l'attribut est vide !");
			return EXECUTION_ECHOUEE;
		}

		m_corps.supprime_attribut(nom_attribut);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static constexpr auto NOM_RANDOM_ATTRIBUT = "Randomisation Attribut";
static constexpr auto AIDE_RANDOM_ATTRIBUT = "Randomise un attribut.";

class OperatriceRandomisationAttribut final : public OperatriceCorps {
public:
	explicit OperatriceRandomisationAttribut(Noeud *noeud)
		: OperatriceCorps(noeud)
	{
		inputs(1);
		outputs(1);
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_randomisation_attribut.jo";
	}

	const char *class_name() const override
	{
		return NOM_RANDOM_ATTRIBUT;
	}

	const char *help_text() const override
	{
		return AIDE_RANDOM_ATTRIBUT;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		m_corps.reinitialise();
		input(0)->requiers_copie_corps(&m_corps, rectangle, temps);

		const auto nom_attribut = evalue_chaine("nom_attribut");
		const auto graine = evalue_entier("graine");

		if (nom_attribut == "") {
			ajoute_avertissement("Le nom de l'attribut est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto attrib = m_corps.attribut(nom_attribut);

		if (attrib == nullptr) {
			ajoute_avertissement("Aucun attribut ne correspond au nom spécifié !");
			return EXECUTION_ECHOUEE;
		}

		switch (attrib->type()) {
			case type_attribut::ATTRIBUT_ENT8:
			{
				std::uniform_int_distribution<char> dist(-128, 127);
				std::mt19937 rng(graine);

				for (auto &v : attrib->ent8()) {
					v = dist(rng);
				}

				break;
			}
			case type_attribut::ATTRIBUT_ENT32:
			{
				std::uniform_int_distribution<int> dist(0, std::numeric_limits<int>::max() - 1);
				std::mt19937 rng(graine);

				for (auto &v : attrib->ent32()) {
					v = dist(rng);
				}

				break;
			}
			case type_attribut::ATTRIBUT_DECIMAL:
			{
				std::uniform_real_distribution<float> dist(0.0f, 1.0f);
				std::mt19937 rng(graine);

				for (auto &v : attrib->decimal()) {
					v = dist(rng);
				}

				break;
			}
			case type_attribut::ATTRIBUT_CHAINE:
			{
				ajoute_avertissement("La randomisation d'attribut de type chaine n'est pas supportée !");
				break;
			}
			case type_attribut::ATTRIBUT_VEC2:
			{
				std::uniform_real_distribution<float> dist(0.0f, 1.0f);
				std::mt19937 rng(graine);

				for (auto &v : attrib->vec2()) {
					v.x = dist(rng);
					v.y = dist(rng);
				}

				break;
			}
			case type_attribut::ATTRIBUT_VEC3:
			{
				std::uniform_real_distribution<float> dist(0.0f, 1.0f);
				std::mt19937 rng(graine);

				for (auto &v : attrib->vec3()) {
					v.x = dist(rng);
					v.y = dist(rng);
					v.z = dist(rng);
				}

				break;
			}
			case type_attribut::ATTRIBUT_VEC4:
			{
				std::uniform_real_distribution<float> dist(0.0f, 1.0f);
				std::mt19937 rng(graine);

				for (auto &v : attrib->vec4()) {
					v.x = dist(rng);
					v.y = dist(rng);
					v.z = dist(rng);
					v.y = dist(rng);
				}

				break;
			}
			case type_attribut::ATTRIBUT_MAT3:
			{
				ajoute_avertissement("La randomisation d'attribut de type mat3 n'est pas supportée !");
				break;
			}
			case type_attribut::ATTRIBUT_MAT4:
			{
				ajoute_avertissement("La randomisation d'attribut de type mat4 n'est pas supportée !");
				break;
			}
			case type_attribut::ATTRIBUT_INVALIDE:
			{
				ajoute_avertissement("Type d'attribut invalide !");
				break;
			}
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static constexpr auto NOM_FUSION_CORPS = "Fusionnage Corps";
static constexpr auto AIDE_FUSION_CORPS = "Fusionnage Corps.";

class OperatriceFusionnageCorps final : public OperatriceCorps {
public:
	explicit OperatriceFusionnageCorps(Noeud *noeud)
		: OperatriceCorps(noeud)
	{
		inputs(2);
		outputs(1);
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *class_name() const override
	{
		return NOM_FUSION_CORPS;
	}

	const char *help_text() const override
	{
		return AIDE_FUSION_CORPS;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		m_corps.reinitialise();

		auto corps1 = input(0)->requiers_corps(rectangle, temps);

		if (corps1 == nullptr) {
			ajoute_avertissement("1er corps manquant !");
			return EXECUTION_ECHOUEE;
		}

		auto corps2 = input(1)->requiers_corps(rectangle, temps);

		if (corps2 == nullptr) {
			ajoute_avertissement("2ème corps manquant !");
			return EXECUTION_ECHOUEE;
		}

		/* fusionne les points */
		auto liste_point  = m_corps.points();
		auto liste_point1 = corps1->points();
		auto liste_point2 = corps2->points();

		liste_point->reserve(liste_point1->taille() + liste_point2->taille());

		for (Point3D *point : liste_point1->points()) {
			auto p = new Point3D;
			p->x = point->x;
			p->y = point->y;
			p->z = point->z;
			liste_point->pousse(p);
		}

		for (Point3D *point : liste_point2->points()) {
			auto p = new Point3D;
			p->x = point->x;
			p->y = point->y;
			p->z = point->z;
			liste_point->pousse(p);
		}

		/* fusionne les polygones */

		auto liste_polys  = m_corps.polys();
		auto liste_polys1 = corps1->polys();
		auto liste_polys2 = corps2->polys();

		liste_polys->reserve(liste_polys1->taille() + liste_polys2->taille());

		for (Polygone *poly : liste_polys1->polys()) {
			auto polygone = Polygone::construit(&m_corps, poly->type, poly->nombre_sommets());

			for (size_t i = 0; i < poly->nombre_sommets(); ++i) {
				polygone->ajoute_sommet(poly->index_point(i));
			}
		}

		const auto decalage_point = liste_point1->taille();

		for (Polygone *poly : liste_polys2->polys()) {
			auto polygone = Polygone::construit(&m_corps, poly->type, poly->nombre_sommets());

			for (size_t i = 0; i < poly->nombre_sommets(); ++i) {
				polygone->ajoute_sommet(decalage_point + poly->index_point(i));
			}
		}

		/* À FAIRE : fusionne les attributs */
		/* À FAIRE : fusionne les groupes */

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_3d(UsineOperatrice *usine)
{
	usine->register_type(NOM_CAMERA, cree_desc<OperatriceCamera>(NOM_CAMERA, AIDE_CAMERA));
	usine->register_type(NOM_SCENE, cree_desc<OperatriceScene>(NOM_SCENE, AIDE_SCENE));
	usine->register_type(NOM_OBJET, cree_desc<OperatriceObjet>(NOM_OBJET, AIDE_OBJET));
	usine->register_type(NOM_LECTURE_OBJET, cree_desc<OperatriceLectureObjet>(NOM_LECTURE_OBJET, AIDE_LECTURE_OBJET));
	usine->register_type(NOM_TEXTURE, cree_desc<OperatriceTexture>(NOM_TEXTURE, AIDE_TEXTURE));

	usine->register_type(NOM_SORTIE_CORPS, cree_desc<OperatriceSortieCorps>(NOM_SORTIE_CORPS, AIDE_SORTIE_CORPS));
	usine->register_type(NOM_CREATION_CORPS, cree_desc<OperatriceCreationCorps>(NOM_CREATION_CORPS, AIDE_CREATION_CORPS));
	usine->register_type(NOM_CREATION_NORMAUX, cree_desc<OperatriceCreationNormaux>(NOM_CREATION_NORMAUX, AIDE_CREATION_NORMAUX));
	usine->register_type(NOM_OPENSUBDIV, cree_desc<OperatriceOpenSubDiv>(NOM_OPENSUBDIV, AIDE_OPENSUBDIV));

	usine->register_type(NOM_CREATION_ATTRIBUT,
						 cree_desc<OperatriceCreationAttribut>(
							 NOM_CREATION_ATTRIBUT,
							 AIDE_CREATION_ATTRIBUT));

	usine->register_type(NOM_SUPPRESSION_ATTRIBUT,
						 cree_desc<OperatriceSuppressionAttribut>(
							 NOM_SUPPRESSION_ATTRIBUT,
							 AIDE_SUPPRESSION_ATTRIBUT));

	usine->register_type(NOM_RANDOM_ATTRIBUT,
						 cree_desc<OperatriceRandomisationAttribut>(
							 NOM_RANDOM_ATTRIBUT,
							 AIDE_RANDOM_ATTRIBUT));

	usine->register_type(NOM_FUSION_CORPS,
						 cree_desc<OperatriceFusionnageCorps>(
							 NOM_FUSION_CORPS,
							 AIDE_FUSION_CORPS));
}

#pragma clang diagnostic pop
